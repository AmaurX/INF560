#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "main.h"

int giveRoleInGroup(int rankGroup)
{
    if (rankGroup == 0)
        return role::groupmaster;
    else
        return role::slave;
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
        task newTask;
        MPI_Status status;

        MPI_Recv(&task, 1, MPI_CUSTOM_TASK, role::master,
                 TASK_TAG, MPI_COMM_WORLD, &status);

        int numberOfPixels = newTask.height * newTask.width;
        pixel *image = malloc(numberOfPixels * sizeof(pixel));

        MPI_Recv(image, numberOfPixels, MPI_CUSTOM_PIXEL, role::master,
                 IMAGE_TAG, MPI_COMM_WORLD, &status);
        // END OF RECEIVING PHASE
    }
}

void slaveGroup(MPI_Comm groupComm)
{
}
