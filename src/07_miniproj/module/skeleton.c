/* skeleton.c */
#include <linux/module.h>	/* needed by all modules */
#include <linux/init.h>		/* needed for macros */
#include <linux/kernel.h>	/* needed for debugging */

#include <linux/gpio.h>			/* needed for i/o handling */

#include "../button_control/config.h"

#define STATUS_LED_GPIO	0 // TODO: Find correct GPIO pin for the status LED

static char* status_led="status_led";

int set_freq_fd, get_freq_fd, auto_freq_fd;

// Get interval in ms to switch the LED, given a 50% duty cycle and the frequency in Hz
static int get_switch_interval(int freq) {
	if (freq <= 0) {
		pr_err ("Invalid frequency: %d. Must be > 0.\n", freq);
		return -1;
	}
	return (1000 / freq) / 2; // Convert frequency to period in ms, then divide by 2 for 50% duty cycle
}

static int __init skeleton_init(void)
{
	// Install status LED
	status = gpio_request(STATUS_LED_GPIO, "status_led");
	if (status < 0) {
		pr_err ("Failed to request GPIO %d for status LED\n", STATUS_LED_GPIO);
		return status;
	}

	// Create the necessary sysfs files if not present
	// - set_freq for setting the frequency (read-only)
	set_freq_fd = open(SET_FREQ_PATH, O_RDONLY | O_CREAT, 0666);
	if (set_freq_fd == -1) {
		pr_err ("Failed to create set_freq file\n");
		gpio_free(STATUS_LED_GPIO);
		return -1;
	}
	close(set_freq_fd);

	// - get_freq for reading the current frequency (read-write)
	get_freq_fd = open(GET_FREQ_PATH, O_RDWR | O_CREAT, 0666);
	if (get_freq_fd == -1) {
		pr_err ("Failed to create get_freq file\n");
		gpio_free(STATUS_LED_GPIO);
		return -1;
	}
	close(get_freq_fd);

	// - auto_freq for automatic frequency adjustment (read-only)
	auto_freq_fd = open(AUTO_FREQ_PATH, O_RDONLY | O_CREAT, 0666);
	if (auto_freq_fd == -1) {
		pr_err ("Failed to create auto_freq file\n");
		gpio_free(STATUS_LED_GPIO);
		return -1;
	}
	close(auto_freq_fd);

	pr_info ("Linux mini-project module loaded\n");

	// TEST: Wait queue 
	// wait_queue_head_t* queue;
	// init_waitqueue_head (queue);

	// wait_event_itimeout(queue, condition, 100); // Wait for condition to become true or timeout after 100 ms

	return status;
}

static void __exit skeleton_exit(void)
{
	gpio_free(STATUS_LED_GPIO);

	pr_info ("Linux mini-project module unloaded\n");
}

module_init (skeleton_init);
module_exit (skeleton_exit);

MODULE_AUTHOR ("CSEL1 - Group 1");
MODULE_DESCRIPTION ("Module skeleton");
MODULE_LICENSE ("GPL");

