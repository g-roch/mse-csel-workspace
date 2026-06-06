/* skeleton.c */
#include <linux/module.h>	/* needed by all modules */
#include <linux/init.h>		/* needed for macros */
#include <linux/kernel.h>	/* needed for debugging */

#include <linux/gpio.h>		/* needed for i/o handling */
#include <linux/kthread.h>	/* needed for kthread_run/kthread_stop */
#include <linux/fs.h>		/* needed for filp_open/filp_close/kernel_read */
#include <linux/string.h>	/* needed for strchr */
#include <linux/kobject.h>	/* needed for the sysfs kobject */
#include <linux/sysfs.h>	/* needed for sysfs attributes/sysfs_notify */

#include "../button_control/config.h"

#define STATUS_LED_GPIO	10

#define CONTROL_PERIOD_MS	100 // How often the control loop samples mode/temperature

static DECLARE_WAIT_QUEUE_HEAD(freq_wq);  // Wakes the blink thread when the frequency changes
static DECLARE_WAIT_QUEUE_HEAD(ctrl_wq);  // Wakes the control thread on module unload
atomic_t current_freq = ATOMIC_INIT(1); // Default frequency of 1 Hz (also the get_freq value)

// Values owned by the daemon via sysfs (no longer backed by /tmp files):
static int set_freq_val = 1;  // Frequency requested in manual mode
static int auto_freq_val = 0; // 0 = manual, 1 = auto

static struct task_struct *blink_thread;
static struct task_struct *control_thread;

static struct kobject *led_kobj; // /sys/kernel/led

// Get interval in ms to switch the LED, given a 50% duty cycle and the frequency in Hz
static int get_switch_interval(int freq) {
	if (freq <= 0) {
		pr_err ("Invalid frequency: %d. Must be > 0.\n", freq);
		return -1;
	}
	return (1000 / freq) / 2; // Convert frequency to period in ms, then divide by 2 for 50% duty cycle
}

// Read a leading integer from a text file. Returns 0 on success, a negative errno otherwise.
// On failure *out is left untouched, so the caller can keep its previous value.
static int read_int_from_file(const char *path, int *out) {
	struct file *fp;
	char buf[32];
	char *nl;
	loff_t pos = 0;
	ssize_t n;

	fp = filp_open(path, O_RDONLY, 0);
	if (IS_ERR(fp)) {
		return PTR_ERR(fp);
	}

	n = kernel_read(fp, buf, sizeof(buf) - 1, &pos);
	filp_close(fp, NULL);
	if (n <= 0) {
		return -EINVAL;
	}

	buf[n] = '\0';
	nl = strchr(buf, '\n');
	if (nl) {
		*nl = '\0';
	}

	return kstrtoint(buf, 10, out);
}

// Map a CPU temperature (in °C) to a blink frequency (in Hz) for automatic mode.
static int freq_from_temp(int temp_c) {
	if (temp_c < 35) {
		return 2;
	}
	if (temp_c < 40) {
		return 5;
	}
	if (temp_c < 45) {
		return 10;
	}
	return 20;
}

// --- sysfs attributes -------------------------------------------------------
// set_freq and auto_freq are written by the daemon; get_freq is read (and
// polled) by the daemon. get_freq fires sysfs_notify whenever it changes so
// the daemon's epoll(EPOLLPRI) wakes immediately.

static ssize_t set_freq_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf) {
	return sysfs_emit(buf, "%d\n", READ_ONCE(set_freq_val));
}

static ssize_t set_freq_store(struct kobject *kobj, struct kobj_attribute *attr,
			      const char *buf, size_t count) {
	int v;
	if (kstrtoint(buf, 10, &v)) {
		return -EINVAL;
	}
	WRITE_ONCE(set_freq_val, v);
	return count;
}

static ssize_t auto_freq_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf) {
	return sysfs_emit(buf, "%d\n", READ_ONCE(auto_freq_val));
}

static ssize_t auto_freq_store(struct kobject *kobj, struct kobj_attribute *attr,
			       const char *buf, size_t count) {
	int v;
	if (kstrtoint(buf, 10, &v)) {
		return -EINVAL;
	}
	WRITE_ONCE(auto_freq_val, !!v);
	return count;
}

static ssize_t get_freq_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf) {
	return sysfs_emit(buf, "%d\n", atomic_read(&current_freq));
}

static struct kobj_attribute set_freq_attr  = __ATTR(set_freq,  0664, set_freq_show,  set_freq_store);
static struct kobj_attribute auto_freq_attr = __ATTR(auto_freq, 0664, auto_freq_show, auto_freq_store);
static struct kobj_attribute get_freq_attr  = __ATTR(get_freq,  0444, get_freq_show,  NULL);

static struct attribute *led_attrs[] = {
	&set_freq_attr.attr,
	&auto_freq_attr.attr,
	&get_freq_attr.attr,
	NULL,
};

static struct attribute_group led_group = {
	.attrs = led_attrs,
};

static int blink_status_led(void *data) {
	int led_state = 0; // 0 = off, 1 = on
	while (!kthread_should_stop()) {
		unsigned int hz = atomic_read(&current_freq);
		long timeout;

		if (hz == 0) {
			gpio_set_value(STATUS_LED_GPIO, 0); // Turn off LED if frequency is 0
			// Sleep indefinitely until frequency changes or module is unloaded
			wait_event_interruptible(freq_wq, atomic_read(&current_freq) != 0 || kthread_should_stop());
			continue;
		}

		// Get half-period interval for blinking
		timeout = msecs_to_jiffies(get_switch_interval(hz));

		// Sleep for half-period, but wake early if frequency changes
		wait_event_interruptible_timeout(freq_wq, atomic_read(&current_freq) != hz || kthread_should_stop(), timeout);

		// Toggle LED state
		led_state = !led_state;
		gpio_set_value(STATUS_LED_GPIO, led_state);
	}

	// Cleanup: Ensure LED is turned off when exiting
	gpio_set_value(STATUS_LED_GPIO, 0);
	return 0;
}

// Periodically decide the blink frequency from the mode and the CPU temperature.
static int control_loop(void *data) {
	while (!kthread_should_stop()) {
		int old_freq = atomic_read(&current_freq);
		int new_freq = old_freq;
		int mode = READ_ONCE(auto_freq_val); // 0 = manual, 1 = auto
		int temp_milli = 0;                  // CPU temperature in millidegrees Celsius

		if (mode) {
			// Auto: derive the frequency from the current CPU temperature
			if (read_int_from_file(GET_TEMP_PATH, &temp_milli) == 0) {
				new_freq = freq_from_temp(temp_milli / 1000);
			}
		} else {
			// Manual: follow the frequency the daemon requested via set_freq
			new_freq = READ_ONCE(set_freq_val);
		}

		// Update the blink thread and notify pollers only when it actually changes
		if (new_freq != old_freq) {
			atomic_set(&current_freq, new_freq);
			sysfs_notify(led_kobj, NULL, "get_freq");
			wake_up_interruptible(&freq_wq);
		}

		// Sample roughly once every CONTROL_PERIOD_MS, but wake up immediately on unload
		wait_event_interruptible_timeout(ctrl_wq, kthread_should_stop(),
						 msecs_to_jiffies(CONTROL_PERIOD_MS));
	}
	return 0;
}

static int __init skeleton_init(void)
{
	int status;

	// Install status LED
	status = gpio_request(STATUS_LED_GPIO, "status_led");
	if (status < 0) {
		pr_err ("Failed to request GPIO %d for status LED\n", STATUS_LED_GPIO);
		return status;
	}
	gpio_direction_output(STATUS_LED_GPIO, 0); // Start with LED off

	// Create /sys/kernel/led with the set_freq/get_freq/auto_freq attributes
	led_kobj = kobject_create_and_add("led", kernel_kobj);
	if (!led_kobj) {
		pr_err ("Failed to create led kobject\n");
		gpio_free(STATUS_LED_GPIO);
		return -ENOMEM;
	}

	status = sysfs_create_group(led_kobj, &led_group);
	if (status) {
		pr_err ("Failed to create sysfs group\n");
		kobject_put(led_kobj);
		gpio_free(STATUS_LED_GPIO);
		return status;
	}

	// Start blinking the status LED in a separate kernel thread
	blink_thread = kthread_run(blink_status_led, NULL, "blink_thread");
	if (IS_ERR(blink_thread)) {
		pr_err ("Failed to create blink thread\n");
		sysfs_remove_group(led_kobj, &led_group);
		kobject_put(led_kobj);
		gpio_free(STATUS_LED_GPIO);
		return PTR_ERR(blink_thread);
	}

	// Start the control loop that drives the frequency from mode/temperature
	control_thread = kthread_run(control_loop, NULL, "freq_control");
	if (IS_ERR(control_thread)) {
		pr_err ("Failed to create control thread\n");
		kthread_stop(blink_thread);
		sysfs_remove_group(led_kobj, &led_group);
		kobject_put(led_kobj);
		gpio_free(STATUS_LED_GPIO);
		return PTR_ERR(control_thread);
	}

	gpio_set_value(STATUS_LED_GPIO, 1); // Turn on LED to indicate module is loaded

	pr_info ("Linux mini-project module loaded\n");

	return 0;
}

static void __exit skeleton_exit(void)
{
	// Stop the control loop first so it stops driving the frequency
	kthread_stop(control_thread);
	// Wake up the blink thread if it's sleeping, then stop it
	kthread_stop(blink_thread);
	wake_up_interruptible(&freq_wq);
	// Remove the sysfs attributes
	sysfs_remove_group(led_kobj, &led_group);
	kobject_put(led_kobj);
	// Free the GPIO for the status LED
	gpio_free(STATUS_LED_GPIO);

	pr_info ("Linux mini-project module unloaded\n");
}

module_init (skeleton_init);
module_exit (skeleton_exit);

MODULE_AUTHOR ("CSEL1 - Group: Carolina, Gaby, Simon");
MODULE_DESCRIPTION ("Module skeleton");
MODULE_LICENSE ("GPL");
