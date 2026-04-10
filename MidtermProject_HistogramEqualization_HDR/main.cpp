// main.cpp
// 
// Complie command:
// g++ -std=c++17 -O2 main.cpp histogram_function.cpp HDR_function.cpp -o image_demo
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include "image_types.h"
#include "histogram_function.h"
#include "HDR_function.h"

#include <iostream>
#include <string>
#include <filesystem>
#include <vector>
#include <algorithm>
#include <stdexcept>

namespace fs = std::filesystem;

// ============================================================
// Image I/O implementation
// ============================================================

Image loadImage(const std::string& path) {
    int w, h, n;
    uint8_t* raw = stbi_load(path.c_str(), &w, &h, &n, 3);
    if (!raw)
        throw std::runtime_error("loadImage: cannot open " + path);

    Image img(h, w, 3);
    for (int r = 0; r < h; ++r) {
        for (int c = 0; c < w; ++c) {
            const uint8_t* px = raw + (r * w + c) * 3;
            img.at(r, c, 0) = px[2]; // B
            img.at(r, c, 1) = px[1]; // G
            img.at(r, c, 2) = px[0]; // R
        }
    }
    stbi_image_free(raw);
    return img;
}

bool saveImage(const std::string& path, const Image& img) {
    std::vector<uint8_t> rgb(img.rows * img.cols * 3);
    for (int r = 0; r < img.rows; ++r) {
        for (int c = 0; c < img.cols; ++c) {
            int idx = (r * img.cols + c) * 3;
            rgb[idx + 0] = img.at(r, c, 2); // R
            rgb[idx + 1] = img.at(r, c, 1); // G
            rgb[idx + 2] = img.at(r, c, 0); // B
        }
    }
    int ok = stbi_write_png(path.c_str(), img.cols, img.rows, 3, rgb.data(), img.cols * 3);
    return ok != 0;
}

static void save(const std::string& path, const Image& img) {
    if (!saveImage(path, img))
        std::cerr << "WARNING: failed to write " << path << "\n";
    else
        std::cout << "  saved: " << path << "\n";
}

// Get all the png/jpg/jpeg in specified folder, sorted by filename
static std::vector<fs::path> getImageFiles(const fs::path& folder) {
    std::vector<fs::path> files;

    if (!fs::exists(folder) || !fs::is_directory(folder)) {
        throw std::runtime_error("Image folder does not exist: " + folder.string());
    }

    for (const auto& entry : fs::directory_iterator(folder)) {
        if (!entry.is_regular_file()) continue;

        std::string ext = entry.path().extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

        if (ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".bmp") {
            files.push_back(entry.path());
        }
    }

    std::sort(files.begin(), files.end());
    return files;
}

int main() {
    try {
        const fs::path inputDir = "Test_image";

        // Get all images in the Test_image folder
        std::vector<fs::path> imageFiles = getImageFiles(inputDir);

        if (imageFiles.empty()) {
            std::cerr << "No images found in folder: " << inputDir << "\n";
            return 1;
        }

        std::cout << "Found " << imageFiles.size() << " images in " << inputDir << "\n\n";

        // ===========================================================
        // Part A – Histogram Equalization for ALL images
        // ===========================================================
        {
            const fs::path outDir = "ResultsHistEq";
            fs::create_directories(outDir);

            std::cout << "=== Histogram Equalization (All Images) ===\n";

            for (const auto& imgPath : imageFiles) {
                std::string filename = imgPath.filename().string();
                std::cout << "Processing: " << filename << "\n";

                Image base = loadImage(imgPath.string());
                std::string stem = imgPath.stem().string();

                bool isGrayImage = (filename.find("gray") != std::string::npos);

                if (isGrayImage) {
                    Image m1 = HistEq_RGB_GrayLevelMapping(base);

                    save((outDir / (stem + "_original.png")).string(), base);
                    save((outDir / (stem + "_HistEq_GrayOnly.png")).string(), m1);
                }
                else {
                    Image m1 = HistEq_RGB_GrayLevelMapping(base);
                    Image m2 = HistEq_RGB_PerChannel(base);
                    Image m3 = HistEq_HSI_IntensityOnly(base);
                    Image combined = makeCombined2x2(base, m1, m2, m3);

                    std::string stem = imgPath.stem().string(); // e.g. image_1

                    save((outDir / (stem + "_original.png")).string(), base);
                    save((outDir / (stem + "_HistEq_RGB_GrayLevelMapping.png")).string(), m1);
                    save((outDir / (stem + "_HistEq_RGB_PerChannel.png")).string(), m2);
                    save((outDir / (stem + "_HistEq_HSI_IntensityOnly.png")).string(), m3);
                    save((outDir / (stem + "_CombinedResults_2x2.png")).string(), combined);
                }
                std::cout << "\n";
            }
        }

        // ===========================================================
        // Part B – HDR only for image_1 and image_2
        // ===========================================================
        {
            const fs::path outDir = "ResultsHDR";
            fs::create_directories(outDir);

            std::cout << "=== HDR (Only image_1 and image_2) ===\n";

            std::vector<std::string> hdrTargets = {"image_1.png", "image_2.png"};

            std::vector<float> exposureTimes  = {0.033f, 0.25f, 2.5f};
            std::vector<float> exposureScales = {0.5f,   1.0f,  2.0f};

            for (const auto& name : hdrTargets) {
                fs::path imgPath = inputDir / name;

                if (!fs::exists(imgPath)) {
                    std::cerr << "WARNING: HDR target not found: " << imgPath << "\n";
                    continue;
                }

                std::cout << "Processing HDR: " << imgPath.filename().string() << "\n";

                Image base = loadImage(imgPath.string());

                std::vector<Image> simulated;
                Image tonemapped = runHDRPipeline(base, exposureTimes, exposureScales, simulated);
                Image combined = makeCombined2x2(simulated[0], simulated[1], simulated[2], tonemapped);

                std::string stem = imgPath.stem().string();

                save((outDir / (stem + "_original.png")).string(), base);
                save((outDir / (stem + "_sim_dark.png")).string(), simulated[0]);
                save((outDir / (stem + "_sim_mid.png")).string(), simulated[1]);
                save((outDir / (stem + "_sim_bright.png")).string(), simulated[2]);
                save((outDir / (stem + "_hdr_tonemapped.png")).string(), tonemapped);
                save((outDir / (stem + "_hdr_combined.png")).string(), combined);

                std::cout << "\n";
            }
        }

        std::cout << "Done.\n";
        return 0;
    }
    catch (const std::exception& e) {
        std::cerr << "ERROR: " << e.what() << "\n";
        return 1;
    }
}