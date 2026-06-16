#include "preferences.h"
#include "window.h"
#include "windowUtils.h"
#include <imgui.h>
#include <string>

//--- Image View ---//
/*
    The main routine for the image viewer
*/
void mainWindow::imageView() {
    bool calcBaseColor = false;
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(appPrefs.prefs.imageBGColor[0], appPrefs.prefs.imageBGColor[1], appPrefs.prefs.imageBGColor[2], appPrefs.prefs.imageBGColor[3]));
    ImGui::SetNextWindowSizeConstraints(ImVec2(500,500), ImVec2(winWidth - 128, winHeight - 128));
    ImGui::SetNextWindowPos(ImVec2(0, menuHeight), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(winWidth * 0.65,winHeight - (menuHeight + thumbHeight)), ImGuiCond_FirstUseEver);
    ImGuiWindowFlags winFlags = 0;
    winFlags |= ImGuiWindowFlags_NoTitleBar;
    winFlags |= ImGuiWindowFlags_NoScrollbar;
    winFlags |= ImGuiWindowFlags_NoScrollWithMouse;
    winFlags |= ImGuiWindowFlags_NoMove;
    winFlags |= ImGuiWindowFlags_NoCollapse;
    if (unsavedChanges()) winFlags |= ImGuiWindowFlags_UnsavedDocument;

    ImGui::Begin("Image Display", 0, winFlags);
    imageWinSize = ImGui::GetWindowSize();
    if (validIm()) {
        // Calculate base scale to fit image in window
        float baseScale;
        int iWidth = activeImage()->imgParam.rotation == 6 || activeImage()->imgParam.rotation == 8 ||
                    activeImage()->imgParam.rotation == 5 || activeImage()->imgParam.rotation == 7 ? activeImage()->height : activeImage()->width;
        int iHeight = activeImage()->imgParam.rotation == 6 || activeImage()->imgParam.rotation == 8 ||
                    activeImage()->imgParam.rotation == 5 || activeImage()->imgParam.rotation == 7 ? activeImage()->width : activeImage()->height;
        float scaleX = ImGui::GetWindowSize().x / (iWidth + ((float)iWidth * 0.01f));
        float scaleY = ImGui::GetWindowSize().y / (iHeight + ((float)iHeight * 0.01f));
        baseScale = scaleX > scaleY ? scaleY : scaleX;

        // Apply relative zoom on top of base scale
        float actualScale = baseScale * dispScale;

        // Pre-calc
        int displayWidth, displayHeight;
        if (activeImage()->imgParam.rotation == 6 || activeImage()->imgParam.rotation == 8 ||
            activeImage()->imgParam.rotation == 5 || activeImage()->imgParam.rotation == 7) {
            // For 90-degree rotations, swap width and height
            displayWidth = activeImage()->dispH;
            displayHeight = activeImage()->dispW;
        } else {
            displayWidth = activeImage()->dispW;
            displayHeight = activeImage()->dispH;
        }
        float effecDisp = dispScale;
        if (activeImage()->imageLoaded && !toggleProxy && !activeImage()->reloading) {
        } else {
            displayWidth = (int)((float)displayWidth * appPrefs.prefs.proxyRes);
            displayHeight = (int)((float)displayHeight * appPrefs.prefs.proxyRes);
            effecDisp = dispScale / appPrefs.prefs.proxyRes;
        }

        float effectiveScale = actualScale;
        if (activeImage()->imageLoaded && !toggleProxy && !activeImage()->reloading) {
        } else {
            effectiveScale = actualScale / appPrefs.prefs.proxyRes;
        }
        dispSize = ImVec2(effectiveScale * displayWidth, effectiveScale * displayHeight);

        // Cartesian-style viewer: the image center is the origin.
        // panOffset tracks how far the image center is displaced from the window
        // center (screen pixels). dispScale (zoom) is never touched on image change,
        // so switching images preserves both zoom level and pan position.
        static image*  lastCenteredImage = nullptr;
        static int     lastRotation      = -1;
        static ImVec2  panOffset         = {0.0f, 0.0f};
        static bool    lastReloading     = false;

        bool imageChanged    = (activeImage() != lastCenteredImage);
        bool rotationChanged = (!imageChanged && activeImage()->imgParam.rotation != lastRotation);
        // Detect the frame where a reload just completed for the current image.
        // GPU render sets reloading=false and dispW/dispH after imageView() runs,
        // so this fires on the first frame we see the post-render full dimensions.
        bool reloadCompleted = (!imageChanged && lastReloading && !activeImage()->reloading);

        // dispSize is only valid once the proxy (or full image) has been rendered.
        // While an image is still loading its dimensions may be zero, which would
        // set viewOffset to imageWinSize/2 (top-left at window center) and corrupt
        // panOffset for every subsequent image in the roll.
        const bool dispSizeValid = (dispSize.x > 1.0f && dispSize.y > 1.0f);

        if (!viewReady) {
            // Very first image: start centered, no pan offset.
            // Defer until we have real dimensions so the initial centering is correct.
            if (dispSizeValid) {
                panOffset = {0.0f, 0.0f};
                viewOffset.x = (imageWinSize.x - dispSize.x) * 0.5f;
                viewOffset.y = (imageWinSize.y - dispSize.y) * 0.5f;
                viewReady = true;
                lastCenteredImage = activeImage();
                lastRotation      = activeImage()->imgParam.rotation;
            }
        } else if (imageChanged) {
            // Different image selected: keep zoom (dispScale unchanged) and apply
            // the same center-relative pan so the viewer feels continuous.
            // Defer if dispSize isn't valid yet (proxy still loading after a buffer
            // reload). By not updating lastCenteredImage, imageChanged stays true
            // and we retry each frame until real dimensions are available — preventing
            // a stale viewOffset and corrupted panOffset from propagating to every
            // image in the roll.
            if (dispSizeValid) {
                viewOffset.x = (imageWinSize.x - dispSize.x) * 0.5f + panOffset.x;
                viewOffset.y = (imageWinSize.y - dispSize.y) * 0.5f + panOffset.y;
                lastCenteredImage = activeImage();
                lastRotation      = activeImage()->imgParam.rotation;
            }
        } else if (rotationChanged) {
            // Same image, rotation changed: dispSize has swapped axes so the old
            // viewOffset no longer centers the image.  Re-center now and clear the
            // pan offset so the next image-switch starts from a clean baseline.
            panOffset = {0.0f, 0.0f};
            viewOffset.x = (imageWinSize.x - dispSize.x) * 0.5f;
            viewOffset.y = (imageWinSize.y - dispSize.y) * 0.5f;
            lastRotation = activeImage()->imgParam.rotation;
        } else if (reloadCompleted && dispSizeValid) {
            // Reload just finished: GPU has written final dispW/dispH and cleared
            // reloading. Re-anchor viewOffset from the preserved panOffset so any
            // proxy/full discrepancy accumulated during the reload is corrected.
            viewOffset.x = (imageWinSize.x - dispSize.x) * 0.5f + panOffset.x;
            viewOffset.y = (imageWinSize.y - dispSize.y) * 0.5f + panOffset.y;
        }

        // Track reloading state for reload-completion detection next frame.
        lastReloading = activeImage()->reloading;

        // Recompute panOffset every frame so it's always current when an image
        // switch is detected next frame.  Skip the update when dispSize isn't
        // valid yet (proxy still loading): computing panOffset against a zero
        // dispSize would corrupt it with a large spurious offset that then
        // gets applied on the first frame real dimensions arrive.
        // Also skip while reloading — proxy dimensions may differ slightly from
        // the final GPU output due to integer truncation, so we freeze panOffset
        // during the reload and let the reloadCompleted branch re-anchor it cleanly.
        if (dispSizeValid && !activeImage()->reloading) {
            panOffset.x = viewOffset.x - (imageWinSize.x - dispSize.x) * 0.5f;
            panOffset.y = viewOffset.y - (imageWinSize.y - dispSize.y) * 0.5f;
        }

        // viewOffset is the screen-space position of the image top-left inside the window.
        // We keep cursorPos/scroll in sync so calculateVisible() still works.
        cursorPos = viewOffset;
        scroll = {0, 0};

        // Place a full-window Dummy so IsItemHovered() covers the entire viewer
        ImGui::SetCursorPos(ImVec2(0, 0));

        // Image draws at viewOffset; imagePos is the screen-space top-left
        ImVec2 imagePos = ImVec2(
            ImGui::GetWindowPos().x + viewOffset.x,
            ImGui::GetWindowPos().y + viewOffset.y
        );

        { // Draw the image based on its rotation
            ImDrawList* draw_list = ImGui::GetWindowDrawList();
            ImVec2 canvas_pos = imagePos;

            // Define UV coordinates for each corner based on rotation
            ImVec2 uv0, uv1, uv2, uv3; // top-left, top-right, bottom-right, bottom-left
            switch (activeImage()->imgParam.rotation) {
                case 1: // Normal (0°)
                     uv0 = ImVec2(0,0); uv1 = ImVec2(1,0);
                     uv2 = ImVec2(1,1); uv3 = ImVec2(0,1);
                     break;
                 case 2: // Horizontal flip
                     uv0 = ImVec2(1,0); uv1 = ImVec2(0,0);
                     uv2 = ImVec2(0,1); uv3 = ImVec2(1,1);
                     break;
                 case 3: // 180°
                     uv0 = ImVec2(1,1); uv1 = ImVec2(0,1);
                     uv2 = ImVec2(0,0); uv3 = ImVec2(1,0);
                     break;
                 case 4: // Vertical flip
                     uv0 = ImVec2(0,1); uv1 = ImVec2(1,1);
                     uv2 = ImVec2(1,0); uv3 = ImVec2(0,0);
                     break;
                 case 5: // 90° CCW + Horizontal flip
                     uv0 = ImVec2(1,1); uv1 = ImVec2(1,0);
                     uv2 = ImVec2(0,0); uv3 = ImVec2(0,1);
                     break;
                 case 6: // 90° CW
                     uv0 = ImVec2(0,1); uv1 = ImVec2(0,0);
                     uv2 = ImVec2(1,0); uv3 = ImVec2(1,1);
                     break;
                 case 7: // 90° CW + Horizontal flip
                     uv0 = ImVec2(0,0); uv1 = ImVec2(0,1);
                     uv2 = ImVec2(1,1); uv3 = ImVec2(1,0);
                     break;
                 case 8: // 90° CCW
                     uv0 = ImVec2(1,0); uv1 = ImVec2(1,1);
                     uv2 = ImVec2(0,1); uv3 = ImVec2(0,0);
                     break;
            }

            // Draw the rotated quad
            if (activeImage()->imageLoaded && !toggleProxy && !activeImage()->reloading && !activeImage()->fullIm && !isExporting && gpu->dispBufIm() == activeImage())
                draw_list->AddImageQuad(
                    static_cast<ImTextureID>(gpu->dispTex()),
                    canvas_pos, // top-left
                    ImVec2(canvas_pos.x + dispSize.x, canvas_pos.y), // top-right
                    ImVec2(canvas_pos.x + dispSize.x, canvas_pos.y + dispSize.y), // bottom-right
                    ImVec2(canvas_pos.x, canvas_pos.y + dispSize.y), // bottom-left
                    uv0, uv1, uv2, uv3
                );
            else
                draw_list->AddImageQuad(
                    static_cast<ImTextureID>(activeImage()->glTextureSm),
                    canvas_pos, // top-left
                    ImVec2(canvas_pos.x + dispSize.x, canvas_pos.y), // top-right
                    ImVec2(canvas_pos.x + dispSize.x, canvas_pos.y + dispSize.y), // bottom-right
                    ImVec2(canvas_pos.x, canvas_pos.y + dispSize.y), // bottom-left
                    uv0, uv1, uv2, uv3
                );

            // Full-window Dummy for hover/key-ownership (image is drawn by draw list at viewOffset)
            ImGui::Dummy(imageWinSize);
        }


        ImGui::SetItemKeyOwner(ImGuiKey_MouseWheelY);
        ImGui::SetItemKeyOwner(ImGuiKey_MouseWheelX);

        ImGuiWindow *win = ImGui::GetCurrentWindow();
        bool currentlyInteracting = false;

        // Variables for drag handling
        static int draggedCorner = -1;
        static bool dragging = false;
        static bool minDrag = false;
        static bool maxDrag = false;

        // Convert crop box coordinates to screen space, accounting for rotation
        ImVec2 cropBoxScreen[4];
        for (int i = 0; i < 4; i++) {
            int x = activeImage()->imgParam.cropBoxX[i] * activeImage()->width;
            int y = activeImage()->imgParam.cropBoxY[i] * activeImage()->height;

            // Transform coordinates based on rotation
            int transformedX = x;
            int transformedY = y;
            transformCoordinates(transformedX, transformedY, activeImage()->imgParam.rotation,
                                 activeImage()->width, activeImage()->height);

            // Calculate screen positions with proper scroll offset
            cropBoxScreen[i].x = imagePos.x + transformedX * actualScale;
            cropBoxScreen[i].y = imagePos.y + transformedY * actualScale;

        }

        // Draw the crop box lines
        ImDrawList* drawList = ImGui::GetWindowDrawList();
        ImU32 lineColor = IM_COL32(255, 255, 0, 255); // Yellow
        float lineThickness = 2.0f;

        // Draw the lines connecting the corners - connect in order: top-left, top-right, bottom-right, bottom-left
        if (cropDisplay && !cropVisible && !activeImage()->imgParam.cropEnable) {
            drawList->AddLine(cropBoxScreen[0], cropBoxScreen[1], lineColor, lineThickness); // Top line
            drawList->AddLine(cropBoxScreen[1], cropBoxScreen[2], lineColor, lineThickness); // Right line
            drawList->AddLine(cropBoxScreen[2], cropBoxScreen[3], lineColor, lineThickness); // Bottom line
            drawList->AddLine(cropBoxScreen[3], cropBoxScreen[0], lineColor, lineThickness); // Left line
        }


        // Debug drawing to verify corner positions
        char cornerText[32];
        for (int i = 0; i < 4; i++) {
            sprintf(cornerText, "C%d", i);
            if (cropDisplay) {
                //drawList->AddText(cropBoxScreen[i], IM_COL32(255, 255, 255, 255), cornerText);
            }
        }

        // Draw the corner handles
        float handleRadius = 8.0f;
        ImU32 handleColor = IM_COL32(255, 0, 0, 255); // Red
        ImU32 handleHoverColor = IM_COL32(255, 128, 0, 255); // Orange

        for (int i = 0; i < 4; i++) {
            // Check if mouse is hovering over this handle
            ImVec2 mousePos = ImGui::GetIO().MousePos;
            float distSq = (mousePos.x - cropBoxScreen[i].x) * (mousePos.x - cropBoxScreen[i].x) +
                           (mousePos.y - cropBoxScreen[i].y) * (mousePos.y - cropBoxScreen[i].y);
            bool handleHovered = distSq <= (handleRadius * handleRadius);

            // Draw the handle with appropriate color
            if (cropDisplay && !cropVisible && !activeImage()->imgParam.cropEnable) {
                drawList->AddCircleFilled(
                    cropBoxScreen[i],
                    handleRadius,
                    handleHovered ? handleHoverColor : handleColor
                );
            }


            // Handle dragging
            if (handleHovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !dragging) {
                draggedCorner = i;
                dragging = true;
                currentlyInteracting = true;
            }
        }
        // Draw min/max positions if available
        if (minMaxDisp && !cropVisible && !activeImage()->imgParam.cropEnable) {
            if (validIm()){
                if (activeImage()->imgParam.minX > 0.001 && activeImage()->imgParam.maxY > 0.001) {
                    // Transform coordinates for minPoint
                    int minX = activeImage()->imgParam.minX * activeImage()->width;
                    int minY = activeImage()->imgParam.minY * activeImage()->height;
                    transformCoordinates(minX, minY, activeImage()->imgParam.rotation,
                                         activeImage()->width, activeImage()->height);

                    // Transform coordinates for maxPoint
                    int maxX = activeImage()->imgParam.maxX * activeImage()->width;
                    int maxY = activeImage()->imgParam.maxY * activeImage()->height;
                    transformCoordinates(maxX, maxY, activeImage()->imgParam.rotation,
                                         activeImage()->width, activeImage()->height);

                    ImVec2 minPoint;
                    minPoint.x = imagePos.x + (float)minX * actualScale;
                    minPoint.y = imagePos.y + (float)minY * actualScale;
                    ImVec2 maxPoint;
                    maxPoint.x = imagePos.x + (float)maxX * actualScale;
                    maxPoint.y = imagePos.y + (float)maxY * actualScale;
                    float pointRadius = 8.0f;

                    // Check if mouse is hovering
                    ImVec2 mousePos = ImGui::GetIO().MousePos;
                    float minDistSq = (mousePos.x - minPoint.x) * (mousePos.x - minPoint.x) +
                                   (mousePos.y - minPoint.y) * (mousePos.y - minPoint.y);
                    float maxDistSq = (mousePos.x - maxPoint.x) * (mousePos.x - maxPoint.x) +
                                   (mousePos.y - maxPoint.y) * (mousePos.y - maxPoint.y);
                    bool minHovered = minDistSq <= (handleRadius * handleRadius);
                    bool maxHovered = maxDistSq <= (handleRadius * handleRadius);


                    ImU32 handleColor = IM_COL32(255, 0, 0, 255);
                    ImU32 handleHoverColor = IM_COL32(255, 128, 0, 255); // Orange
                    char minText[4] = "Min";
                    char maxText[4] = "Max";
                    drawList->AddText(minPoint, IM_COL32(255, 255, 255, 255), minText);
                    drawList->AddText(maxPoint, IM_COL32(255, 255, 255, 255), maxText);
                    drawList->AddCircleFilled(minPoint, pointRadius, minHovered ? handleHoverColor : handleColor);
                    drawList->AddCircleFilled(maxPoint, pointRadius, maxHovered ? handleHoverColor : handleColor);

                    if (minHovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !minDrag) {
                        minDrag = true;
                        currentlyInteracting = true;
                    }
                    if (maxHovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !maxDrag) {
                        maxDrag = true;
                        currentlyInteracting = true;
                    }
                }
            }
        }

        // If dragging a corner, update its position
        if (dragging && draggedCorner >= 0) {
            if (ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
                // Calculate the new position in screen space
                ImVec2 mousePos = ImGui::GetIO().MousePos;

                // Convert to image space (rotated)
                ImVec2 newPosRotated;
                newPosRotated.x = (mousePos.x - imagePos.x) / actualScale;
                newPosRotated.y = (mousePos.y - imagePos.y) / actualScale;

                // Cases 5-8 all swap the display width/height relative to the original
                bool swapDims = (activeImage()->imgParam.rotation >= 5 && activeImage()->imgParam.rotation <= 8);
                int rotatedWidth  = swapDims ? activeImage()->height : activeImage()->width;
                int rotatedHeight = swapDims ? activeImage()->width  : activeImage()->height;

                newPosRotated.x = ImClamp(newPosRotated.x, 0.0f, (float)rotatedWidth);
                newPosRotated.y = ImClamp(newPosRotated.y, 0.0f, (float)rotatedHeight);

                // Convert from rotated to original image coordinates
                int origX = newPosRotated.x;
                int origY = newPosRotated.y;
                inverseTransformCoordinates(origX, origY, activeImage()->imgParam.rotation,
                                            activeImage()->width, activeImage()->height);

                // Update the corner position
                activeImage()->imgParam.cropBoxX[draggedCorner] = (float)origX / activeImage()->width;
                activeImage()->imgParam.cropBoxY[draggedCorner] = (float)origY / activeImage()->height;

                currentlyInteracting = true;
            } else {
                // Mouse released, stop dragging
                dragging = false;
                draggedCorner = -1;
                activeRoll()->rollUpState();
            }
        }

        // If dragging min
        if (minDrag) {
            if (ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
                // Calculate the new position in screen space
                ImVec2 mousePos = ImGui::GetIO().MousePos;
                // Convert to image space (rotated)
                ImVec2 newPosRotated;
                newPosRotated.x = (mousePos.x - imagePos.x) / actualScale;
                newPosRotated.y = (mousePos.y - imagePos.y) / actualScale;
                bool swapDims = (activeImage()->imgParam.rotation >= 5 && activeImage()->imgParam.rotation <= 8);
                int rotatedWidth  = swapDims ? activeImage()->height : activeImage()->width;
                int rotatedHeight = swapDims ? activeImage()->width  : activeImage()->height;

                newPosRotated.x = ImClamp(newPosRotated.x, 0.0f, (float)rotatedWidth);
                newPosRotated.y = ImClamp(newPosRotated.y, 0.0f, (float)rotatedHeight);
                // Convert from rotated to original image coordinates
                int origX = newPosRotated.x;
                int origY = newPosRotated.y;
                inverseTransformCoordinates(origX, origY, activeImage()->imgParam.rotation,
                                            activeImage()->width, activeImage()->height);
                // Update the corner position
                activeImage()->imgParam.minX = (float)origX / activeImage()->width;
                activeImage()->imgParam.minY = (float)origY / activeImage()->height;

                activeImage()->minSel = true;
                // Does this need to be on it's own thread?
                activeImage()->setMinMax(dispOCIO);
                if (appPrefs.prefs.cmykSliders)
                    activeImage()->imgParam.rgb_to_cmyk();
                // We want to be sure our changes are immediately rendered
                renderCall = true;
            } else {
                minDrag = false;
                activeRoll()->rollUpState();
            }
        }
        // If dragging max
        if (maxDrag) {
            if (ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
                // Calculate the new position in screen space
                ImVec2 mousePos = ImGui::GetIO().MousePos;
                // Convert to image space (rotated)
                ImVec2 newPosRotated;
                newPosRotated.x = (mousePos.x - imagePos.x) / actualScale;
                newPosRotated.y = (mousePos.y - imagePos.y) / actualScale;
                bool swapDims = (activeImage()->imgParam.rotation >= 5 && activeImage()->imgParam.rotation <= 8);
                int rotatedWidth  = swapDims ? activeImage()->height : activeImage()->width;
                int rotatedHeight = swapDims ? activeImage()->width  : activeImage()->height;

                newPosRotated.x = ImClamp(newPosRotated.x, 0.0f, (float)rotatedWidth);
                newPosRotated.y = ImClamp(newPosRotated.y, 0.0f, (float)rotatedHeight);
                // Convert from rotated to original image coordinates
                int origX = newPosRotated.x;
                int origY = newPosRotated.y;
                inverseTransformCoordinates(origX, origY, activeImage()->imgParam.rotation,
                                            activeImage()->width, activeImage()->height);
                // Update the corner position
                activeImage()->imgParam.maxX = (float)origX / activeImage()->width;
                activeImage()->imgParam.maxY = (float)origY / activeImage()->height;

                activeImage()->minSel = false;
                // Does this need to be on it's own thread?
                activeImage()->setMinMax(dispOCIO);
                if (appPrefs.prefs.cmykSliders)
                    activeImage()->imgParam.rgb_to_cmyk();
                // We want to be sure our changes are immediately rendered
                renderCall = true;
            } else {
                maxDrag = false;
                activeRoll()->rollUpState();
            }
        }

        // RECTANGULAR BOX SELECTION IMPLEMENTATION
        // Variables for rectangle selection
        static bool isSelecting = false;
        static ImVec2 selectionStart;
        static ImVec2 selectionEnd;

        // Check if Ctrl+Shift is pressed
        bool ctrlShiftPressed = ImGui::GetIO().KeyCtrl && ImGui::GetIO().KeyShift;

        // Handle rectangle selection
        if (ImGui::IsItemHovered()) {
            // Calculate mouse position in screen coordinates
            ImVec2 mousePos = ImGui::GetIO().MousePos;

            // Convert to rotated image coordinates
            ImVec2 mousePosInRotatedImage;
            mousePosInRotatedImage.x = (mousePos.x - imagePos.x) / actualScale;
            mousePosInRotatedImage.y = (mousePos.y - imagePos.y) / actualScale;

            // Convert from rotated to original image coordinates
            ImVec2 mousePosInImage = mousePosInRotatedImage;
            if (activeImage()->imgParam.rotation != 1) {
                int origX = mousePosInRotatedImage.x;
                int origY = mousePosInRotatedImage.y;
                inverseTransformCoordinates(origX, origY, activeImage()->imgParam.rotation,
                                            activeImage()->width, activeImage()->height);
                mousePosInImage.x = origX;
                mousePosInImage.y = origY;
            }

            // Clamp to original image boundaries
            mousePosInImage.x = ImClamp(mousePosInImage.x, 0.0f, (float)activeImage()->width);
            mousePosInImage.y = ImClamp(mousePosInImage.y, 0.0f, (float)activeImage()->height);

            // Start selection when mouse is clicked while holding Ctrl+Shift
            if (ctrlShiftPressed && ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !isSelecting && !dragging && !cropVisible && !activeImage()->imgParam.cropEnable) {
                isSelecting = true;
                selectionStart = mousePosInImage;  // This is in original image coordinates
                selectionEnd = selectionStart;     // Initialize end position as same as start

                // Store in sample arrays (in original image coordinates)
                activeImage()->imgParam.sampleX[0] = (float)selectionStart.x / activeImage()->width;
                activeImage()->imgParam.sampleY[0] = (float)selectionStart.y / activeImage()->height;
                activeImage()->imgParam.sampleX[1] = (float)selectionEnd.x / activeImage()->width;
                activeImage()->imgParam.sampleY[1] = (float)selectionEnd.y / activeImage()->height;

                currentlyInteracting = true;
            }

            // Update selection while dragging
            if (isSelecting && ctrlShiftPressed && ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
                selectionEnd = mousePosInImage;  // This is in original image coordinates

                // Store in sample arrays (in original image coordinates)
                activeImage()->imgParam.sampleX[1] = (float)selectionEnd.x / activeImage()->width;
                activeImage()->imgParam.sampleY[1] = (float)selectionEnd.y / activeImage()->height;

                currentlyInteracting = true;
            }

            // End selection when mouse is released
            if (isSelecting && !ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
                isSelecting = false;

                // Coordinates are already in original image space
                // Final update of sample arrays
                activeImage()->imgParam.sampleX[0] = (float)selectionStart.x / activeImage()->width;
                activeImage()->imgParam.sampleY[0] = (float)selectionStart.y / activeImage()->height;
                activeImage()->imgParam.sampleX[1] = (float)selectionEnd.x / activeImage()->width;
                activeImage()->imgParam.sampleY[1] = (float)selectionEnd.y / activeImage()->height;

                calcBaseColor = true;
                currentlyInteracting = true;
            }
        }

        // Draw the selection rectangle if actively selecting
        if (validIm()){
            if (((isSelecting && ctrlShiftPressed ) || sampleVisible) && !cropVisible && !activeImage()->imgParam.cropEnable) {
                // Get selection coordinates in original image space
                int stX = activeImage()->imgParam.sampleX[0] * activeImage()->width;
                int stY = activeImage()->imgParam.sampleY[0] * activeImage()->height;
                int edX = activeImage()->imgParam.sampleX[1] * activeImage()->width;
                int edY = activeImage()->imgParam.sampleY[1] * activeImage()->height;

                // Transform to rotated image coordinates
                int rotStX = stX;
                int rotStY = stY;
                transformCoordinates(rotStX, rotStY, activeImage()->imgParam.rotation,
                                    activeImage()->width, activeImage()->height);

                int rotEdX = edX;
                int rotEdY = edY;
                transformCoordinates(rotEdX, rotEdY, activeImage()->imgParam.rotation,
                                    activeImage()->width, activeImage()->height);

                // Convert to screen coordinates
                ImVec2 selStartScreen, selEndScreen;
                selStartScreen.x = imagePos.x + rotStX * actualScale;
                selStartScreen.y = imagePos.y + rotStY * actualScale;
                selEndScreen.x = imagePos.x + rotEdX * actualScale;
                selEndScreen.y = imagePos.y + rotEdY * actualScale;

                // Draw selection rectangle
                ImU32 selectionColor = IM_COL32(0, 255, 255, 128); // Cyan with transparency
                ImU32 selectionBorderColor = IM_COL32(0, 255, 255, 255); // Solid cyan for border

                drawList->AddRectFilled(selStartScreen, selEndScreen, selectionColor);
                drawList->AddRect(selStartScreen, selEndScreen, selectionBorderColor, 0.0f, 0, 2.0f);
            }
        }

        // Handle the cropping
        windowCrop(imagePos, dragging, isInteracting, currentlyInteracting, actualScale);


        // Controls for panning and zooming the image
        if (ImGui::IsItemHovered()) {
            // Mouse position in image space.
            // viewOffset is where the image top-left sits in window space.
            // mousePosInImage = (winMouse - viewOffset) / dispScale = imageCoord * baseScale
            ImVec2 mousePosInImage;
            mousePosInImage.x = (ImGui::GetIO().MousePos.x - ImGui::GetWindowPos().x - viewOffset.x) / dispScale;
            mousePosInImage.y = (ImGui::GetIO().MousePos.y - ImGui::GetWindowPos().y - viewOffset.y) / dispScale;

            if (ImGui::GetIO().MouseWheel != 0 && (ImGui::GetIO().KeyShift || ImGui::GetIO().KeyAlt || !appPrefs.prefs.trackpadMode)) {
                dispScale = dispScale * pow(1.05f, ImGui::GetIO().MouseWheel);

                // Clamp the relative scale
                if (dispScale < 0.1f) dispScale = 0.1f;
                if (dispScale > 30.0f) dispScale = 30.0f;

                // Recalculate actual scale
                actualScale = baseScale * dispScale;
                float newEffectiveScale = (activeImage()->imageLoaded && !toggleProxy && !activeImage()->reloading)
                                          ? actualScale
                                          : actualScale / appPrefs.prefs.proxyRes;
                dispSize = ImVec2(newEffectiveScale * displayWidth, newEffectiveScale * displayHeight);

                float winMouseX = ImGui::GetIO().MousePos.x - ImGui::GetWindowPos().x;
                float winMouseY = ImGui::GetIO().MousePos.y - ImGui::GetWindowPos().y;

                // Keep the image point under the cursor fixed:
                //   viewOffset_new = winMouse - mousePosInImage * dispScale_new
                //                  = winMouse - imageCoord * effectiveScale_new  ✓
                // No clamping — the image is allowed to extend beyond window edges.
                viewOffset.x = winMouseX - mousePosInImage.x * dispScale;
                viewOffset.y = winMouseY - mousePosInImage.y * dispScale;

                currentlyInteracting = true;
            }

            if ((ImGui::GetIO().MouseWheel != 0 || ImGui::GetIO().MouseWheelH != 0) && !ImGui::GetIO().KeyShift && !ImGui::GetIO().KeyAlt && appPrefs.prefs.trackpadMode) {
                viewOffset.x += ImGui::GetIO().MouseWheelH * 12;
                viewOffset.y += ImGui::GetIO().MouseWheel * 12;
                currentlyInteracting = true;
            }

            if ((ImGui::IsKeyPressed(ImGuiKey_H) && !ImGui::IsKeyPressed(ImGuiMod_Ctrl)) || firstImage) {
                dispScale = 0.98f;
                actualScale = baseScale * dispScale;
                // Recompute dispSize for the reset scale so centering is accurate
                float resetEffScale = (activeImage()->imageLoaded && !toggleProxy && !activeImage()->reloading)
                                      ? actualScale
                                      : actualScale / appPrefs.prefs.proxyRes;
                ImVec2 resetDispSize = ImVec2(resetEffScale * displayWidth, resetEffScale * displayHeight);
                viewOffset.x = (imageWinSize.x - resetDispSize.x) * 0.5f;
                viewOffset.y = (imageWinSize.y - resetDispSize.y) * 0.5f;
                currentlyInteracting = true;
                firstImage = false;
            }

            // Panning: no clamping, image can extend beyond window edges
            if ((ImGui::IsMouseDragging(ImGuiMouseButton_Left, 0.0f) || ImGui::IsMouseDragging(ImGuiMouseButton_Middle, 0.0f)) &&
                    !dragging && !isSelecting && !minDrag && !maxDrag && !draggingImageCrop) {
                viewOffset.x += ImGui::GetIO().MouseDelta.x;
                viewOffset.y += ImGui::GetIO().MouseDelta.y;
                currentlyInteracting = true;
            }
        }

        // Pixel value inspector — shown as a tooltip when Alt is held and the
        // cursor is over the image. RGB values are read directly from the floating-point processed texture
        // (glTexture / glTextureSm) via a lightweight read-only FBO, bypassing
        // the display framebuffer entirely.  The display framebuffer is GL_RGBA8
        // (unsigned normalised), which clamps anything outside [0, 1] before
        if (altHeld && ImGui::IsItemHovered() && validIm()) {
            ImVec2 mousePos = ImGui::GetIO().MousePos;

            // When a crop is active the GPU renders only the cropped region into
            // dispTex (dimensions = crop pixel size, not full image size).  The
            // EXIF rotation is NOT applied in the shader — it is handled solely
            // by AddImageQuad's UV mapping.  So dispTex is always in original
            // (pre-rotation) image space, cropped.  We therefore need to:
            //   1. Use crop dimensions for the rotation inverse-transform so that
            //      the display-space coords map into the crop's coordinate range.
            //   2. Clamp to crop dimensions.
            //   3. Add the crop origin when displaying X/Y so we report the
            //      full-image pixel position rather than a crop-relative one.
            bool cropActive = activeImage()->imgParam.cropEnable;
            int cropOriginX = 0, cropOriginY = 0;
            int effW = activeImage()->width;   // effective image width  (full or crop)
            int effH = activeImage()->height;  // effective image height (full or crop)

            if (cropActive) {
                cropOriginX = (int)(activeImage()->imgParam.imageCropMinX * activeImage()->width);
                cropOriginY = (int)(activeImage()->imgParam.imageCropMinY * activeImage()->height);
                effW = std::max(1, (int)((activeImage()->imgParam.imageCropMaxX -
                                          activeImage()->imgParam.imageCropMinX) * activeImage()->width));
                effH = std::max(1, (int)((activeImage()->imgParam.imageCropMaxY -
                                          activeImage()->imgParam.imageCropMinY) * activeImage()->height));
            }

            // Screen → rotated-image-space → original pixel coordinates
            // actualScale is always calibrated to the full image, so dividing by
            // it yields coordinates in original-image pixels regardless of whether
            // a proxy or the full texture is active.  When crop is active the
            // result is crop-relative (0-based from the crop top-left corner).
            float rotX = (mousePos.x - imagePos.x) / actualScale;
            float rotY = (mousePos.y - imagePos.y) / actualScale;

            int origX = (int)rotX;
            int origY = (int)rotY;
            if (activeImage()->imgParam.rotation != 1) {
                // Use effW/effH (crop or full image) so the inverse maps into
                // the correct [0, effW-1] × [0, effH-1] range.
                inverseTransformCoordinates(origX, origY,
                    activeImage()->imgParam.rotation,
                    effW, effH);
            }
            origX = std::clamp(origX, 0, effW - 1);
            origY = std::clamp(origY, 0, effH - 1);

            // Full-image pixel coordinates for the X/Y readout.
            int displayX = origX + cropOriginX;
            int displayY = origY + cropOriginY;

            // Mirror the texture selection logic used by the draw call
            bool useFullTex = activeImage()->imageLoaded && !toggleProxy &&
                              !activeImage()->reloading   && !activeImage()->fullIm;
            GLuint texHandle = useFullTex
                ? (GLuint)(uintptr_t)gpu->dispTex()
                : (GLuint)(uintptr_t)activeImage()->glTextureSm;

            // Map coordinates to texture pixel coordinates.
            // dispTex / glTextureSm both span only the crop region (effW × effH
            // at full res, scaled by proxyRes for the proxy).  origX/origY are
            // already crop-relative, so they index directly into the texture.
            int texX, texY;
            if (useFullTex) {
                texX = origX;
                texY = origY;
            } else {
                // dispW/dispH reflect the proxy-scaled crop (or full image) size.
                texX = (int)((float)origX / (float)effW * (float)activeImage()->dispW);
                texY = (int)((float)origY / (float)effH * (float)activeImage()->dispH);
                texX = std::clamp(texX, 0, (int)activeImage()->dispW - 1);
                texY = std::clamp(texY, 0, (int)activeImage()->dispH - 1);
            }

            // One-time FBO creation; reused for every subsequent hover frame
            static GLuint inspectorFBO = 0;
            if (!inspectorFBO)
                glGenFramebuffers(1, &inspectorFBO);
            float pixel[3] = {0.0f, 0.0f, 0.0f};
            int pixel8bit[3] = {0, 0, 0};
            int pixel10bit[3] = {0, 0, 0};
            glBindFramebuffer(GL_READ_FRAMEBUFFER, inspectorFBO);
            glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                                   GL_TEXTURE_2D, texHandle, 0);
            glReadBuffer(GL_COLOR_ATTACHMENT0);
            if (glCheckFramebufferStatus(GL_READ_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE)
                glReadPixels(texX, texY, 1, 1, GL_RGB, GL_FLOAT, pixel);
            glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
            glReadBuffer(GL_BACK); // restore default read target

            ImGui::BeginTooltip();
            ImGui::Text("X: %d  Y: %d", displayX, displayY);
            ImGui::Separator();
            switch (appPrefs.prefs.pixelScale) {
                case 0: // 8-bit
                    pixel8bit[0] = std::clamp((int)(pixel[0] * 255.0), -255, 255);
                    pixel8bit[1] = std::clamp((int)(pixel[1] * 255.0), -255, 255);
                    pixel8bit[2] = std::clamp((int)(pixel[2] * 255.0), -255, 255);
                    switch (channelView) {
                        case 0: // RGB
                            ImGui::TextColored(ImVec4(1.0f, 0.45f, 0.45f, 1.0f), "R  %i", pixel8bit[0]);
                            ImGui::TextColored(ImVec4(0.45f, 1.0f, 0.45f, 1.0f), "G  %i", pixel8bit[1]);
                            ImGui::TextColored(ImVec4(0.45f, 0.65f, 1.0f,  1.0f), "B  %i", pixel8bit[2]);
                            break;
                        case 1: // R
                            ImGui::TextColored(ImVec4(1.0f, 0.45f, 0.45f, 1.0f), "R  %i", pixel8bit[0]);
                            break;
                        case 2: // G
                            ImGui::TextColored(ImVec4(0.45f, 1.0f, 0.45f, 1.0f), "G  %i", pixel8bit[1]);
                            break;
                        case 3: // B
                            ImGui::TextColored(ImVec4(0.45f, 0.65f, 1.0f,  1.0f), "B  %i", pixel8bit[2]);
                            break;
                    }
                    break;
                case 1: // 10-bit
                    pixel10bit[0] = std::clamp((int)(pixel[0] * 1024.0), -1024, 1024);
                    pixel10bit[1] = std::clamp((int)(pixel[1] * 1024.0), -1024, 1024);
                    pixel10bit[2] = std::clamp((int)(pixel[2] * 1024.0), -1024, 1024);
                    switch (channelView) {
                        case 0: // RGB
                            ImGui::TextColored(ImVec4(1.0f, 0.45f, 0.45f, 1.0f), "R  %i", pixel10bit[0]);
                            ImGui::TextColored(ImVec4(0.45f, 1.0f, 0.45f, 1.0f), "G  %i", pixel10bit[1]);
                            ImGui::TextColored(ImVec4(0.45f, 0.65f, 1.0f,  1.0f), "B  %i", pixel10bit[2]);
                            break;
                        case 1: // R
                            ImGui::TextColored(ImVec4(1.0f, 0.45f, 0.45f, 1.0f), "R  %i", pixel10bit[0]);
                            break;
                        case 2: // G
                            ImGui::TextColored(ImVec4(0.45f, 1.0f, 0.45f, 1.0f), "G  %i", pixel10bit[1]);
                            break;
                        case 3: // B
                            ImGui::TextColored(ImVec4(0.45f, 0.65f, 1.0f,  1.0f), "B  %i", pixel10bit[2]);
                            break;
                    }
                    break;
                case 2: // Float
                    switch (channelView) {
                        case 0: // RGB
                            ImGui::TextColored(ImVec4(1.0f, 0.45f, 0.45f, 1.0f), "R  %.4f", pixel[0]);
                            ImGui::TextColored(ImVec4(0.45f, 1.0f, 0.45f, 1.0f), "G  %.4f", pixel[1]);
                            ImGui::TextColored(ImVec4(0.45f, 0.65f, 1.0f,  1.0f), "B  %.4f", pixel[2]);
                            break;
                        case 1: // R
                            ImGui::TextColored(ImVec4(1.0f, 0.45f, 0.45f, 1.0f), "R  %.4f", pixel[0]);
                            break;
                        case 2: // G
                            ImGui::TextColored(ImVec4(0.45f, 1.0f, 0.45f, 1.0f), "G  %.4f", pixel[1]);
                            break;
                        case 3: // B
                            ImGui::TextColored(ImVec4(0.45f, 0.65f, 1.0f,  1.0f), "B  %.4f", pixel[2]);
                            break;
                    }
                    break;
            }

            ImGui::EndTooltip();
        }

        if (currentlyInteracting) {
            interactionTimer = 0.0f;
            isInteracting = true;
        } else if (isInteracting) {
            interactionTimer += ImGui::GetIO().DeltaTime;
            if (interactionTimer > INTERACTION_TIMEOUT) {
                renderCall = true;
                isInteracting = false;
                interactCall = true;
            }
        }
        calculateVisible();
    } // If ValidIm
    if (gradeBypass) {
        // Get the current window and draw list
        ImGuiWindow* window = ImGui::GetCurrentWindow();
        ImDrawList* drawList = ImGui::GetWindowDrawList();

        // Calculate position relative to the visible window area
        ImVec2 windowPos = ImGui::GetWindowPos();
        const float uiScale = ImGui::GetFontSize() / 18.0f;
        ImVec2 textPos = ImVec2(windowPos.x + 5.0f * uiScale, windowPos.y + 5.0f * uiScale);

        // Draw the text directly to the draw list so it's always visible
        ImU32 textColor = IM_COL32(255, 0, 0, 255); // Red color
        drawList->AddText(textPos, textColor, "GRADE BYPASS");
    }
    if (channelView != 0) {
        ImGuiWindow* window = ImGui::GetCurrentWindow();
        ImDrawList* drawList = ImGui::GetWindowDrawList();

        ImVec2 windowPos = ImGui::GetWindowPos();
        const float uiScale = ImGui::GetFontSize() / 18.0f;
        float offset = gradeBypass ? 25.0f * uiScale : 5.0f * uiScale;
        ImVec2 textPos = ImVec2(windowPos.x + 5.0f * uiScale, windowPos.y + offset);

        // Draw the text directly to the draw list so it's always visible
        ImU32 textColor = IM_COL32(255, 0, 0, 255); // Red color
        switch (channelView) {
            case 1:
                drawList->AddText(textPos, textColor, "RED CHANNEL");
                break;
            case 2:
                drawList->AddText(textPos, textColor, "GREEN CHANNEL");
                break;
            case 3:
                drawList->AddText(textPos, textColor, "BLUE CHANNEL");
                break;
            default:
                break;
        }

    }
    if (appPrefs.prefs.showStats) {
        static uint64_t statsCachedFrame = UINT64_MAX;
        static image* lastImage = nullptr;
        ImDrawList* drawList = ImGui::GetWindowDrawList();
        ImVec2 windowSize = ImGui::GetWindowSize();
        ImVec2 windowPos  = ImGui::GetWindowPos();
        ImU32  textColor  = IM_COL32(255, 255, 255, 200);

        const float uiScale  = ImGui::GetFontSize() / 18.0f;
        const float fontSize = 13.0f * uiScale;
        const float padding  = 6.0f  * uiScale;
        const float minGap   = 14.0f * uiScale;
        ImFont* font = ImGui::GetFont();

        // Derive line height from the font at the target size
        float lineH = font->CalcTextSizeA(fontSize, FLT_MAX, 0.0f, "Ag").y * 1.3f;

        // Static labels — never change, no need to recompute
        static const char* labels[] = {
            "FPS:",
            "Raw Res:",
            "Display Res:",
            "Image RAM:",

            "Images in Roll:",
            "Roll RAM:",

            "Total Rolls:",
            "Total RAM:",
            "Total VRAM:"
        };
        const int lineCount = 9;

        static std::string vals[9];
        static float labelW[9] = {};
        static float valW[9]   = {};
        static float maxLabelW = 0.0f;
        static float maxValW   = 0.0f;
        static float panelW    = 0.0f;

        uint64_t currentFrame = ImGui::GetFrameCount();
        image* curImage = validIm() ? activeImage() : nullptr;
        bool needsUpdate = (currentFrame - statsCachedFrame) >= 30
                        || curImage != lastImage;

        if (needsUpdate && validRoll() && validIm()) {
            statsCachedFrame = currentFrame;

            uint64_t totalRam = 0;
            uint64_t totalVram = 0;
            for (auto& roll : activeRolls) {
                totalRam += roll.rollRamUsage();
                totalVram += roll.rollVramUsage();
            }


            // vals[0] = FPS — updated every frame below
            vals[1] = fmt::format("{}x{}", activeImage()->rawWidth,  activeImage()->rawHeight);
            vals[2] = fmt::format("{}x{}", activeImage()->width,     activeImage()->height);
            vals[3] = byteFormat(activeImage()->ramUsage());
            vals[4] = fmt::format("{}", activeRollSize());
            vals[5] = byteFormat(activeRoll()->rollRamUsage());
            vals[6] = fmt::format("{}", activeRolls.size());
            vals[7] = byteFormat(totalRam);
            vals[8] = byteFormat(totalVram + gpu->activeBytes());


            // Measure everything at the actual render font size
            maxLabelW = 0.0f;
            maxValW   = 0.0f;
            for (int i = 0; i < lineCount; i++) {
                labelW[i] = font->CalcTextSizeA(fontSize, FLT_MAX, 0.0f, labels[i]).x;
                valW[i]   = font->CalcTextSizeA(fontSize, FLT_MAX, 0.0f, vals[i].c_str()).x;
                maxLabelW = std::max(maxLabelW, labelW[i]);
                maxValW   = std::max(maxValW,   valW[i]);
            }
            panelW = padding + maxLabelW + minGap + maxValW + padding;
        }

        // FPS updates every frame
        if (validIm()) {
            vals[0]  = fmt::format("{:.1f}", fps);
            valW[0]  = font->CalcTextSizeA(fontSize, FLT_MAX, 0.0f, vals[0].c_str()).x;
            maxValW  = std::max(maxValW, valW[0]);
            panelW   = padding + maxLabelW + minGap + maxValW + padding;
        }

        if (validIm() && panelW > 0.0f) {
            float panelH = lineH * lineCount + padding * 2.0f;

            // Top-right corner in screen coordinates
            float rightEdge = windowPos.x + windowSize.x;
            float topEdge   = windowPos.y;
            ImVec2 rectMin  = ImVec2(rightEdge - panelW, topEdge);
            ImVec2 rectMax  = ImVec2(rightEdge,           topEdge + panelH);

            drawList->AddRectFilled(rectMin, rectMax, IM_COL32(0, 0, 0, 160), 4.0f);

            float lx  = rectMin.x + padding;      // label left edge
            float rx  = rectMax.x - padding;      // value right edge (right-justify from here)
            float ty  = rectMin.y + padding;

            for (int i = 0; i < lineCount; i++) {
                float y = ty + lineH * (float)i;
                drawList->AddText(font, fontSize, ImVec2(lx,            y), textColor, labels[i]);
                drawList->AddText(font, fontSize, ImVec2(rx - valW[i],  y), textColor, vals[i].c_str());
            }
        }
    }
    if (ratingSet) {
        if (validIm()) {
            // Get the current window and draw list
            ImGuiWindow* window = ImGui::GetCurrentWindow();
            ImDrawList* drawList = ImGui::GetWindowDrawList();
            ImVec2 curWinSize = ImGui::GetWindowSize();
            std::string rateMsg = "Image Rating ";
            rateMsg += std::to_string(activeImage()->imgMeta.rating);
            // Calculate position relative to the visible window area
            ImVec2 windowPos = ImGui::GetWindowPos();
            ImGui::PushID("Rating");
            ImGui::SetWindowFontScale(2.0f);
            ImVec2 txtSize = ImGui::CalcTextSize(rateMsg.c_str());
            float textPosX = (curWinSize.x / 2) - (txtSize.x / 2);
            float textPosY = (curWinSize.y - txtSize.y);

            ImVec2 textPos = ImVec2(windowPos.x + textPosX - 10, windowPos.y + textPosY - 20); // 10px from left, 30px from top
            int txtAlpha = ((float)ratingFrameCount / 60.0) * 255.0f;
            txtAlpha = ratingFrameCount > 59 ? 255 : txtAlpha;
            // Draw the text directly to the draw list so it's always visible
            ImU32 textColor = IM_COL32(5, 112, 242, txtAlpha); // Red color
            drawList->AddText(textPos, textColor, rateMsg.c_str());
            ratingFrameCount--;
            ratingSet = ratingFrameCount == 0 ? false : true;
            ImGui::SetWindowFontScale(1.0f);
            ImGui::PopID();
        }

    }
    ImGui::End();
    ImGui::PopStyleColor();

    // Calculate base color if needed:
    if (calcBaseColor) {
        if (validIm()) {
            activeImage()->processBaseColor();
            activeRoll()->rollUpState();
        }
    }
}
