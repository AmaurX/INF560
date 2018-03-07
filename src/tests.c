#include <stdio.h>
#include <stdlib.h>

#include "tests.h"
#include "role.h"

void test()
{
    // testProcessAttribution();
    testLineRepartition();
}

void testProcessAttribution()
{
    const int frameCard = 3;
    const int processCard = 3;
    int nFrameList[] = {1, 4, 16};
    int nProcessList[] = {1, 2, 8, 16};
    for (int i = 0; i < frameCard; i++)
    {
        for (int j = 0; j < processCard; j++)
        {
            int nFrame = nFrameList[i];
            int nProcess = nProcessList[j];
            printf("==========================================\n"
                   "===== test with #Proc=%d #Frames=%d ======\n",
                   nProcessList[j], nFrame);
            animated_gif fakeImage = {
                .n_images = nFrame};
            int nMaxGroup;
            int *workgroupList = attributeNumberOfProcess(nProcess, &fakeImage, &nMaxGroup);

            /*
            printing the workgroup list
            */
            printf("Workgroup list (#=%d)\n", nMaxGroup);
            for (int k = 0; k < nMaxGroup; k++)
            {
                printf("%d ", workgroupList[k]);
            }

            /*
            Process attribution
            */
            printf("\n Attribution:\n");
            int *groupAttribution = (int *)calloc(nProcess, sizeof(int));
            for (int k = 0; k < nProcess; k++)
            {
                printf("%d ", whichCommunicator(workgroupList, nMaxGroup, k));
            }
            // printf("\n");

            /*
            Group masters
            */
            int groupMasterNum;
            int *groupMasterList = createGroupMasterList(workgroupList, nMaxGroup, &groupMasterNum);
            printf("\n Group Masters : #=%d\n", groupMasterNum);
            for (int k = 0; k < groupMasterNum; k++)
            {
                printf("%d ", groupMasterList[k]);
            }
            printf("\n");

            /*
            Check static frames attribution
            */
            // printf("computing static job repartition...\n");
            int *imagesPerGroup = calloc(nMaxGroup, sizeof(int));
            int *treatedImages = calloc(nFrame, sizeof(int));

            for (int iGMaster = 0; iGMaster < nMaxGroup; iGMaster++)
            {
                int iProc = groupMasterList[iGMaster];
                int groupIndex = whichCommunicator(workgroupList, nMaxGroup, iProc);
                int *imagesToTreat = getImagesToTreat(groupIndex, workgroupList, nMaxGroup, nFrame);
                // checks
                for (int iFrame = 0; iFrame < nFrame; iFrame++)
                {
                    int toTreat = imagesToTreat[iFrame];
                    treatedImages[iFrame] += toTreat;
                    imagesPerGroup[iGMaster] += toTreat;
                }
                free(imagesToTreat);
            }
            // checks that each frame is treated exactly once
            for (int i = 0; i < nFrame; i++)
            {
                if (treatedImages[i] != 1)
                {
                    printf("WARNING : frame %d is treated %d times (expected once)\n", i, treatedImages[i]);
                }
            }
            //prints job repartition
            printf("Job repartition (num of Frames per Group)\n");
            for (int i = 0; i < nMaxGroup; i++)
            {
                printf("%d ", imagesPerGroup[i]);
            }
            printf("\n");

            free(workgroupList);
            free(groupAttribution);
            free(groupMasterList);
            free(imagesPerGroup);
            free(treatedImages);
        }
    }

}

void testLineRepartition()
    {
        // waitForDebug();
        printf("--\nTesting line repartition \n");
        int heights[] = {100, 642, 0};
        int groupSizes[] = {1, 5, 0};

        for (int *ph = heights; *ph != 0; ph++)
        {
            int height = *ph;
            for (int *pg = groupSizes; *pg != 0; pg++)
            {
                int groupSize = *pg;
                printf("=== Height=%d ; groupSize=%d\n", height, groupSize);

                int *recvCountsTab, *displacementsTab;
                createCountsDisplacements(height, 1, 1, groupSize, &recvCountsTab, &displacementsTab);
                for (int iProc = 0; iProc < groupSize; iProc++)
                {
                    int lineMin, lineMax;
                    getLineWindow(height, groupSize, iProc, &lineMin, &lineMax);
                    printf("#%d:\t %4d->%4d \t(c=%d)\n", iProc, lineMin, lineMax, lineMax - lineMin);
                }
                printf("Counts tab\n");
                for (int iProc = 0; iProc < groupSize; iProc++)
                {
                    printf("%d ", recvCountsTab[iProc]);
                }
                printf("\nDisplacements tab\n");
                for (int iProc = 0; iProc < groupSize; iProc++)
                {
                    printf("%d ", displacementsTab[iProc]);
                }


            }
        }
    }