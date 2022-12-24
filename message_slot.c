// Declare what kind of code we want
// from the header files. Defining __KERNEL__
// and MODULE allows us to access kernel-level
// code not usually available to userspace programs.
#undef __KERNEL__
#define __KERNEL__
#undef MODULE
#define MODULE


#include <linux/kernel.h>   /* We're doing kernel work */
#include <linux/module.h>   /* Specifically, a module */
#include <linux/fs.h>       /* for register_chrdev */
#include <linux/uaccess.h>  /* for get_user and put_user */
#include <linux/string.h>   /* for memset. NOTE - not string.h!*/
#include <linux/slab.h>     /* for GFP_KERNEL flag */

MODULE_LICENSE("GPL");

//Our custom definitions of IOCTL operations
#include "message_slot.h"
#define MAX_MINOR 256
#define MAX_CHANNELS (1u << 20)

static dev_ch_list* dev_minors[MAX_MINOR];

//================== HELP FUNCTIONS ===========================

int add_new_ch(unsigned int ch, unsigned int minor, dev_ch *tail, dev_ch *res) {
    res = (dev_ch*) kmalloc(sizeof(dev_ch), GFP_KERNEL);
    if (res == NULL) { // Allocation fail
        printk(KERN_ERR "Allocation fail Minor %u channel %u.\n", minor, ch);
        return -ENOMEM;
    }
    res->minor = minor;
    res->id = ch;
    res->next = NULL;
    res->msg_len = 0; // No msg written in the channel
    tail->next = res;

    return SUCCESS;
}

int find_ch_in_minor(unsigned int ch, unsigned int minor, dev_ch *res) {
    dev_ch *prev;

    res = dev_minors[minor]->head;
    while (res != NULL && res->id != ch) {
        prev = res;
        res = res->next;
    }
    if (res == NULL) {
        if (dev_minors[minor]->len == MAX_CHANNELS) {
            printk(KERN_ERR "Can't use channel %u. # of channels in minor %u has exceeded the limit.\n", ch,  minor);
            return -EINVAL;
        }
        return add_new_ch(ch, minor, prev, res);
    }
    return SUCCESS;
}

//================== DEVICE FUNCTIONS ===========================
static int device_open(struct inode *inode, struct file *file) {
    unsigned int minor;
    dev_ch_list* new_minor;
    dev_ch *zero_ch;

    minor = iminor(inode);
    printk("Invoking device_open(%p) with minor #: %u.\n", file, minor);

    if (dev_minors[minor] == NULL) { // New minor == new device
        printk("Add new Message Slot device. Minor #: %u.\n", minor);
        new_minor = (dev_ch_list*) kmalloc(sizeof(dev_ch_list), GFP_KERNEL);
        zero_ch = (dev_ch) kmalloc(sizeof(dev_ch), GFP_KERNEL);
        if (new_minor == NULL || zero_ch == NULL) { // Allocation fail
            printk(KERN_ERR "Allocation fail Minor #: %u.\n", minor);
            return -ENOMEM;
        }
        zero_ch->minor = minor;
        zero_ch->id = 0;
        new_minor->head = zero_ch;
        new_minor->len = 0;
    }
    file->private_data = (void *)dev_minors[minor]->head; // Uninitialized channel == zero channel

    printk("Open succeeded in file %p.\n", file);
    return SUCCESS;
}

//---------------------------------------------------------------
static int device_release(struct inode *inode, struct file *file) {
    unsigned int minor;

    minor = iminor(inode);
    printk("Invoking device_release(%p,%p) with minor #: %u.\n", inode, file, minor);

    return SUCCESS;
}

//---------------------------------------------------------------
// a process which has already opened
// the device file attempts to read from it
static ssize_t device_read(struct file *file, char __user* buffer, size_t length, loff_t* offset) {
    ssize_t i;
    dev_ch *read_ch;

    printk("Invoking device_read(%p,%ld).\n", file, length);
    if (length == 0 || length > MSG_MAX_LEN) {
    printk(KERN_ERR "Invalid message length: %d.\n", length);
    return -EMSGSIZE;
    }

    read_ch = (dev_ch *) file->private_data;
    if (write_ch == NULL) { // Unopened file
        printk(KERN_ERR "Unopened file %p.\n", file);
        return -EBADF;
    }
    if (read_ch->id == 0) { // Unset channel
        printk(KERN_ERR "Unset channel in file %p.\n", file);
        return -EINVAL;
    }
    if (read_ch->msg_len == 0) { // No msg in the channel
        printk(KERN_ERR "No msg in channel %u for file %p.\n", read_ch->id, file);
        return -EWOULDBLOCK;
    }
    if (read_ch->msg_len > length) { // User buffer length too small
        printk(KERN_ERR "file %p: User buffer length (%d) too small to hold msg (%u).\n", file, length, read_ch->msg_len);
        return -ENOSPC;
    }

    for(i = 0; i < length; ++i) { // Write to user buffer
        if (put_user(read_ch->msg[i], &buffer[i]) < 0) {
            printk(KERN_ERR "Error writing to user buffer in file %p.\n", file);
            return -EFAULT;
        }
    }

    printk("Read succeeded in file %p.\n", file);
    return length; // On success return the red msg length
}

//---------------------------------------------------------------
// A process which has already opened
// the device file attempts to write to it
static ssize_t device_write(struct file *file, const char __user* buffer, size_t length, loff_t* offset) {
    ssize_t i;
    char temp_buf[MSG_MAX_LEN];
    dev_ch *write_ch;

    printk("Invoking device_write(%p,%ld).\n", file, length);
    if (length == 0 || length > MSG_MAX_LEN) {
        printk(KERN_ERR "Invalid message length: %d.\n", length);
        return -EMSGSIZE;
    }

    write_ch = (dev_ch *) file->private_data;
    if (write_ch == NULL) { // Unopened file
        printk(KERN_ERR "Unopened file %p.\n", file);
        return -EBADF;
    }
    if (write_ch->id == 0) { // Unset channel
        printk(KERN_ERR "Unset channel in file %p.\n", file);
        return -EINVAL;
    }

    for(i = 0; i < length; ++i) { // Read from user buffer
        if (get_user(temp_buf[i], &buffer[i]) < 0) {
            printk(KERN_ERR "Error reading from user buffer in file %p.\n", file);
            return -EFAULT;
        }
    }

    for(i = 0; i < length; ++i) { // Write to channel
        write_ch->msg[i] = temp_buf[i];
    }
    write_ch->msg_len = length;

    printk("Write succeeded in file %p.\n", file);
    return length; // On success return the written msg length
}

//----------------------------------------------------------------
static long device_ioctl(struct file *file, unsigned int ioctl_command_id, unsigned long ch_id) {
    unsigned int minor;
    int find_res;
    dev_ch *ch;

    printk("Invoking ioctl: setting file %p with channel %u.\n", file, ch_id);
    if (ioctl_command_id != MSG_SLOT_CHANNEL) {
        printk(KERN_ERR "Unrecognized command id: %u.\n", ioctl_command_id);
        return -EINVAL;
    }
    if (ch_id == 0) {
        printk(KERN_ERR "Channel zero.\n");
        return -EINVAL;
    }

    // Get the parameter given to ioctl by the process
    ch = (dev_ch *) file->private_data;
    find_res = find_ch_in_minor(ch_id, ch->minor, ch);
    if (find_res < 0)
        return find_res;
    file->private_data = (void *) ch;

    printk("Ioctl succeeded in file %p.\n", file);
    return SUCCESS;
}

//==================== DEVICE SETUP =============================

// This structure will hold the functions to be called
// when a process does something to the device we created
struct file_operations Fops = {
        .owner      = THIS_MODULE,
        .read           = device_read,
        .write          = device_write,
        .open           = device_open,
        .unlocked_ioctl = device_ioctl,
        .release        = device_release,
};

//---------------------------------------------------------------
// Initialize the module - Register the character device
static int __init simple_init(void) {
    int rc, i;

    // Register driver capabilities. Obtain major num
    rc = register_chrdev(MAJOR_NUM, DEVICE_NAME, &Fops);

    // Negative values signify an error
    if (rc < 0) {
        printk(KERN_ALERT "%s registration failed for %d.\n", DEVICE_NAME, MAJOR_NUM);
        return rc;
    }

    printk("Registration is successful. Device: %s, Major: %d.\n", DEVICE_NAME, MAJOR_NUM);
    for (i=0; i < MAX_MINOR; i++) {
        dev_minors[i] = NULL;
    }

    return SUCCESS;
}

//---------------------------------------------------------------
static void __exit simple_cleanup(void) {
    // Unregister the device Should always succeed
    int i;
    dev_ch *curr, *next;

    for (i=0; i< MAX_MINOR; ++i) {
        if (dev_minors[i] == NULL) continue;
        curr = dev_minors[i]->head;
        while (curr != NULL) {
            next = curr->next;
            kfree(curr);
            curr = next;
        }
        kfree(dev_minors[i]);
    }
    unregister_chrdev(MAJOR_NUM, DEVICE_NAME);
}

//---------------------------------------------------------------
module_init(simple_init);
module_exit(simple_cleanup);

//========================= END OF FILE =========================
