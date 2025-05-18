/*
 * Copyright (c) 2012 Arvin Schnell <arvin.schnell@gmail.com>
 * Copyright (c) 2012 Rob Clark <rob@ti.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sub license,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

/* Based on a egl cube test app originally written by Arvin Schnell */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>

#include "common.h"
#include "drm-common.h"

static const struct egl *egl;
static const struct gbm *gbm;
static const struct drm *drm;

static const char *shortopts = "Ac:D:f:gM:m:n:NOp:S:s:V:v:x";

static const struct option longopts[] = {
	{"atomic", no_argument,       0, 'A'},
	{"count",  required_argument, 0, 'c'},
	{"device", required_argument, 0, 'D'},
	{"format", required_argument, 0, 'f'},
	{"gears",  no_argument,       0, 'g'},
	{"mode",   required_argument, 0, 'M'},
	{"modifier", required_argument, 0, 'm'},
	{"connector_id", required_argument, 0, 'n'},
	{"offscreen", no_argument,    0, 'O'},
	{"samples",  required_argument, 0, 's'},
	{"surfaceless", no_argument,  0, 'x'},
	{"nonblocking", no_argument,  0, 'N'},
	{0, 0, 0, 0}
};

static void usage(const char *name)
{
	printf("Usage: %s [-ADfgMmNSsVvx]\n"
			"\n"
			"options:\n"
			"    -A, --atomic             use atomic modesetting and fencing\n"
			"    -c, --count=N            run for the specified number of frames\n"
			"    -D, --device=DEVICE      use the given device\n"
			"    -f, --format=FOURCC      framebuffer format\n"
			"    -g, --gears              render gears on each cube face\n"
			"    -M, --mode=MODE          specify mode, one of:\n"
			"        smooth    -  smooth shaded cube (default)\n"
			"        rgba      -  rgba textured cube\n"
			"        nv12-2img -  yuv textured (color conversion in shader)\n"
			"        nv12-1img -  yuv textured (single nv12 texture)\n"
			"    -m, --modifier=MODIFIER  hardcode the selected modifier\n"
			"    -n, --connector_id=N     use connector ID N (see drm_info)\n"
			"    -S, --shadertoy=FILE     use specified shadertoy shader\n"
			"    -s, --samples=N          use MSAA\n"
			"    -x, --surfaceless        use surfaceless mode, instead of gbm surface\n"
			"    -N, --nonblocking        do not poll for input\n"
			,
			name);
}

int main(int argc, char *argv[])
{
	const char *device = NULL;
	const char *shadertoy = NULL;
	char mode_str[DRM_DISPLAY_MODE_LEN] = "";
	char *p;
	enum mode mode = SMOOTH;
	uint32_t format = DRM_FORMAT_XRGB8888;
	uint64_t modifier = DRM_FORMAT_MOD_LINEAR;
	int samples = 0;
	int atomic = 0;
	int gears = 0;
	int connector_id = -1;
	int opt;
	unsigned int len;
	unsigned int vrefresh = 0;
	unsigned int count = ~0;
	bool surfaceless = false;
	bool nonblocking = false;

	while ((opt = getopt_long_only(argc, argv, shortopts, longopts, NULL)) != -1) {
		switch (opt) {
		case 'A':
			atomic = 1;
			break;
		case 'c':
			count = strtoul(optarg, NULL, 0);
			break;
		case 'D':
			device = optarg;
			break;
		case 'f': {
			char fourcc[4] = "    ";
			int length = strlen(optarg);
			if (length > 0)
				fourcc[0] = optarg[0];
			if (length > 1)
				fourcc[1] = optarg[1];
			if (length > 2)
				fourcc[2] = optarg[2];
			if (length > 3)
				fourcc[3] = optarg[3];
			format = fourcc_code(fourcc[0], fourcc[1],
					     fourcc[2], fourcc[3]);
			break;
		}
		case 'g':
			gears = 1;
			break;
		case 'M':
			if (strcmp(optarg, "smooth") == 0) {
				mode = SMOOTH;
			} else if (strcmp(optarg, "rgba") == 0) {
				mode = RGBA;
			} else if (strcmp(optarg, "nv12-2img") == 0) {
				mode = NV12_2IMG;
			} else if (strcmp(optarg, "nv12-1img") == 0) {
				mode = NV12_1IMG;
			} else {
				printf("invalid mode: %s\n", optarg);
				usage(argv[0]);
				return -1;
			}
			break;
		case 'm':
			modifier = strtoull(optarg, NULL, 0);
			break;
		case 'n':
			connector_id = strtoul(optarg, NULL, 0);
			break;
		case 'N':
			nonblocking = true;
			break;
		case 'S':
			mode = SHADERTOY;
			shadertoy = optarg;
			break;
		case 's':
			samples = strtoul(optarg, NULL, 0);
			break;
		case 'x':
			surfaceless = true;
			break;
		default:
			usage(argv[0]);
			return -1;
		}
	}

	if (atomic)
		drm = init_drm_atomic(device, mode_str, connector_id, vrefresh, count, nonblocking);
	else
		drm = init_drm_legacy(device, mode_str, connector_id, vrefresh, count, nonblocking);
	if (!drm) {
		printf("failed to initialize %s DRM\n", atomic ? "atomic" : "legacy");
		return -1;
	}

	gbm = init_gbm(drm->fd, drm->mode->hdisplay, drm->mode->vdisplay,
			format, modifier, surfaceless);
	if (!gbm) {
		printf("failed to initialize GBM\n");
		return -1;
	}

	if (gears)
		egl = init_cube_gears(gbm, samples);
	else if (mode == SMOOTH)
		egl = init_cube_smooth(gbm, samples);
	else if (mode == SHADERTOY)
		egl = init_cube_shadertoy(gbm, shadertoy, samples);
	else
		egl = init_cube_tex(gbm, mode, samples);

	if (!egl) {
		printf("failed to initialize EGL\n");
		return -1;
	}

	/* clear the color buffer */
	glClearColor(0.5, 0.5, 0.5, 1.0);
	glClear(GL_COLOR_BUFFER_BIT);

	return drm->run(gbm, egl);
}
