/**
 * Generate an RGB histogram from an SDL image buffer and store it in another SDL image buffer
 *
 * @param sourceBuffer The source SDL_Surface containing the image to analyze
 * @param histogramWidth Width of the output histogram image (recommended 256 for direct bin mapping)
 * @param histogramHeight Height of the output histogram image
 * @param showChannels Bitflag to control which channels to display (1=R, 2=G, 4=B, 7=All)
 * @return SDL_Surface* The generated histogram as an SDL surface (must be freed by caller)
 */
SDL_Surface* GenerateRGBHistogram(SDL_Surface* sourceBuffer, int histogramWidth, int histogramHeight, uint8_t showChannels) {
    if (!sourceBuffer) {
        return nullptr;
    }

    // Lock the source surface
    if (SDL_MUSTLOCK(sourceBuffer)) {
        SDL_LockSurface(sourceBuffer);
    }

    // Create histogram data arrays (256 bins for each channel)
    const int numBins = 256;
    std::vector<uint32_t> histogramR(numBins, 0);
    std::vector<uint32_t> histogramG(numBins, 0);
    std::vector<uint32_t> histogramB(numBins, 0);

    // Calculate the histogram by counting pixel values
    uint8_t* pixels = static_cast<uint8_t*>(sourceBuffer->pixels);
    int pitch = sourceBuffer->pitch;
    int bpp = sourceBuffer->format->BytesPerPixel;

    uint32_t totalPixels = 0;
    for (int y = 0; y < sourceBuffer->h; y++) {
        for (int x = 0; x < sourceBuffer->w; x++) {
            uint8_t r, g, b;
            uint32_t pixel = *reinterpret_cast<uint32_t*>(&pixels[y * pitch + x * bpp]);
            SDL_GetRGB(pixel, sourceBuffer->format, &r, &g, &b);

            histogramR[r]++;
            histogramG[g]++;
            histogramB[b]++;
            totalPixels++;
        }
    }

    // Unlock the source surface
    if (SDL_MUSTLOCK(sourceBuffer)) {
        SDL_UnlockSurface(sourceBuffer);
    }

    // Find the maximum value in the histograms for normalization
    uint32_t maxValue = 1; // Avoid division by zero
    for (int i = 0; i < numBins; i++) {
        maxValue = std::max(maxValue, histogramR[i]);
        maxValue = std::max(maxValue, histogramG[i]);
        maxValue = std::max(maxValue, histogramB[i]);
    }

    // Create the output histogram surface
    SDL_Surface* histogramSurface = SDL_CreateRGBSurfaceWithFormat(
        0, histogramWidth, histogramHeight, 32, SDL_PIXELFORMAT_RGBA32);

    if (!histogramSurface) {
        return nullptr;
    }

    // Fill with black background (transparent)
    SDL_FillRect(histogramSurface, nullptr, SDL_MapRGBA(histogramSurface->format, 0, 0, 0, 255));

    // Lock the histogram surface
    if (SDL_MUSTLOCK(histogramSurface)) {
        SDL_LockSurface(histogramSurface);
    }

    // Draw the histogram
    uint8_t* histPixels = static_cast<uint8_t*>(histogramSurface->pixels);
    int histPitch = histogramSurface->pitch;

    // Calculate scaling factors
    float xScale = static_cast<float>(numBins) / histogramWidth;
    float yScale = static_cast<float>(histogramHeight) / maxValue;

    // Clear the surface to black with alpha
    for (int y = 0; y < histogramHeight; y++) {
        for (int x = 0; x < histogramWidth; x++) {
            uint32_t* pixel = reinterpret_cast<uint32_t*>(&histPixels[y * histPitch + x * 4]);
            *pixel = SDL_MapRGBA(histogramSurface->format, 0, 0, 0, 255);
        }
    }

    // Draw each channel if enabled
    for (int x = 0; x < histogramWidth; x++) {
        int bin = static_cast<int>(x * xScale);
        if (bin >= numBins) bin = numBins - 1;

        // Calculate heights for each channel
        int heightR = static_cast<int>(histogramR[bin] * yScale);
        int heightG = static_cast<int>(histogramG[bin] * yScale);
        int heightB = static_cast<int>(histogramB[bin] * yScale);

        // Clamp heights
        heightR = std::min(heightR, histogramHeight);
        heightG = std::min(heightG, histogramHeight);
        heightB = std::min(heightB, histogramHeight);

        // Draw the histogram bars (from bottom to top)
        for (int y = 0; y < histogramHeight; y++) {
            int invY = histogramHeight - y - 1; // Invert Y so bars grow from bottom
            uint32_t* pixel = reinterpret_cast<uint32_t*>(&histPixels[invY * histPitch + x * 4]);

            uint8_t r = 0, g = 0, b = 0, a = 255;

            // Add color components if the respective channel is enabled and the bar reaches this height
            if ((showChannels & 1) && y < heightR) r = 255;
            if ((showChannels & 2) && y < heightG) g = 255;
            if ((showChannels & 4) && y < heightB) b = 255;

            // Only draw if at least one channel is active at this position
            if (r > 0 || g > 0 || b > 0) {
                *pixel = SDL_MapRGBA(histogramSurface->format, r, g, b, a);
            }
        }
    }

    // Unlock the histogram surface
    if (SDL_MUSTLOCK(histogramSurface)) {
        SDL_UnlockSurface(histogramSurface);
    }

    return histogramSurface;
}

/**
 * Example usage with ImGui
 */
void DisplayHistogramInImGui(SDL_Surface* sourceImage) {
    // Create histogram surface
    SDL_Surface* histogramSurface = GenerateRGBHistogram(sourceImage, 256, 100, 7); // 7 = show all channels

    if (histogramSurface) {
        // Create texture from the histogram surface
        SDL_Texture* histogramTexture = SDL_CreateTextureFromSurface(renderer, histogramSurface);

        if (histogramTexture) {
            // Get ImGui to display the texture
            ImGui::Begin("RGB Histogram");

            // Get the texture ID from your SDL_Texture wrapper
            ImTextureID texID = YourSDLTextureToImTextureIDFunction(histogramTexture);

            // Display the histogram texture
            ImGui::Image(texID, ImVec2(histogramSurface->w, histogramSurface->h));

            // Add channel toggle buttons
            static uint8_t channels = 7; // All channels by default
            bool redChannel = (channels & 1);
            bool greenChannel = (channels & 2);
            bool blueChannel = (channels & 4);

            if (ImGui::Checkbox("Red Channel", &redChannel)) channels = (redChannel ? channels | 1 : channels & ~1);
            if (ImGui::Checkbox("Green Channel", &greenChannel)) channels = (greenChannel ? channels | 2 : channels & ~2);
            if (ImGui::Checkbox("Blue Channel", &blueChannel)) channels = (blueChannel ? channels | 4 : channels & ~4);

            // If channels changed, regenerate the histogram
            static uint8_t lastChannels = channels;
            if (lastChannels != channels) {
                SDL_DestroyTexture(histogramTexture);
                SDL_FreeSurface(histogramSurface);
                histogramSurface = GenerateRGBHistogram(sourceImage, 256, 100, channels);
                histogramTexture = SDL_CreateTextureFromSurface(renderer, histogramSurface);
                lastChannels = channels;
            }

            ImGui::End();

            // Clean up
            SDL_DestroyTexture(histogramTexture);
        }

        SDL_FreeSurface(histogramSurface);
    }
}
