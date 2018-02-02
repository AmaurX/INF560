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

void testProcessAttribution()
{
    const int frameCard = 3;
    const int processCard = 3;
    int nFrameList[] = {1, 4, 40};
    int nProcessList[] = {2, 8, 40};
    for (int i = 0; i < frameCard; i++)
    {
        for (int j = 0; j < processCard; j++)
        {
            int nFrame = nFrameList[i];
            int nProcess = nProcessList[j];
            printf("================================\n"
                   "===== test with M=%d L=%d ======\n",
                   nProcessList[j], nFrame);
            int nMaxGroup = nFrame + 1;
            int *workgroupList = (int *)calloc(nMaxGroup, sizeof(int));
            animated_gif fakeImage = {
                .n_images = nFrame};
            attributeNumberOfProcess(workgroupList, nProcessList[j], &fakeImage);

            for (int k = 0; k < nMaxGroup; k++)
            {
                printf("%d ", workgroupList[k]);
            }
            printf("\n Attribution:\n");

            int *groupAttribution = (int *)calloc(nProcess, sizeof(int));
            for (int k = 0; k < nProcess; k++)
            {
                printf("%d ", whichCommunicator(workgroupList, nMaxGroup, k));
            }
            printf("\n");

            free(workgroupList);
            free(groupAttribution);
        }
    }
}

/**
 * \brief assign processes to groups
 * 
 * returned tab is of the form [1, x, y, ..] because first index is for the MASTER group
 */
void attributeNumberOfProcess(int *workgroupList, int numberOfProcess, animated_gif *image)
{
    if (numberOfProcess < 2)
    {
        fprintf(stderr, "Too few processes. Aborting\n");
        return;
    }
    int maxGroup = image->n_images + 1;
    int freeProcess = numberOfProcess - 1;
    int compteur = image->n_images;
    int i = 1;
    workgroupList[0] = 1;

    while (freeProcess > 0)
    {
        workgroupList[i]++;
        if (workgroupList[i] == 4 && i + 1 < maxGroup)
        {
            //next group
            i++;
        }
        freeProcess--;
    }

    // while (compteur >= 4 && i < image->n_images - 1)
    // {
    //     workgroupList[i] = 4;
    //     compteur -= 4;
    // }

    // workgroupList[image->n_images - 1] = compteur;
}

int whichCommunicator(int *workgroupList, int listSize, int rankWorld)
{
    int comm = 0;
    while (rankWorld >= 0 && comm < listSize)
    {
        rankWorld -= workgroupList[comm];
        comm++;
    }
    return comm - 1;
}

int main(int argc, char **argv)
{
    int rankWorld, commWorldSize, groupIndex;
    /// GroupMaster or Slave ? everything lies in the value of that integer...
    int groupRank;
    /// Size of the work group
    int groupSize;
    /// Used to determine the role of the current process
    enum role groupRole;
    /// Group communicator ( MPI_COMM_NULL if master )
    MPI_Comm groupComm;

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
    // Add a MPI_Type_struct
    int blocksizes[] = {4, 4, 4, 4};
    MPI_Aint displacements[] = {0, 0, 0, 0};
    MPI_Datatype types[] = {MPI_INT, MPI_INT, MPI_INT, MPI_INT};
    MPI_Type_create_struct(4, blocksizes, displacements,
                           types, &MPI_CUSTOM_TASK);
    MPI_Type_commit(&MPI_CUSTOM_TASK);

    int blocksizes2[] = {4, 4, 4};
    MPI_Aint displacements2[] = {0, 0, 0};
    MPI_Datatype types2[] = {MPI_INT, MPI_INT, MPI_INT};
    MPI_Type_create_struct(3, blocksizes2, displacements2,
                           types2, &MPI_CUSTOM_PIXEL);
    MPI_Type_commit(&MPI_CUSTOM_PIXEL);

    input_filename = argv[1];
    output_filename = argv[2];

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

    MPI_Comm_size(MPI_COMM_WORLD, &commWorldSize);
    MPI_Comm_rank(MPI_COMM_WORLD, &rankWorld);

    // process attribution
    /// desperately under-optimized value
    int listSize = image->n_images + 1;
    int *workgroupList = (int *)calloc(listSize, sizeof(int));
    attributeNumberOfProcess(workgroupList, commWorldSize, image);
    groupIndex = whichCommunicator(workgroupList, listSize, rankWorld);
    MPI_Comm_split(MPI_COMM_WORLD, groupIndex, rankWorld, &groupComm);

    if (rankWorld == 0)
    {
        //master is working here
        printf("Hello from thread master %d/%d\n", rankWorld, commWorldSize);
        // testProcessAttribution();
    }
    else
    {
        //groupmaster & slaves
        MPI_Comm_rank(groupComm, &groupRank);
        MPI_Comm_size(groupComm, &groupSize);
        enum role groupRole = giveRoleInGroup(groupRank);
        if (groupRole == groupmaster)
        {
            // groupMaster loop
            // do nothing.
            // for now...
            printf("Hello from groupMaster (group : %d/%d, world: %d/%d)\n", groupRank, groupSize, rankWorld, commWorldSize);
        }
        else
        {
            // slave loop
            // do nothing - but another way
            printf("Hello from slave of group %d: (%d/%d), in world: %d/%d\n", groupIndex, groupRank, groupSize, rankWorld, commWorldSize);
        }
    }

    if (0)
    {

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
    }

    MPI_Finalize();
    return 0;
}
