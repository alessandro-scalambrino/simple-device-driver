#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/kdev_t.h> /* macros minor - major */
#include <linux/uaccess.h> /* copy_to_user  - copy_from_user */

#undef pr_fmt
#define pr_fmt(fmt) "%s: " fmt, __func__ /* modified pr_info formatting stuff */

#define DEV_MEM_SIZE 512

char device_buffer[DEV_MEM_SIZE]; /* Buffer for the device */

/* Device number */
dev_t device_number;

/* VFS registration variables */
struct cdev pcd_cdev;

// Struct file_operations from linKern
loff_t pcd_llseek(struct file *filp, loff_t offset, int whence) {
    /* 
     * Handle the file position adjustment 
     * when seeking in the device.
     */

    loff_t temp;
    pr_info("pcd_llseek requested\n");
    pr_info("Current file position: %lld\n", filp->f_pos);


    switch(whence) {
        case SEEK_SET:
            if ((offset > DEV_MEM_SIZE ) || (offset < 0 )) {
                return -EINVAL;
            }
            filp->f_pos = offset;
            break;
        case SEEK_CUR:
            temp = filp->f_pos + offset;
            if ((temp > DEV_MEM_SIZE) || (temp < 0)) { 
                return -EINVAL;
            }
            filp->f_pos = temp;
            break;
        case SEEK_END:
            temp = DEV_MEM_SIZE + offset;
            if ((temp > DEV_MEM_SIZE) || (temp < 0)) {
                return -EINVAL;
            }
            filp->f_pos = temp;
            break;
        default:
            return -EINVAL;
    }

    pr_info("pcd_llseek completed\n");
    pr_info("New file position: %lld\n", filp->f_pos);
    return filp->f_pos;

}

ssize_t pcd_read(struct file *filp, char __user *buff, size_t count, loff_t *f_pos) {
    /* 
     * Handle read requests from user space.
     * - Verify if the requested read exceeds the buffer size.
     * - If it does, adjust the count to read only available data.
     */
    pr_info("pcd_read requested read of: %zd bytes\n", count);
    pr_info("initial file position before read: %lld bytes\n",  *f_pos);

    /* 
     * Check if the read request exceeds the DEV_MEM_SIZE (max buffer size) 
     */
    if (*f_pos + count > DEV_MEM_SIZE) {
        /* 
         * Readjust count --> the user can read till the end of buffer 
         */
        count = DEV_MEM_SIZE - *f_pos;  
    }

    /* 
     * Copy data from kernel to user space buffer. 
     * Use copy_to_user to safely write in user space 
     * (check *buff validity).
     */
    if (copy_to_user(buff, &device_buffer[*f_pos], count)) {
        pr_info("error copying file from kernel to usr space buffer");
        return -EFAULT; 
    }
       
    *f_pos += count; /* Update file position */
    
    pr_info("pcd_read: %zd bytes\n", count);
    pr_info("updated file position after read: %lld bytes\n",  *f_pos);

    return count; /* Return the number of bytes read */
}

ssize_t pcd_write(struct file *filp, const char __user *buff, size_t count, loff_t *f_pos) {
    /* 
     * Handle write requests from user space.
     * - Read data from user space into the device buffer.
     */
    pr_info("pcd_write requested write of: %zd bytes\n", count);
    pr_info("initial file position before write: %lld\n",  *f_pos);


    /* 
     * Copy data from user space to the kernel buffer.
     * Use copy_from_user to safely read from user space.
     */
    if (*f_pos + count > DEV_MEM_SIZE) {
        /* 
         * Readjust count --> the user can read till the end of buffer 
         */
        count = DEV_MEM_SIZE - *f_pos;  
    }

    if (!count) {
        return -ENOMEM;
    }

    if (copy_from_user(&device_buffer[*f_pos], buff, count)) {
        pr_info("error copying file from usr space buffer to kernel");
        return -EFAULT; 
    }
    
    *f_pos += count; /* Update file position */
    
    pr_info("pcd_write: %zd bytes\n", count);
    pr_info("updated file position after write: %lld\n",  *f_pos);
    
    return count; /* Return the number of bytes written */
}

int pcd_open(struct inode *inode, struct file *filp) {
    /* 
     * Handle the opening of the device file.
     * - Initialize necessary resources for the device.
     */
    pr_info("pcd_open successfull\n");
    return 0;
}

int pcd_release(struct inode *inode, struct file *filp) {
    /* 
     * Handle the closing of the device file.
     * - Clean up resources allocated for the device.
     */
    pr_info("pcd_release successfull\n");
    return 0;
}

/* 
 * File operations of the driver definition and in-place initialization 
 */
struct file_operations pcd_fops = {
    .llseek = pcd_llseek,
    .read = pcd_read,
    .write = pcd_write,
    .open = pcd_open,
    .release = pcd_release,
    .owner = THIS_MODULE
};

struct class *class_pcd; /* Device class */
struct device *device_pcd; /* Device structure */

static int __init pcd_driver_init(void) {
    /* 
     * Module initialization routine:
     * 1. Dynamically allocate a device number.
     * 2. Initialize the character device structure.
     * 3. Register the device with the kernel VFS.
     * 4. Create a device class under /sys/class.
     * 5. Populate the sysfs with device information.
     */
    
    int ret = 0;
    /* 1. Dynamically allocate device number */
    ret = alloc_chrdev_region(&device_number, 0, 1, "pcd_devices");
   
    if (ret<0) {
        pr_err ("Cdev alloc failed\n");
        goto out;
    }
    pr_info("Device number ‹major> : <minor> = %d:%d\n", MAJOR(device_number), MINOR(device_number));

    /* 2. Initialize the cdev struct and opstruct */
    cdev_init(&pcd_cdev, &pcd_fops);
    
    /* 3. Add device to the kernel VFS */
    pcd_cdev.owner = THIS_MODULE;
    ret = cdev_add(&pcd_cdev, device_number, 1);
     if (ret<0) {
        pr_err ("Cdev add failed\n");
        goto unreg_chrdev;
     }
    /* 4. Create device class under /sys/class */
    class_pcd = class_create(THIS_MODULE, "pcd_class");
    if (IS_ERR(class_pcd)) {
        pr_err ("Class creation failed\n");
        ret = PTR_ERR(class_pcd);
        goto cdev_del;
    }
    /* 5. Populate the sysfs with device information */
    device_pcd = device_create(class_pcd, NULL, device_number, NULL, "pcd");
    if (IS_ERR(device_pcd)) {
        pr_err ("Device creation failed\n");
        ret = PTR_ERR(device_pcd);
        goto class_del;
    }
    
    pr_info("Module initialization SUCCESSFUL\n");

    return 0;

class_del:
    class_destroy(class_pcd); /* Destroy the class */

cdev_del:
    cdev_del(&pcd_cdev); /* Delete the character device */



unreg_chrdev:
    unregister_chrdev_region(device_number, 1); /* Unregister the device number */


out:
    pr_info("Module insertion failed");
    return ret;

}

static void __exit pcd_driver_cleanup(void) {
    /* 
     * Module cleanup routine:
     * - Inverse process of initialization.
     * - Release resources allocated during initialization.
     */
    /* Inverse process */
    device_destroy(class_pcd, device_number); /* Destroy the device */
    class_destroy(class_pcd); /* Destroy the class */
    cdev_del(&pcd_cdev); /* Delete the character device */
    unregister_chrdev_region(device_number, 1); /* Unregister the device number */
    pr_info("Module unload SUCCESSFUL\n");
}

module_init(pcd_driver_init);
module_exit(pcd_driver_cleanup);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Alessandro Scalambrino");
MODULE_DESCRIPTION("Simple char device driver");
MODULE_INFO(host, "Linux 24...");
