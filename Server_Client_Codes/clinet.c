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

    int option = 0;
    struct Matrix matrixA;
    struct Matrix matrixB;
    int rowsA = 0, columnsB = 0, columnsA = 0, rowsB = 0;

    while(option != -1){

        printf("=================\n");
        printf("=   Client  UI  =\n");
        printf("=================\n");

        printf("=> Matrix A: %d * %d\n",rowsA,columnsA);
        printf("=> Matrix B: %d * %d\n",rowsB, columnsB);
        printf("=================\n");
        printf("=> Options: \n");
        printf("=> Enter 1 To Fill New Matrices: \n");
        printf("=> Enter 2 To Open The Services List: \n");
        printf("=> Enter -1 To Exit The System: \n");
        printf("=> ");
        scanf("%d",&option);

        switch (option)
        {
        case 1:
            printf("=> Enter the number of rows for matrix A: \n");
            printf("=> ");
            

            scanf("%d",&rowsA);

            printf("=> Enter the number of columns for matrix A/ rows for matrix B:\n");
            printf("=> ");

            scanf("%d",&columnsA);
            rowsB = columnsA;

            printf("=> Enter the number of columns for matrix B: \n");
            printf("=> ");
            scanf("%d",&columnsB);

            matrixA = createMatrix(rowsA, columnsA);
            matrixB = createMatrix(rowsB, columnsB);
            fillMatrix(&matrixA);
            fillMatrix(&matrixB);
            break;
        case 2:
            printf("    Services List   \n");
            printf("=> Choose An Operation to perform on the Matrices: \n");
            printf("=> (1) => Multiplication: Matrix A * Matrix B: \n");
            printf("=> (2) => Transpose Matrix A: \n");
            printf("=> (3) => Transpose Matrix B: \n");
            printf("=> (4) => Find The Average For Matrix A: \n");
            printf("=> (5) => Find The Average For Matrix B: \n");
            printf("=> (6) => Go Back:\n");
            printf("=> ");

            int option2 = 0;
            if(rowsA == 0 || rowsB == 0 || columnsA == 0 || columnsB == 0){
                printf("=! Note The Matrices Are Empty You Cant Perform Any Operation !=\n");
                printf("=! Please Fill The Matrices First !=\n");
                break;
            }else{
                scanf("%d",&option2);
                switch (option2)
                {
                case 1:
                    /* code */
                    break;
                case 2:
                    break;
                case 3:
                    break;
                case 4:
                    break;
                case 5:
                    break;
                case 6:
                    continue;
                default:
                    break;
                }
            }
            break;
        case -1:
            continue;
        default:
            break;
        }
        

        


    }


 close(sockFD);

 return 0;

}