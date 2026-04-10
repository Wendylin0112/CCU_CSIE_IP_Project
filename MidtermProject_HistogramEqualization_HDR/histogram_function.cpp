#include "histogram_function.h"
#include <cmath>
#include <algorithm>

// ============================================================
//  Internal helpers
// ============================================================

static inline float clamp01(float x) {
    if (x < 0.f) return 0.f;
    if (x > 1.f) return 1.f;
    return x;
}

// Build a 256-entry lookup table which works on a single-channel image.
static std::vector<uint8_t> buildHistEqMap(const Image& img) {
    const int num_pixels = img.rows * img.cols;

    // 1. Count intensity histogram (pmf)
    std::vector<int> counts(256, 0);
    for (int r = 0; r < img.rows; ++r) {
        for (int c = 0; c < img.cols; ++c) {
            counts[img.at(r, c, 0)]++;
        }
    }

    // 2. Cumulative distribution function
    std::vector<int> cdf(256, 0);
    cdf[0] = counts[0];
    for (int v = 1; v < 256; ++v) { // prefix sum
        cdf[v] = cdf[v - 1] + counts[v];
    }

    // 3. Normalized map
    std::vector<uint8_t> map(256);
    for (int v = 0; v < 256; ++v) {
        int mv = static_cast<int>(std::floor(255.0 * static_cast<double>(cdf[v]) / num_pixels));
        if (mv < 0)   mv = 0;
        if (mv > 255) mv = 255;
        map[v] = static_cast<uint8_t>(mv);
    }
    return map;
}

// ============================================================
//  RGB ↔ HSI (all in [0,1])
// ============================================================

static void RGB2HSI(const ImageF& rgbf, ImageF& H, ImageF& S, ImageF& I) {
    const int rows = rgbf.rows;
    const int cols = rgbf.cols;
    H = ImageF(rows, cols, 1);
    S = ImageF(rows, cols, 1);
    I = ImageF(rows, cols, 1);

    const float eps     = 1e-12f;
    const float two_pi  = 2.0f * static_cast<float>(M_PI);

    for (int r = 0; r < rows; ++r) {
        for (int c = 0; c < cols; ++c) {
            float R = rgbf.at(r, c, 0);
            float G = rgbf.at(r, c, 1);
            float B = rgbf.at(r, c, 2);

            float x    = 0.5f * ((R - G) + (R - B));
            float y    = std::sqrt((R - G) * (R - G) + (R - B) * (G - B));
            float ratio = x / (y + eps);
            if (ratio >  1.f) ratio =  1.f;
            if (ratio < -1.f) ratio = -1.f;

            float theta = std::acos(ratio);          // [0, pi]
            float h;
            if(B>G){
                h = two_pi - theta;
            }
            else{
                h = theta;
            }
            h /= two_pi;                              // normalise to [0,1]

            float minRGB = std::min(R, std::min(G, B));
            float sumRGB = R + G + B;
            float s      = 1.0f - (3.0f / (sumRGB + eps)) * minRGB;
            s = clamp01(s);

            float i = sumRGB / 3.0f;

            H.at(r, c, 0) = h;
            S.at(r, c, 0) = s;
            I.at(r, c, 0) = i;
        }
    }
}

static ImageF HSI2RGB(const ImageF& H, const ImageF& S, const ImageF& I) {
    const int rows    = H.rows;
    const int cols    = H.cols;
    const float eps   = 1e-12f;
    const float two_pi = 2.0f * static_cast<float>(M_PI);

    ImageF out(rows, cols, 3);

    for (int y = 0; y < rows; ++y) {
        for (int x = 0; x < cols; ++x) {
            float h = H.at(y, x, 0) * two_pi;
            float s = S.at(y, x, 0);
            float i = I.at(y, x, 0);

            float R = 0.f, G = 0.f, B = 0.f;

            if (h >= 0.f && h < 2.0f * M_PI / 3.0f) {
                // RG sector (0° <= H < 120°)
                float hh = h;
                B = i * (1.0f - s);
                R = i * (1.0f + s * std::cos(hh) / (std::cos((float)M_PI / 3.0f - hh) + eps));
                G = 3.0f * i - (R + B);
            } else if (h < 4.0f * M_PI / 3.0f) {
                // GB sector (120° <= H < 240°)
                float hh = h - 2.0f * (float)M_PI / 3.0f;
                R = i * (1.0f - s);
                G = i * (1.0f + s * std::cos(hh) / (std::cos((float)M_PI / 3.0f - hh) + eps));
                B = 3.0f * i - (R + G);
            } else {
                // BR sector (240° <= H < 360°)
                float hh = h - 4.0f * (float)M_PI / 3.0f;
                G = i * (1.0f - s);
                B = i * (1.0f + s * std::cos(hh) / (std::cos((float)M_PI / 3.0f - hh) + eps));
                R = 3.0f * i - (G + B);
            }

            out.at(y, x, 0) = clamp01(R);
            out.at(y, x, 1) = clamp01(G);
            out.at(y, x, 2) = clamp01(B);
        }
    }
    return out;
}

// ============================================================
//  Conversion helpers between Image (uint8 BGR) and ImageF (float RGB)
// ============================================================

// BGR uint8 → RGB float [0,1] (normalise to [0,1] for processing)
static ImageF bgrToRGBf(const Image& bgr) {
    ImageF out(bgr.rows, bgr.cols, 3);
    for (int r = 0; r < bgr.rows; ++r)
        for (int c = 0; c < bgr.cols; ++c) {
            out.at(r, c, 0) = bgr.at(r, c, 2) / 255.0f;   // R
            out.at(r, c, 1) = bgr.at(r, c, 1) / 255.0f;   // G
            out.at(r, c, 2) = bgr.at(r, c, 0) / 255.0f;   // B
        }
    return out;
}

// RGB float [0,1] → BGR uint8
static Image rgbfToBGR(const ImageF& rgbf) {
    Image out(rgbf.rows, rgbf.cols, 3);
    for (int r = 0; r < rgbf.rows; ++r)
        for (int c = 0; c < rgbf.cols; ++c) {
            out.at(r, c, 2) = static_cast<uint8_t>(clamp01(rgbf.at(r, c, 0)) * 255.0f + 0.5f); // R→R
            out.at(r, c, 1) = static_cast<uint8_t>(clamp01(rgbf.at(r, c, 1)) * 255.0f + 0.5f); // G→G
            out.at(r, c, 0) = static_cast<uint8_t>(clamp01(rgbf.at(r, c, 2)) * 255.0f + 0.5f); // B→B
        }
    return out;
}

// RGB float [0,1] → Grayscale uint8 (single-channel:luma Y = 0.299R + 0.587G + 0.114B)
static Image rgbfToGray8u(const ImageF& rgbf) {
    Image out(rgbf.rows, rgbf.cols, 1);
    for (int r = 0; r < rgbf.rows; ++r)
        for (int c = 0; c < rgbf.cols; ++c) {
            float gray = 0.299f * rgbf.at(r, c, 0)
                       + 0.587f * rgbf.at(r, c, 1)
                       + 0.114f * rgbf.at(r, c, 2);
            out.at(r, c, 0) = static_cast<uint8_t>(clamp01(gray) * 255.0f + 0.5f);
        }
    return out;
}

// ============================================================
//  Method 1 – Grey-level mapping applied to every RGB channel
// ============================================================
//
// Build one histogram-equalization map from the grayscale version of the
// image, then apply that same map to every R, G, B channel independently.

Image HistEq_RGB_GrayLevelMapping(const Image& imgBGR) {
    ImageF rgbf = bgrToRGBf(imgBGR);

    // Build equalization map from the grayscale image
    Image gray8u = rgbfToGray8u(rgbf);
    std::vector<uint8_t> map = buildHistEqMap(gray8u);

    // Apply the single map to each channel of the RGB float image
    ImageF outRGBf(rgbf.rows, rgbf.cols, 3);
    for (int r = 0; r < rgbf.rows; ++r) {
        for (int c = 0; c < rgbf.cols; ++c) {
            for (int ch = 0; ch < 3; ++ch) {
                int v = static_cast<int>(rgbf.at(r, c, ch) * 255.0f + 0.5f);
                if (v < 0)   v = 0;
                if (v > 255) v = 255;
                outRGBf.at(r, c, ch) = map[v] / 255.0f;
            }
        }
    }

    return rgbfToBGR(outRGBf);
}

// ============================================================
//  Method 2 – Per-channel histogram equalization
// ============================================================
//
// Build an independent equalization map for every R, G, B channel and
// apply each map only to its own channel.

Image HistEq_RGB_PerChannel(const Image& imgBGR) {
    const int rows = imgBGR.rows;
    const int cols = imgBGR.cols;

    // Work with a temporary single-channel image for each color channel
    Image out(rows, cols, 3);

    for (int ch = 0; ch < 3; ++ch) {
        Image singleCh(rows, cols, 1);
        for (int r = 0; r < rows; ++r)
            for (int c = 0; c < cols; ++c)
                singleCh.at(r, c, 0) = imgBGR.at(r, c, ch);

        std::vector<uint8_t> map = buildHistEqMap(singleCh);

        for (int r = 0; r < rows; ++r)
            for (int c = 0; c < cols; ++c)
                out.at(r, c, ch) = map[singleCh.at(r, c, 0)];
    }

    return out;
}

// ============================================================
//  Method 3 – HSI intensity-only histogram equalization
// ============================================================
//
// Convert BGR→RGB→HSI, equalize only the I channel, then HSI→RGB→BGR.

Image HistEq_HSI_IntensityOnly(const Image& imgBGR) {
    ImageF rgbf = bgrToRGBf(imgBGR);

    // RGB float → HSI
    ImageF H, S, I;
    RGB2HSI(rgbf, H, S, I);

    // Quantise I to uint8 and build the equalization map
    const int rows = I.rows;
    const int cols = I.cols;

    Image Intensity(rows, cols, 1);
    for (int r = 0; r < rows; ++r)
        for (int c = 0; c < cols; ++c)
            Intensity.at(r, c, 0) = static_cast<uint8_t>(clamp01(I.at(r, c, 0)) * 255.0f + 0.5f);

    std::vector<uint8_t> mapI = buildHistEqMap(Intensity);

    // Apply map to I channel → Ieq (float)
    ImageF Ieq(rows, cols, 1);
    for (int r = 0; r < rows; ++r)
        for (int c = 0; c < cols; ++c)
            Ieq.at(r, c, 0) = mapI[Intensity.at(r, c, 0)] / 255.0f;

    // HSI → RGB → BGR
    ImageF outRGBf = HSI2RGB(H, S, Ieq);
    return rgbfToBGR(outRGBf);
}

// ============================================================
//  Layout helper
// ============================================================

Image makeCombined2x2(const Image& ori, const Image& m1,
                      const Image& m2,  const Image& m3) {
    const int rows = ori.rows;
    const int cols = ori.cols;
    const int ch   = ori.channels;

    // 2 × (rows × cols) grid → 2*rows × 2*cols
    Image combined(rows * 2, cols * 2, ch);

    auto copyBlock = [&](const Image& src, int rowOff, int colOff) {
        for (int r = 0; r < rows; ++r)
            for (int c = 0; c < cols; ++c)
                for (int k = 0; k < ch; ++k)
                    combined.at(rowOff + r, colOff + c, k) = src.at(r, c, k);
    };

    copyBlock(ori, 0,    0);     // top-left
    copyBlock(m1,  0,    cols);  // top-right
    copyBlock(m2,  rows, 0);     // bottom-left
    copyBlock(m3,  rows, cols);  // bottom-right

    return combined;
}
