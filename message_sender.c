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
    unsigned int ch_id;

    if (argc < 4) { // Not enough arguments
        fprintf(stderr, "# of arguments should be at least 4: %s.", strerror(EINVAL));
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

    if (write(file_desc, argv[3], strlen(argv[3])) < 0) { // Write to fd
        perror("Write failed");
        exit(EXIT_FAILURE);
    }

    close(file_desc);
    return SUCCESS;
}
