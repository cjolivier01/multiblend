#include <gtest/gtest.h>
#include <tiffio.h>
#include <vector>
#include <string>
#include <cstdio>
#include <unistd.h>

#include "src/geotiff.h"
#include "src/image.h"

static std::string make_geotiff_rgb(const char* name, int w, int h) {
    char path[256];
    snprintf(path, sizeof(path), "/tmp/%s-%d.tif", name, getpid());
    TIFF* tif = TIFFOpen(path, "w");
    if (!tif) return "";
    TIFFSetField(tif, TIFFTAG_IMAGEWIDTH, w);
    TIFFSetField(tif, TIFFTAG_IMAGELENGTH, h);
    TIFFSetField(tif, TIFFTAG_BITSPERSAMPLE, 8);
    TIFFSetField(tif, TIFFTAG_SAMPLESPERPIXEL, 3);
    TIFFSetField(tif, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT);
    TIFFSetField(tif, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
    TIFFSetField(tif, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_RGB);
    TIFFSetField(tif, TIFFTAG_COMPRESSION, COMPRESSION_NONE);
    TIFFSetField(tif, TIFFTAG_ROWSPERSTRIP, h);

    std::vector<uint8_t> buf(w * h * 3, 0);
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            size_t p = (y * w + x) * 3;
            buf[p + 0] = (uint8_t)(x * 32);
            buf[p + 1] = (uint8_t)(y * 32);
            buf[p + 2] = 128;
        }
    }
    TIFFWriteEncodedStrip(tif, 0, buf.data(), buf.size());

    GeoTIFFInfo info;
    info.XCellRes = 1.0;
    info.YCellRes = 1.0;
    info.XGeoRef = 100.0;
    info.YGeoRef = 200.0;
    info.nodata = -1;
    geotiff_write(tif, &info);

    TIFFClose(tif);
    return std::string(path);
}

TEST(ImageTest, OpenGeoTiffBasic) {
    std::string path = make_geotiff_rgb("image-open-tiff", 4, 3);
    ASSERT_FALSE(path.empty());
    char fname[256];
    snprintf(fname, sizeof(fname), "%s", path.c_str());
    Image img(fname);
    EXPECT_NO_THROW(img.Open());
    EXPECT_GT(img.tiff_width, 0);
    EXPECT_GT(img.tiff_height, 0);
    // Should have geotiff tags parsed
    EXPECT_TRUE(img.geotiff.set);
    remove(path.c_str());
}

