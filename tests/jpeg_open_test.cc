#include <gtest/gtest.h>
#include <jpeglib.h>
#include <vector>
#include <cstdio>
#include <string>
#include <unistd.h>

#include "src/image.h"

static std::string make_jpeg_rgb(const char* name, int w, int h) {
    char path[256];
    snprintf(path, sizeof(path), "/tmp/%s-%d.jpg", name, getpid());
    FILE* fp = fopen(path, "wb");
    if (!fp) return "";
    jpeg_compress_struct cinfo;
    jpeg_error_mgr jerr;
    cinfo.err = jpeg_std_error(&jerr);
    jpeg_create_compress(&cinfo);
    jpeg_stdio_dest(&cinfo, fp);
    cinfo.image_width = w;
    cinfo.image_height = h;
    cinfo.input_components = 3;
    cinfo.in_color_space = JCS_RGB;
    jpeg_set_defaults(&cinfo);
    jpeg_set_quality(&cinfo, 75, TRUE);
    jpeg_start_compress(&cinfo, TRUE);
    std::vector<uint8_t> row(w * 3);
    while (cinfo.next_scanline < cinfo.image_height) {
        for (int x = 0; x < w; ++x) {
            row[3 * x + 0] = (uint8_t)(x * 32);
            row[3 * x + 1] = (uint8_t)(cinfo.next_scanline * 32);
            row[3 * x + 2] = 128;
        }
        JSAMPROW r = row.data();
        jpeg_write_scanlines(&cinfo, &r, 1);
    }
    jpeg_finish_compress(&cinfo);
    jpeg_destroy_compress(&cinfo);
    fclose(fp);
    return std::string(path);
}

TEST(ImageTest, OpenJpegBasic) {
    std::string path = make_jpeg_rgb("image-open-jpg", 4, 3);
    ASSERT_FALSE(path.empty());
    char fname[256];
    snprintf(fname, sizeof(fname), "%s", path.c_str());
    Image img(fname);
    EXPECT_NO_THROW(img.Open());
    EXPECT_GT(img.tiff_width, 0);
    EXPECT_GT(img.tiff_height, 0);
    remove(path.c_str());
}

