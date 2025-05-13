#include "window.h"
#include <SDL_render.h>
#include <chrono>
#include <algorithm>
#include <thread>
#include <atomic>

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

void mainWindow::updateSDLTexture(image* actImage) {

    if (!actImage)
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

    // Update the histogram
    std::thread histoThread = std::thread([this, &actImage]{
        updateSDLHistogram(actImage);
    });
    //histoThread.detach();


    void* pixelData = nullptr;
    int pitch = 0;
    if (SDL_LockTexture((SDL_Texture*)actImage->texture, nullptr, &pixelData, &pitch) != 0) {
        LOG_ERROR("Unable to lock SDL texture for image: {}", actImage->srcFilename);
        actImage->sdlUpdate = false;
        histoThread.join();
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
        histoThread.join();
        return;
    }

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

        SDL_UnlockTexture((SDL_Texture*)actImage->texture);
        histoThread.join();
        actImage->sdlUpdate = false;
        actImage->delDispBuf();
}

void calculateHistogramFromRGBA(const uint8_t *rgba_buffer, int width,
                                int height, HistogramData &histogram) {
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
      int end =
          (t == num_threads - 1) ? pixel_count : (t + 1) * pixels_per_thread;

      // Process pixels in this thread's range
      for (int i = start; i < end; ++i) {
        const int idx = i * 4;
        uint8_t r = rgba_buffer[idx];
        uint8_t g = rgba_buffer[idx + 1];
        uint8_t b = rgba_buffer[idx + 2];

        // Count RGB values
        thread_histograms[t].r_hist[r]++;
        thread_histograms[t].g_hist[g]++;
        thread_histograms[t].b_hist[b]++;

        // Calculate luminance (Y = 0.299R + 0.587G + 0.114B)
        uint8_t luminance =
            static_cast<uint8_t>(0.2126f * r + 0.7152f * g + 0.0722f * b);
        thread_histograms[t].luminance_hist[luminance]++;
      }
    });
  }

  // Wait for all threads to complete
  for (auto &thread : threads) {
    thread.join();
  }

  // Combine thread-local histograms
  for (const auto &thread_hist : thread_histograms) {
    for (int i = 0; i < 256; ++i) {
      histogram.r_hist[i] += thread_hist.r_hist[i];
      histogram.g_hist[i] += thread_hist.g_hist[i];
      histogram.b_hist[i] += thread_hist.b_hist[i];
      histogram.luminance_hist[i] += thread_hist.luminance_hist[i];
    }
  }
}

//--- HISTOGRAM ---//

#define HISTWIDTH 512
#define HISTHEIGHT 256


void mainWindow::updateSDLHistogram(image* img) {
    if (!img)
        return;
    if (!img->dispImgData)
        return;
    if (!img->histTex) {
        SDL_Texture* texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STREAMING,
                                                    HISTWIDTH, HISTHEIGHT);
        if (texture) {
            img->histTex = texture;
        }
        else {
            LOG_WARN("Unable to create SDL texture for histogram: {}", img->srcFilename);
            return;
        }
    }

    void* pixels;
    int pitch;
    if (SDL_LockTexture((SDL_Texture*)img->histTex, nullptr, &pixels, &pitch) != 0) {
        LOG_ERROR("Unable to lock SDL texture for histogram: {}", img->srcFilename);
        return;
    }

    uint32_t* pixelBuffer = static_cast<uint32_t*>(pixels);
    // Clear to dark gray background
    const uint32_t bg_color = 0xFF202020;
    for (int y = 0; y < HISTHEIGHT; ++y) {
        for (int x = 0; x < HISTWIDTH; ++x) {
            pixelBuffer[y * (pitch / 4) + x] = bg_color;
        }
    }


    HistogramData histogram;

    calculateHistogramFromRGBA(img->dispImgData, img->width, img->height, histogram);

    // Find max values for scaling
    int max_r = *std::max_element(histogram.r_hist.begin(), histogram.r_hist.end());
    int max_g = *std::max_element(histogram.g_hist.begin(), histogram.g_hist.end());
    int max_b = *std::max_element(histogram.b_hist.begin(), histogram.b_hist.end());
    int max_lum = *std::max_element(histogram.luminance_hist.begin(), histogram.luminance_hist.end());

    // Calculate percentile-based maximum to reduce spike impact
    auto calculatePercentileMax = [](const std::array<int, 256>& hist, float percentile = 98.0f) -> int {
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
            // If the max is more than 3x the percentile, dampen it
            return percentile_val + static_cast<int>(std::log(max_val - percentile_val + 1) * percentile_val * 0.5);
        }
        return max_val;
    };

    max_r = dampenMax(max_r, percentile_max_r);
    max_g = dampenMax(max_g, percentile_max_g);
    max_b = dampenMax(max_b, percentile_max_b);
    max_lum = dampenMax(max_lum, percentile_max_lum);

    int max_value = std::max({max_r, max_g, max_b, max_lum});

    // Ensure minimum scaling to keep smaller values visible
    int min_visible_height = HISTHEIGHT / 20; // 5% minimum visibility
    if (max_value == 0) max_value = 1; // Prevent division by zero

    // Calculate bin width
    float bin_width = static_cast<float>(HISTWIDTH) / 256.0f;

    // Draw histogram bars
    for (int i = 0; i < 256; ++i) {
        int x_start = static_cast<int>(i * bin_width);
        int x_end = static_cast<int>((i + 1) * bin_width);

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

        // Draw bars from bottom up
        for (int x = x_start; x < x_end && x < HISTWIDTH; ++x) {
            if (true) {
                // Draw luminance histogram in white
                for (int y = HISTHEIGHT - 1; y >= HISTHEIGHT - lum_height && y >= 0; --y) {
                    pixelBuffer[y * (pitch / 4) + x] = 0xFFFFFFFF; // White
                }
            }

            if (true) {
                // Draw RGB histograms with transparency
                // Red channel
                for (int y = HISTHEIGHT - 1; y >= HISTHEIGHT - r_height && y >= 0; --y) {
                    uint32_t& pixel = pixelBuffer[y * (pitch / 4) + x];
                    uint8_t r = (pixel >> 0) & 0xFF;
                    uint8_t g = (pixel >> 8) & 0xFF;
                    uint8_t b = (pixel >> 16) & 0xFF;
                    uint8_t a = (pixel >> 24) & 0xFF;

                    // Add red channel (blend with existing color)
                    r = std::min(255, r + 128);

                    // Recompose pixel
                    pixel = (a << 24) | (b << 16) | (g << 8) | r;
                }

                // Green channel
                for (int y = HISTHEIGHT - 1; y >= HISTHEIGHT - g_height && y >= 0; --y) {
                    uint32_t& pixel = pixelBuffer[y * (pitch / 4) + x];
                    uint8_t r = (pixel >> 0) & 0xFF;
                    uint8_t g = (pixel >> 8) & 0xFF;
                    uint8_t b = (pixel >> 16) & 0xFF;
                    uint8_t a = (pixel >> 24) & 0xFF;

                    // Add green channel
                    g = std::min(255, g + 128);

                    // Recompose pixel
                    pixel = (a << 24) | (b << 16) | (g << 8) | r;
                }

                // Blue channel
                for (int y = HISTHEIGHT - 1; y >= HISTHEIGHT - b_height && y >= 0; --y) {
                    uint32_t& pixel = pixelBuffer[y * (pitch / 4) + x];
                    uint8_t r = (pixel >> 0) & 0xFF;
                    uint8_t g = (pixel >> 8) & 0xFF;
                    uint8_t b = (pixel >> 16) & 0xFF;
                    uint8_t a = (pixel >> 24) & 0xFF;

                    // Add blue channel
                    b = std::min(255, b + 128);

                    // Recompose pixel
                    pixel = (a << 24) | (b << 16) | (g << 8) | r;
                }
            }
        }
    }

    SDL_UnlockTexture((SDL_Texture*)img->histTex);
    return;

}
