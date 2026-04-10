#include "HDR_function.h"
#include <cmath>
#include <algorithm>
#include <stdexcept>

// ============================================================
//  Internal helpers
// ============================================================

static inline float clampf(float x, float lo, float hi) {
    if (x < lo) return lo;
    if (x > hi) return hi;
    return x;
}

// ============================================================
//  Step 1 – Simulate different exposures
// ============================================================
//
// Multiply every pixel value by `scale`, then clamp to [0, 255].

Image applyExposureScale(const Image& bgr8u, float scale) {
    const int rows = bgr8u.rows;
    const int cols = bgr8u.cols;
    const int ch   = bgr8u.channels;
    Image out(rows, cols, ch);

    for (int r = 0; r < rows; ++r) {
        for (int c = 0; c < cols; ++c) {
            for (int k = 0; k < ch; ++k) {
                float val = bgr8u.at(r, c, k) * scale;
                out.at(r, c, k) = static_cast<uint8_t>(clampf(val, 0.f, 255.f) + 0.5f);
            }
        }
    }
    return out;
}

// ============================================================
//  Step 2 – Camera response function (CRF)
// ============================================================
//
// Debevec & Malik (1997) model: Z = g(E·Δt)  →  log E = g⁻¹(Z) - log Δt
//
// We use the standard hat-shaped weighting function and solve for g(z)
// with the linear system approach.  For simplicity we assume the camera
// response is the natural logarithm (i.e. the camera is linear in log
// space), which is a reasonable approximation for most digital sensors.
//
// More precisely, if the camera is linear:  Z = E · Δt  ⟹  E = Z / Δt
// Taking logs:  log E = log Z - log Δt
//
// We use this directly rather than estimating g from data.
//
// Weight function: hat/triangle over [0,255], peaks at 128.
static float weightZ(int z) {
    const float zmin = 0.f, zmax = 255.f, zmid = 127.5f;
    if (z <= static_cast<int>(zmid))
        return static_cast<float>(z - zmin + 1);
    else
        return static_cast<float>(zmax - z + 1);
}

// ============================================================
//  Step 3 – Merge exposures into an HDR radiance map (Debevec)
// ============================================================
//
// For each pixel p and channel k:
//
//   log Ep,k = ( Σ_i  w(Z_i)·(log Z_i - log Δt_i) )
//              ─────────────────────────────────────
//                      Σ_i  w(Z_i)
//
// where Z_i is the pixel value in exposure i and Δt_i is its exposure time.
// The result is a floating-point radiance image (linear, not tone-mapped).

ImageF mergeDebevec(const std::vector<Image>& imgs,
                    const std::vector<float>& exposureTimes) {
    if (imgs.empty() || imgs.size() != exposureTimes.size())
        throw std::invalid_argument("mergeDebevec: mismatched image / exposure count");

    const int rows = imgs[0].rows;
    const int cols = imgs[0].cols;
    const int ch   = imgs[0].channels;
    const int N    = static_cast<int>(imgs.size());

    ImageF hdr(rows, cols, ch);

    for (int r = 0; r < rows; ++r) {
        for (int c = 0; c < cols; ++c) {
            for (int k = 0; k < ch; ++k) {
                double numerator   = 0.0;
                double denominator = 0.0;

                for (int i = 0; i < N; ++i) {
                    int    Z = imgs[i].at(r, c, k);
                    float  w = weightZ(Z);

                    // Avoid log(0): clamp Z to at least 1
                    float logZ = std::log(std::max(Z, 1));
                    float logT = std::log(exposureTimes[i]);

                    numerator   += w * (logZ - logT);
                    denominator += w;
                }

                float logE = (denominator > 1e-8)
                             ? static_cast<float>(numerator / denominator)
                             : 0.f;

                hdr.at(r, c, k) = std::exp(logE);   // linear radiance
            }
        }
    }
    return hdr;
}

// ============================================================
//  Step 4 – Reinhard tone mapping (luminance-based)
// ============================================================
//
// 完整流程（參考 Reinhard et al. 2002 及 brunopop/hdr 實作）：
//
//   [1] 計算每個像素的 world luminance（ITU-R BT.709）
//           Lw(p) = 0.2126·R + 0.7152·G + 0.0722·B
//
//   [2] 計算場景的 log-average luminance（只對非零像素做加總）
//           Lavg = exp( Σ log(Lw(p)) / N_nonzero )
//
//   [3] 把 world luminance 對應到 key value（中灰色調 a = 0.18）
//           Lscaled(p) = (a / Lavg) · Lw(p)
//
//   [4] Reinhard global tone map
//           Ld(p) = Lscaled(p) / (1 + Lscaled(p))
//
//   [5] 用 saturation 指數把色彩從 HDR 還原回 LDR（保留色相）
//           Ic(p) = Ld(p) · (Iw_c(p) / Lw(p))^sat    ，c ∈ {R,G,B}
//           sat=1 → 完整保留色彩；sat=0 → 退化為灰階
//
//   [6] Gamma 校正
//           Ic(p) = Ic(p)^(1/gamma)
//
// Parameters:
//   key   – middle-grey key value，通常取 0.18
//   sat   – saturation exponent，控制色彩飽和度，範圍 [0,1]
//   gamma – 輸出 gamma，通常取 1.0~2.2（1.0 = 不校正）
 
ImageF tonemapReinhard(const ImageF& hdr,
                       float key,
                       float sat,
                       float gamma) {
    const int   rows = hdr.rows;
    const int   cols = hdr.cols;
    const int   ch   = hdr.channels;  // 預期為 3 (BGR)
    const float eps  = 1e-8f;
    const float inv_gamma = (gamma > 1e-4f) ? (1.0f / gamma) : 1.0f;
 
    // ---- [1] 計算每個像素的 world luminance ----
    // 儲存格式與 hdr 相同（row-major），單獨存成 1D vector
    // 注意：hdr 以 BGR 順序儲存，係數對應 B=0.0722, G=0.7152, R=0.2126
    std::vector<float> Lw(rows * cols, 0.f);
    for (int r = 0; r < rows; ++r) {
        for (int c = 0; c < cols; ++c) {
            float B = hdr.at(r, c, 0);
            float G = hdr.at(r, c, 1);
            float R = hdr.at(r, c, 2);
            Lw[r * cols + c] = 0.2126f * R + 0.7152f * G + 0.0722f * B;
        }
    }
 
    // ---- [2] Log-average luminance（只對非零像素加總）----
    double logSum   = 0.0;
    int    nonZeroN = 0;
    for (int i = 0; i < rows * cols; ++i) {
        if (Lw[i] > 0.f) {
            logSum += std::log(Lw[i]);
            ++nonZeroN;
        }
    }
    // 若全黑圖片（極端情況），退回到 eps 保護
    float Lavg = (nonZeroN > 0)
                 ? static_cast<float>(std::exp(logSum / nonZeroN))
                 : eps;
 
    // ---- [3][4][5][6] 逐像素 tone map ----
    ImageF ldr(rows, cols, ch);
 
    for (int r = 0; r < rows; ++r) {
        for (int c = 0; c < cols; ++c) {
            float lw = Lw[r * cols + c];
 
            // [3] scale world luminance
            float Lscaled = (key / (Lavg + eps)) * lw;
 
            // [4] Reinhard global operator → display luminance Ld
            float Ld = Lscaled / (1.0f + Lscaled);
 
            // [5][6] 還原每個色頻並套用 gamma
            for (int k = 0; k < ch; ++k) {
                float Iw = hdr.at(r, c, k);   // 該頻道的 HDR radiance
 
                float Ic;
                if (lw > eps) {
                    // Ic = Ld · (Iw / Lw)^sat
                    float ratio = Iw / (lw + eps);
                    // sat=1 時 ratio^1 = ratio，sat=0 時 ratio^0 = 1（灰階）
                    Ic = Ld * std::pow(clampf(ratio, 0.f, 10.f), sat);
                } else {
                    // 純黑像素直接設為 0
                    Ic = 0.f;
                }
 
                // gamma 校正
                Ic = std::pow(clampf(Ic, 0.f, 1.f), inv_gamma);
 
                ldr.at(r, c, k) = clampf(Ic, 0.f, 1.f);
            }
        }
    }
    return ldr;
}

// ============================================================
//  Step 5 – Convert floating-point LDR [0,1] to uint8 BGR
// ============================================================

Image ldrToImage(const ImageF& ldr) {
    const int rows = ldr.rows;
    const int cols = ldr.cols;
    const int ch   = ldr.channels;
    Image out(rows, cols, ch);

    for (int r = 0; r < rows; ++r)
        for (int c = 0; c < cols; ++c)
            for (int k = 0; k < ch; ++k)
                out.at(r, c, k) = static_cast<uint8_t>(
                    clampf(ldr.at(r, c, k), 0.f, 1.f) * 255.0f + 0.5f);
    return out;
}

// ============================================================
//  Full HDR pipeline (convenience wrapper)
// ============================================================
//
// 1. Simulate exposures from a single base image
// 2. Merge into HDR radiance map
// 3. Tone-map with Reinhard
// Returns simulated exposures and the final LDR result.

std::vector<Image> simulateExposures(const Image& base,
                                     const std::vector<float>& scales) {
    std::vector<Image> imgs;
    imgs.reserve(scales.size());
    for (float s : scales)
        imgs.push_back(applyExposureScale(base, s));
    return imgs;
}

Image runHDRPipeline(const Image& base,
                     const std::vector<float>& exposureTimes,
                     const std::vector<float>& exposureScales,
                     std::vector<Image>& outSimulated) {
    if (exposureTimes.size() != exposureScales.size())
        throw std::invalid_argument("runHDRPipeline: exposure arrays must have same length");

    outSimulated = simulateExposures(base, exposureScales);

    // Merge
    ImageF hdr = mergeDebevec(outSimulated, exposureTimes);
 
    // Tone map  (key=0.18, sat=1.0, gamma=2.2)
    ImageF ldr = tonemapReinhard(hdr, 0.18f, 1.0f, 2.2f);

    return ldrToImage(ldr);
}
