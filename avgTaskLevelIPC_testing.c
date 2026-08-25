#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

// Define the Matrix structure
struct Matrix {
    int rows;
    int columns;
    int *arr;  // Flattened array (row-major order)
};

// Global variable for tasks per worker
int tasksPerWorker = 4;  // you can adjust this

// Your matrixAverage function (already written above)
double matrixAverage(struct Matrix matrix){

    


    int totalTasks = matrix.rows * matrix.columns;
    int workerCount = totalTasks / tasksPerWorker;

    if (totalTasks % tasksPerWorker != 0) {
        workerCount = workerCount + 1;
    }

    if (workerCount == 0) {
        workerCount = 1;
    }

    int resultPipes[workerCount][2];
    int pipeLastProcess[2];
    pipe(pipeLastProcess);
    pid_t workerPids[workerCount];

    for (int worker = 0; worker < workerCount; worker++) {
        if (pipe(resultPipes[worker]) == -1) {
            perror("pipe");
            return 0.0;
        }

        
        
        
        

        pid_t pid = fork();

        if (pid == 0) {

            int firstTask = worker * tasksPerWorker;
            int lastTask = firstTask + tasksPerWorker;

            if (lastTask > totalTasks) {
                lastTask = totalTasks;
            }

            double avg = 0.0;
            int sum = 0;
            int numberOfTasksForTheLastProcess = lastTask - firstTask;
            for (int i = firstTask; i < lastTask; i++) {

                

                sum = sum + matrix.arr[i];

                

                if ((lastTask - firstTask) - 4 == 0){
                    //avg = (sum) / tasksPerWorker;
                }else {
                    numberOfTasksForTheLastProcess = ((lastTask - firstTask) - 4) * -1;
                    
                }

               

            }

            if(worker == workerCount - 1){
                avg = (double)sum  / numberOfTasksForTheLastProcess;
                close(pipeLastProcess[0]);
                write(pipeLastProcess[1],&numberOfTasksForTheLastProcess,sizeof (numberOfTasksForTheLastProcess));
                close(pipeLastProcess[1]);

                printf("avg for last %f \n",avg);
            }else{
                avg = (double)sum / tasksPerWorker;
                printf("avg for first %f \n",avg);
            }

            close(resultPipes[worker][0]);
            write(resultPipes[worker][1], &avg, sizeof avg);
            close(resultPipes[worker][1]);

            fflush(stdout);
            _exit(0);
        }

        workerPids[worker] = pid;
        close(resultPipes[worker][1]);

    }

    double totalAvg = 0.0;
    int numberOfTasksForTheLastProcess;

    for (int worker = 0; worker < workerCount; worker++) {

        if (worker == workerCount -1 ){
            double avg = 0.0;
            read(resultPipes[worker][0],&avg,sizeof avg);
            close(resultPipes[worker][0]);

            
            close(pipeLastProcess[1]);
            read(pipeLastProcess[0],&numberOfTasksForTheLastProcess,sizeof (numberOfTasksForTheLastProcess));
            close(pipeLastProcess[0]);
            printf("%d \n",numberOfTasksForTheLastProcess);
            totalAvg = totalAvg + (numberOfTasksForTheLastProcess * avg);

        }else{
            double avg = 0.0;
            read(resultPipes[worker][0],&avg,sizeof avg);
            close(resultPipes[worker][0]);
            totalAvg = totalAvg + (tasksPerWorker * avg);
        }

        printf("total avg sdo far: %f\n",totalAvg);
        
    }

    totalAvg = totalAvg / (((workerCount - 1) * tasksPerWorker) + numberOfTasksForTheLastProcess);


    //avg = (double)sum / (matrix.columns * matrix.rows);
    return totalAvg;

}

// Test driver
int main() {
    // Example: Matrix A = [[1,2],[3,4],[5,6]]
    struct Matrix matrix;
    matrix.rows = 3;
    matrix.columns = 2;
    matrix.arr = (int*)malloc(matrix.rows * matrix.columns * sizeof(int));

    // Fill matrix data
    int data[] = {1, 2, 3, 4, 5, 6};
    for (int i = 0; i < matrix.rows * matrix.columns; i++) {
        matrix.arr[i] = data[i];
    }

    // Call the function
    double avg = matrixAverage(matrix);

    // Print result
    printf("Matrix average = %.2f\n", avg);

    // Cleanup
    free(matrix.arr);
    return 0;
}
