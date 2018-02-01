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

#include "main.h"
#include "gif_utils.h"
#include "filters.h"
#include "role.h"

void attributeNumberOfProcess(int *workgroupList, int numberOfProcess, animated_gif *image)
{
    int compteur = numberOfProcess;
    int i = 1;
    workgroupList[0] = 1;

    while (compteur >= 4 && i < image->n_images - 1)
    {
        workgroupList[i] = 4;
        compteur -= 4;
    }

    workgroupList[image->n_images - 1] = compteur;
}

int main(int argc, char **argv)
{

    MPI_Init(&argc, &argv);

    // Add a MPI_Type_struct
    MPI_Datatype MPI_CUSTOM_TASK;
    int blocksizes[] = {4, 4, 4, 4};
    MPI_Aint displacements[] = {0, 0, 0, 0};
    MPI_Datatype types[] = {MPI_INT, MPI_INT, MPI_INT, MPI_INT};
    MPI_Type_create_struct(4, blocksizes, displacements,
                           types, &MPI_CUSTOM_TASK);
    MPI_Type_commit(&MPI_CUSTOM_TASK);

    MPI_Datatype MPI_CUSTOM_PIXEL;
    int blocksizes2[] = {4, 4, 4};
    MPI_Aint displacements2[] = {0, 0, 0};
    MPI_Datatype types2[] = {MPI_INT, MPI_INT, MPI_INT};
    MPI_Type_create_struct(3, blocksizes2, displacements2,
                           types2, &MPI_CUSTOM_PIXEL);
    MPI_Type_commit(&MPI_CUSTOM_PIXEL);

    char *input_filename;
    char *output_filename;
    animated_gif *image;
    struct timeval t1, t2;
    double duration;

    // MPI STARTS HERE
    MPI_Init(&argc, &argv);

    if (argc < 3)
    {
        fprintf(stderr, "Usage: %s input.gif output.gif \n", argv[0]);
        return 1;
    }

    input_filename = argv[1];
    output_filename = argv[2];

    //

    /* IMPORT Timer start */
    gettimeofday(&t1, NULL);

    /* Load file and store the pixels in array */
    image = load_pixels(input_filename);
    if (image == NULL)
    {
        return 1;
    }

    /* IMPORT Timer stop */
    gettimeofday(&t2, NULL);

    duration = (t2.tv_sec - t1.tv_sec) + ((t2.tv_usec - t1.tv_usec) / 1e6);

    printf("GIF loaded from file %s with %d image(s) in %lf s\n",
           input_filename, image->n_images, duration);

    /* FILTER Timer start */
    gettimeofday(&t1, NULL);

    /* Convert the pixels into grayscale */
    apply_gray_filter(image);

    /* FILTER Timer stop */
    gettimeofday(&t2, NULL);

    duration = (t2.tv_sec - t1.tv_sec) + ((t2.tv_usec - t1.tv_usec) / 1e6);

    printf("GRAY_FILTER done in %lf s\n", duration);

    /* FILTER Timer start */
    gettimeofday(&t1, NULL);

    /* Apply blur filter with convergence value */
    apply_blur_filter(image, 5, 20);

    /* FILTER Timer stop */
    gettimeofday(&t2, NULL);

    duration = (t2.tv_sec - t1.tv_sec) + ((t2.tv_usec - t1.tv_usec) / 1e6);

    printf("BLUR_FILTER done in %lf s\n", duration);

    /* FILTER Timer start */
    gettimeofday(&t1, NULL);

    /* Apply sobel filter on pixels */
    apply_sobel_filter(image);

    /* FILTER Timer stop */
    gettimeofday(&t2, NULL);

    duration = (t2.tv_sec - t1.tv_sec) + ((t2.tv_usec - t1.tv_usec) / 1e6);

    printf("SOBEL_FILTER done in %lf s\n", duration);

    /* EXPORT Timer start */
    gettimeofday(&t1, NULL);

    /* Store file from array of pixels to GIF file */
    if (!store_pixels(output_filename, image))
    {
        return 1;
    }

    /* EXPORT Timer stop */
    gettimeofday(&t2, NULL);

    duration = (t2.tv_sec - t1.tv_sec) + ((t2.tv_usec - t1.tv_usec) / 1e6);

    printf("Export done in %lf s in file %s\n", duration, output_filename);

    return 0;
}
