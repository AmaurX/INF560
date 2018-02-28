#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "main.h"
#include "filters.h"
#include <time.h>
#include <mpi.h>

#include "role.h"

enum role giveRoleInGroup(int rankGroup)
{
    if (rankGroup == 0)
        return groupmaster;
    else
        return slave;
}

void masterLoop(int *groupMasterList, int numberOfGroupMaster, animated_gif *image, int *imageToTreat, MPI_Comm groupComm)
{
    printf("Entering master loop \n");
    int worldRank, groupSize;
    MPI_Comm_rank(MPI_COMM_WORLD, &worldRank);
    MPI_Comm_size(groupComm, &groupSize);
    int count = 0;
    int numberOfImages = image->n_images;

    /// allows tasks and pixels-related send to have different tags
    /// and to have a offset multiple of 100 for better reading
    const int TASK_TAG_OFFSET = 100 * (1 + numberOfImages/100);
    struct task *taskHistory = (struct task *)malloc(numberOfImages * sizeof(struct task));

    MPI_Status *statusListImage = (MPI_Status *)malloc(numberOfImages * sizeof(MPI_Status));
    MPI_Status *statusListTask = (MPI_Status *)malloc(numberOfImages * sizeof(MPI_Status));

    MPI_Request *requestListTask = (MPI_Request *)malloc(numberOfImages * sizeof(MPI_Request));
    MPI_Request *requestListImage = (MPI_Request *)malloc(numberOfImages * sizeof(MPI_Request));

    // int doneImages[numberOfImages] = {0};
    int numberOfProcessedImages = 0;

    /*
    MASTER OPEN RECEIVE SLOTS FOR EACH FOREIGN IMAGE
    */
    for (int i = 0; i < numberOfImages; i++)
    {
        if (imageToTreat[i] == 0)
        {
            MPI_Irecv((void *)&(taskHistory[i]), sizeof(task), MPI_BYTE, MPI_ANY_SOURCE,
                        TASK_TAG_OFFSET + i, MPI_COMM_WORLD, &(requestListTask[i]));

            int numberOfPixels = image->height[i] * image->width[i];

            MPI_Irecv((void *)((pixel *)image->p[i]), numberOfPixels * sizeof(pixel), MPI_BYTE, MPI_ANY_SOURCE,
                      i, MPI_COMM_WORLD, &(requestListImage[i]));
        }
    }

    /*
    MASTER TREATS ITS OWN IMAGES
    */
    for (int i = 0; i < numberOfImages; i++)
    {
        if (imageToTreat[i] == 1)
        {
            double startWorkTime, endWorkTime, totalWorkDuration = 0;
            struct pixel *pixelList = image->p[i];
            int height, width;

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
            totalWorkDuration = (endWorkTime - startWorkTime);

            // Group Master jst finished gathering all parts
            taskHistory[i].id = i;
            taskHistory[i].frameNumber = i;
            taskHistory[i].width = *singleFrameGif.width;
            taskHistory[i].height = *singleFrameGif.height;
            taskHistory[i].totalTimeTaken = MPI_Wtime() - startWorkTime;
            taskHistory[i].totalTimeWorking = totalWorkDuration;
            taskHistory[i].groupIndex = 0;
            taskHistory[i].workgroupSize = groupSize;
        }
    }

    /*
    MASTER WAITS FOR ALL IMAGES AND FINALIZE
    */
    for (int i = 0; i < numberOfImages; i++)
    {
        if (imageToTreat[i] == 0)
        {
            MPI_Wait(&requestListTask[i], &statusListTask[i]);
            MPI_Wait(&requestListImage[i], &statusListImage[i]);

            // taskHistory[i].endTimestamp = MPI_Wtime();
            // taskHistory[i].masterTime = taskHistory[i].endTimestamp - taskHistory[i].startTimestamp;

            if (statusListImage[i].MPI_ERROR == 0)
            {
                numberOfProcessedImages++;
                printf("M: Frame %d on %d was processed successfully. %d frame completed in total \n", i, numberOfImages, numberOfProcessedImages);
            }
        }
    }
    autosave_taskHistory(taskHistory, numberOfImages);
}

void groupMasterLoop(MPI_Comm groupComm, animated_gif *image, int *imageToTreat)
{
    int worldRank, groupSize;
    MPI_Comm_rank(MPI_COMM_WORLD, &worldRank);
    MPI_Comm_size(groupComm, &groupSize);
    // if (groupRank == 0)
    // {
    // waitForDebug();
    // }
    printf("\t\tGM : Entering group master loop \n");

    int numberOfImages = image->n_images;
    const int TASK_TAG_OFFSET = 100 * (1 + numberOfImages/100);
    double startWorkTime, endWorkTime, endTotalTime, totalWorkDuration = 0;

    for (int i = 0; i < numberOfImages; i++)
    {
        if (imageToTreat[i] == 1)
        {
            struct pixel *pixelList = image->p[i];

            startWorkTime = MPI_Wtime();
            struct task workTask;
            // END OF RECEIVING PHASE
            animated_gif singleFrameGif;
            singleFrameGif.n_images = 1;
            singleFrameGif.height = &(image->height[i]);
            singleFrameGif.width = &(image->width[i]);
            singleFrameGif.p = &(pixelList);
            int numberOfPixels = singleFrameGif.height[0] * singleFrameGif.width[0];
            // APPLY FILTERS -- ONLY GROUPMASTER IS WORKING FOR NOW !
            apply_gray_filter(&singleFrameGif);
            apply_blur_filter(&singleFrameGif, 5, 20);
            apply_sobel_filter(&singleFrameGif);

            // Group Master just finished on his part - just counting pure effective working time
            endWorkTime = MPI_Wtime();
            totalWorkDuration += (endWorkTime - startWorkTime);

            // Group Master jst finished gathering all parts
            workTask.id = i;
            workTask.frameNumber = i;
            workTask.height = image->height[i];
            workTask.width = image->width[i];
            workTask.totalTimeTaken = MPI_Wtime() - startWorkTime;
            workTask.totalTimeWorking = totalWorkDuration;
            workTask.groupIndex = -1;
            workTask.workgroupSize = groupSize;

            printf("\t\tGM : Sending treated frame %d back to master \n", workTask.frameNumber);

            MPI_Send((void *)&workTask, sizeof(task), MPI_BYTE,
                     (int)master, TASK_TAG_OFFSET + i, MPI_COMM_WORLD);

            MPI_Send((void *)pixelList, numberOfPixels * sizeof(pixel), MPI_BYTE,
                     (int)master, i, MPI_COMM_WORLD);

            // printf("\t\tGM : Sent treated  frame %d back to master successfully\n", workTask.frameNumber);
        }
    }
}

void slaveGroupLoop(MPI_Comm groupComm)
{
}
