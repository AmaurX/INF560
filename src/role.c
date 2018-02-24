#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "main.h"
#include "filters.h"
#include <mpi.h>

#include "role.h"

enum role giveRoleInGroup(int rankGroup)
{
    if (rankGroup == 0)
        return groupmaster;
    else
        return slave;
}

void masterLoop(int *groupMasterList, int numberOfGroupMaster, animated_gif *image, int *imageToTreat)
{
    printf("Entering master loop \n");
    int count = 0;
    int numberOfImages = image->n_images;
    struct task *taskHistory = (struct task *)malloc(numberOfImages * sizeof(struct task));

    MPI_Status *statusList = (MPI_Status *)malloc(numberOfImages * sizeof(MPI_Status));

    // int doneImages[numberOfImages] = {0};
    int numberOfProcessedImages = 0;

    for (int i = 0; i < numberOfImages; i++)
    {
        if (imageToTreat[i] == 0)
        {
            MPI_Irecv((void *)&(taskHistory[i]), sizeof(task), MPI_BYTE, MPI_ANY_SOURCE,
                      -(i + 1), MPI_COMM_WORLD, &(statusList[i]));

            int frameNumber = taskHistory[i].frameNumber;
            printf("M : Receiving treated frame %d\n", frameNumber);

            int numberOfPixels = taskHistory[i].width * taskHistory[i].height;
            int sender = statusList[i].MPI_SOURCE;

            MPI_Irecv((void *)((pixel *)image->p[frameNumber]), numberOfPixels * sizeof(pixel), MPI_BYTE, sender,
                      i + 1, MPI_COMM_WORLD, &(statusList[i]));

            taskHistory[i].endTimestamp = MPI_Wtime();
            taskHistory[i].masterTime = taskHistory[i].endTimestamp - taskHistory[i].startTimestamp;
            printf("M : Received treated frame %d\n", frameNumber);
            // store done task for analysis
            copyTask(taskHistory[i], &taskHistory[taskHistory[i].frameNumber]);

            // doneImages[taskHistory[i].frameNumber] = 1;
            if (statusList[i].MPI_ERROR == 0)
            {
                numberOfProcessedImages++;
                printf("M: Frame %d on %d was processed successfully. %d frame completed in total \n", frameNumber, numberOfImages, numberOfProcessedImages);
            }
        }
    }

    for (int i = 0; i < numberOfImages; i++)
    {
        if (imageToTreat[i] == 1)
        {
            struct pixel *pixelList = image->p[i];

            startWorkTime = MPI_Wtime();

            // END OF RECEIVING PHASE
            animated_gif singleFrameGif;
            singleFrameGif.n_images = 1;
            singleFrameGif.height = &(image->height[i]);
            singleFrameGif.width = &(image->width[i]);
            singleFrameGif.p = &(pixelList);

            // APPLY FILTERS -- ONLY GROUPMASTER IS WORKING FOR NOW !
            apply_gray_filter(&singleFrameGif);
            apply_blur_filter(&singleFrameGif, 5, 20);
            apply_sobel_filter(&singleFrameGif);

            // Group Master just finished on his part - just counting pure effective working time
            endWorkTime = MPI_Wtime();
            totalWorkDuration += (endWorkTime - startWorkTime);

            // Group Master jst finished gathering all parts
            taskHistory[i].totalTimeTaken = MPI_Wtime() - startWorkTime;
            taskHistory[i].totalTimeWorking = totalWorkDuration;
            taskHistory[i].workgroupSize = groupSize;
        }
    }
    write_taskHistory("results/out.csv", taskHistory, numberOfImages);
}

void groupMasterLoop(MPI_Comm groupComm, animated_gif *image)
{
    int worldRank, groupSize;
    MPI_Comm_rank(MPI_COMM_WORLD, &worldRank);
    MPI_Comm_size(groupComm, &groupSize);
    // if (groupRank == 0)
    // {
    // waitForDebug();
    // }
    printf("\t\tGM : Entering group master loop \n");

    while (true)
    {
        // RECEIVING FROM MASTER
        struct task newTask;
        double startWorkTime, endWorkTime, endTotalTime, totalWorkDuration = 0;
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
        //sensitive malloc : will cherck allocation
        // struct pixel *pixelList = (struct pixel *)malloc(sizeRequested);
        // if (pixelList == NULL)
        // {
        //     //allocation failed
        //     fprintf(stderr, "!!! FATAL : memory allocation failed on group master #w=%d ", worldRank);
        //     break;
        // }
        //MPI_Recv((void *)pixelList, numberOfPixels, MPI_CUSTOM_PIXEL, (int)master,

        // MPI_Recv((void *)pixelList, numberOfPixels * sizeof(pixel), MPI_BYTE, (int)master,
        //         IMAGE_TAG, MPI_COMM_WORLD, &status);

        struct pixel *pixelList = image->p[newTask.frameNumber];

        printf("\t\tGM : Received frame %d successfully\n", newTask.frameNumber);

        startWorkTime = MPI_Wtime();

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

        // Group Master just finished on his part - just counting pure effective working time
        endWorkTime = MPI_Wtime();
        totalWorkDuration += (endWorkTime - startWorkTime);

        // Group Master jst finished gathering all parts
        newTask.totalTimeTaken = MPI_Wtime() - startWorkTime;
        newTask.totalTimeWorking = totalWorkDuration;
        newTask.workgroupSize = groupSize;
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
