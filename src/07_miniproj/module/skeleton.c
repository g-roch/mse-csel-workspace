/* skeleton.c */
#include <linux/module.h>	/* needed by all modules */
#include <linux/init.h>		/* needed for macros */
#include <linux/kernel.h>	/* needed for debugging */

#include <linux/gpio.h>		/* needed for i/o handling */
#include <linux/kthread.h>	/* needed for kthread_run/kthread_stop */
#include <linux/fs.h>		/* needed for filp_open/filp_close/kernel_read/kernel_write */
#include <linux/string.h>	/* needed for strchr */

#include "../button_control/config.h"

#define STATUS_LED_GPIO	10

#define CONTROL_PERIOD_MS	100 // How often the control loop samples mode/temperature

static DECLARE_WAIT_QUEUE_HEAD(freq_wq);  // Wakes the blink thread when the frequency changes
static DECLARE_WAIT_QUEUE_HEAD(ctrl_wq);  // Wakes the control thread on module unload
atomic_t current_freq = ATOMIC_INIT(1); // Default frequency of 1 Hz

static struct task_struct *blink_thread;
static struct task_struct *control_thread;

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

	// Keep only the first line so stale trailing bytes (the daemon does not
	// truncate set_freq/auto_freq) don't break parsing.
	buf[n] = '\0';
	nl = strchr(buf, '\n');
	if (nl) {
		*nl = '\0';
	}

	return kstrtoint(buf, 10, out);
}

// Write an integer (followed by a newline) to a file, truncating any previous content.
static int write_int_to_file(const char *path, int value) {
	struct file *fp;
	char buf[16];
	loff_t pos = 0;
	int len;
	ssize_t n;

	fp = filp_open(path, O_WRONLY | O_TRUNC, 0);
	if (IS_ERR(fp)) {
		return PTR_ERR(fp);
	}

	len = scnprintf(buf, sizeof(buf), "%d\n", value);
	n = kernel_write(fp, buf, len, &pos);
	filp_close(fp, NULL);

	return (n < 0) ? (int)n : 0;
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
		int mode = 0;       // 0 = manual, 1 = auto; the daemon owns auto_freq
		int temp_milli = 0; // CPU temperature in millidegrees Celsius

		// Read the mode set by the daemon (stay manual if it can't be read)
		read_int_from_file(AUTO_FREQ_PATH, &mode);

		if (mode) {
			// Auto: derive the frequency from the current CPU temperature
			if (read_int_from_file(GET_TEMP_PATH, &temp_milli) == 0) {
				new_freq = freq_from_temp(temp_milli / 1000);
			}
		} else {
			// Manual: follow the frequency the daemon requested via set_freq
			int req = 0;
			if (read_int_from_file(SET_FREQ_PATH, &req) == 0) {
				new_freq = req;
			}
		}

		// Publish the active frequency back to the daemon via get_freq
		write_int_to_file(GET_FREQ_PATH, new_freq);

		// Update the blink thread only when the frequency actually changes
		if (new_freq != old_freq) {
			atomic_set(&current_freq, new_freq);
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
	struct file *fp;

	// Install status LED
	status = gpio_request(STATUS_LED_GPIO, "status_led");
	if (status < 0) {
		pr_err ("Failed to request GPIO %d for status LED\n", STATUS_LED_GPIO);
		return status;
	}
	gpio_direction_output(STATUS_LED_GPIO, 0); // Start with LED off

	// Create the necessary files if not present
	// - set_freq for setting the frequency (write-only for daemon, read-only for module)
	fp = filp_open(SET_FREQ_PATH, O_WRONLY | O_CREAT, 0666);
	if (IS_ERR(fp)) {
		pr_err ("Failed to create set_freq file\n");
		gpio_free(STATUS_LED_GPIO);
		return PTR_ERR(fp);
	}
	filp_close(fp, NULL);

	// - get_freq for reading the current frequency (read-write)
	fp = filp_open(GET_FREQ_PATH, O_RDWR | O_CREAT, 0666);
	if (IS_ERR(fp)) {
		pr_err ("Failed to create get_freq file\n");
		gpio_free(STATUS_LED_GPIO);
		return PTR_ERR(fp);
	}
	filp_close(fp, NULL);

	// - auto_freq for automatic frequency adjustment (write-only for daemon, read-only for module)
	fp = filp_open(AUTO_FREQ_PATH, O_WRONLY | O_CREAT, 0666);
	if (IS_ERR(fp)) {
		pr_err ("Failed to create auto_freq file\n");
		gpio_free(STATUS_LED_GPIO);
		return PTR_ERR(fp);
	}
	filp_close(fp, NULL);

	// Start blinking the status LED in a separate kernel thread
	blink_thread = kthread_run(blink_status_led, NULL, "blink_thread");
	if (IS_ERR(blink_thread)) {
		pr_err ("Failed to create blink thread\n");
		gpio_free(STATUS_LED_GPIO);
		return PTR_ERR(blink_thread);
	}

	// Start the control loop that drives the frequency from mode/temperature
	control_thread = kthread_run(control_loop, NULL, "freq_control");
	if (IS_ERR(control_thread)) {
		pr_err ("Failed to create control thread\n");
		kthread_stop(blink_thread);
		gpio_free(STATUS_LED_GPIO);
		return PTR_ERR(control_thread);
	}

	gpio_set_value(STATUS_LED_GPIO, 1); // Turn on LED to indicate module is loaded

	pr_info ("Linux mini-project module loaded\n");

	return 0;
}

static void __exit skeleton_exit(void)
{
	// Stop the control loop first so it stops driving the frequency / files
	kthread_stop(control_thread);
	// Wake up the blink thread if it's sleeping, then stop it
	kthread_stop(blink_thread);
	wake_up_interruptible(&freq_wq);
	// Remove the sysfs files
	// unlink(SET_FREQ_PATH);
	// unlink(GET_FREQ_PATH);
	// unlink(AUTO_FREQ_PATH);
	// Free the GPIO for the status LED
	gpio_free(STATUS_LED_GPIO);

	pr_info ("Linux mini-project module unloaded\n");
}

module_init (skeleton_init);
module_exit (skeleton_exit);

MODULE_AUTHOR ("CSEL1 - Group 1");
MODULE_DESCRIPTION ("Module skeleton");
MODULE_LICENSE ("GPL");

