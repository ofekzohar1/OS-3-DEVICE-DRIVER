#ifndef MESSAGE_SLOT_H
#define MESSAGE_SLOT_H

#include <linux/ioctl.h>

#define MAJOR_NUM 235 // The major device number.

// Set the command for the ioctl
#define MSG_SLOT_CHANNEL _IOW(MAJOR_NUM, 0, unsigned long)

#define DEVICE_NAME "message_slot"
#define MSG_MAX_LEN 128 // Message max length
#define SUCCESS 0

#endif
