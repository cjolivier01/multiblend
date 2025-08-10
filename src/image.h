#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

#include "image_types.h"
#include "geotiff.h"
#include <tiffio.h>
#include <jpeglib.h>

class Channel {
public:
    explicit Channel(size_t _bytes) : data(nullptr), file(nullptr), bytes(_bytes) {}
    void* data;
    FILE* file;
    size_t bytes;
};
class Pyramid;
class Flex;
// GeoTIFFInfo included above; TIFF comes from tiffio.h
typedef struct png_struct_def* png_structp;

class Image {
public:
    Image(char* _filename);
    ~Image();
    char* filename;
    ImageType type;
    int width;
    int height;
    int xpos;
    int ypos;
    int xpos_add = 0;
    int ypos_add = 0;
    std::vector<Channel*> channels;
    Pyramid* pyramid = NULL;
    GeoTIFFInfo geotiff;
    int tiff_width;
    int tiff_height;
    int tiff_u_height;
    int rows_per_strip;
    int first_strip;
    int end_strip;
    uint16_t bpp;
    uint16_t spp;
    void Open();
    void Read(void* data, bool gamma);
    void MaskPng(int i);
    //
    size_t untrimmed_bytes;
    Flex* tiff_mask;
    float tiff_xres, tiff_yres;
    uint64_t mask_state;
    int mask_count;
    int mask_limit;
    bool seam_present;
    std::vector<Flex*> masks;

private:
    TIFF* tiff;
    FILE* file;
    struct jpeg_decompress_struct cinfo;
    struct jpeg_error_mgr jerr;
    png_structp png_ptr;
};
