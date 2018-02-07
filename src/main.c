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

#include "gif_utils.h"
#include "filters.h"
#include "role.h"
#include "tests.h"
#include "main.h"

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

    if (mode == MODE_TEST)
    {
        if (rank == 0)
            test();
    }
    else if (mode == MODE_SEQUENTIAL)
    {
        if (rank == 0)
            sequential_process(input_filename, output_filename);
    }
    else if (mode == MODE_PARALLEL)
    {
        parallel_process(input_filename, output_filename);
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
