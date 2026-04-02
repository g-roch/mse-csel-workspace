/* skeleton.c */
#include <linux/module.h>		/* needed by all modules */
#include <linux/init.h>			/* needed for macros */
#include <linux/kernel.h>		/* needed for debugging */

#include <linux/kthread.h>		/* needed for kernel thread management */
#include <linux/wait.h>		/* needed for waitqueues handling */
#include <linux/delay.h>		/* needed for delay fonctions */
#include <linux/gpio.h>		/* needed for GPIO handling */
#include <linux/interrupt.h>	/* needed for interrupt handling */

static irqreturn_t k1_handler(int irq, void *dev_id);
static irqreturn_t k2_handler(int irq, void *dev_id);
static irqreturn_t k3_handler(int irq, void *dev_id);


static int __init skeleton_init(void)
{
	pr_info ("Linux module MA_CSEL-02-08 loaded\n");

	/* Declare ahead to avoid mixed declarations/code warnings */
	int k1_irq;
	int k2_irq;
	int k3_irq;

	// Acquire GPIO pins
	// - k1
	gpio_request(0, "k1");
	// - k2
	gpio_request(2, "k2");
	// - k3
	gpio_request(3, "k3");

	// Get interrupton vectors for GPIO pins
	k1_irq = gpio_to_irq(0);
	k2_irq = gpio_to_irq(2);
	k3_irq = gpio_to_irq(3);

	// Print interrupt vectors for debugging
	pr_info("k1_irq: %d\n", k1_irq);
	pr_info("k2_irq: %d\n", k2_irq);
	pr_info("k3_irq: %d\n", k3_irq);

	// Setup interrupt handlers for GPIO pins
	request_irq(k1_irq, k1_handler, IRQF_TRIGGER_FALLING, "k1_handler", NULL);
	request_irq(k2_irq, k2_handler, IRQF_TRIGGER_FALLING, "k2_handler", NULL);
	request_irq(k3_irq, k3_handler, IRQF_TRIGGER_FALLING, "k3_handler", NULL);

	return 0;
}

static irqreturn_t k1_handler(int irq, void *dev_id)
{
	// Print message when k1 is pressed
	pr_info("k1 pressed\n");
	return IRQ_HANDLED;
}

static irqreturn_t k2_handler(int irq, void *dev_id)
{
	// Print message when k2 is pressed
	pr_info("k2 pressed\n");
	return IRQ_HANDLED;
}

static irqreturn_t k3_handler(int irq, void *dev_id)
{
	// Print message when k3 is pressed
	pr_info("k3 pressed\n");
	return IRQ_HANDLED;
}

static void __exit skeleton_exit(void)
{
	// Release GPIO pins
	gpio_free(0);
	gpio_free(2);
	gpio_free(3);

	// Free interrupt handlers for GPIO pins
	free_irq(gpio_to_irq(0), NULL);
	free_irq(gpio_to_irq(2), NULL);
	free_irq(gpio_to_irq(3), NULL);

	pr_info ("Linux module MA_CSEL-02-08 unloaded\n");
}

module_init (skeleton_init);
module_exit (skeleton_exit);

MODULE_AUTHOR ("Carolina Pereira <carolina.pereira@master.hes-so.ch>");
MODULE_DESCRIPTION ("MA_CSEL - Lab 2 - Exercise 8");
MODULE_LICENSE ("GPL");

