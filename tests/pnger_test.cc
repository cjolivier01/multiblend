#include <gtest/gtest.h>
#include <png.h>
#include <cstdio>
#include <vector>
#include <string>
#include <unistd.h>

#include "src/pnger.h"

static std::string tmp_path(const char* name) {
    char buf[256];
    snprintf(buf, sizeof(buf), "/tmp/%s-%d.png", name, getpid());
    return std::string(buf);
}

TEST(PngerTest, QuickWritesValidPNG) {
    const int W = 2, H = 2;
    std::vector<uint8_t> data(W * H, 0);
    data[0] = 0; data[1] = 64; data[2] = 128; data[3] = 255;

    std::string path = tmp_path("pnger-quick");
    char fname[256];
    snprintf(fname, sizeof(fname), "%s", path.c_str());
    Pnger::Quick(fname, data.data(), W, H, W, PNG_COLOR_TYPE_GRAY);

    FILE* f = fopen(path.c_str(), "rb");
    ASSERT_NE(f, nullptr);
    uint8_t sig[8];
    size_t r = fread(sig, 1, 8, f);
    ASSERT_EQ(r, 8u);
    ASSERT_TRUE(png_check_sig(sig, 8));
    fclose(f);
    remove(path.c_str());
}

