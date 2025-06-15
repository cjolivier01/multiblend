/*
  multiblend (c) 2013 David Horman
  ARM64/aarch64 support added

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2, or (at your option)
  any later version.
*/

#include <algorithm>
using namespace std;
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef __APPLE__
  #define memalign(a,b) malloc((b))
#else
  #include <malloc.h>
#endif

// Platform-specific SIMD includes
#ifdef __aarch64__
  // ARM64/aarch64 architecture - use NEON intrinsics
  #include <arm_neon.h>
  // Include sse2neon for SSE to NEON translation
  #include "sse2neon.h"
#else
  // x86/x86_64 architecture - use SSE2 intrinsics
  #include <emmintrin.h>
#endif

#ifdef WIN32
#define NOMINMAX
#include <Windows.h>
#endif

#define PY(i,l) g_images[i].pyramid[l]

#include <stdarg.h>
#include <jpeglib.h>
#include <png.h>
#include <tiffio.h>

// Platform-specific includes with ARM64 support
#include "globals_aarch64.cpp"
#include "functions_aarch64.cpp"
#include "geotiff.cpp"
//#include "loadimages.cpp"
//#include "seaming_aarch64.cpp"
//#include "maskpyramids_aarch64.cpp"
#include "blending_aarch64.cpp"
//#include "write.cpp"
//#include "pseudowrap.cpp"
//#include "go.cpp"

#ifdef WIN32
#pragma comment(lib,"libtiff.lib")
#pragma comment(lib,"turbojpeg-static.lib")
#pragma comment(lib,"libpng.lib")
#pragma comment(lib,"zlib.lib")
#endif

void help() {
	printf("multiblend version 2.0.0 (ARM64 compatible)\n");
	printf("Usage: multiblend [options] [-o OUTPUT] INPUT1 [INPUTn]\n\n");
	printf("Options:\n");
	printf("  -o|--output <file>     Output file (default: final.tif)\n");
	printf("  --compression X        TIFF compression. For TIFF output, X may be:\n");
	printf("                         NONE (default), PACKBITS, or LZW\n");
	printf("                         For JPEG output, X is JPEG quality (0-100, default 75)\n");
	printf("  --cache                cache input images to disk to minimise memory usage\n");
	printf("  --save-seams <file>    Save seams to PNG file for external editing\n");
	printf("  --no-output            Don't perform blend (for use with --save-seams)\n");
	printf("  --load-seams <file>    Load seams from PNG file\n");
	printf("  --bigtiff              BigTIFF output (not well tested)\n");
	printf("  --reverse              reverse image priority (last=highest)\n");
	printf("  --quiet                suppress output (except warnings)\n");
	printf("\n");
	printf("Pass a single image as input to blend around the left/right boundary.\n");
	printf("\n");
#ifdef __aarch64__
	printf("Running on ARM64/aarch64 architecture with NEON support.\n");
#else
	printf("Running on x86/x86_64 architecture with SSE2 support.\n");
#endif
	exit(0);
}

int main(int argc, char* argv[]) {
	int i;
	int input_args;
	int temp;
	my_timer timer_all;

	timer_all.set();
	TIFFSetWarningHandler(0);

	printf("multiblend v2.0.0\n");
#ifdef __aarch64__
	printf("Compiled for ARM64/aarch64 architecture\n");
#endif

	if (argc < 2) {
		printf("Error: No input files specified\n\n");
		help();
	}

	// Parse command line arguments
	char* output_filename = strdup("final.tif");
	char* seams_load_filename = NULL;
	char* seams_save_filename = NULL;
	bool cache_images = false;
	bool no_output = false;
	bool reverse_order = false;
	bool quiet = false;
	bool bigtiff = false;
	int compression = COMPRESSION_NONE;
	int jpeg_quality = 75;

	input_args = 0;
	for (i = 1; i < argc; i++) {
		if (strcmp(argv[i], "-o") == 0 || strcmp(argv[i], "--output") == 0) {
			if (++i < argc) {
				free(output_filename);
				output_filename = strdup(argv[i]);
			}
		} else if (strcmp(argv[i], "--compression") == 0) {
			if (++i < argc) {
				if (strcasecmp(argv[i], "NONE") == 0) {
					compression = COMPRESSION_NONE;
				} else if (strcasecmp(argv[i], "PACKBITS") == 0) {
					compression = COMPRESSION_PACKBITS;
				} else if (strcasecmp(argv[i], "LZW") == 0) {
					compression = COMPRESSION_LZW;
				} else {
					jpeg_quality = atoi(argv[i]);
					if (jpeg_quality < 0) jpeg_quality = 0;
					if (jpeg_quality > 100) jpeg_quality = 100;
				}
			}
		} else if (strcmp(argv[i], "--cache") == 0) {
			cache_images = true;
		} else if (strcmp(argv[i], "--save-seams") == 0) {
			if (++i < argc) {
				seams_save_filename = strdup(argv[i]);
			}
		} else if (strcmp(argv[i], "--load-seams") == 0) {
			if (++i < argc) {
				seams_load_filename = strdup(argv[i]);
			}
		} else if (strcmp(argv[i], "--no-output") == 0) {
			no_output = true;
		} else if (strcmp(argv[i], "--reverse") == 0) {
			reverse_order = true;
		} else if (strcmp(argv[i], "--quiet") == 0) {
			quiet = true;
		} else if (strcmp(argv[i], "--bigtiff") == 0) {
			bigtiff = true;
		} else if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
			help();
		} else if (argv[i][0] != '-') {
			// This is an input file
			input_args++;
		} else {
			printf("Unknown option: %s\n", argv[i]);
			help();
		}
	}

	if (input_args == 0) {
		printf("Error: No input files specified\n\n");
		help();
	}

	// The rest of the main function would continue with the actual blending logic
	// This is a framework showing how to handle the ARM64 compatibility

	return 0;
}
