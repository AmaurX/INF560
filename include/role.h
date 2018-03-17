/**
 * role.h
 * Used to attribute role to the processes
 * 
 */

#include <mpi.h>

#define TASK_TAG 0
#define IMAGE_TAG 1

/// \brief hierarchical position of the process
enum role
{
    master,
    groupmaster,
    slave,
} role;

enum role
giveRoleInGroup(int rankGroup);

void masterLoop(int *groupMasterList, int numberOfGroupMaster, animated_gif *image, int *imageToTreat, MPI_Comm groupComm);

void groupMasterLoop(MPI_Comm groupComm, int groupIndex, animated_gif *image, int *imageToTreat);

void slaveGroupLoop(MPI_Comm groupComm, animated_gif *image, int *imagesToProcess);

void createCountsDisplacements(int frameHeight, int frameWidth, int pixelSize, int groupSize, int **countsTabOut, int **displacementsTabOut);

void getLineWindow(int frameHeight, int groupSize, int groupRank, int *lineMinOut, int *lineMaxOut);

int groupMasterizeFrame(MPI_Comm groupComm, animated_gif *image, int iFrame, int groupIndex, struct task *taskOut, struct pixel** pixelTabOut);

void processFrameAlone(int frameHeight, int frameWidth, struct pixel *pixelTab);
