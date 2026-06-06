/* skeleton.c */
#include <linux/module.h>	/* needed by all modules */
#include <linux/init.h>		/* needed for macros */
#include <linux/kernel.h>	/* needed for debugging */

#include <linux/gpio.h>		/* needed for i/o handling */
#include <linux/kthread.h>	/* needed for kthread_run/kthread_stop */
#include <linux/fs.h>		/* needed for filp_open/filp_close */

#include "../button_control/config.h"

#define STATUS_LED_GPIO	10 // TODO: Find correct GPIO pin for the status LED

static DECLARE_WAIT_QUEUE_HEAD(freq_wq);
atomic_t current_freq = ATOMIC_INIT(1); // Default frequency of 1 Hz

static struct task_struct *blink_thread;

// Get interval in ms to switch the LED, given a 50% duty cycle and the frequency in Hz
static int get_switch_interval(int freq) {
	if (freq <= 0) {
		pr_err ("Invalid frequency: %d. Must be > 0.\n", freq);
		return -1;
	}
	return (1000 / freq) / 2; // Convert frequency to period in ms, then divide by 2 for 50% duty cycle
}

static int blink_status_led(void *data) {
	int led_state = 0; // 0 = off, 1 = on
	while (!kthread_should_stop()) {
		unsigned int hz = atomic_read(&current_freq);
		long timeout;

		// if (hz == 0) {
		// 	gpio_set_value(STATUS_LED_GPIO, 0); // Turn off LED if frequency is 0
		// 	// Sleep indefinitely until frequency changes or module is unloaded
		// 	wait_event_interruptible(freq_wq, atomic_read(&current_freq) != 0 || kthread_should_stop());
		// 	continue;
		// }

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

	gpio_set_value(STATUS_LED_GPIO, 1); // Turn on LED to indicate module is loaded

	pr_info ("Linux mini-project module loaded\n");

	return 0;
}

static void __exit skeleton_exit(void)
{
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

