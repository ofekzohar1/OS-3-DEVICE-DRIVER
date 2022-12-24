#ifndef MESSAGE_SLOT_H
#define MESSAGE_SLOT_H

#include <linux/ioctl.h>

// The major device number.
// We don't rely on dynamic registration
// any more. We want ioctls to know this number at compile time.
#define MAJOR_NUM 235

// Set the message of the device driver
#define MSG_SLOT_CHANNEL _IOW(MAJOR_NUM, 0, unsigned long)

#define DEVICE_NAME "message_slot"
#define MSG_MAX_LEN 128
#define SUCCESS 0

struct dev_ch {
    unsigned int minor;
    unsigned int id;
    char msg[MSG_MAX_LEN];
    unsigned int msg_len;
    struct dev_ch *next;
} typedef dev_ch;

struct dev_ch_list {
    dev_ch *head;
    unsigned int len;
} typedef dev_ch_list;



#endif
