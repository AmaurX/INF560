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
    // waitForDebug();
    dbprintf("Entering master loop \n");
    int worldRank, groupSize;
    MPI_Comm_rank(MPI_COMM_WORLD, &worldRank);
    MPI_Comm_size(groupComm, &groupSize);
    int count = 0;
    int numberOfImages = image->n_images;

    /// allows tasks and pixels-related send to have different tags
    /// and to have a offset multiple of 100 for better reading
    const int TASK_TAG_OFFSET = 100 * (1 + numberOfImages / 100);
#if USE_METRICS
    struct task *taskHistory = (struct task *)malloc(numberOfImages * sizeof(struct task));
    MPI_Status *statusListTask = (MPI_Status *)malloc(numberOfImages * sizeof(MPI_Status));
    MPI_Request *requestListTask = (MPI_Request *)malloc(numberOfImages * sizeof(MPI_Request));
#endif

    MPI_Status *statusListImage = (MPI_Status *)malloc(numberOfImages * sizeof(MPI_Status));

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
#if USE_METRICS
            MPI_Irecv((void *)&(taskHistory[i]), sizeof(task), MPI_BYTE, MPI_ANY_SOURCE,
                      TASK_TAG_OFFSET + i, MPI_COMM_WORLD, &(requestListTask[i]));
#endif
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
        if (imageToTreat[i] != 1)
        {
            continue;
        }
#if 0            
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
#else
        struct pixel *pixelTab;
        task *taskHist;
#if USE_METRICS
        taskHist = &taskHistory[i];
#else
        taskHist = NULL;
#endif
        groupMasterizeFrame(groupComm, image, i, 0, taskHist, &pixelTab);
#endif
    }

    /*
    MASTER WAITS FOR ALL IMAGES AND FINALIZE
    */
    for (int i = 0; i < numberOfImages; i++)
    {
        if (imageToTreat[i] == 0)
        {
#if USE_METRICS
            MPI_Wait(&requestListTask[i], &statusListTask[i]);
#endif
            MPI_Wait(&requestListImage[i], &statusListImage[i]);

            // taskHistory[i].endTimestamp = MPI_Wtime();
            // taskHistory[i].masterTime = taskHistory[i].endTimestamp - taskHistory[i].startTimestamp;

            if (statusListImage[i].MPI_ERROR == 0)
            {
                numberOfProcessedImages++;
                dbprintf("M: Frame %d on %d was processed successfully. %d frame completed in total \n", i, numberOfImages, numberOfProcessedImages);
            }
        }
    }
#if USE_METRICS
    autosave_taskHistory(taskHistory, numberOfImages);
#endif
}

void groupMasterLoop(MPI_Comm groupComm, int groupIndex, animated_gif *image, int *imageToTreat)
{
    int worldRank, groupSize;
    MPI_Comm_rank(MPI_COMM_WORLD, &worldRank);
    MPI_Comm_size(groupComm, &groupSize);
    // if (groupRank == 0)
    // {
    // waitForDebug();
    // }
    dbprintf("\t\tGM : Entering group master loop \n");

    int numberOfImages = image->n_images;
    const int TASK_TAG_OFFSET = 100 * (1 + numberOfImages / 100);

    for (int iFrame = 0; iFrame < numberOfImages; iFrame++)
    {
        if (imageToTreat[iFrame] != 1)
        {
            //don't process image, go to next
            continue;
        }

        //frame should be processed. Getting context infos
        int frameHeight = image->height[iFrame];
        int frameWidth = image->width[iFrame];
        // struct pixel *pixelTab = image->p[iFrame];
        int numberOfPixels = frameHeight * frameWidth;

        struct task workTask;

        struct pixel *pixelTab;
        groupMasterizeFrame(groupComm, image, iFrame, groupIndex, &workTask, &pixelTab);

        dbprintf("\t\tGM : Sending treated frame %d back to master \n", workTask.frameNumber);

#if USE_METRICS

        MPI_Send((void *)&workTask, sizeof(task), MPI_BYTE,
                 (int)master, TASK_TAG_OFFSET + iFrame, MPI_COMM_WORLD);

#endif

        MPI_Send((void *)pixelTab, numberOfPixels * sizeof(pixel), MPI_BYTE,
                 (int)master, iFrame, MPI_COMM_WORLD);

        // dbprintf("\t\tGM : Sent treated  frame %d back to master successfully\n", workTask.frameNumber);
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
void slaveGroupLoop(MPI_Comm groupComm, animated_gif *image, int *imagesToProcess)
{
#if USE_SLAVES
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
        int lineMin, lineMax;
        getLineWindow(frameHeight, groupSize, groupRank, &lineMin, &lineMax);
        int *recvCountsTab, *displacementsTab;
        createCountsDisplacements(frameHeight, frameWidth, sizeof(struct pixel), groupSize, &recvCountsTab, &displacementsTab);

        //building count and displacement in terms of bytes

        // partial filter 1 on a region of pixelTab
        lined_gray_filter(pixelTab, frameHeight, frameWidth, lineMin, lineMax);

        // in-place collection of computed data
        MPI_Allgatherv(MPI_IN_PLACE, recvCountsTab[groupRank], MPI_BYTE, pixelTab, recvCountsTab, displacementsTab, MPI_BYTE, groupComm);

#if DISTRIBUTED_BLUR_FILTER
        // partial filter 2
        MPI_Allgatherv(MPI_IN_PLACE, recvCountsTab[groupRank], MPI_BYTE, pixelTab, recvCountsTab, displacementsTab, MPI_BYTE, groupComm);

#else
        // do nothing and wait for the sacred truth to be sent by our holy groupMaster
        MPI_Bcast(pixelTab, frameWidth * frameHeight * sizeof(struct pixel), MPI_BYTE, 0, groupComm);
#endif

        // partial filter 3
        lined_sobelf(pixelTab, frameHeight, frameWidth, lineMin, lineMax);

        //pixel tab is a tab of pixel, therefore the addition is pixel* + int
        MPI_Gatherv(pixelTab + lineMin * frameWidth, recvCountsTab[groupRank], MPI_BYTE, NULL, NULL, NULL, MPI_BYTE, 0, groupComm);

        free(recvCountsTab);
        free(displacementsTab);
    }
#else
    return;
#endif
}

int groupMasterizeFrame(MPI_Comm groupComm, animated_gif *image, int iFrame, int groupIndex, struct task *taskOut, struct pixel **pixelTabOut)
{
    // waitForDebug();
    double startWorkTime, endWorkTime, endTotalTime, totalWorkDuration = 0;
    int groupSize, groupRank = 0;
    MPI_Comm_size(groupComm, &groupSize);

    //frame should be processed. Getting context infos
    int frameHeight = image->height[iFrame];
    int frameWidth = image->width[iFrame];
    // struct pixel *pixelTab = image->p[iFrame];
    int numberOfPixels = frameHeight * frameWidth;

    struct pixel *pixelTab = image->p[iFrame];

    startWorkTime = MPI_Wtime();
    struct task workTask;
    // END OF RECEIVING PHASE

#if USE_SLAVES
    // finding region
    int lineMin, lineMax;
    getLineWindow(frameHeight, groupSize, 0, &lineMin, &lineMax);
    int *recvCountsTab, *displacementsTab;
    createCountsDisplacements(frameHeight, frameWidth, sizeof(pixel), groupSize, &recvCountsTab, &displacementsTab);

    // partial filter 1 on a region of pixelTab
    lined_gray_filter(pixelTab, frameHeight, frameWidth, lineMin, lineMax);

    // in-place collection of computed data
    MPI_Allgatherv(MPI_IN_PLACE, recvCountsTab[groupRank], MPI_BYTE, pixelTab, recvCountsTab, displacementsTab, MPI_BYTE, groupComm);

#if DISTRIBUTED_BLUR_FILTER
    // partial filter 2
    MPI_Allgatherv(MPI_IN_PLACE, recvCountsTab[groupRank], MPI_BYTE, pixelTab, recvCountsTab, displacementsTab, MPI_BYTE, groupComm);
#else
    // CENTRAL filter for now
    central_blur_filter(pixelTab, frameHeight, frameWidth, 5, 20);
    MPI_Bcast(pixelTab, numberOfPixels * sizeof(struct pixel), MPI_BYTE, 0, groupComm);
#endif

    // partial filter 3
    lined_sobelf(pixelTab, frameHeight, frameWidth, lineMin, lineMax);

    //pixel tab is a tab of pixel, therefore the addition is pixel* + int
    MPI_Gatherv(MPI_IN_PLACE, recvCountsTab[groupRank], MPI_BYTE, pixelTab, recvCountsTab, displacementsTab, MPI_BYTE, 0, groupComm);

    free(recvCountsTab);
    free(displacementsTab);
#else
    animated_gif singleFrameGif;
    singleFrameGif.n_images = 1;
    singleFrameGif.height = &frameHeight;
    singleFrameGif.width = &frameWidth;
    singleFrameGif.p = &(pixelTab);
    // APPLY FILTERS -- ONLY GROUPMASTER IS WORKING FOR NOW !
    apply_gray_filter(&singleFrameGif);
    apply_blur_filter(&singleFrameGif, 5, 20);
    apply_sobel_filter(&singleFrameGif);
#endif

    if (taskOut != NULL)
    {
        //filling a task structure as feedback
        endWorkTime = MPI_Wtime();
        totalWorkDuration += (endWorkTime - startWorkTime);

        // Group Master jst finished gathering all parts
        workTask.id = iFrame;
        workTask.frameNumber = iFrame;
        workTask.height = image->height[iFrame];
        workTask.width = image->width[iFrame];
        workTask.totalTimeTaken = MPI_Wtime() - startWorkTime;
        workTask.totalTimeWorking = totalWorkDuration;
        workTask.groupIndex = groupIndex;
        workTask.workgroupSize = groupSize;
        *taskOut = workTask;
    }
    if (pixelTabOut != NULL)
    {
        *pixelTabOut = pixelTab;
    }

    return 0;
}

void getLineWindow(int frameHeight, int groupSize, int groupRank, int *lineMinOut, int *lineMaxOut)
{
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
        lineMin = groupRank * baseLen + remainderLen;
        lineMax = (groupRank + 1) * baseLen + remainderLen;
    }
    *lineMinOut = lineMin;
    *lineMaxOut = lineMax;
}

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
            recvCountsTab[rank] = (baseLen + 1) * frameWidth * pixelSize;
            displacementsTab[rank] = rank * (baseLen + 1) * frameWidth * pixelSize;
        }
        else
        {
            recvCountsTab[rank] = baseLen * frameWidth * pixelSize;
            displacementsTab[rank] = (rank * baseLen + remainderLen) * frameWidth * pixelSize;
        }
    }
    *countsTabOut = recvCountsTab;
    *displacementsTabOut = displacementsTab;
}
