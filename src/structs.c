#include "structs.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

void init_custom_datatypes()
{
	// Add a MPI_Type_struct
	// according to documentation, a "block" is a set of successive variables
	// 	of the same type
	// 4 ints are then 1 block of size 4, displacement 0
	int ct_blockCount = 2;
	int ct_blocksizes[] = {6, 5};
	MPI_Aint ct_displacements[] = {0, 0};
	MPI_Datatype ct_types[] = {MPI_INT, MPI_DOUBLE};
	MPI_Type_create_struct(ct_blockCount, ct_blocksizes, ct_displacements,
						   ct_types, &MPI_CUSTOM_TASK);
	MPI_Type_commit(&MPI_CUSTOM_TASK);

	int cp_blockCount = 1;
	int cp_blocksizes[] = {3};
	MPI_Aint cp_displacements[] = {0};
	MPI_Datatype cp_types[] = {MPI_INT};
	MPI_Type_create_struct(cp_blockCount, cp_blocksizes, cp_displacements,
						   cp_types, &MPI_CUSTOM_PIXEL);
	MPI_Type_commit(&MPI_CUSTOM_PIXEL);
}

int write_taskHistory(char* filename, struct task * taskHistory, int nTasks)
{
	printf("writing taskHistory to file %s\n", filename);
	char header[] = "id, frame, height, width, groupIndex, groupSize, totalTime, workTime, masterTime, start, end";
	FILE* outFile = fopen(filename,  "w");
	fprintf(outFile, "%s\n", header);
	for(int i = 0 ; i<nTasks ; i++)
	{
		char* line = string_of_task(&taskHistory[i], false);
		fprintf(outFile, "%s\n", line);
		free(line);
	}
	fclose(outFile);
	return 0;
}

char *string_of_task(struct task *task, bool formatted)
{
	// const int max_size = 200;
	char *resl = malloc(PRINTTASK_MAXSIZE * sizeof(char));
	char *format_string;
	char pure[] = "%d,%d,%d,%d,%d,%d,%lf,%lf,%lf,%lf,%lf";
	char pretty[] = "task{\n"
									 "\tid=%d\n"
									 "\tframe=%d\n"
									 "\tdim=%d.%d\n"
									 "\tgroupIndex=%d\n"
									 "\tgroupSize=%d\n"
									 "\ttotalTime=%lf\n"
									 "\tworkTime=%lf\n"
									 "\tmasterTime=%lf\n"
									 "\tstart=%lf\n"
									 "\tend=%lf\n"
									 "}";
	if (formatted)
	{
		format_string = pretty;
	}
	else
	{
		format_string = pure;
	}
	snprintf(resl, PRINTTASK_MAXSIZE,
			 format_string,
			 task->id, task->frameNumber,
			 task->width, task->height, task->groupIndex, task->workgroupSize,
			 task->totalTimeTaken, task->totalTimeWorking, task->masterTime,
			 task->startTimestamp, task->endTimestamp);
	return resl;
}

void copyTask(struct task taskIn, struct task *taskOut){
	memcpy(taskOut, &taskIn,sizeof(struct task));
}

int autosave_taskHistory(struct task* taskHistory, int numberOfImages)
{
	// save to file
    char name[200], date[200];
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    strftime(date, sizeof(date)-1, "%d%m-%H:%M:%S", t);
    snprintf(name, sizeof(name)-1, "results/out-%s.csv", date);
    printf("%s\n", name);

    return write_taskHistory(name, taskHistory, numberOfImages);
}