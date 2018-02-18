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
} task;

MPI_Datatype MPI_CUSTOM_PIXEL;
MPI_Datatype MPI_CUSTOM_TASK;

void init_custom_datatypes()
{
    // Add a MPI_Type_struct
    // according to documentation, a "block" is a set of successive variables
    // 	of the same type
    // 4 ints are then 1 block of size 4, displacement 0
    int ct_blockCount = 2;
    int ct_blocksizes[] = {5, 4};
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

/**
 * \brief computes the workGroup list
 * 
 * This list helps positionning the process in the hierarchy 
*/
void waitForDebug();

void attributeNumberOfProcess(int *workgroupList, int numberOfProcess, animated_gif *image);
int whichCommunicator(int *workgroupList, int listSize, int rankWorld);
int *createGroupMasterList(const int *workgroupList, const int workgroupListSize, int *gmListSizeOut);

int parallel_process(char *input_filename, char *output_filename);
int sequential_process(char *input_filename, char *output_filename);

#endif