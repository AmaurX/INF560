#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "main.h"
#include "filters.h"

#include "role.h"

enum role giveRoleInGroup(int rankGroup)
{
    if (rankGroup == 0)
        return groupmaster;
    else
        return slave;
}

void masterLoop(int *groupMasterList, int numberOfGroupMaster, animated_gif *image)
{
    int count = 0;
    int numberOfImages = image->n_images;

    // int doneImages[numberOfImages] = {0};
    int numberOfProcessedImages = 0;
    for (int i = 0; i < numberOfGroupMaster; i++)
    {
        if (count < numberOfImages)
        {
            struct task newTask;

            newTask.id = count;
            newTask.frameNumber = count;
            newTask.width = image->width[count];
            newTask.height = image->height[count];

            MPI_Send((void *)&newTask, 1, MPI_CUSTOM_TASK,
                     groupMasterList[i], TASK_TAG, MPI_COMM_WORLD);

            int numberOfPixels = newTask.height * newTask.width;

            printf("Sending frame %d on %d", count, numberOfImages);

            MPI_Send((void *)(image + count), numberOfPixels, MPI_CUSTOM_PIXEL,
                     (int)groupMasterList[i], IMAGE_TAG, MPI_COMM_WORLD);
            count++;
        }
    }

    while (numberOfProcessedImages < numberOfImages)
    {
        struct task doneTask;
        MPI_Status status;

        MPI_Recv((void *)&doneTask, 1, MPI_CUSTOM_TASK, MPI_ANY_SOURCE,
                 TASK_TAG, MPI_COMM_WORLD, &status);

        int frameNumber = doneTask.frameNumber;
        int numberOfPixels = doneTask.width * doneTask.height;
        int sender = status.MPI_SOURCE;
        MPI_Recv((void *)(image + frameNumber), numberOfPixels, MPI_CUSTOM_PIXEL, sender,
                 IMAGE_TAG, MPI_COMM_WORLD, &status);

        // doneImages[doneTask.frameNumber] = 1;
        if (status.MPI_ERROR == 0)
        {
            numberOfProcessedImages++;
            printf("Frame %d on %d was processed successfully. %d frame completed in total", frameNumber, numberOfImages, numberOfProcessedImages);
        }

        if (count < numberOfImages)
        {
            struct task newTask;

            newTask.id = count;
            newTask.frameNumber = count;
            newTask.width = image->width[count];
            newTask.height = image->height[count];

            MPI_Send((void *)&newTask, 1, MPI_CUSTOM_TASK,
                     sender, TASK_TAG, MPI_COMM_WORLD);

            int numberOfPixels = newTask.height * newTask.width;

            MPI_Send((void *)(image + count), numberOfPixels, MPI_CUSTOM_PIXEL,
                     sender, IMAGE_TAG, MPI_COMM_WORLD);
            count++;
        }
    }
}

void groupMasterLoop(MPI_Comm groupComm)
{
    while (true)
    {
        // RECEIVING FROM MASTER
        struct task newTask;
        MPI_Status status;

        MPI_Recv((void *)&newTask, 1, MPI_CUSTOM_TASK, (int)master,
                 TASK_TAG, MPI_COMM_WORLD, &status);

        if (newTask.id == -1)
        {
            int rankWorld;
            MPI_Comm_rank(MPI_COMM_WORLD, &rankWorld);
            printf("Process %d exiting groupMasterLoop", rankWorld);
            break;
        }

        int numberOfPixels = newTask.height * newTask.width;
        struct pixel *image = malloc(numberOfPixels * sizeof(pixel));

        MPI_Recv((void *)image, numberOfPixels, MPI_CUSTOM_PIXEL, (int)master,
                 IMAGE_TAG, MPI_COMM_WORLD, &status);
        // END OF RECEIVING PHASE
        animated_gif singleFrameGif;
        singleFrameGif.n_images = 1;
        *singleFrameGif.height = newTask.height;
        *singleFrameGif.width = newTask.width;
        *singleFrameGif.p = image;

        // APPLY FILTERS -- ONLY GROUPMASTER IS WORKING FOR NOW !
        apply_gray_filter(&singleFrameGif);
        apply_blur_filter(&singleFrameGif, 5, 20);
        apply_sobel_filter(&singleFrameGif);

        // SEND BACK TO MASTER
        MPI_Send((void *)&newTask, 1, MPI_CUSTOM_TASK,
                 (int)master, TASK_TAG, MPI_COMM_WORLD);

        MPI_Send((void *)image, numberOfPixels, MPI_CUSTOM_PIXEL,
                 (int)master, IMAGE_TAG, MPI_COMM_WORLD);
    }
}

void slaveGroup(MPI_Comm groupComm)
{
}
