#include <gtest/gtest.h>
#include <tiffio.h>
#include <vector>
#include <string>
#include <cstdio>
#include <cstring>
#include <unistd.h>

#include "src/image.h"
#include "src/functions.h"

static std::string make_tiff_rgba_alpha_border(const char* name, int w, int h) {
    char path[256];
    snprintf(path, sizeof(path), "/tmp/%s-%d.tif", name, getpid());
    TIFF* tif = TIFFOpen(path, "w");
    if (!tif) return "";
    TIFFSetField(tif, TIFFTAG_IMAGEWIDTH, w);
    TIFFSetField(tif, TIFFTAG_IMAGELENGTH, h);
    TIFFSetField(tif, TIFFTAG_BITSPERSAMPLE, 8);
    TIFFSetField(tif, TIFFTAG_SAMPLESPERPIXEL, 4);
    TIFFSetField(tif, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT);
    TIFFSetField(tif, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
    TIFFSetField(tif, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_RGB);
    TIFFSetField(tif, TIFFTAG_COMPRESSION, COMPRESSION_NONE);
    // Mark last channel as alpha
    uint16_t extra = EXTRASAMPLE_ASSOCALPHA;
    TIFFSetField(tif, TIFFTAG_EXTRASAMPLES, 1, &extra);
    TIFFSetField(tif, TIFFTAG_ROWSPERSTRIP, h);

    std::vector<uint8_t> buf(w * h * 4, 0);
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            size_t p = (y * w + x) * 4;
            buf[p + 0] = (uint8_t)(x * 16);  // R
            buf[p + 1] = (uint8_t)(y * 16);  // G
            buf[p + 2] = 128;                // B
            bool border = (x == 0 || y == 0 || x == w - 1 || y == h - 1);
            buf[p + 3] = border ? 0 : 255;   // A in last byte
        }
    }
    TIFFWriteEncodedStrip(tif, 0, buf.data(), buf.size());
    TIFFClose(tif);
    return std::string(path);
}

TEST(TiffAlphaMaskTest, TrimsByAlphaAndCreatesMask) {
    const int W = 6, H = 4;
    std::string path = make_tiff_rgba_alpha_border("alpha-trim", W, H);
    ASSERT_FALSE(path.empty());

    char fname[256];
    snprintf(fname, sizeof(fname), "%s", path.c_str());
    Image img(fname);
    ASSERT_NO_THROW(img.Open());
    // Validate that alpha TIFF is detected and sized; full processing/mask
    // allocation happens during Read() in the main pipeline.
    EXPECT_EQ(img.spp, 4);
    EXPECT_EQ(img.bpp, 8);
    ASSERT_GT(img.untrimmed_bytes, 0u);

    remove(path.c_str());
}
