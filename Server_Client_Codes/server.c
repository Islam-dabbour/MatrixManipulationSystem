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

struct Matrix{

    int rows;
    int columns;
    int *arr;
};

struct Matrix createMatrix(int rows, int columns){

    
    struct Matrix matrix = {
        .rows = rows,
        .columns = columns,
        .arr = calloc((size_t)rows * columns, sizeof *matrix.arr)
    };

    return matrix;

}

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

    
    while(1){
       int option;
       printf("=> Waiting For Client Request ...\n");
       read(client_sock, &option, sizeof(int));

       struct Matrix matrixA;
       struct Matrix matrixB;
       int rowsA = 0, columnsB = 0, columnsA = 0, rowsB = 0;

       switch (option)
                {
                case 1:
                   
                    read(client_sock,&rowsA, sizeof(int));
                    read(client_sock,&columnsA, sizeof(int));
                    matrixA = createMatrix(rowsA, columnsA);
                    read(client_sock, matrixA.arr, (size_t)rowsA * columnsA * sizeof matrixA.arr[0]);

                    read(client_sock,&rowsB, sizeof(int));
                    read(client_sock,&columnsB, sizeof(int));
                    matrixB = createMatrix(rowsB, columnsB);
                    read(client_sock, matrixB.arr, (size_t)rowsB * columnsB * sizeof matrixA.arr[0]);

                       
                    mkfifo("multiplication_response", 0666);
                    mkfifo("multiplication_request", 0666);

                    int fd = open("multiplication_response", O_RDONLY);
                    int fd2 = open("multiplication_request", O_WRONLY);

                    write(fd2,&rowsA,sizeof(int));
                    write(fd2,&columnsA,sizeof(int));
                    write(fd2,matrixA.arr,(size_t)rowsA * columnsA * sizeof matrixA.arr[0]);

                    write(fd2,&rowsB,sizeof(int));
                    write(fd2,&columnsB,sizeof(int));
                    write(fd2,matrixB.arr,(size_t)rowsB * columnsB * sizeof matrixB.arr[0]);

                    struct Matrix matrixC1 = createMatrix(rowsA, columnsB);

                    read(fd,matrixC1.arr,(size_t)rowsA * columnsB * sizeof matrixC1.arr[0]);

                    write(client_sock, matrixC1.arr, (size_t)rowsA * columnsB * sizeof matrixC1.arr[0]);

                    break;
                case 2:
                    read(client_sock,&rowsA, sizeof(int));
                    read(client_sock,&columnsA, sizeof(int));
                    matrixA = createMatrix(rowsA, columnsA);
                    read(client_sock, matrixA.arr, (size_t)rowsA * columnsA * sizeof matrixA.arr[0]);


                    break;
                case 3:
                    read(client_sock,&rowsB, sizeof(int));
                    read(client_sock,&columnsB, sizeof(int));
                    matrixA = createMatrix(rowsB, columnsB);
                    read(client_sock, matrixA.arr, (size_t)rowsB * columnsB * sizeof matrixA.arr[0]);


                    break;
                case 4:
                    read(client_sock,&rowsA, sizeof(int));
                    read(client_sock,&columnsA, sizeof(int));
                    matrixA = createMatrix(rowsA, columnsA);
                    read(client_sock, matrixA.arr, (size_t)rowsA * columnsA * sizeof matrixA.arr[0]);


                    break;
                case 5:
                    read(client_sock,&rowsB, sizeof(int));
                    read(client_sock,&columnsB, sizeof(int));
                    matrixA = createMatrix(rowsB, columnsB);
                    read(client_sock, matrixA.arr, (size_t)rowsB * columnsB * sizeof matrixA.arr[0]);


                    break;
                case 6:
                    continue;
                default:
                    break;
                }
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
