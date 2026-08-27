#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>
#include <sys/wait.h>
#include <pthread.h>

struct Matrix{

    int rows;
    int columns;
    int *arr;
};

struct MatricesArgs{
    struct Matrix *matrixA;
    struct Matrix *matrixB;
    struct Matrix result;
    struct Matrix transposedMatrix;
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

void *matrixMultipicationThread (void *arg){

    struct MatricesArgs *matrices = arg;

    struct timeval startM, endM; 

    printf("============================\n");
    printf("   Matrix Multipication     \n");
    //printf("============================\n");
    gettimeofday(&startM, NULL); 
    matrices->result = matrixMultipication(*matrices->matrixA, *matrices->matrixB);
    gettimeofday(&endM, NULL); 
    double timeTakenM =(endM.tv_sec - startM.tv_sec) +(endM.tv_usec - startM.tv_usec) / 1000000.0;
    printf("\n");
    printf("> Time taken to finish multipcation > %f\n",timeTakenM);

    printf("\n");

    return NULL;
}

void *matrixTranspositionThread (void *arg){

    struct MatricesArgs *matrices = arg;

    struct timeval startT, endT;

    printf("============================\n");
    printf("   Matrix Transposition     \n");
    //printf("============================\n");
    gettimeofday(&startT, NULL); 
    matrices->transposedMatrix = matrixTransposition(*matrices->matrixA);
    //struct Matrix matrixTranspositionResult = matrixTransposition(matrixA);
    gettimeofday(&endT, NULL);
    double timeTakenT =(endT.tv_sec - startT.tv_sec) +(endT.tv_usec - startT.tv_usec) / 1000000.0;
    printf("\n");
    printf("> Time taken to finish Transposition > %f\n",timeTakenT);

    printf("\n");

    return NULL;

}

void *matrixAverageThread (void *arg){

    struct timeval startA, endA;

    struct MatricesArgs *matrices = arg;

    printf("============================\n");
    printf("      Matrix Average        \n");
    //printf("============================\n");
    gettimeofday(&startA, NULL); 
    double avg = matrixAverage(*matrices->matrixA);
    gettimeofday(&endA, NULL);
    double timeTakenA =(endA.tv_sec - startA.tv_sec) +(endA.tv_usec - startA.tv_usec) / 1000000.0;
    printf("\n");
    printf("> Time taken to finish Average calculation > %f\n",timeTakenA);
    printf("============================\n");

}


int main(){

    srand(time(NULL));
    struct timeval start, end;  
     

    pthread_t operations[3];

    gettimeofday(&start, NULL); 

    printf("============================\n");
    printf(" Matrix Manipulation System \n");
    printf("============================\n");

    int rowsA = (rand() % 100) + 1;
    int columnsB = (rand() % 100) + 1;
    int columnsA = (rand() % 100) + 1;
    int rowsB = columnsA;
    
    

    struct Matrix matrixA = createMatrix(rowsA, columnsA);
    struct Matrix matrixB = createMatrix(rowsB, columnsB);
    struct MatricesArgs matriecesArgs = {
        .matrixA = &matrixA,
        .matrixB = &matrixB,
        .result = {0},
        .transposedMatrix = {0}
    };

    if (pthread_create(&operations[0], NULL, matrixMultipicationThread, &matriecesArgs) != 0) {
        fprintf(stderr, "Failed to create multiplication thread\n");
        freeAllocatedMemory(&matrixA);
        freeAllocatedMemory(&matrixB);
        return EXIT_FAILURE;
    }

    

    //printMatrix(matrixA);

   
    if (pthread_create(&operations[1], NULL, matrixTranspositionThread, &matriecesArgs) != 0) {
        fprintf(stderr, "Failed to create transposition thread\n");
        freeAllocatedMemory(&matrixA);
        freeAllocatedMemory(&matrixB);
        return EXIT_FAILURE;
    }

    

    if (pthread_create(&operations[2], NULL, matrixAverageThread, &matriecesArgs) != 0) {
        fprintf(stderr, "Failed to create transposition thread\n");
        freeAllocatedMemory(&matrixA);
        freeAllocatedMemory(&matrixB);
        return EXIT_FAILURE;
    }

    pthread_join(operations[0], NULL);
    pthread_join(operations[1], NULL);
    pthread_join(operations[2], NULL);


    freeAllocatedMemory(&matrixA);
    freeAllocatedMemory(&matrixB);
    freeAllocatedMemory(&matriecesArgs.result);
    freeAllocatedMemory(&matriecesArgs.transposedMatrix);
    

    gettimeofday(&end, NULL); 
    double timeTaken =(end.tv_sec - start.tv_sec) +(end.tv_usec - start.tv_usec) / 1000000.0;
    printf("\n");

    printf("> Time taken to finish the whole program > %f\n",timeTaken);

    return 0;
}