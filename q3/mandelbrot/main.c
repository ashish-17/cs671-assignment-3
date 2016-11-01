#include <stdio.h>
#include <getopt.h>
#include <string.h>
#include <stdlib.h>
#include <mpi.h>

#define TIME_TAG 4

extern void mandelbrotSerial(
        float x0, float y0, float x1, float y1,
        int width, int height,
        int startRow, int numRows,
        int startCol, int totalColumns,
        int maxIterations,
        int output[]);

extern void workerStatic(int rank, int nWorkers, float x0, float y0, float x1, float y1, int width, int height, int maxIterations, int chunk);

extern void masterStatic(float x0, float y0, float x1, float y1, int width, int height, int maxIterations, int chunk, int output[]);

extern void workerDynamic(int rank, float x0, float y0, float x1, float y1, int width, int height, int maxIterations);

extern void masterDynamic(int numWorkers, float x0, float y0, float x1, float y1, int width, int height, int maxIterations, int output[]);

extern void writePPMImage(
        int* data,
        int width, int height,
        const char *filename,
        int maxIterations);

void scaleAndShift(float* x0, float* x1, float* y0, float* y1,
        float scale,
        float shiftX, float shiftY)
{

    *x0 *= scale;
    *x1 *= scale;
    *y0 *= scale;
    *y1 *= scale;
    *x0 += shiftX;
    *x1 += shiftX;
    *y0 += shiftY;
    *y1 += shiftY;
}

void usage(const char* progname) {
    printf("Usage: %s [options]\n", progname);
    printf("Program Options:\n");
    printf("  -v  --view <INT>   Use specified view settings\n");
    printf("  -?  --help         This message\n");
}

int verifyResult (int *gold, int *result, int width, int height) {

    int i, j;

    for (i = 0; i < height; i++) {
        for (j = 0; j < width; j++) {
            if (gold[i * width + j] != result[i * width + j]) {
                printf ("Mismatch : [%d][%d], Expected : %d, Actual : %d\n",
                        i, j, gold[i * width + j], result[i * width + j]);
                return 0;
            }
        }
    }

    return 1;
}

#define MODE_STATIC 0
#define MODE_DYNAMIC 1

int main(int argc, char** argv) {

    const unsigned int width = 1200;
    const unsigned int height = 800;
    const int maxIterations = 256;

    float x0 = -2;
    float x1 = 1;
    float y0 = -1;
    float y1 = 1;

    // parse commandline options ////////////////////////////////////////////
    int opt;
    static struct option long_options[] = {
        {"threads", 1, 0, 't'},
        {"view", 1, 0, 'v'},
        {"help", 0, 0, '?'},
        {0 ,0, 0, 0}
    };

    int i = 0, j = 0;
    int viewIndex = 0;
    float scaleValue = .015f;
    float shiftX = -.986f;
    float shiftY = .30f;

    int chunk_size = 5;
    int mode = MODE_STATIC; // 

    while ((opt = getopt_long(argc, argv, "m:t:v:?", long_options, NULL)) != EOF) {

        switch (opt) {
            case 'm' : {
                           mode = atoi(optarg);
                           break;
                       }
            case 't': {
                          chunk_size = atoi(optarg);
                          break;
                      }
            case 'v':
                      {
                          viewIndex = atoi(optarg);
                          // change view settings
                          if (viewIndex == 2) {
                              scaleValue = .015f;
                              shiftX = -.986f;
                              shiftY = .30f;
                              scaleAndShift(&x0, &x1, &y0, &y1, scaleValue, shiftX, shiftY);
                          } else if (viewIndex > 1) {
                              fprintf(stderr, "Invalid view index\n");
                              return 1;
                          }
                          break;
                      }
            case '?':
            default:
                      usage(argv[0]);
                      return 1;
        }
    }
    // end parsing of commandline options

    int rank = 0, comm_size = 0;
    double startTime = 0.0;
    double endTime = 0.0;
    double deltaT = 0.0;
    double tmpT = 0.0;
    double minSerial = 1e30;
    double minThread = 1e30;
    MPI_Status status;

    int* output_serial = NULL;
    int* output_thread = NULL;

    MPI_Init(NULL, NULL);

    MPI_Comm_size(MPI_COMM_WORLD, &comm_size);

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    if (rank == 0) {
        output_serial = (int*)malloc(sizeof(int)*width*height);

        //
        // Run the serial implementation.  Run the code three times and
        // take the minimum to get a good estimate.
        //
        memset(output_serial, 0, width * height * sizeof(int));
        for (i = 0; i < 3; ++i) {
            startTime = MPI_Wtime();
            mandelbrotSerial(x0, y0, x1, y1, width, height, 0, height, 0, width, maxIterations, output_serial);
            endTime = MPI_Wtime();
            deltaT = endTime - startTime;
            if (deltaT < minSerial) {
                minSerial = deltaT;
            }
        }

        //printf("[mandelbrot serial]:\t\t[%.3f] ms\n", minSerial * 1000);
        //writePPMImage(output_serial, width, height, "mandelbrot-serial.ppm", maxIterations);
    }


    if (rank == 0) {
        output_thread = (int*)malloc(sizeof(int)*width*height);
        memset(output_thread, 0, width * height * sizeof(int));
    }

    int num_workers = comm_size - 1;

    MPI_Barrier(MPI_COMM_WORLD);

    if (mode == MODE_STATIC) {
        for (i = 0; i < 3; ++i) {     
            startTime = MPI_Wtime();

            if (rank == 0) {
                masterStatic(x0, y0, x1, y1, width, height, maxIterations, chunk_size, output_thread);
            } else {
                workerStatic(rank, num_workers, x0, y0, x1, y1, width, height, maxIterations, chunk_size);
            }

            endTime = MPI_Wtime();
            deltaT = endTime - startTime;

            if (rank != 0) {
                MPI_Send(&startTime, 1, MPI_DOUBLE, 0, TIME_TAG, MPI_COMM_WORLD);
            } else {
                for (j = 1; j < comm_size; ++j) {
                    MPI_Recv(&tmpT, 1, MPI_DOUBLE, j, TIME_TAG, MPI_COMM_WORLD, &status);
                    if (startTime > tmpT) {
                        startTime = tmpT;
                    }
                }

                deltaT = endTime - startTime;
            }

            if (deltaT < minThread) {
                minThread = deltaT;
            }

            MPI_Barrier(MPI_COMM_WORLD);
        }
    } else {
        for (i = 0; i < 3; ++i) {     
            startTime = MPI_Wtime();

            if (rank == 0) {
                masterDynamic(num_workers, x0, y0, x1, y1, width, height, maxIterations, output_thread);
            } else {
                workerDynamic(rank, x0, y0, x1, y1, width, height, maxIterations);
            }

            endTime = MPI_Wtime();
            deltaT = endTime - startTime;

            if (rank != 0) {
                MPI_Send(&startTime, 1, MPI_DOUBLE, 0, TIME_TAG, MPI_COMM_WORLD);
            } else {
                for (j = 1; j < comm_size; ++j) {
                    MPI_Recv(&tmpT, 1, MPI_DOUBLE, j, TIME_TAG, MPI_COMM_WORLD, &status);
                    if (startTime > tmpT) {
                        startTime = tmpT;
                    }
                }

                deltaT = endTime - startTime;
            }

            if (deltaT < minThread) {
                minThread = deltaT;
            }

            MPI_Barrier(MPI_COMM_WORLD);
        }
    }

    if (rank == 0) {
        // compute speedup
        printf("%d,%d,%.2f\n", comm_size, width*height, minSerial/minThread);
        //printf("[mandelbrot thread]:\t\t[%.3f] ms\n", minThread * 1000);
        //writePPMImage(output_thread, width, height, "mandelbrot-thread.ppm", maxIterations);

        if (! verifyResult (output_serial, output_thread, width, height)) {
            printf ("Error : Output from threads does not match serial output\n");
        }
    }

    if (output_serial != NULL) {
        free(output_serial);
    }
    if (output_thread != NULL) {
        free(output_thread);
    }

    MPI_Finalize();


    return 0;
}
