#pragma once

#include <mpi.h>
#include "gif_lib.h"

#define PRINTTASK_MAXSIZE 200

/// \brief Represent one pixel from the image
typedef struct pixel
{
	int r; /* Red */
	int g; /* Green */
	int b; /* Blue */
} pixel;

/// \brief Represent one GIF image (animated or not )
typedef struct animated_gif
{
	int n_images;   /* Number of images */
	int *width;		/* Width of each image */
	int *height;	/* Height of each image */
	pixel **p;		/* Pixels of each image */
	GifFileType *g; /* Internal representation.
                         DO NOT MODIFY */
} animated_gif;

/// \brief elementary task given to a workgroup
typedef struct task
{
	//***** TASK DEFINITION *************
	// filled by master before the task
	/** -1 if no next task */
	int id;
	/** index of the frame - purely informative for now*/
	int frameNumber;
	/** Width of each image */
	int width;
	/** Height of each image */
	int height;

	//******* TASK METRICS ***************
	// filled during the task
	/** filled by GroupMaster : size of the workgroup who processed the task */
	int workgroupSize;
	/** filled by GroupMaster : real duration of the work : groupCard*((before send to Master)- (after receive from master))*/
	double totalTimeTaken;
	/** filled by GroupMaster : effective duration of work (sum of of slave work duration)*/
	double totalTimeWorking; /* ms */
	/** filled by master : gives the date when master launched the task*/
	double startTimestamp;
	/** filled by master : gives the date when master finished parsing the task and moved on to the next*/
	double endTimestamp;
	/** difference between 2 precedent values */
	double masterTime;
} task;

MPI_Datatype MPI_CUSTOM_PIXEL;
MPI_Datatype MPI_CUSTOM_TASK;

void init_custom_datatypes();

char *string_of_task(struct task *task, bool formatted);
void copyTask(struct task taskIn, struct task *taskOut);