/**
 * Main.h
 * Used to represent all types and prototypes introduced by main.c
 * Stores also global parameters
 */

#ifndef MAIN_H
#define MAIN_H

#include <gif_lib.h>
#include <mpi.h>

#define SOBELF_DEBUG 0

#define MODE_SEQUENTIAL 0
#define MODE_PARALLEL 1
#define MODE_TEST 2

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
    int *width;     /* Width of each image */
    int *height;    /* Height of each image */
    pixel **p;      /* Pixels of each image */
    GifFileType *g; /* Internal representation.
                         DO NOT MODIFY */
} animated_gif;

/// \brief elemental task given to a workgroup
typedef struct task
{
    int id; /* -1 if no next task */
    int frameNumber;
    int width;  /* Width of each image */
    int height; /* Height of each image */
} task;

MPI_Datatype MPI_CUSTOM_PIXEL;
MPI_Datatype MPI_CUSTOM_TASK;

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