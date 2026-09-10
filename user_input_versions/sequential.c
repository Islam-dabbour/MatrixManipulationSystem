#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>
#include <sys/wait.h>

struct Matrix{

    int rows;
    int columns;
    int *arr;
};

struct Matrix fillMatrix(struct Matrix *matrix){

    srand(time(NULL));

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

    matrix = fillMatrix(&matrix);

    return matrix;

}

void printMatrix(struct Matrix matrix){

    for (int i = 0; i < matrix.rows; i++) {

        for (int j = 0; j < matrix.columns; j++) {

            //matrix.arr[i * matrix->columns + j] = (rand() % 100) + 1; 
            printf("%d ",matrix.arr[i * matrix.columns + j]);

        }

        printf("\n");

    }
    
}


void freeAllocatedMemory(struct Matrix *matrix){
   
    free(matrix->arr);
    
}

struct Matrix matrixMultipication(struct Matrix matrixA, struct Matrix matrixB){

    if (matrixA.columns != matrixB.rows) {
        return (struct Matrix){0};
    }

    struct Matrix matrixC = createMatrix(matrixA.rows, matrixB.columns);

    for (int i = 0; i < matrixC.rows; i++) {

        for (int j = 0; j < matrixC.columns; j++) {

            matrixC.arr[i * matrixC.columns + j] = 0;

            for (int k = 0; k < matrixA.columns; k++) {

                matrixC.arr[i * matrixC.columns + j] += matrixA.arr[i * matrixA.columns + k] * matrixB.arr[k * matrixB.columns + j];
            
            }
        
        }
    
    }

    return matrixC;
}

struct Matrix matrixTransposition(struct Matrix matrix){

    struct Matrix matrixC = createMatrix(matrix.columns, matrix.rows);

    for (int i = 0; i < matrixC.rows; i++) {

        for (int j = 0; j < matrixC.columns; j++) {

            matrixC.arr[i * matrixC.columns + j] = matrix.arr[j * matrix.columns + i];

        }

    }

    return matrixC;

}

double matrixAverage(struct Matrix matrix){

    double avg = 0.0;
    int sum = 0;

    for (int i = 0; i < matrix.rows; i++) {

        for (int j = 0; j < matrix.columns; j++) {

            sum = sum + matrix.arr[i * matrix.columns + j];

        }

    }

    avg = sum / (matrix.columns * matrix.rows);
    return avg;

}


int main(int argc, char *argv[]){

    if (argc != 4) {
        fprintf(stderr, "Usage: %s <rowsA> <columnsA> <columnsB>\n", argv[0]);
        return EXIT_FAILURE;
    }

    srand(time(NULL));
    struct timeval start, end; 
    struct timeval startM, endM; 
    struct timeval startT, endT; 
    struct timeval startA, endA; 

    gettimeofday(&start, NULL); 

    printf("============================\n");
    printf(" Matrix Manipulation System \n");
    printf("============================\n");

    int rowsA = atoi(argv[1]);
    int columnsA = atoi(argv[2]);
    int columnsB = atoi(argv[3]);
    int rowsB = columnsA;
    
    

    struct Matrix matrixA = createMatrix(rowsA, columnsA);
    struct Matrix matrixB = createMatrix(rowsB, columnsB);

    printMatrix(matrixA);

    printf("============================\n");
    printf("   Matrix Multipication     \n");
    //printf("============================\n");
    gettimeofday(&startM, NULL); 
    struct Matrix matrixMultipicationResult = matrixMultipication(matrixA, matrixB);
    gettimeofday(&endM, NULL); 
    double timeTakenM =(endM.tv_sec - startM.tv_sec) +(endM.tv_usec - startM.tv_usec) / 1000000.0;
    printf("\n");
    printf("> Time taken to finish multipcation > %f\n",timeTakenM);

    printf("\n");

    printf("============================\n");
    printf("   Matrix Transposition     \n");
    //printf("============================\n");
    gettimeofday(&startT, NULL); 
    struct Matrix matrixTranspositionResult = matrixTransposition(matrixA);
    gettimeofday(&endT, NULL);
    double timeTakenT =(endT.tv_sec - startT.tv_sec) +(endT.tv_usec - startT.tv_usec) / 1000000.0;
    printf("\n");
    printf("> Time taken to finish Transposition > %f\n",timeTakenT);

    printf("\n");

    printf("============================\n");
    printf("      Matrix Average        \n");
    //printf("============================\n");
    gettimeofday(&startA, NULL); 
    double avg = matrixAverage(matrixA);
    gettimeofday(&endA, NULL);
    double timeTakenA =(endA.tv_sec - startA.tv_sec) +(endA.tv_usec - startA.tv_usec) / 1000000.0;
    printf("\n");
    printf("> Time taken to finish Average calculation > %f\n",timeTakenA);
    printf("============================\n");

    freeAllocatedMemory(&matrixA);
    freeAllocatedMemory(&matrixB);
    freeAllocatedMemory(&matrixMultipicationResult);
    freeAllocatedMemory(&matrixTranspositionResult);
    

    gettimeofday(&end, NULL); 
    double timeTaken =(end.tv_sec - start.tv_sec) +(end.tv_usec - start.tv_usec) / 1000000.0;
    printf("\n");

    printf("> Time taken to finish the whole program > %f\n",timeTaken);

    return 0;
}