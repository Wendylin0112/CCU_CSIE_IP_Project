#pragma once
// image_types.h

#include <cstdint>
#include <vector>
#include <string>
#include <stdexcept>

// ============================================================
//  Image  –  uint8_t, arbitrary number of channels
// ============================================================
struct Image {
    int rows     = 0;
    int cols     = 0;
    int channels = 0;
    std::vector<uint8_t> data;  // row-major, interleaved channels, e.g. BGR|BGR|BGR...

    Image() = default;
    Image(int rows, int cols, int channels)
        : rows(rows), cols(cols), channels(channels),
          data(rows * cols * channels, 0) {}

    // ----- Element accessor: at(row, col, channel)  ------
    uint8_t& at(int r, int c, int ch) {               // read/write
        return data[(r * cols + c) * channels + ch];
    }
    const uint8_t& at(int r, int c, int ch) const {   // read only
        return data[(r * cols + c) * channels + ch];
    }

    // --- check if image is empty (no data) ---
    bool empty() const { return data.empty(); }
};

// ============================================================
//  ImageF  –  float, arbitrary number of channels
// ============================================================
struct ImageF {
    int rows     = 0;
    int cols     = 0;
    int channels = 0;
    std::vector<float> data;    // row-major, interleaved channels

    ImageF() = default;
    ImageF(int rows, int cols, int channels)
        : rows(rows), cols(cols), channels(channels),
          data(rows * cols * channels, 0.f) {}

    // ----- Element accessor: at(row, col, channel)  ------
    float& at(int r, int c, int ch) {               // read/write
        return data[(r * cols + c) * channels + ch];
    }
    const float& at(int r, int c, int ch) const {   // read only
        return data[(r * cols + c) * channels + ch];
    }

    // --- check if image is empty (no data) ---
    bool empty() const { return data.empty(); }
};

// ============================================================
//  I/O  –  read/write PNG/BMP via stb_image
// ============================================================
// These two functions are implemented in main.cpp
// They rely on the single-header libraries stb_image.h and stb_image_write.h,
// (Download from https://github.com/nothings/stb)
// ============================================================
Image  loadImage (const std::string& path);
bool   saveImage (const std::string& path, const Image& img);
