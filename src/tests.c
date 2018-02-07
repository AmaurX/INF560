#include <stdio.h>
#include <stdlib.h>

#include "tests.h"

void test(){
	testProcessAttribution();
}

void testProcessAttribution()
{
    const int frameCard = 3;
    const int processCard = 3;
    int nFrameList[] = {1, 4, 40};
    int nProcessList[] = {2, 8, 40};
    for (int i = 0; i < frameCard; i++)
    {
        for (int j = 0; j < processCard; j++)
        {
            int nFrame = nFrameList[i];
            int nProcess = nProcessList[j];
            printf("================================\n"
                   "===== test with M=%d L=%d ======\n",
                   nProcessList[j], nFrame);
            int nMaxGroup = nFrame + 1;
            int *workgroupList = (int *)calloc(nMaxGroup, sizeof(int));
            animated_gif fakeImage = {
                .n_images = nFrame};
            attributeNumberOfProcess(workgroupList, nProcessList[j], &fakeImage);

            for (int k = 0; k < nMaxGroup; k++)
            {
                printf("%d ", workgroupList[k]);
            }
            printf("\n Attribution:\n");

            int *groupAttribution = (int *)calloc(nProcess, sizeof(int));
            for (int k = 0; k < nProcess; k++)
            {
                printf("%d ", whichCommunicator(workgroupList, nMaxGroup, k));
            }
            printf("\n");

            free(workgroupList);
            free(groupAttribution);
        }
    }
}