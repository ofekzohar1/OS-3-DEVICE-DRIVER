#include <fcntl.h>      /* open */
#include <unistd.h>     /* exit */
#include <sys/ioctl.h>  /* ioctl */
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include "message_slot.h"

int main(int argc, char **argv) {
    int file_desc;
    int ret_len;
    unsigned int ch_id;
    char **end_ptr, msg_read[MSG_MAX_LEN];

    if (argc < 3) {
        fprintf(stderr, "# of arguments should be at least 3: %s.", strerror(EINVAL));
        exit(EXIT_FAILURE);
    }
    ch_id = (unsigned int) atol(argv[2]);

    file_desc = open(argv[1], O_RDWR);
    if (file_desc < 0) {
        perror("Open failed")
        exit(EXIT_FAILURE);
    }

    if (rioctl(file_desc, MSG_SLOT_CHANNEL, ch_id) != 0) {
        perror("Channel assign failed")
        exit(EXIT_FAILURE);
    }

    ret_len = read(file_desc, &msg_read, MSG_MAX_LEN);
    if (ret_len < 0) {
        perror("Read failed")
        exit(EXIT_FAILURE);
    }
    close(file_desc);

    if (write(STDOUT_FILENO, msg_read, ret_len) != ret_len) {
        fprintf(stderr, "Write to stdout failed.", strerror(ENOSPC));
        exit(EXIT_FAILURE);
    }

    return SUCCESS;
}
