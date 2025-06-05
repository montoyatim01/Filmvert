#include "gpu.h"
#include "utils.h"

//--- Process Image ---//
/*
    Process the histogram based on
    the provided image
 */
void openglGPU::procHistIm(image* img) {
    if (!img) {
        LOG_WARN("Cannot process histogram, bad pointers!");
        return;
    }
    int hWidth = 0;
    int hHeight = 0;
    getHistTexture(img, imgPixels, hWidth, hHeight);
    updateHistPixels(img, imgPixels, histPixels, hWidth, hHeight, appPrefs.prefs.histInt);
    setHistTexture(histPixels);
    return;

}

//--- Calculate Histogram from RGBA ---//
void calculateHistogramFromRGBA(const float *rgba_buffer, int width,
                                int height, HistogramData &histogram,
                                const unsigned int* xPoints, const unsigned int* yPoints) {
    if (!rgba_buffer || !xPoints || !yPoints) {
        return;
    }

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

                // Convert float values (0.0-1.0) to 8-bit values (0-255)
                float r_f = std::clamp(rgba_buffer[idx], 0.0f, 1.0f);
                float g_f = std::clamp(rgba_buffer[idx + 1], 0.0f, 1.0f);
                float b_f = std::clamp(rgba_buffer[idx + 2], 0.0f, 1.0f);

                uint8_t r = static_cast<uint8_t>(r_f * 255.0f + 0.5f);
                uint8_t g = static_cast<uint8_t>(g_f * 255.0f + 0.5f);
                uint8_t b = static_cast<uint8_t>(b_f * 255.0f + 0.5f);

                // For 512 bins, each 8-bit value maps to 2 bins
                // This ensures smooth distribution without gaps
                int r_bin_low = r * 2;
                int r_bin_high = std::min(511, r * 2 + 1);
                int g_bin_low = g * 2;
                int g_bin_high = std::min(511, g * 2 + 1);
                int b_bin_low = b * 2;
                int b_bin_high = std::min(511, b * 2 + 1);

                // Count RGB values in both bins for smooth interpolation
                local_r_hist[r_bin_low]++;
                local_r_hist[r_bin_high]++;
                local_g_hist[g_bin_low]++;
                local_g_hist[g_bin_high]++;
                local_b_hist[b_bin_low]++;
                local_b_hist[b_bin_high]++;

                // Calculate luminance with ITU-R BT.709 standard coefficients
                float luminance_f = 0.2126f * r_f + 0.7152f * g_f + 0.0722f * b_f;
                int luminance = static_cast<int>(luminance_f * 255.0f + 0.5f);

                // Map luminance to two bins
                int lum_bin_low = luminance * 2;
                int lum_bin_high = std::min(511, luminance * 2 + 1);

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

//--- Update Histogram to Float Buffer ---//
void openglGPU::updateHistPixels(image* img, float* imgPixels, float* histPixels, int width, int height, float intensityMultiplier) {

    if (!img || !imgPixels || !histPixels) {
        return;
    }
    // Clamp intensity multiplier to valid range
    intensityMultiplier = std::clamp(intensityMultiplier, 0.0f, 1.0f);

    // Clear to dark gray background (0.06 in float = ~15/255)
    const float bg_color = 0.06f;
    for (int y = 0; y < HISTHEIGHT; ++y) {
        for (int x = 0; x < HISTWIDTH; ++x) {
            int idx = (y * HISTWIDTH + x) * 4;
            histPixels[idx + 0] = bg_color; // R
            histPixels[idx + 1] = bg_color; // G
            histPixels[idx + 2] = bg_color; // B
            histPixels[idx + 3] = 1.0f;     // A
        }
    }

    unsigned int cropBoxX[4];
        unsigned int cropBoxY[4];
        for (int i = 0; i < 4; i++) {
            cropBoxX[i] = img->imgParam.cropBoxX[i] * width;
            cropBoxY[i] = img->imgParam.cropBoxY[i] * height;
        }


    // Calculate histogram
    HistogramData histogram;
    calculateHistogramFromRGBA(imgPixels, width, height,
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

    // Calculate base intensities with multiplier applied (convert to float)
    const float red_intensity = (200.0f / 255.0f) * intensityMultiplier;
    const float green_intensity = (200.0f / 255.0f) * intensityMultiplier;
    const float blue_intensity = (200.0f / 255.0f) * intensityMultiplier;
    const float white_intensity = intensityMultiplier; // 1.0 * multiplier

    // Scale histogram width to match buffer width
    float x_scale = static_cast<float>(HISTWIDTH) / 512.0f;

    // Draw histogram with 512 bins scaled to histogram width
    for (int bin = 0; bin < 512; ++bin) {
        // Map bin to x coordinate(s)
        int x_start = static_cast<int>(bin * x_scale);
        int x_end = static_cast<int>((bin + 1) * x_scale);
        if (x_end >= HISTWIDTH) x_end = HISTWIDTH - 1;

        // Calculate heights with channel-specific scaling
        int r_height = (histogram.r_hist[bin] * (HISTHEIGHT - 10)) / max_r;
        int g_height = (histogram.g_hist[bin] * (HISTHEIGHT - 10)) / max_g;
        int b_height = (histogram.b_hist[bin] * (HISTHEIGHT - 10)) / max_b;
        int lum_height = (histogram.luminance_hist[bin] * (HISTHEIGHT - 10)) / max_lum;

        // Apply minimum visibility threshold
        if (histogram.r_hist[bin] > 0 && r_height < 2) r_height = 2;
        if (histogram.g_hist[bin] > 0 && g_height < 2) g_height = 2;
        if (histogram.b_hist[bin] > 0 && b_height < 2) b_height = 2;
        if (histogram.luminance_hist[bin] > 0 && lum_height < 2) lum_height = 2;

        // Draw for all x coordinates this bin maps to
        for (int x = x_start; x <= x_end; ++x) {
            if (x >= HISTWIDTH) break;

            // Draw luminance histogram in white (lowest layer)
            for (int y = HISTHEIGHT - 1; y >= HISTHEIGHT - lum_height && y >= 0; --y) {
                int idx = (y * HISTWIDTH + x) * 4;
                histPixels[idx + 0] = white_intensity; // R
                histPixels[idx + 1] = white_intensity; // G
                histPixels[idx + 2] = white_intensity; // B
                histPixels[idx + 3] = 1.0f;           // A
            }

            // Draw RGB histograms with proper additive blending
            for (int y = HISTHEIGHT - 1; y >= 0; --y) {
                int idx = (y * HISTWIDTH + x) * 4;

                // Skip if it's already white (luminance)
                if (histPixels[idx + 0] == white_intensity &&
                    histPixels[idx + 1] == white_intensity &&
                    histPixels[idx + 2] == white_intensity) continue;

                // Extract current color components
                float current_r = histPixels[idx + 0];
                float current_g = histPixels[idx + 1];
                float current_b = histPixels[idx + 2];

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
                    if (current_r == bg_color && current_g == bg_color && current_b == bg_color) {
                        // If it's background, start fresh
                        current_r = 0;
                        current_g = 0;
                        current_b = 0;
                    }

                    current_r = std::min(1.0f, current_r + add_r);
                    current_g = std::min(1.0f, current_g + add_g);
                    current_b = std::min(1.0f, current_b + add_b);

                    // Update pixel
                    histPixels[idx + 0] = current_r;
                    histPixels[idx + 1] = current_g;
                    histPixels[idx + 2] = current_b;
                    histPixels[idx + 3] = 1.0f;
                }
            }
        }
    }
}
