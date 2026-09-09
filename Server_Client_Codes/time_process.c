#define _POSIX_C_SOURCE 200809L

#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

static int get_network_timestamp(char *timestamp, size_t timestamp_size)
{
    const char *server_name = "pool.ntp.org";
    const char *service = "123";
    unsigned char request[48] = {0};
    unsigned char response[48];
    struct addrinfo hints = {0};
    struct addrinfo *addresses = NULL;
    struct addrinfo *address;
    struct timeval timeout = {5, 0};
    int socket_fd = -1;
    int result;

    request[0] = 0x1b;
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_protocol = IPPROTO_UDP;

    result = getaddrinfo(server_name, service, &hints, &addresses);
    if (result != 0) {
        fprintf(stderr, "NTP address lookup failed: %s\n",
                gai_strerror(result));
        return -1;
    }

    for (address = addresses; address != NULL; address = address->ai_next) {
        socket_fd = socket(address->ai_family, address->ai_socktype,
                           address->ai_protocol);
        if (socket_fd < 0) {
            continue;
        }

        setsockopt(socket_fd, SOL_SOCKET, SO_RCVTIMEO, &timeout,
                   sizeof timeout);

        if (sendto(socket_fd, request, sizeof request, 0,
                   address->ai_addr, address->ai_addrlen) == sizeof request &&
            recvfrom(socket_fd, response, sizeof response, 0, NULL, NULL) >=
                (ssize_t)sizeof response) {
            break;
        }

        close(socket_fd);
        socket_fd = -1;
    }

    freeaddrinfo(addresses);

    if (socket_fd < 0) {
        fprintf(stderr, "NTP server could not be reached\n");
        return -1;
    }

    close(socket_fd);

    uint32_t ntp_seconds = ((uint32_t)response[40] << 24) |
                           ((uint32_t)response[41] << 16) |
                           ((uint32_t)response[42] << 8) |
                           (uint32_t)response[43];
    const uint32_t ntp_to_unix_seconds = 2208988800U;

    if (ntp_seconds < ntp_to_unix_seconds) {
        fprintf(stderr, "NTP server returned an invalid timestamp\n");
        return -1;
    }

    time_t unix_seconds = (time_t)(ntp_seconds - ntp_to_unix_seconds);
    struct tm current_time;

    if (gmtime_r(&unix_seconds, &current_time) == NULL ||
        strftime(timestamp, timestamp_size, "%Y-%m-%d %H:%M:%S",
                 &current_time) == 0) {
        fprintf(stderr, "Could not format NTP timestamp\n");
        return -1;
    }

    return 0;
}

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
        if (get_network_timestamp(timestamp, sizeof timestamp) < 0) {
            fprintf(stderr, "time_process: timestamp request failed\n");
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