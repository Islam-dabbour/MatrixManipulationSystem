#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 

struct Matrix{

    int rows;
    int columns;
    int *arr;
};

void error(const char *msg){
    perror(msg);
    exit(0);
}

struct Matrix fillMatrix(struct Matrix *matrix){

    for (int i = 0; i < matrix->rows; i++) {

        for (int j = 0; j < matrix->columns; j++) {

            matrix->arr[i * matrix->columns + j] = (rand() % 100) + 1; 

        }

    }

    return *matrix;

}

struct Matrix createMatrix(int rows, int columns){

    
    struct Matrix matrix = {
        .rows = rows,
        .columns = columns,
        .arr = calloc((size_t)rows * columns, sizeof *matrix.arr)
    };

    return matrix;

}

int main(int argc, char **argv){

    if(argc != 2){
        printf("You need to provide a port number\n");
        exit(0);
    }

    int port = atoi(argv[1]);

    int sockFD;

    struct sockaddr_in addr;

    char buffer[1024];

    sockFD = socket(AF_INET, SOCK_STREAM, 0);

    if(sockFD < 0){
        perror("Socket Error");
        exit(1);
    }

    printf("[+] Client socket created.\n");
    printf("=================\n");
    printf("=   Client  UI  =\n");
    printf("=================\n");

    memset(&addr, '\0', sizeof(addr));

    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;

    if(connect(sockFD,
              (struct sockaddr*)&addr,
              sizeof(addr)) < 0)
    {
        error("Connection Error");
    }

    printf("[+] Connected to server.\n"); //senario

    printf("=> Enter the number of rows for matrix A: \n");
    printf("=> ");
    int rowsA, columnsB, columnsA, rowsB;

    scanf("%d",&rowsA);

    printf("=> Enter the number of columns for matrix A/ rows for matrix B:\n");
    printf("=> ");

    scanf("%d",&columnsA);
    rowsB = columnsA;

    printf("=> Enter the number of columns for matrix B: \n");
    printf("=> ");
    scanf("%d",&columnsB);

    struct Matrix matrixA = createMatrix(rowsA, columnsA);
    struct Matrix matrixB = createMatrix(rowsB, columnsB);
    fillMatrix(&matrixA);
    fillMatrix(&matrixB);





 close(sockFD);

 return 0;

}