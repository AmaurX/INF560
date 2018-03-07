#include <sys/time.h>
#include <stdlib.h>
#include <stdio.h>

#include "gif_utils.h"
#include "filters.h"

#include "main.h"

#define SEQ_GRAY 1
#define SEQ_BLUR 1
#define SEQ_SOBEL 1
#define SEQ_EXPORT 1

int sequential_process(char *input_filename, char *output_filename)
{
	animated_gif *image;
	struct timeval t1, t2;
	double duration;

	/* IMPORT Timer start */
	gettimeofday(&t1, NULL);

	/* Load file and store the pixels in array */
	image = load_pixels(input_filename);
	if (image == NULL)
	{
		return 1;
	}

	/* IMPORT Timer stop */
	gettimeofday(&t2, NULL);

	duration = (t2.tv_sec - t1.tv_sec) + ((t2.tv_usec - t1.tv_usec) / 1e6);

	printf("GIF loaded from file %s with %d image(s) in %lf s\n",
		   input_filename, image->n_images, duration);

#if SEQ_GRAY
	/* FILTER Timer start */
	gettimeofday(&t1, NULL);

	/* Convert the pixels into grayscale */
	apply_gray_filter(image);

	/* FILTER Timer stop */
	gettimeofday(&t2, NULL);

	duration = (t2.tv_sec - t1.tv_sec) + ((t2.tv_usec - t1.tv_usec) / 1e6);

	printf("GRAY_FILTER done in %lf s\n", duration);
#endif

#if SEQ_BLUR
	/* FILTER Timer start */
	gettimeofday(&t1, NULL);

	/* Apply blur filter with convergence value */
	apply_blur_filter(image, 5, 20);

	/* FILTER Timer stop */
	gettimeofday(&t2, NULL);

	duration = (t2.tv_sec - t1.tv_sec) + ((t2.tv_usec - t1.tv_usec) / 1e6);

	printf("BLUR_FILTER done in %lf s\n", duration);
#endif

#if SEQ_SOBEL

	/* FILTER Timer start */
	gettimeofday(&t1, NULL);

	/* Apply sobel filter on pixels */
	apply_sobel_filter(image);

	/* FILTER Timer stop */
	gettimeofday(&t2, NULL);

	duration = (t2.tv_sec - t1.tv_sec) + ((t2.tv_usec - t1.tv_usec) / 1e6);

	printf("SOBEL_FILTER done in %lf s\n", duration);
#endif

#if SEQ_EXPORT

	/* EXPORT Timer start */
	gettimeofday(&t1, NULL);

	/* Store file from array of pixels to GIF file */
	if (!store_pixels(output_filename, image))
	{
		return 1;
	}

	/* EXPORT Timer stop */
	gettimeofday(&t2, NULL);

	duration = (t2.tv_sec - t1.tv_sec) + ((t2.tv_usec - t1.tv_usec) / 1e6);

	printf("Export done in %lf s in file %s\n", duration, output_filename);

#endif

	return 0;
}