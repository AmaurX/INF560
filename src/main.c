/*
 * INF560
 *
 * Image Filtering Project
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <sys/time.h>
//#include <filters.h>
#include <gif_lib.h>
#include <omp.h>
#include <mpi.h>
#include <stdarg.h>
#include <unistd.h>
#include <sys/types.h>

#include "gif_utils.h"
#include "filters.h"
#include "role.h"
#include "tests.h"
#include "main.h"

#if USE_CUDA
#include "cuda_filters.h"
#endif

int main(int argc, char **argv)
{

    char *input_filename;
    char *output_filename;

    // MPI STARTS HERE
    MPI_Init(&argc, &argv);

    if (argc < 4)
    {
        fprintf(stderr, "Usage: %s mode input.gif output.gif \n"
                        "mode : 0-sequential\n"
                        "       1-parallel\n"
                        "       2-test\n",
                argv[0]);
        return 1;
    }

    int mode = (int)strtol(argv[1], NULL, 10);
    input_filename = argv[2];
    output_filename = argv[3];

    int rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    struct timeval t1, t2;
    double duration;
    gettimeofday(&t1, NULL);
    if (mode == MODE_TEST)
    {
        if (rank == 0)
        {
            printf("// of %s\n"
                   "OMP:%d\n"
                   "CUDA:%d\n",
                   input_filename, USE_OMP, USE_CUDA);
#if USE_CUDA
            cuda_test();
#endif
            test();
        }
    }
    else if (mode == MODE_SEQUENTIAL)
    {
        if (rank == 0)
        {
            sequential_process(input_filename, output_filename);
        }
    }
    else if (mode == MODE_PARALLEL)
    {
        if (rank == 0)
        {
            printf("// of %s\n"
                   "OMP:%d\n"
                   "CUDA:%d\n",
                   input_filename, USE_OMP, USE_CUDA);
        }
        parallel_process(input_filename, output_filename);
    }
    if (rank == 0)
    {
        gettimeofday(&t2, NULL);
        duration = (t2.tv_sec - t1.tv_sec) + ((t2.tv_usec - t1.tv_usec) / 1e6);
        printf("Task done in %lf s\n", duration);
    }

    MPI_Finalize();
    return 0;
}

void waitForDebug()
{
    volatile int i = 0;
    char hostname[256];
    gethostname(hostname, sizeof(hostname));
    printf("[DEBUG] PID %d on %s ready for attach\n", getpid(), hostname);
    fflush(stdout);
    while (0 == i)
        sleep(5);
}

int dbprintf(char *format_string, ...)
{
#if DEBUG_PRINTS
    va_list args;
    va_start(args, format_string);
    int a = vprintf(format_string, args);
    va_end(args);
    return a;
#else
        return 0;
#endif
}
