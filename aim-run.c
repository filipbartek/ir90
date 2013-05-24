#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <pbm.h>

#include "aim.h"


#if CHAR_BIT != 8
#error char size larger than 8 is not supported
#endif


static struct image *
image_init(int cols, int rows)
{
	struct image *img = malloc(sizeof(*img));
	img->cols = cols, img->rows = rows;
	img->bitmap = malloc(img->rows * image_rowbytes(img));
	return img;
}

static struct image *
image_load(char *filename)
{
	FILE *f = fopen(filename, "rb");
	if (!f) { perror("load"); exit(EXIT_FAILURE); }

	int cols, rows, fmt;
	pbm_readpbminit(f, &cols, &rows, &fmt);
	struct image *img = image_init(cols, rows);

	for (int i = 0; i < img->rows; i++)
		pbm_readpbmrow_packed(f, img->bitmap + i * image_rowbytes(img),
				img->cols, fmt);

	fclose(f);
	return img;
}

static void
image_save(char *filename, struct image *img)
{
	FILE *f = fopen(filename, "wb");
	if (!f) { perror("save"); exit(EXIT_FAILURE); }

	pbm_writepbminit(f, img->cols, img->rows, 0);
	for (int i = 0; i < img->rows; i++)
		pbm_writepbmrow_packed(f, img->bitmap + i * image_rowbytes(img),
				img->cols, 0);

	fclose(f);
}

static void
image_free(struct image *img)
{
	free(img->bitmap);
	free(img);
}


typedef int_fast64_t timestamp_t;

static timestamp_t
get_timer(void)
{
	struct timeval t;
	gettimeofday(&t, NULL);
	return 1000000*t.tv_sec + t.tv_usec;
}


int
main(int argc, char *argv[])
{
	if (argc != 4) {
		fprintf(stderr, "%s ITERATIONS SRCIMAGE.pbm DSTIMAGE.pbm\n", argv[0]);
		return EXIT_FAILURE;
	}

	struct image *img_in = image_load(argv[2]);
	struct image *img_out = image_init(img_in->cols, img_in->rows);
	timestamp_t t0 = get_timer();

	/* Pre-warm the CPU! Neccessary for benchmarking w/ dynamic
	 * cpufreq policy. */
	while (get_timer() - t0 < 500000);
	t0 = get_timer();

	int iters = atoi(argv[1]);
	for (int j = 0; j < iters; j++) {
		exercise(img_in, img_out);
	}

	t0 = get_timer() - t0;
	// time spent:
	printf("%.3f\n", (double) t0/1e6);
	image_save(argv[3], img_out);
	image_free(img_in);
	image_free(img_out);
	return EXIT_SUCCESS;
}
