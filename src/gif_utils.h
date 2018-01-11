/**
 * Gif Utils module
 * useful tools to manipulate Gif images
 */

#ifndef GIF_UTILS_H
#define GIF_UTILS_H

#include "main.h"


animated_gif * load_pixels(char *filename);

int store_pixels(char *filename, animated_gif *image);

#endif