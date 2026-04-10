#pragma once
// HDR_function.h
#include "image_types.h"
#include <vector>

// ============================================================
//  Individual pipeline steps
// ============================================================

// Simulate a different exposure by scaling every pixel value and clamping.
Image  applyExposureScale(const Image& bgr8u, float scale);

// Debevec & Malik (1997) weighted log-irradiance approach.
// imgs and exposureTimes must have the same length.
ImageF mergeDebevec(const std::vector<Image>& imgs,
                    const std::vector<float>& exposureTimes);

// Luminance-based global Reinhard (2002) tone mapping with saturation control.
//
// 流程：world luminance Lw → log-average Lavg → scale → Reinhard → 色彩還原 → gamma
//
//   key   – middle-grey key value，通常取 0.18（越高整體越亮）
//   sat   – saturation exponent：1.0 = 完整色彩，0.0 = 灰階輸出
//   gamma – 輸出 gamma 校正：1.0 = 線性，2.2 = sRGB 標準
ImageF tonemapReinhard(const ImageF& hdr,
                       float key   = 0.18f,
                       float sat   = 1.0f,
                       float gamma = 2.2f);

// Convert a floating-point LDR image in [0,1] to a uint8 BGR image.
Image  ldrToImage(const ImageF& ldr);

// ============================================================
//  Convenience wrappers
// ============================================================

// Produce one simulated exposure per entry in `scales`.
std::vector<Image> simulateExposures(const Image& base,
                                     const std::vector<float>& scales);

// Full pipeline: simulate → merge → tone-map → convert.
Image runHDRPipeline(const Image& base,
                     const std::vector<float>& exposureTimes,
                     const std::vector<float>& exposureScales,
                     std::vector<Image>& outSimulated);
