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

            MPI_Send((void *)&newTask, sizeof(task), MPI_BYTE,
                     groupMasterList[i], TASK_TAG, MPI_COMM_WORLD);

            int numberOfPixels = newTask.height * newTask.width;

            printf("M: Sending frame %d on %d\n", count, numberOfImages);

            //  MPI_Send((void *)(image->p[count]), numberOfPixels, MPI_CUSTOM_PIXEL,
            MPI_Send((void *)(image->p[count]), numberOfPixels * sizeof(pixel), MPI_BYTE,
                     (int)groupMasterList[i], IMAGE_TAG, MPI_COMM_WORLD);
            printf("M: Sent frame %d on %d successfully\n", count, numberOfImages);

            count++;
        }
    }

    while (numberOfProcessedImages < numberOfImages)
    {
        struct task doneTask;
        MPI_Status status;

        MPI_Recv((void *)&doneTask, sizeof(task), MPI_BYTE, MPI_ANY_SOURCE,
                 TASK_TAG, MPI_COMM_WORLD, &status);

        int frameNumber = doneTask.frameNumber;
        printf("M : Receiving treated frame %d\n", frameNumber);

        int numberOfPixels = doneTask.width * doneTask.height;
        int sender = status.MPI_SOURCE;

        MPI_Recv((void *)((pixel *)image->p[frameNumber]), numberOfPixels * sizeof(pixel), MPI_BYTE, sender,
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

            MPI_Send((void *)&newTask, sizeof(task), MPI_BYTE,
                     sender, TASK_TAG, MPI_COMM_WORLD);

            int numberOfPixels = newTask.height * newTask.width;

            MPI_Send((void *)image->p[count], numberOfPixels * sizeof(pixel), MPI_BYTE,
                     sender, IMAGE_TAG, MPI_COMM_WORLD);
            count++;
        }
    }

    for (int i = 0; i < numberOfGroupMaster; i++)
    {
        struct task endTask;

        endTask.id = -1;

        MPI_Send((void *)&endTask, sizeof(task), MPI_BYTE,
                 groupMasterList[i], TASK_TAG, MPI_COMM_WORLD);
    }
}

void groupMasterLoop(MPI_Comm groupComm)
{
    // if (groupRank == 0)
    // {
    // waitForDebug();
    // }
    printf("\t\tGM : Entering group master loop \n");

    while (true)
    {
        // RECEIVING FROM MASTER
        struct task newTask;
        MPI_Status status;

        MPI_Recv((void *)&newTask, sizeof(task), MPI_BYTE, (int)master,
                 TASK_TAG, MPI_COMM_WORLD, &status);

        if (newTask.id == -1)
        {
            int rankWorld;
            MPI_Comm_rank(MPI_COMM_WORLD, &rankWorld);
            printf("\t\tGM: Process %d exiting groupMasterLoop\n", rankWorld);
            break;
        }
        printf("\t\tGM : Receiving frame %d\n", newTask.frameNumber);

        // TODO: le segfault est au MPI_RECV dessous
        int numberOfPixels = newTask.height * newTask.width;
        int sizeRequested = numberOfPixels * sizeof(struct pixel);
        // struct pixel *image = (struct pixel *)malloc(sizeRequested);
        // struct pixel* image = (struct pixel*) calloc(numberOfPixels, sizeof(struct pixel));
        struct pixel *pixelList = (struct pixel *)malloc(sizeRequested);
        MPI_Recv((void *)pixelList, numberOfPixels, MPI_CUSTOM_PIXEL, (int)master,

        // MPI_Recv((void *)pixelList, numberOfPixels * sizeof(pixel), MPI_BYTE, (int)master,
                 IMAGE_TAG, MPI_COMM_WORLD, &status);

        printf("\t\tGM : Received frame %d successfully\n", newTask.frameNumber);

        // END OF RECEIVING PHASE
        animated_gif singleFrameGif;
        singleFrameGif.n_images = 1;
        singleFrameGif.height = &(newTask.height);
        singleFrameGif.width = &(newTask.width);
        singleFrameGif.p = &(pixelList);

        // APPLY FILTERS -- ONLY GROUPMASTER IS WORKING FOR NOW !
        apply_gray_filter(&singleFrameGif);
        apply_blur_filter(&singleFrameGif, 5, 20);
        apply_sobel_filter(&singleFrameGif);

        // SEND BACK TO MASTER
        printf("\t\tGM : Sending treated frame %d back to master \n", newTask.frameNumber);

        MPI_Send((void *)&newTask, sizeof(task), MPI_BYTE,
                 (int)master, TASK_TAG, MPI_COMM_WORLD);

        MPI_Send((void *)pixelList, numberOfPixels * sizeof(pixel), MPI_BYTE,
                 (int)master, IMAGE_TAG, MPI_COMM_WORLD);

        printf("\t\tGM : Sent treated frame %d back to master successfully\n", newTask.frameNumber);
        //free(image);
    }
}

void slaveGroupLoop(MPI_Comm groupComm)
{
}
