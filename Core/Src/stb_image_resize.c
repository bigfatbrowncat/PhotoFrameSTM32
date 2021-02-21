static void my_progress_report(float progress);
#define STBIR_PROGRESS_REPORT(val) my_progress_report(val)

#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include "stb_image_resize.h"

#include <stdio.h>

static void my_progress_report(float progress)
{
	printf("Progress: %d%%\r", (int)(progress * 100));
}
