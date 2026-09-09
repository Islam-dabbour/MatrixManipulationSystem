#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

int main(int argc, char **argv)
{
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <request_fd> <response_fd>\n", argv[0]);
        return EXIT_FAILURE;
    }

    int request_fd = atoi(argv[1]);
    int response_fd = atoi(argv[2]);
    char request;
    char timestamp[64];

    while (read(request_fd, &request, sizeof request) > 0) {
        time_t now = time(NULL);
        struct tm current_time;

        if (localtime_r(&now, &current_time) == NULL ||
            strftime(timestamp, sizeof timestamp, "%Y-%m-%d %H:%M:%S",
                     &current_time) == 0) {
            perror("create timestamp");
            break;
        }

        if (write(response_fd, timestamp, sizeof timestamp) !=
            sizeof timestamp) {
            perror("write timestamp");
            break;
        }
    }

    close(request_fd);
    close(response_fd);
    return EXIT_SUCCESS;
}