/**
 * Main.h
 * Used to represent all types and prototypes introduced by main.c
 * Stores also global parameters
 */

#ifndef MAIN_H
#define MAIN_H

#include <mpi.h>
#include <omp.h>

#include "structs.h"
#define SOBELF_DEBUG 0

#define MODE_SEQUENTIAL 0
#define MODE_PARALLEL 1
#define MODE_TEST 2




void waitForDebug();

/**
 * \brief computes the workGroup list
 * 
 * This list helps positionning the process in the hierarchy 
*/
void attributeNumberOfProcess(int *workgroupList, int numberOfProcess, animated_gif *image);
int whichCommunicator(int *workgroupList, int listSize, int rankWorld);
int *createGroupMasterList(const int *workgroupList, const int workgroupListSize, int *gmListSizeOut);

int parallel_process(char *input_filename, char *output_filename);
int sequential_process(char *input_filename, char *output_filename);

#endif