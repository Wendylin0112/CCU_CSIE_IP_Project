#pragma once
// histogram_function.h
#include "image_types.h"

// ============================================================
//  Three histogram-equalization methods
// ============================================================

// Method 1 – Build ONE equalization map from the grayscale version of the
//             image, then apply that same map to every R, G, B channel.
Image HistEq_RGB_GrayLevelMapping(const Image& imgBGR);

// Method 2 – Build an INDEPENDENT equalization map for each channel
//             (R, G, B treated separately).
Image HistEq_RGB_PerChannel(const Image& imgBGR);

// Method 3 – Convert to HSI, equalize only the Intensity channel, then
//             convert back to RGB.
Image HistEq_HSI_IntensityOnly(const Image& imgBGR);

// ============================================================
//  tile four same-sized images into a 2×2 grid
// ============================================================
Image makeCombined2x2(const Image& ori, const Image& m1,
                      const Image& m2,  const Image& m3);
