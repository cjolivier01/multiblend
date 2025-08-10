#include <gtest/gtest.h>
#include <png.h>
#include <vector>
#include <cstdio>
#include <string>
#include <unistd.h>

#include "src/image.h"

static std::string make_png_rgb(const char* name, int w, int h) {
    char path[256];
    snprintf(path, sizeof(path), "/tmp/%s-%d.png", name, getpid());
    FILE* fp = fopen(path, "wb");
    if (!fp) return "";
    png_structp png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    png_infop info_ptr = png_create_info_struct(png_ptr);
    png_init_io(png_ptr, fp);
    png_set_IHDR(png_ptr, info_ptr, w, h, 8, PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
    png_write_info(png_ptr, info_ptr);
    std::vector<uint8_t> row(w * 3, 0);
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            row[3 * x + 0] = (uint8_t)(x * 16);
            row[3 * x + 1] = (uint8_t)(y * 16);
            row[3 * x + 2] = 128;
        }
        png_write_row(png_ptr, row.data());
    }
    png_write_end(png_ptr, NULL);
    png_destroy_write_struct(&png_ptr, &info_ptr);
    fclose(fp);
    return std::string(path);
}

TEST(ImageTest, OpenPngBasic) {
    std::string path = make_png_rgb("image-open", 4, 3);
    ASSERT_FALSE(path.empty());
    char fname[256];
    snprintf(fname, sizeof(fname), "%s", path.c_str());
    Image img(fname);
    EXPECT_NO_THROW(img.Open());
    EXPECT_GT(img.tiff_width, 0);
    EXPECT_GT(img.tiff_height, 0);
    remove(path.c_str());
}

