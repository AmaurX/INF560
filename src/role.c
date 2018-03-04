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
    const int TASK_TAG_OFFSET = 100 * (1 + numberOfImages / 100);
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
    const int TASK_TAG_OFFSET = 100 * (1 + numberOfImages / 100);
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

/**
 * Slave work
 * 
 * To define properly a slave job, we need
 * - the image
 * - a frame ID
 * - a region of the frame to work on
 * - eventual parameters of filters
 * Image and parameters are fixed, therefore universally accessible
 * Frame ID can be static or dynamic 
 * region can be computed with groupRank (first try : regions are an interval of lines)
 * 
 * Between taks, slaves and the groupmaster must rebuild the image in order to re-dispatch the infos for the next treatment (problem of ghost pixels). The process should roughly be
 * 1/ Everyone agrees one what regions to treat - can be static if everyone knows what image they are treating -> possible if they know their group
 * 2/ first filter
 * 3/ allgather
 * 4/ 2nd filter
 * 5/ allgather
 * 6/ 3rdd filder
 * 7/ gather by groupmaster
 * 
 **/
void createCountsDisplacements(int frameHeight, int frameWidth, int pixelSize, int groupSize, int **countsTabOut, int **displacementsTabOut)
{
    int baseLen = frameHeight / groupSize;
    int remainderLen = frameHeight % groupSize;
    int *recvCountsTab = (int *)malloc(groupSize * sizeof(int));
    int *displacementsTab = (int *)malloc(groupSize * sizeof(int));
    for (int rank = 0; rank < groupSize; rank++)
    {
        if (rank < remainderLen)
        {
            recvCountsTab[rank] = (baseLen + 1) * frameWidth * sizeof(pixel);
            displacementsTab[rank] = rank * (baseLen + 1) * frameWidth * sizeof(pixel);
        }
        else
        {
            recvCountsTab[rank] = baseLen * frameWidth * sizeof(pixel);
            displacementsTab[rank] = (groupSize * baseLen + remainderLen) * frameWidth * sizeof(pixel);
        }
    }
    *countsTabOut = recvCountsTab;
    *displacementsTabOut = displacementsTab;
}

void slaveGroupLoop(MPI_Comm groupComm, animated_gif *image, int *imagesToProcess)
{
    int groupRank, groupSize;
    MPI_Comm_size(groupComm, &groupSize);
    MPI_Comm_rank(groupComm, &groupRank);
    int nFrames = image->n_images;
    for (int iFrame = 0; iFrame < nFrames; iFrame++)
    {
        if (imagesToProcess[iFrame] != 1)
        {
            // don't process the frame, go to the next
            continue;
        }
        //frame should be processed. Getting context infos
        int frameHeight = image->height[iFrame];
        int frameWidth = image->width[iFrame];
        pixel *pixelTab = image->p[iFrame];

        // finding region
        // first (remainderLen) processes get one line more
        // thus all lines are covered
        int baseLen = frameHeight / groupSize;
        int remainderLen = frameHeight % groupSize;
        //line Max is NOT included
        int lineMin, lineMax;
        if (groupRank < remainderLen)
        {
            lineMin = groupRank * (baseLen + 1);
            lineMax = (groupRank + 1) * (baseLen + 1);
        }
        else
        {
            lineMin = groupSize * baseLen + remainderLen;
            lineMax = (groupSize + 1) * baseLen + remainderLen;
        }
        int *recvCountsTab, *displacementsTab;
        createCountsDisplacements(frameHeight, frameWidth, sizeof(pixel), groupSize, &recvCountsTab, &displacementsTab);

        //building count and displacement in terms of bytes

        // partial filter 1 on a region of pixelTab

        // in-place collection of computed data
        MPI_Allgatherv(MPI_IN_PLACE, recvCountsTab[groupRank], MPI_BYTE, pixelTab, recvCountsTab, displacementsTab, MPI_BYTE, groupComm);

        // partial filter 2
        MPI_Allgatherv(MPI_IN_PLACE, recvCountsTab[groupRank], MPI_BYTE, pixelTab, recvCountsTab, displacementsTab, MPI_BYTE, groupComm);

        // partial filter 3

        //pixel tab is a tab of pixel, therefore the addition is pixel* + int
        MPI_Gatherv(pixelTab + lineMin * frameWidth, recvCountsTab[groupRank], MPI_BYTE, NULL, NULL, NULL, MPI_BYTE, 0, groupComm);

    }
}
