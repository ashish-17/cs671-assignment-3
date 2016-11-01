#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <mpi.h>

#define RESULT_TAG 0
#define INPUT_TAG 1
#define STOP_TAG 2

extern void mandelbrotProcessRowHelper(
        float x0, float y0, float x1, float y1,
        int width, int height,
        int startRow, int totalRows,
        int startCol, int totalColumns,
        int maxIterations,
        int output[]);

//
// workerStatic --
//
// Thread entrypoint.
void workerStatic(int rank, int nWorkers, float x0, float y0, float x1, float y1, int width, int height, int maxIterations, int chunk) {
    unsigned int i = 0, j = 0;

    int size = chunk*chunk + 4;
    int *output = (int*)malloc(sizeof(int) * size);
    int nRows = 0, nCols = 0;
    for (i = 0; i < height; i+= chunk) {
        nRows = (height - i) < chunk ? (height - i) : chunk;
        for (j = chunk*(rank-1); j < width; j+= chunk*nWorkers) {
            nCols = (width - j) < chunk ? (width - j) : chunk;

            output[0] = i;
            output[1] = nRows;
            output[2] = j;
            output[3] = nCols;

            mandelbrotProcessRowHelper(x0, y0, x1, y1, width, height, i, nRows, j, nCols, maxIterations, output+4);
            
            MPI_Send(output, size, MPI_INT, 0, RESULT_TAG, MPI_COMM_WORLD);
        }
    }

    free(output);
}

//
// masterStatic --
//
// Thread entrypoint.
void masterStatic(float x0, float y0, float x1, float y1, int width, int height, int maxIterations, int chunk, int output[]) {
    int size = chunk*chunk + 4;
    int* tmp = (int*)malloc(sizeof(int)*size);
    memset(tmp, 0 , sizeof(int)*size);
    MPI_Status status;

    int startRow = 0;
    int startCol = 0;
    int nRows = 0;
    int nCols = 0;

    int elementCount = 0;
    int index = 0;
    int i = 0, j = 0;

    while (elementCount < width*height) {
        MPI_Recv(tmp, size, MPI_INT, MPI_ANY_SOURCE, RESULT_TAG, MPI_COMM_WORLD, &status);

        startRow = tmp[0];
        nRows = tmp[1];
        startCol = tmp[2];
        nCols = tmp[3];
        
        for (i = startRow, j = 0; i < (startRow + nRows); ++i, ++j) {
            index = i*width + startCol;
            memcpy((void*)(output + index), tmp+4 + j*nCols, sizeof(int)*nCols);
        }

        elementCount += nRows*nCols;
    }
    
    free(tmp);
}

//
// workerDynamic --
//
// Thread entrypoint.
void workerDynamic(int rank, float x0, float y0, float x1, float y1, int width, int height, int maxIterations) {
    int row = 0;
    MPI_Status status;
    int size = width + 1;
    int *output = (int*)malloc(sizeof(int) * size);
    while ((MPI_Recv(&row, 1, MPI_INT, 0, MPI_ANY_TAG, MPI_COMM_WORLD, &status) == MPI_SUCCESS) && 
            (status.MPI_TAG == INPUT_TAG)) {
        
        output[0] = row;
        mandelbrotProcessRowHelper(x0, y0, x1, y1, width, height, row, 1, 0, width, maxIterations, output+1);

        MPI_Send(output, size, MPI_INT, 0, RESULT_TAG, MPI_COMM_WORLD);
    }

    free(output);
}

//
// masterDynamic --
//
// Thread entrypoint.
void masterDynamic(int numWorkers, float x0, float y0, float x1, float y1, int width, int height, int maxIterations, int output[]) {
    int size = width + 1;
    int* tmp = (int*)malloc(sizeof(int)*size);
    memset(tmp, 0 , sizeof(int)*size);
    MPI_Status status;

    int row = 0;
    int src = 0;
    int todo = 0;
    int next = 0;
    int i = 0;

    for (i = 0; i < numWorkers; ++i) {
        MPI_Send(&next, 1, MPI_INT, i+1, INPUT_TAG, MPI_COMM_WORLD);
        next++;
        todo++;
    }

    while (todo > 0) {
        MPI_Recv(tmp, size, MPI_INT, MPI_ANY_SOURCE, RESULT_TAG, MPI_COMM_WORLD, &status);
        src = status.MPI_SOURCE;

        row = tmp[0];
        --todo;

        if (next < height) {
            MPI_Send(&next, 1, MPI_INT, src, INPUT_TAG, MPI_COMM_WORLD);
            ++next;
            ++todo;
        } else {
            MPI_Send(&next, 0, MPI_INT, src, STOP_TAG, MPI_COMM_WORLD);
        }

        memcpy((void*)(output + row*width), tmp + 1, sizeof(int)*width);
    }

    free(tmp);
}
