/* skeleton.c */
#include <linux/init.h>   /* needed for macros */
#include <linux/kernel.h> /* needed for debugging */
#include <linux/module.h> /* needed by all modules */

#include <linux/cdev.h>    /* needed for char device driver */
#include <linux/fs.h>      /* needed for device drivers */
#include <linux/uaccess.h> /* needed to copy data to/from user */

#include <linux/poll.h>  /* needed for polling handling */
#include <linux/sched.h> /* needed for scheduling constants */
#include <linux/wait.h>  /* needed for wating */

#include <linux/gpio.h>      /* needed for i/o handling */
#include <linux/interrupt.h> /* needed for interrupt handling */
#include <linux/io.h>        /* needed for mmio handling */
#include <linux/ioport.h>    /* needed for memory region handling */
#include <linux/miscdevice.h>

// Define GPIO pins for the keyboard
#define K1 0
#define K2 2
#define K3 3

static char* k1 = "gpio_a.0-k1";
static char* k2 = "gpio_a.2-k2";
static char* k3 = "gpio_a.3-k3";
// ---

// Number of interrupts received and wait queue for polling
static atomic_t nb_of_interrupts;
DECLARE_WAIT_QUEUE_HEAD(queue);
// ---

// Handler for the GPIO interrupts
irqreturn_t gpio_isr(int irq, void* handle)
{
    atomic_inc(&nb_of_interrupts);
    wake_up_interruptible(&queue);

    pr_info("interrupt %s raised...\n", (char*)handle);

    return IRQ_HANDLED;
}
// ---

// Define read function for the device (does nothing in this case)
static ssize_t skeleton_read(struct file* f,
                             char __user* buf,
                             size_t sz,
                             loff_t* off)
{
    return 0;
}

// Poll function to wake up the polling thread when an interrupt is received
static unsigned int skeleton_poll(struct file* f, poll_table* wait)
{
    // Mask to be returned to the polling thread
    unsigned mask = 0;
    // Add the wait queue to the poll table
    poll_wait(f, &queue, wait);
    // Check if an interrupt has been received and update the mask accordingly
    if (atomic_read(&nb_of_interrupts) != 0) {
        // Set the mask to indicate that the device is ready for reading (POLLIN | POLLRDNORM)
        mask |= POLLIN | POLLRDNORM; /* read operation */
        /* mask |= POLLOUT | POLLWRNORM;   write operation */

        // Decrement the number of interrupts and print a message
        atomic_dec(&nb_of_interrupts);
        pr_info("polling thread waked-up...\n");
    }
    return mask;
}

// Define file operations for the character device
static struct file_operations skeleton_fops = {
    .owner = THIS_MODULE,
    .read  = skeleton_read,
    .poll  = skeleton_poll,
};

// Define a misc device with the file operations
struct miscdevice misc_device = {
    .minor = MISC_DYNAMIC_MINOR,
    .fops  = &skeleton_fops,
    .name  = "mymodule",
    .mode  = 0777,
};

// Function called when the module is loaded
static int __init skeleton_init(void)
{
    // Init status variable and atomic counter for interrupts
    int status = 0;
    atomic_set(&nb_of_interrupts, 0);
    // ---

    // Register the misc device
    status = misc_register(&misc_device);
    // ---

    // install k1 with falling edge trigger
    if (status == 0)
        status = devm_request_irq(misc_device.this_device,
                                  gpio_to_irq(K1),
                                  gpio_isr,
                                  IRQF_TRIGGER_FALLING | IRQF_SHARED,
                                  k1,
                                  k1);

    // install k2 with rising edge trigger
    if (status == 0)
        status = devm_request_irq(misc_device.this_device,
                                  gpio_to_irq(K2),
                                  gpio_isr,
                                  IRQF_TRIGGER_RISING | IRQF_SHARED,
                                  k2,
                                  k2);

    // install k3 with both edge trigger
    if (status == 0)
        status = devm_request_irq(
            misc_device.this_device,
            gpio_to_irq(K3),
            gpio_isr,
            IRQF_TRIGGER_FALLING | IRQF_TRIGGER_RISING | IRQF_SHARED,
            k3,
            k3);

    pr_info("Linux module skeleton loaded(status=%d)\n", status);
    return status;
}

// Function called when the module is unloaded
static void __exit skeleton_exit(void)
{
    misc_deregister(&misc_device);
    pr_info("Linux module skeleton unloaded\n");
}

module_init(skeleton_init);
module_exit(skeleton_exit);

MODULE_AUTHOR("Daniel Gachet <daniel.gachet@hefr.ch>");
MODULE_DESCRIPTION("Module skeleton");
MODULE_LICENSE("GPL");
