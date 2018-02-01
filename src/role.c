#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "main.h"
#include "role.h"

enum role giveRoleInGroup(int rankGroup)
{
    if (rankGroup == 0)
        return groupmaster;
    else
        return slave;
}

void masterLoop(MPI_Comm *groupCommList, animated_gif *image)
{
    int count = 0;

    while (count < image->n_images)
    {
        count++;
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
        apply_blur_filter(&singleFrameGif);
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
