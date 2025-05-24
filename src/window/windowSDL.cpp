#include "preferences.h"
#include "window.h"
#include "utils.h"
#include <SDL_render.h>
#include <chrono>
#include <algorithm>
#include <thread>
#include <atomic>

#define HISTWIDTH 512
#define HISTHEIGHT 256

//--- Create SDL Texture ---//
/*
    Create a valid SDL texture with the given image
    taking into account the image's rotation
*/
void mainWindow::createSDLTexture(image* actImage) {

    if (!actImage)
        return;
    // For rotations 6 and 8 (90 degrees), we need to swap width and height
    int textureWidth = (actImage->imRot == 6 || actImage->imRot == 8) ? actImage->height : actImage->width;
    int textureHeight = (actImage->imRot == 6 || actImage->imRot == 8) ? actImage->width : actImage->height;

    SDL_Texture* texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STREAMING,
                                                textureWidth, textureHeight);
    if (texture) {
        actImage->texture = texture;
        actImage->sdlRotation = actImage->imRot;
    }
    else {
        LOG_WARN("Unable to create SDL texture for image: {}", actImage->srcFilename);
    }

}

//--- Update SDL Texture ---//
/*
    Update the image's SDL texture based on the Dispbuf.
    Also queue up a separate thread to process the
    image's histogram
*/
void mainWindow::updateSDLTexture(image* actImage) {

    if (!actImage || !actImage->imageLoaded)
        return;

    if (actImage->texture == nullptr) {
            createSDLTexture(actImage);
    }
    if (actImage->sdlRotation != actImage->imRot) {
        if (actImage->texture) {
            SDL_DestroyTexture((SDL_Texture*)actImage->texture);
            actImage->texture = nullptr;
        }
        createSDLTexture(actImage);
    }



    void* pixelData = nullptr;
    int pitch = 0;
    if (SDL_LockTexture((SDL_Texture*)actImage->texture, nullptr, &pixelData, &pitch) != 0) {
        LOG_ERROR("Unable to lock SDL texture for image: {}", actImage->srcFilename);
        actImage->sdlUpdate = false;
        return;
    }

    const int srcWidth = actImage->width;
    const int srcHeight = actImage->height;
    const int bytesPerPixel = 4; // Assuming RGBA with 4 bytes per pixel
    const int srcRowBytes = srcWidth * bytesPerPixel;

    if (!actImage->dispImgData) {
        //LOG_WARN("Image needing update has no buffer! {}", actImage->srcFilename);
        actImage->sdlUpdate = false;
        SDL_UnlockTexture((SDL_Texture*)actImage->texture);
        return;
    }

    // Histogram Processing
    if (!actImage->histTex) {
        SDL_Texture* texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STREAMING,
                                                    HISTWIDTH, HISTHEIGHT);
        if (texture) {
            actImage->histTex = texture;
        }
        else {
            LOG_WARN("Unable to create SDL texture for histogram: {}", actImage->srcFilename);
            actImage->sdlUpdate = false;
            SDL_UnlockTexture((SDL_Texture*)actImage->texture);
            return;
        }
    }

    void* hpixels;
    int hpitch;
    if (SDL_LockTexture((SDL_Texture*)actImage->histTex, nullptr, &hpixels, &hpitch) != 0) {
        LOG_ERROR("Unable to lock SDL texture for histogram: {}", actImage->srcFilename);
        actImage->sdlUpdate = false;
        SDL_UnlockTexture((SDL_Texture*)actImage->texture);
        return;
    }
    // Launch the thread for the histo calc
    float histInt = appPrefs.prefs.histInt;
    std::thread histoThread = std::thread([this, &actImage, &hpixels, &hpitch, histInt]{
        updateSDLHistogram(actImage, hpixels, hpitch, histInt);
    });


    // Apply rotation based on imRot value
     switch (actImage->imRot) {
        case 1: // Normal (0 degrees)
            for (int row = 0; row < srcHeight; ++row) {
                const void* srcRow = static_cast<const uint8_t*>(actImage->dispImgData) + (row * srcRowBytes);
                void* dstRow = static_cast<uint8_t*>(pixelData) + (row * pitch);
                memcpy(dstRow, srcRow, srcRowBytes);
            }
            break;

        case 6: // Left (90 degrees counterclockwise)
            for (int y = 0; y < srcHeight; ++y) {
                for (int x = 0; x < srcWidth; ++x) {
                    // Destination coordinates (rotated)
                    // x' = height - 1 - y, y' = x
                    int destX = srcHeight - 1 - y;
                    int destY = x;

                    // Source pixel
                    const uint8_t* srcPixel = static_cast<const uint8_t*>(actImage->dispImgData) +
                                            (y * srcRowBytes) + (x * bytesPerPixel);

                    // Destination pixel
                    uint8_t* dstPixel = static_cast<uint8_t*>(pixelData) +
                                        (destY * pitch) + (destX * bytesPerPixel);

                    // Copy pixel data (RGBA)
                    memcpy(dstPixel, srcPixel, bytesPerPixel);
                }
            }
            break;

        case 3: // Upside-down (180 degrees)
            for (int y = 0; y < srcHeight; ++y) {
                for (int x = 0; x < srcWidth; ++x) {
                    // Destination coordinates (rotated)
                    // x' = width - 1 - x, y' = height - 1 - y
                    int destX = srcWidth - 1 - x;
                    int destY = srcHeight - 1 - y;

                    // Source pixel
                    const uint8_t* srcPixel = static_cast<const uint8_t*>(actImage->dispImgData) +
                                            (y * srcRowBytes) + (x * bytesPerPixel);

                    // Destination pixel
                    uint8_t* dstPixel = static_cast<uint8_t*>(pixelData) +
                                        (destY * pitch) + (destX * bytesPerPixel);

                    // Copy pixel data (RGBA)
                    memcpy(dstPixel, srcPixel, bytesPerPixel);
                }
            }
            break;

        case 8: // Right (90 degrees clockwise)
            for (int y = 0; y < srcHeight; ++y) {
                for (int x = 0; x < srcWidth; ++x) {
                    // Destination coordinates (rotated)
                    // x' = y, y' = width - 1 - x
                    int destX = y;
                    int destY = srcWidth - 1 - x;

                    // Source pixel
                    const uint8_t* srcPixel = static_cast<const uint8_t*>(actImage->dispImgData) +
                                            (y * srcRowBytes) + (x * bytesPerPixel);

                    // Destination pixel
                    uint8_t* dstPixel = static_cast<uint8_t*>(pixelData) +
                                        (destY * pitch) + (destX * bytesPerPixel);

                    // Copy pixel data (RGBA)
                    memcpy(dstPixel, srcPixel, bytesPerPixel);
                }
            }
            break;

        default:
            // For any other values, just do normal rendering
            for (int row = 0; row < srcHeight; ++row) {
                const void* srcRow = static_cast<const uint8_t*>(actImage->dispImgData) + (row * srcRowBytes);
                void* dstRow = static_cast<uint8_t*>(pixelData) + (row * pitch);
                memcpy(dstRow, srcRow, srcRowBytes);
            }
            break;
        }
        histoThread.join();
        SDL_UnlockTexture((SDL_Texture*)actImage->histTex);
        SDL_UnlockTexture((SDL_Texture*)actImage->texture);
        actImage->sdlUpdate = false;
        actImage->delDispBuf();
}

//--- Calculate Histogram from RGBA ---//
/*
    Calculate the histogram 'bins' from
    the Dispimg. Process luma as well based
    on Rec709 primaries
*/
void calculateHistogramFromRGBA(const uint8_t *rgba_buffer, int width,
                                int height, HistogramData &histogram,
                                const unsigned int* xPoints, const unsigned int* yPoints) {
    // Clear histogram data
    histogram = HistogramData();

    const int pixel_count = width * height;
    const int num_threads = std::thread::hardware_concurrency();
    const int pixels_per_thread = pixel_count / num_threads;

    // Create thread-local histograms to avoid contention
    std::vector<HistogramData> thread_histograms(num_threads);
    std::vector<std::thread> threads;

    // Launch threads
    for (int t = 0; t < num_threads; ++t) {
        threads.emplace_back([&, t]() {
            int start = t * pixels_per_thread;
            int end = (t == num_threads - 1) ? pixel_count : (t + 1) * pixels_per_thread;

            // Use local variables to reduce memory access
            std::array<int, 512> local_r_hist{};
            std::array<int, 512> local_g_hist{};
            std::array<int, 512> local_b_hist{};
            std::array<int, 512> local_lum_hist{};

            // Process pixels in this thread's range
            for (int i = start; i < end; ++i) {
                const int idx = i * 4;
                int y = i / width;
                int x = i % width;
                if (!isPointInBox(x, y, xPoints, yPoints))
                    continue;
                uint8_t r = rgba_buffer[idx];
                uint8_t g = rgba_buffer[idx + 1];
                uint8_t b = rgba_buffer[idx + 2];

                // For 512 bins, each 8-bit value maps to 2 bins
                // This ensures smooth distribution without gaps
                int r_bin_low = r * 2;
                int r_bin_high = r * 2 + 1;
                int g_bin_low = g * 2;
                int g_bin_high = g * 2 + 1;
                int b_bin_low = b * 2;
                int b_bin_high = b * 2 + 1;

                // Count RGB values in both bins for smooth interpolation
                local_r_hist[r_bin_low]++;
                local_r_hist[r_bin_high]++;
                local_g_hist[g_bin_low]++;
                local_g_hist[g_bin_high]++;
                local_b_hist[b_bin_low]++;
                local_b_hist[b_bin_high]++;

                // Calculate luminance with improved coefficients
                // Using ITU-R BT.709 standard coefficients
                float luminance_f = 0.2126f * r + 0.7152f * g + 0.0722f * b;
                int luminance = static_cast<int>(luminance_f + 0.5f); // Proper rounding

                // Map luminance to two bins
                int lum_bin_low = luminance * 2;
                int lum_bin_high = luminance * 2 + 1;

                // Clamp to valid range (luminance can exceed 255 in edge cases)
                if (lum_bin_high > 511) lum_bin_high = 511;

                local_lum_hist[lum_bin_low]++;
                local_lum_hist[lum_bin_high]++;
            }

            // Copy local histograms to thread histogram
            thread_histograms[t].r_hist = local_r_hist;
            thread_histograms[t].g_hist = local_g_hist;
            thread_histograms[t].b_hist = local_b_hist;
            thread_histograms[t].luminance_hist = local_lum_hist;
        });
    }

    // Wait for all threads to complete
    for (auto &thread : threads) {
        thread.join();
    }

    // Combine thread-local histograms using SIMD-friendly pattern
    for (const auto &thread_hist : thread_histograms) {
        for (int i = 0; i < 512; i += 4) { // Process 4 bins at a time
            histogram.r_hist[i] += thread_hist.r_hist[i];
            histogram.r_hist[i+1] += thread_hist.r_hist[i+1];
            histogram.r_hist[i+2] += thread_hist.r_hist[i+2];
            histogram.r_hist[i+3] += thread_hist.r_hist[i+3];

            histogram.g_hist[i] += thread_hist.g_hist[i];
            histogram.g_hist[i+1] += thread_hist.g_hist[i+1];
            histogram.g_hist[i+2] += thread_hist.g_hist[i+2];
            histogram.g_hist[i+3] += thread_hist.g_hist[i+3];

            histogram.b_hist[i] += thread_hist.b_hist[i];
            histogram.b_hist[i+1] += thread_hist.b_hist[i+1];
            histogram.b_hist[i+2] += thread_hist.b_hist[i+2];
            histogram.b_hist[i+3] += thread_hist.b_hist[i+3];

            histogram.luminance_hist[i] += thread_hist.luminance_hist[i];
            histogram.luminance_hist[i+1] += thread_hist.luminance_hist[i+1];
            histogram.luminance_hist[i+2] += thread_hist.luminance_hist[i+2];
            histogram.luminance_hist[i+3] += thread_hist.luminance_hist[i+3];
        }
    }
}

//--- Update SDL Histogram ---//
/*
    Based on the calculated bins, generate the resulting
    histogram image and save it to the image texture
*/
void mainWindow::updateSDLHistogram(image* img, void* pixels, int pitch, float intensityMultiplier) {
    auto start = std::chrono::steady_clock::now();
    uint32_t* pixelBuffer = static_cast<uint32_t*>(pixels);

    if (!appPrefs.prefs.histEnable) {
        std::memset(pixelBuffer, 0, HISTWIDTH * HISTHEIGHT * sizeof(uint32_t));
        return;
    }

    // Clamp intensity multiplier to valid range
    intensityMultiplier = std::clamp(intensityMultiplier, 0.0f, 1.0f);

    // Clear to dark gray background
    const uint32_t bg_color = 0x7F101010;
    for (int y = 0; y < HISTHEIGHT; ++y) {
        for (int x = 0; x < HISTWIDTH; ++x) {
            pixelBuffer[y * (pitch / 4) + x] = bg_color;
        }
    }
    unsigned int cropBoxX[4];
    unsigned int cropBoxY[4];
    for (int i = 0; i < 4; i++) {
        cropBoxX[i] = img->imgParam.cropBoxX[i] * img->width;
        cropBoxY[i] = img->imgParam.cropBoxY[i] * img->height;
    }

    HistogramData histogram;
    calculateHistogramFromRGBA(img->dispImgData, img->width, img->height,
                            histogram, cropBoxX, cropBoxY);

    // Find max values for scaling
    int max_r = *std::max_element(histogram.r_hist.begin(), histogram.r_hist.end());
    int max_g = *std::max_element(histogram.g_hist.begin(), histogram.g_hist.end());
    int max_b = *std::max_element(histogram.b_hist.begin(), histogram.b_hist.end());
    int max_lum = *std::max_element(histogram.luminance_hist.begin(), histogram.luminance_hist.end());

    // Calculate percentile-based maximum to reduce spike impact
    auto calculatePercentileMax = [](const std::array<int, 512>& hist, float percentile = 98.0f) -> int {
        std::vector<int> sorted_values;
        for (int val : hist) {
            if (val > 0) sorted_values.push_back(val);
        }
        if (sorted_values.empty()) return 1;

        std::sort(sorted_values.begin(), sorted_values.end());
        int idx = static_cast<int>(sorted_values.size() * (percentile / 100.0f));
        if (idx >= sorted_values.size()) idx = sorted_values.size() - 1;

        return sorted_values[idx];
    };

    // Use percentile-based maximum with a minimum floor to prevent over-scaling
    int percentile_max_r = calculatePercentileMax(histogram.r_hist);
    int percentile_max_g = calculatePercentileMax(histogram.g_hist);
    int percentile_max_b = calculatePercentileMax(histogram.b_hist);
    int percentile_max_lum = calculatePercentileMax(histogram.luminance_hist);

    // Apply logarithmic dampening to extreme values
    auto dampenMax = [](int max_val, int percentile_val) -> int {
        if (max_val > percentile_val * 3) {
            return percentile_val + static_cast<int>(std::log(max_val - percentile_val + 1) * percentile_val * 0.5);
        }
        return max_val;
    };

    max_r = dampenMax(max_r, percentile_max_r);
    max_g = dampenMax(max_g, percentile_max_g);
    max_b = dampenMax(max_b, percentile_max_b);
    max_lum = dampenMax(max_lum, percentile_max_lum);

    // Ensure minimum scaling to prevent division by zero
    if (max_r == 0) max_r = 1;
    if (max_g == 0) max_g = 1;
    if (max_b == 0) max_b = 1;
    if (max_lum == 0) max_lum = 1;

    // Calculate base intensities with multiplier applied
    const int red_intensity = static_cast<int>(200 * intensityMultiplier);
    const int green_intensity = static_cast<int>(200 * intensityMultiplier);
    const int blue_intensity = static_cast<int>(200 * intensityMultiplier);
    const int white_intensity = static_cast<int>(255 * intensityMultiplier);

    // Draw histogram with 512 bins
    for (int i = 0; i < 512; ++i) {
        // Each bin is exactly 1 pixel wide for 512-width histogram
        int x = i;

        // Calculate heights with channel-specific scaling
        int r_height = (histogram.r_hist[i] * (HISTHEIGHT - 10)) / max_r;
        int g_height = (histogram.g_hist[i] * (HISTHEIGHT - 10)) / max_g;
        int b_height = (histogram.b_hist[i] * (HISTHEIGHT - 10)) / max_b;
        int lum_height = (histogram.luminance_hist[i] * (HISTHEIGHT - 10)) / max_lum;

        // Apply minimum visibility threshold
        if (histogram.r_hist[i] > 0 && r_height < 2) r_height = 2;
        if (histogram.g_hist[i] > 0 && g_height < 2) g_height = 2;
        if (histogram.b_hist[i] > 0 && b_height < 2) b_height = 2;
        if (histogram.luminance_hist[i] > 0 && lum_height < 2) lum_height = 2;

        // Draw luminance histogram in white (lowest layer) with intensity multiplier
        for (int y = HISTHEIGHT - 1; y >= HISTHEIGHT - lum_height && y >= 0; --y) {
            uint8_t intensity = static_cast<uint8_t>(white_intensity);
            pixelBuffer[y * (pitch / 4) + x] = 0xFF000000 | (intensity << 16) | (intensity << 8) | intensity;
        }

        // Draw RGB histograms with proper additive blending
        for (int y = HISTHEIGHT - 1; y >= 0; --y) {
            uint32_t& pixel = pixelBuffer[y * (pitch / 4) + x];

            // Skip if it's already white (luminance) - check for max white with current intensity
            uint8_t pixel_r = (pixel >> 0) & 0xFF;
            uint8_t pixel_g = (pixel >> 8) & 0xFF;
            uint8_t pixel_b = (pixel >> 16) & 0xFF;
            if (pixel_r == white_intensity && pixel_g == white_intensity && pixel_b == white_intensity) continue;

            // Extract current color components
            uint8_t current_r = pixel_r;
            uint8_t current_g = pixel_g;
            uint8_t current_b = pixel_b;
            uint8_t current_a = (pixel >> 24) & 0xFF;

            // Determine what to add based on histogram heights
            float add_r = 0, add_g = 0, add_b = 0;

            if (y >= HISTHEIGHT - r_height) {
                add_r = red_intensity;
            }
            if (y >= HISTHEIGHT - g_height) {
                add_g = green_intensity;
            }
            if (y >= HISTHEIGHT - b_height) {
                add_b = blue_intensity;
            }

            // Mix colors additively
            if (add_r > 0 || add_g > 0 || add_b > 0) {
                // Remove background and add new colors
                if (current_r == 0x10 && current_g == 0x10 && current_b == 0x10) {
                    // If it's background, start fresh
                    current_r = 0;
                    current_g = 0;
                    current_b = 0;
                }

                current_r = std::min(255, current_r + static_cast<int>(add_r));
                current_g = std::min(255, current_g + static_cast<int>(add_g));
                current_b = std::min(255, current_b + static_cast<int>(add_b));

                // Recompose pixel
                pixel = (current_a << 24) | (current_b << 16) | (current_g << 8) | current_r;
            }
        }
    }
    auto end = std::chrono::steady_clock::now();
    auto dur = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    //LOG_INFO("Histo: {}us, {}ms", dur.count(), dur.count()/1000);
}
