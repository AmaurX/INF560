#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <omp.h>
#include <mpi.h>
#include <math.h>

#include "gif_utils.h"
#include "filters.h"
#include "role.h"
#include "tests.h"
#include "main.h"

int parallel_process(char *input_filename, char *output_filename)
{
	int rankWorld, commWorldSize, groupIndex;
	/// GroupMaster or Slave ? everything lies in the value of that integer...
	int groupRank;
	/// Size of the work group
	int groupSize;
	/// Used to determine the role of the current process
	enum role groupRole;
	/// Group communicator ( MPI_COMM_NULL if master )
	MPI_Comm groupComm;

	animated_gif *image;
	int numFrames;
	double timeStart, timeEnd;
	// struct timeval t1, t2;
	double duration;

	int *workgroupList;
	int listSize;
	int *imagesToProcess;

	init_custom_datatypes();

	/* IMPORT Timer start */
	timeStart = MPI_Wtime();

	/* Load file and store the pixels in array */
	image = load_pixels(input_filename);
	if (image == NULL)
	{
		fprintf(stderr, "Image failed to load. Exiting\n");
		return 1;
	}
	numFrames = image->n_images;

	/* IMPORT Timer stop */
	timeEnd = MPI_Wtime();
	duration = timeEnd - timeStart;
	dbprintf("GIF loaded from file %s with %d image(s) in %lf s\n",
		   input_filename, image->n_images, duration);

	MPI_Comm_size(MPI_COMM_WORLD, &commWorldSize);
	MPI_Comm_rank(MPI_COMM_WORLD, &rankWorld);

	timeStart = MPI_Wtime();

	// waitForDebug();
	// process attribution
	workgroupList = attributeNumberOfProcess(commWorldSize, image, &listSize);
	groupIndex = whichCommunicator(workgroupList, listSize, rankWorld);
	imagesToProcess = getImagesToTreat(groupIndex, workgroupList, listSize, numFrames);

	MPI_Comm_split(MPI_COMM_WORLD, groupIndex, rankWorld, &groupComm);
	
	
	if (rankWorld == 0)
	{
		//master is working here
		dbprintf("Hello from thread master %d/%d\n", rankWorld, commWorldSize);
		// testProcessAttribution();

		int numberOfGroupMaster;
		int *groupMasterList = createGroupMasterList(workgroupList, commWorldSize, &numberOfGroupMaster);
		//waitForDebug();

		masterLoop(groupMasterList, numberOfGroupMaster, image, imagesToProcess, groupComm);

		if (!store_pixels(output_filename, image))
		{
			return 1;
		}
		timeEnd = MPI_Wtime();

		duration = timeEnd - timeStart;
		printf("Total work completed in %lf s\n", duration);
	}
	else
	{
		//groupmaster & slaves
		MPI_Comm_rank(groupComm, &groupRank);
		MPI_Comm_size(groupComm, &groupSize);
		enum role groupRole = giveRoleInGroup(groupRank);
		if (groupRole == groupmaster)
		{
			// groupMaster loop
			// do nothing.
			// for now...
			dbprintf("Hello from groupMaster (group : %d/%d, world: %d/%d)\n", groupRank, groupSize, rankWorld, commWorldSize);
			groupMasterLoop(groupComm, groupIndex, image, imagesToProcess);
		}
		else
		{
			// slave loop
			// do nothing - but another way
			dbprintf("Hello from slave of group %d: (%d/%d), in world: %d/%d\n", groupIndex, groupRank, groupSize, rankWorld, commWorldSize);
			slaveGroupLoop(groupComm, image, imagesToProcess);
		}
	}

	return 0;
}

/**
 * \brief assign processes to groups
 * 
 * 
 */
int *attributeNumberOfProcess(const int numberOfProcess, const animated_gif *image, int *workgroupListSize)
{
	const int preferredMaxPerGroup = (int)ceil((double)numberOfProcess / image->n_images);
	const int maxGroupNumber = image->n_images;
	int *rawWorkgroupList = (int *)calloc(maxGroupNumber, sizeof(int));
	// dbprintf("maxPerGroup=%d\n", preferredMaxPerGroup);
	// if (numberOfProcess < 2)
	// {
	// 	fprintf(stderr, "Too few processes. Aborting\n");
	// 	return NULL;
	// }
	int freeProcess = numberOfProcess;
	int currGroup = 0;

	do
	{
		rawWorkgroupList[currGroup]++;
		freeProcess--;
		// if the max size of group is reached
		if (rawWorkgroupList[currGroup] == preferredMaxPerGroup
			// there must be room for a next group
			&& currGroup + 1 < maxGroupNumber
			// no need if last process to handle
			&& freeProcess)
		{
			//next group
			currGroup++;
		}
	} while (freeProcess > 0);

	int totalNumOfGroup = currGroup + 1;
	int *workGroupList = (int *)malloc(totalNumOfGroup * sizeof(int));

	for (int i = 0; i < totalNumOfGroup; i++)
	{
		workGroupList[i] = rawWorkgroupList[i];
	}
	*workgroupListSize = totalNumOfGroup;
	return workGroupList;

	// while (compteur >= 4 && i < image->n_images - 1)
	// {
	//     workgroupList[i] = 4;
	//     compteur -= 4;
	// }

	// workgroupList[image->n_images - 1] = compteur;
}

/**
 * attributes a process to its group Index
 * @param workgroupList pre-filled workgroup list
 * @param listSize the length of this  list
 * @param rankWorld rank of process in the MPI_COMM_WORLD
 * 
 * @return group Index
 */
int whichCommunicator(int *workgroupList, int listSize, int rankWorld)
{
	int comm = 0;
	while (rankWorld >= 0 && comm < listSize)
	{
		rankWorld -= workgroupList[comm];
		comm++;
	}
	return comm - 1;
}

/**
 * creates and fills the list of group master processes
 */
int *createGroupMasterList(const int *workgroupList, const int workgroupListSize, int *gmListSizeOut)
{
	int groupNum = 0;
	int currRank = 0;
	// int *item = workgroupList + 1;
	for (int i = 0; i < workgroupListSize; i++)
	{
		if (workgroupList[i] == 0)
		{
			break;
		}
		// dbprintf("rank %d ", currRank);
		currRank += workgroupList[i];
		groupNum++;
	}
	// dbprintf("\nSaw %d different groups\n", groupNum);

	int *gmList = (int *)malloc(groupNum * sizeof(int));

	groupNum = 0;
	currRank = 0;
	// int *item = workgroupList + 1;
	for (int i = 0; i < workgroupListSize; i++)
	{
		if (workgroupList[i] == 0)
		{
			break;
		}
		// dbprintf("rank %d ", currRank);
		gmList[groupNum] = currRank;
		currRank += workgroupList[i];
		groupNum++;
	}

	*gmListSizeOut = groupNum;
	return gmList;
}

/**
 * creates and fills a tab of size nFrames with a boolean 
 * Used to know whether to treat a frame or not
 */
int *getImagesToTreat(const int groupIndex, const int *workgroupList, const int workgroupListSize, const int numberOfImages)
{
	int *filledTabOut = (int *)calloc(numberOfImages, sizeof(int));

	int nGroups = workgroupListSize;
	int iFrame = 0;
	while (iFrame < numberOfImages)
	{
		int assignedGroup = (iFrame + 1) % nGroups;
		filledTabOut[iFrame] = (groupIndex == assignedGroup);
		iFrame++;
	}
	return filledTabOut;
}
