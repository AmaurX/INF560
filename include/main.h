/**
 * Main.h
 * Used to represent all types and prototypes introduced by main.c
 * Stores also global parameters
 */

#ifndef MAIN_H
#define MAIN_H

#include <gif_lib.h>

#define SOBELF_DEBUG 0

/* Represent one pixel from the image */
typedef struct pixel
{
    int r; /* Red */
    int g; /* Green */
    int b; /* Blue */
} pixel;

/* Represent one GIF image (animated or not */
typedef struct animated_gif
{
    int n_images;   /* Number of images */
    int *width;     /* Width of each image */
    int *height;    /* Height of each image */
    pixel **p;      /* Pixels of each image */
    GifFileType *g; /* Internal representation.
                         DO NOT MODIFY */
} animated_gif;

typedef struct task
{
    int id; /* -1 if no next task */
    int frameNumber;
    int width;  /* Width of each image */
    int height; /* Height of each image */
} task;

/**
 * \brief computes
 * 
 * 
 * 
 * 
*/
void attributeNumberOfProcess(int *workgroupList, int numberOfProcess, animated_gif *image);

int whichCommunicator(int *workgroupList, int rankWorld);

#endif