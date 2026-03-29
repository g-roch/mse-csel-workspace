/* skeleton.c */
#include <linux/cdev.h>        /* needed for char device driver */
#include <linux/fs.h>          /* needed for device drivers */
#include <linux/init.h>        /* needed for macros */
#include <linux/kernel.h>      /* needed for debugging */
#include <linux/module.h>      /* needed by all modules */
#include <linux/moduleparam.h> /* needed for module parameters */
#include <linux/slab.h>        /* needed for dynamic memory management */
#include <linux/uaccess.h>     /* needed to copy data to/from user */

// This is the kernel param for the nb of instances
static int instances = 3;
module_param(instances, int, 0);
// ---

// Here the buffer size constant is the same
#define BUFFER_SZ 10000
// But here we need multiple bufferse, one for each instance we got
static char** buffers = 0;
// ---

// This is called when one of the supported device files is opened
static int skeleton_open(struct inode* i, struct file* f)
{
    // Same as before, we print the major and the minor
    pr_info("skeleton : open operation... major:%d, minor:%d\n",
            imajor(i),
            iminor(i));
    // ---

    // But here we error if the user is trying to access an instance that is not supported
    if (iminor(i) >= instances) {
        return -EFAULT;
    }
    // ---

    // Log the mode in which the file is opened (read, write or both)
    if ((f->f_mode & (FMODE_READ | FMODE_WRITE)) != 0) {
        pr_info("skeleton : opened for reading & writing...\n");
    } else if ((f->f_mode & FMODE_READ) != 0) {
        pr_info("skeleton : opened for reading...\n");
    } else if ((f->f_mode & FMODE_WRITE) != 0) {
        pr_info("skeleton : opened for writing...\n");
    }
    // ---

    // Here we write the corresponding buffer address into the private data of the file     
    f->private_data = buffers[iminor(i)];
    // And we log the content of the private data
    pr_info("skeleton: private_data=%p\n", f->private_data);
    // ---

    return 0;
}

// This is called when the device file is closed
static int skeleton_release(struct inode* i, struct file* f)
{
    pr_info("skeleton: release operation...\n");

    return 0;
}

// This is called when the device file is read
static ssize_t skeleton_read(struct file* f,
                             char __user* buf,
                             size_t count,
                             loff_t* off)
{
    // Same as the previous version: compute remaining bytes to copy, update count and pointers
    // We don't need to adapt according to the instance, 
    // because the private data is already set to the right buffer in the open function
    ssize_t remaining = BUFFER_SZ - (ssize_t)(*off);
    char* ptr         = (char*)f->private_data + *off;
    if (count > remaining) count = remaining;
    *off += count;
    // ---

    // Copy the required number of bytes
    if (copy_to_user(buf, ptr, count) != 0) count = -EFAULT;
    // Log the read operation and the number of bytes read
    pr_info("skeleton: read operation... read=%ld\n", count);
    // ---

    return count;
}

// This is called when the device file is written
static ssize_t skeleton_write(struct file* f,
                              const char __user* buf,
                              size_t count,
                              loff_t* off)
{
    // Same thing as for the read operation: compute remaining space in buffer and update pointers
    // Again, we don't need to adapt according to the instance, because the private data 
    // is already set to the right buffer in the open function
    ssize_t remaining = BUFFER_SZ - (ssize_t)(*off);
    // ---
    
    // Check if still remaining space to store additional bytes
    if (count >= remaining) count = -EIO;
    // ---

    // Store additional bytes into internal buffer
    // This works because we have a pointer to the buffer, it's not a copy
    if (count > 0) {
        char* ptr = f->private_data + *off;
        *off += count;
        ptr[count] = 0;  // make sure string is null terminated
        if (copy_from_user(ptr, buf, count)) count = -EFAULT;
    }
    // ---

    // Finally, we log the write operation and the number of bytes written, 
    // as well as the content of the private data
    pr_info("skeleton: write operation... private_data=%p, written=%ld\n",
            f->private_data,
       
            count);
    // ---

    return count;
}

// Same as before: define the file operations
static struct file_operations skeleton_fops = {
    .owner   = THIS_MODULE,
    .open    = skeleton_open,
    .read    = skeleton_read,
    .write   = skeleton_write,
    .release = skeleton_release,
};

// Here as well, nothing changed
static dev_t skeleton_dev;
static struct cdev skeleton_cdev;
// ---

// This is called when the module is loaded
static int __init skeleton_init(void)
{
    // Here we need to adapt to support i instances
    int i;
    int status = -EFAULT;
    // And also make sure a valid number of instances is provided
    if (instances <= 0) return -EFAULT;
    // ---

    // Here we also need to allocate the required number of device numbers instead of just one
    status = alloc_chrdev_region(&skeleton_dev, 0, instances, "mymodule");
    // Same below, we need to add the required number of cdevs instead of just one
    if (status == 0) {
        cdev_init(&skeleton_cdev, &skeleton_fops);
        skeleton_cdev.owner = THIS_MODULE;
        status              = cdev_add(&skeleton_cdev, skeleton_dev, instances);
    }
    // ---

    // If everything goes well, we allocate the required number of buffers, one for each instance
    if (status == 0) {
        buffers = kzalloc(sizeof(char*) * instances, GFP_KERNEL);
        for (i = 0; i < instances; i++)
            buffers[i] = kzalloc(BUFFER_SZ, GFP_KERNEL);
    }
    // ---

    // Finally, we log the loading of the module and the number of instances
    pr_info("Linux module skeleton loaded\n");
    pr_info("The number of instances: %d\n", instances);
    // ---
    return status;
}

// This is called when the module is unloaded
static void __exit skeleton_exit(void)
{
    int i;

    // Here we only have to delete and unregister one cdev and one device number, 
    // because we allocated them for all the instances together
    cdev_del(&skeleton_cdev);
    unregister_chrdev_region(skeleton_dev, instances);
    // ---

    // But we need to free all the allocated buffers, one for each instance
    for (i = 0; i < instances; i++) kfree(buffers[i]);
    // And also the array of buffers itself
    kfree(buffers);
    // ---

    pr_info("Linux module skeleton unloaded\n");
}

module_init(skeleton_init);
module_exit(skeleton_exit);

MODULE_AUTHOR("Daniel Gachet <daniel.gachet@hefr.ch>");
MODULE_DESCRIPTION("Module skeleton");
MODULE_LICENSE("GPL");
