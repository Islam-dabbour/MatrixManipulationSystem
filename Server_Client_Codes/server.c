#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <time.h>

void error(const char *msg){
    perror(msg);
    exit(EXIT_FAILURE);
}

void *handle_client(void *socket_pointer){
    int client_sock = *(int *)socket_pointer;
    char buffer[1024];
    ssize_t bytes_received;

    free(socket_pointer);
    printf("[+] Client connected.\n");

    /* Keep the connection owned by this thread until the client disconnects. */
    while((bytes_received = recv(client_sock, buffer, sizeof(buffer), 0)) > 0){
        (void)bytes_received;
    }

    if(bytes_received < 0){
        perror("Receive Error");
    }

    close(client_sock);
    printf("[-] Client disconnected.\n");
    return NULL;
}

int main(int argc, char **argv){

    if(argc != 2){
        printf("You need to provide a port number\n");
        exit(0);
    }

    int port = atoi(argv[1]);

    int server_sock;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_size;
    int reuse_address = 1;

    server_sock = socket(AF_INET, SOCK_STREAM, 0);

    if(server_sock < 0){
        perror("Socket Error");
        exit(1);
    }

    printf("[+] Server socket created.\n");

    if(setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR,
                  &reuse_address, sizeof(reuse_address)) < 0)
    {
        error("Setsockopt Error");
    }

    memset(&server_addr, '\0', sizeof(server_addr));

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if(bind(server_sock,
           (struct sockaddr*)&server_addr,
           sizeof(server_addr)) < 0)
    {
        perror("Bind Error");
        exit(1);
    }

    printf("[+] Bound to port %d\n", port);

    if(listen(server_sock, 5) < 0){
        error("Listen Error");
    }

    printf("Listening. Press Ctrl+C to stop the server.\n");

    while(1){
        pthread_t client_thread;
        int *client_sock = malloc(sizeof(*client_sock));

        if(client_sock == NULL){
            perror("Memory Allocation Error");
            continue;
        }

        addr_size = sizeof(client_addr);
        *client_sock = accept(server_sock, (struct sockaddr *)&client_addr, &addr_size);

        if(*client_sock < 0){
            perror("Accept Error");
            free(client_sock);
            continue;
        }

        if(pthread_create(&client_thread, NULL, handle_client, client_sock) != 0){
            perror("Thread Creation Error");
            close(*client_sock);
            free(client_sock);
            continue;
        }

        pthread_detach(client_thread);
    }

    close(server_sock);
    return EXIT_SUCCESS;
 }