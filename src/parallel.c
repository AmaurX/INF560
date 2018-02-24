#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <omp.h>
#include <mpi.h>

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
	double timeStart, timeEnd;
	// struct timeval t1, t2;
	double duration;

	init_custom_datatypes();

	/* IMPORT Timer start */
	timeStart = MPI_Wtime();

	/* Load file and store the pixels in array */
	image = load_pixels(input_filename);
	if (image == NULL)
	{
		return 1;
	}

	/* IMPORT Timer stop */
	timeEnd = MPI_Wtime();

	duration = timeEnd - timeStart;

	printf("GIF loaded from file %s with %d image(s) in %lf s\n",
		   input_filename, image->n_images, duration);

	MPI_Comm_size(MPI_COMM_WORLD, &commWorldSize);
	MPI_Comm_rank(MPI_COMM_WORLD, &rankWorld);

	timeStart = MPI_Wtime();

	// process attribution
	/// desperately under-optimized value
	int listSize = image->n_images + 1;
	int *workgroupList = (int *)calloc(listSize, sizeof(int));
	int * imagesToProcess;
	attributeNumberOfProcess(workgroupList, commWorldSize, image);
	groupIndex = whichCommunicator(workgroupList, listSize, rankWorld);
	MPI_Comm_split(MPI_COMM_WORLD, groupIndex, rankWorld, &groupComm);

	if (rankWorld == 0)
	{
		//master is working here
		printf("Hello from thread master %d/%d\n", rankWorld, commWorldSize);
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
			printf("Hello from groupMaster (group : %d/%d, world: %d/%d)\n", groupRank, groupSize, rankWorld, commWorldSize);
			groupMasterLoop(groupComm, image);
		}
		else
		{
			// slave loop
			// do nothing - but another way
			printf("Hello from slave of group %d: (%d/%d), in world: %d/%d\n", groupIndex, groupRank, groupSize, rankWorld, commWorldSize);
			slaveGroupLoop(groupComm);
		}
	}

	return 0;
}

/**
 * \brief assign processes to groups
 * 
 * returned tab is of the form [1, x, y, ..] because first index is for the MASTER group
 */
void attributeNumberOfProcess(int *workgroupList, int numberOfProcess, animated_gif *image)
{
	if (numberOfProcess < 2)
	{
		fprintf(stderr, "Too few processes. Aborting\n");
		return;
	}
	int maxGroup = image->n_images + 1;
	int freeProcess = numberOfProcess - 1;
	int compteur = image->n_images;
	int i = 1;
	workgroupList[0] = 1;

	while (freeProcess > 0)
	{
		workgroupList[i]++;
		if (workgroupList[i] == 1 && i + 1 < maxGroup)
		{
			//next group
			i++;
		}
		freeProcess--;
	}

	// while (compteur >= 4 && i < image->n_images - 1)
	// {
	//     workgroupList[i] = 4;
	//     compteur -= 4;
	// }

	// workgroupList[image->n_images - 1] = compteur;
}

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

int *createGroupMasterList(const int *workgroupList, const int workgroupListSize, int *gmListSizeOut)
{
	int groupNum = 0;
	int currRank = 1;
	// int *item = workgroupList + 1;
	for (int i = 1; i < workgroupListSize; i++)
	{
		if (workgroupList[i] == 0)
		{
			break;
		}
		// printf("rank %d ", currRank);
		currRank += workgroupList[i];
		groupNum++;
	}
	// printf("\nSaw %d different groups\n", groupNum);

	int *gmList = (int *)malloc(groupNum * sizeof(int));

	groupNum = 0;
	currRank = 1;
	// int *item = workgroupList + 1;
	for (int i = 1; i < workgroupListSize; i++)
	{
		if (workgroupList[i] == 0)
		{
			break;
		}
		// printf("rank %d ", currRank);
		gmList[groupNum] = currRank;
		currRank += workgroupList[i];
		groupNum++;
	}

	*gmListSizeOut = groupNum;
	return gmList;
}
