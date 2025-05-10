#include "window.h"

void mainWindow::createSDLTexture(image* actImage) {

    // For rotations 6 and 8 (90 degrees), we need to swap width and height
    int textureWidth = (actImage->imRot == 6 || actImage->imRot == 8) ? actImage->height : actImage->width;
    int textureHeight = (actImage->imRot == 6 || actImage->imRot == 8) ? actImage->width : actImage->height;

    SDL_Texture* texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STREAMING,
                                                textureWidth, textureHeight);
    if (texture)
        actImage->texture = texture;
    else
        LOG_WARN("Unable to create SDL texture for image: {}", actImage->srcFilename);
}

void mainWindow::updateSDLTexture(image* actImage) {

    if (!actImage)
        return;

    if (actImage->texture == nullptr) {
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
        LOG_WARN("Image needing update has no buffer! {}", actImage->srcFilename);
        actImage->sdlUpdate = false;
        SDL_UnlockTexture((SDL_Texture*)actImage->texture);
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
        actImage->sdlUpdate = false;
        actImage->delDispBuf();
}
