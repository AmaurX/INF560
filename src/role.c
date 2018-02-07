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
    printf("Entering master loop \n");
    int count = 0;
    int numberOfImages = image->n_images;

    // int doneImages[numberOfImages] = {0};
    int numberOfProcessedImages = 0;
    printf("M: Beginning first round of distribution \n");

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

            printf("M: Sending frame %d on %d\n", count, numberOfImages);

            MPI_Send((void *)(image->p[count]), numberOfPixels, MPI_CUSTOM_PIXEL,
                     (int)groupMasterList[i], IMAGE_TAG, MPI_COMM_WORLD);
            count++;
            printf("M: Sent frame %d on %d successfully\n", count, numberOfImages);
        }
    }

    while (numberOfProcessedImages < numberOfImages)
    {
        struct task doneTask;
        MPI_Status status;

        MPI_Recv((void *)&doneTask, 1, MPI_CUSTOM_TASK, MPI_ANY_SOURCE,
                 TASK_TAG, MPI_COMM_WORLD, &status);

        int frameNumber = doneTask.frameNumber;
        printf("M : Receiving treated frame %d\n", frameNumber);

        int numberOfPixels = doneTask.width * doneTask.height;
        int sender = status.MPI_SOURCE;

        MPI_Recv((void *)((pixel *)image->p[frameNumber]), numberOfPixels, MPI_CUSTOM_PIXEL, sender,
                 IMAGE_TAG, MPI_COMM_WORLD, &status);

        printf("M : Received treated frame %d\n", frameNumber);

        // doneImages[doneTask.frameNumber] = 1;
        if (status.MPI_ERROR == 0)
        {
            numberOfProcessedImages++;
            printf("M: Frame %d on %d was processed successfully. %d frame completed in total \n", frameNumber, numberOfImages, numberOfProcessedImages);
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

            MPI_Send((void *)image->p[count], numberOfPixels, MPI_CUSTOM_PIXEL,
                     sender, IMAGE_TAG, MPI_COMM_WORLD);
            count++;
        }
    }
}

void groupMasterLoop(MPI_Comm groupComm)
{
    printf("\t\tGM : Entering group master loop \n");

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
        printf("\t\tGM : Receiving frame %d\n", newTask.frameNumber);

        int numberOfPixels = newTask.height * newTask.width;
        struct pixel *image = (struct pixel *)malloc(numberOfPixels * sizeof(pixel));

        MPI_Recv((void *)image, numberOfPixels, MPI_CUSTOM_PIXEL, (int)master,
                 IMAGE_TAG, MPI_COMM_WORLD, &status);

        printf("\t\tGM : Received frame %d successfully\n", newTask.frameNumber);

        // END OF RECEIVING PHASE
        animated_gif singleFrameGif;
        singleFrameGif.n_images = 1;
        singleFrameGif.height = &(newTask.height);
        singleFrameGif.width = &(newTask.width);
        singleFrameGif.p = &(image);

        // APPLY FILTERS -- ONLY GROUPMASTER IS WORKING FOR NOW !
        apply_gray_filter(&singleFrameGif);
        apply_blur_filter(&singleFrameGif, 5, 20);
        apply_sobel_filter(&singleFrameGif);

        // SEND BACK TO MASTER
        printf("\t\tGM : Sending treated frame %d back to master \n", newTask.frameNumber);

        MPI_Send((void *)&newTask, 1, MPI_CUSTOM_TASK,
                 (int)master, TASK_TAG, MPI_COMM_WORLD);

        MPI_Send((void *)image, numberOfPixels, MPI_CUSTOM_PIXEL,
                 (int)master, IMAGE_TAG, MPI_COMM_WORLD);

        printf("\t\tGM : Sent treated frame %d back to master successfully\n", newTask.frameNumber);
    }
}

void slaveGroupLoop(MPI_Comm groupComm)
{
}
