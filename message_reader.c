#include <fcntl.h>      /* open */
#include <unistd.h>     /* exit */
#include <sys/ioctl.h>  /* ioctl */
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include "message_slot.h" // Device macros

int main(int argc, char **argv) {
    int file_desc;
    int ret_len;
    unsigned int ch_id;
    char msg_read[MSG_MAX_LEN];

    if (argc < 3) {
        fprintf(stderr, "# of arguments should be at least 3: %s.", strerror(EINVAL));
        exit(EXIT_FAILURE);
    }
    ch_id = (unsigned int) atol(argv[2]); // Parse channel id. Assumed non-negative integer.

    file_desc = open(argv[1], O_RDWR); // Open fd
    if (file_desc < 0) {
        perror("Open failed");
        exit(EXIT_FAILURE);
    }

    if (ioctl(file_desc, MSG_SLOT_CHANNEL, ch_id) != 0) { // Set fd's channel
        perror("Channel assign failed");
        exit(EXIT_FAILURE);
    }

    ret_len = read(file_desc, &msg_read, MSG_MAX_LEN); // Read from fd
    if (ret_len < 0) {
        perror("Read failed");
        exit(EXIT_FAILURE);
    }
    close(file_desc);

    if (write(STDOUT_FILENO, msg_read, ret_len) != ret_len) { // Write msg to stdout
        fprintf(stderr, "Write to stdout failed: %s.", strerror(ENOSPC));
        exit(EXIT_FAILURE);
    }

    return SUCCESS;
}
