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
#define DEBUG_PRINTS

#define MODE_SEQUENTIAL 0
#define MODE_PARALLEL 1
#define MODE_TEST 2

void waitForDebug();
/**
 * Proxy for printf
 * 
 * Does something if DEBUG_PRINTS is defined
 */
int dbprintf(char *format_string, ...);

/**
 * \brief computes the workGroup list
 * 
 * This list helps positionning the process in the hierarchy 
*/
int *attributeNumberOfProcess(const int numberOfProcess, const animated_gif *image, int *workgroupListSize);
int whichCommunicator(int *workgroupList, int listSize, int rankWorld);
int *createGroupMasterList(const int *workgroupList, const int workgroupListSize, int *gmListSizeOut);
int *getImagesToTreat(const int groupIndex, const int *workgroupList, const int workgroupListSize, const int numberOfImages);

int parallel_process(char *input_filename, char *output_filename);
int sequential_process(char *input_filename, char *output_filename);

#endif