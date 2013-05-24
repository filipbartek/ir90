#include <stdio.h>
#include "aim.h"

void
exercise(struct image * restrict in, struct image * restrict out)
{
	/* Do your image transformation here. Delete the code below
	 * first, especially when testing with large images! Modify
	 * the image, not just the printing code. */
	for (int y = 0; y < in->rows; y++) {
		for (int x = 0; x < in->cols; x++) {
			bool p = image_getpixel(in, x, y);
			putchar(".#"[p]);
			image_putpixel(out, x, y, p);
		}
		putchar('\n');
	}
}
