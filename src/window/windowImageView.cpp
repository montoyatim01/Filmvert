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
    ImGui::SetNextWindowSizeConstraints(ImVec2(500,500), ImVec2(winWidth - 128, winHeight - 128));
    ImGui::SetNextWindowPos(ImVec2(0,25), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(winWidth * 0.65,winHeight - (25 + thumbHeight)), ImGuiCond_FirstUseEver);
    ImGuiWindowFlags winFlags = 0;
    winFlags |= ImGuiWindowFlags_NoTitleBar;
    winFlags |= ImGuiWindowFlags_HorizontalScrollbar;
    winFlags |= ImGuiWindowFlags_NoScrollWithMouse;
    winFlags |= ImGuiWindowFlags_NoMove;
    winFlags |= ImGuiWindowFlags_NoCollapse;
    if (unsavedChanges()) winFlags |= ImGuiWindowFlags_UnsavedDocument;

    ImGui::Begin("Image Display", 0, winFlags);
    imageWinSize = ImGui::GetWindowSize();
    if (validIm()) {
        // Pre-calc
        int displayWidth, displayHeight;
        if (activeImage()->imgParam.rotation == 6 || activeImage()->imgParam.rotation == 8) {
            // For 90-degree rotations, swap width and height
            displayWidth = activeImage()->height;
            displayHeight = activeImage()->width;
        } else {
            displayWidth = activeImage()->width;
            displayHeight = activeImage()->height;
        }
        float effecDisp = dispScale;
        if (activeImage()->imageLoaded && !toggleProxy && !activeImage()->reloading) {
        } else {
            displayWidth = (int)((float)displayWidth * appPrefs.prefs.proxyRes);
            displayHeight = (int)((float)displayHeight * appPrefs.prefs.proxyRes);
            effecDisp = dispScale / appPrefs.prefs.proxyRes;
        }
        dispSize = ImVec2(effecDisp * displayWidth,
                        effecDisp * displayHeight);

        cursorPos.x = (ImGui::GetWindowSize().x - dispSize.x) * 0.5f;
        cursorPos.y = (ImGui::GetWindowSize().y - dispSize.y) * 0.5f;
        if (cursorPos.x < 0)
            cursorPos.x = 0;
        if (cursorPos.y < 0)
            cursorPos.y = 0;

        ImGui::SetCursorPos(cursorPos);

        // Store the current position for later reference
        ImVec2 imagePos = ImVec2(
            ImGui::GetWindowPos().x + cursorPos.x - ImGui::GetScrollX(),
            ImGui::GetWindowPos().y + cursorPos.y - ImGui::GetScrollY()
        );

        { // Draw the image based on its rotation
            ImDrawList* draw_list = ImGui::GetWindowDrawList();
            ImVec2 canvas_pos = ImGui::GetCursorScreenPos();

            // Define UV coordinates for each corner based on rotation
            ImVec2 uv0, uv1, uv2, uv3; // top-left, top-right, bottom-right, bottom-left
            switch (activeImage()->imgParam.rotation) {
                case 1: // Normal (0°)
                    uv0 = ImVec2(0,0); uv1 = ImVec2(1,0);
                    uv2 = ImVec2(1,1); uv3 = ImVec2(0,1);
                    break;
                case 6: // 90° clockwise
                    uv0 = ImVec2(0,1); uv1 = ImVec2(0,0);
                    uv2 = ImVec2(1,0); uv3 = ImVec2(1,1);
                    break;
                case 3: // 180°
                    uv0 = ImVec2(1,1); uv1 = ImVec2(0,1);
                    uv2 = ImVec2(0,0); uv3 = ImVec2(1,0);
                    break;
                case 8: // 90° counter-clockwise
                    uv0 = ImVec2(1,0); uv1 = ImVec2(1,1);
                    uv2 = ImVec2(0,1); uv3 = ImVec2(0,0);
                    break;
            }

            // Draw the rotated quad
            if (activeImage()->imageLoaded && !toggleProxy && !activeImage()->reloading && !activeImage()->fullIm)
                draw_list->AddImageQuad(
                    static_cast<ImTextureID>(activeImage()->glTexture),
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

            // Reserve space in ImGui layout
            ImGui::Dummy(dispSize);
        }


        ImGui::SetItemKeyOwner(ImGuiKey_MouseWheelY);
        ImGui::SetItemKeyOwner(ImGuiKey_MouseWheelX);

        scroll.x = ImGui::GetScrollX();
        scroll.y = ImGui::GetScrollY();

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
            cropBoxScreen[i].x = imagePos.x + transformedX * dispScale;
            cropBoxScreen[i].y = imagePos.y + transformedY * dispScale;

        }

        // Draw the crop box lines
        ImDrawList* drawList = ImGui::GetWindowDrawList();
        ImU32 lineColor = IM_COL32(255, 255, 0, 255); // Yellow
        float lineThickness = 2.0f;

        // Draw the lines connecting the corners - connect in order: top-left, top-right, bottom-right, bottom-left
        if (cropDisplay) {
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
            if (cropDisplay) {
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
        if (minMaxDisp) {
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
                    minPoint.x = imagePos.x + (float)minX * dispScale;
                    minPoint.y = imagePos.y + (float)minY * dispScale;
                    ImVec2 maxPoint;
                    maxPoint.x = imagePos.x + (float)maxX * dispScale;
                    maxPoint.y = imagePos.y + (float)maxY * dispScale;
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
                newPosRotated.x = (mousePos.x - imagePos.x) / dispScale;
                newPosRotated.y = (mousePos.y - imagePos.y) / dispScale;

                // Constrain to rotated images boundaries
                int rotatedWidth = (activeImage()->imgParam.rotation == 6 || activeImage()->imgParam.rotation == 8) ?
                                    activeImage()->height : activeImage()->width;
                int rotatedHeight = (activeImage()->imgParam.rotation == 6 || activeImage()->imgParam.rotation == 8) ?
                                    activeImage()->width : activeImage()->height;

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
                newPosRotated.x = (mousePos.x - imagePos.x) / dispScale;
                newPosRotated.y = (mousePos.y - imagePos.y) / dispScale;
                // Constrain to rotated images boundaries
                int rotatedWidth = (activeImage()->imgParam.rotation == 6 || activeImage()->imgParam.rotation == 8) ?
                                    activeImage()->height : activeImage()->width;
                int rotatedHeight = (activeImage()->imgParam.rotation == 6 || activeImage()->imgParam.rotation == 8) ?
                                    activeImage()->width : activeImage()->height;

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
                activeImage()->setMinMax();
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
                newPosRotated.x = (mousePos.x - imagePos.x) / dispScale;
                newPosRotated.y = (mousePos.y - imagePos.y) / dispScale;
                // Constrain to rotated images boundaries
                int rotatedWidth = (activeImage()->imgParam.rotation == 6 || activeImage()->imgParam.rotation == 8) ?
                                    activeImage()->height : activeImage()->width;
                int rotatedHeight = (activeImage()->imgParam.rotation == 6 || activeImage()->imgParam.rotation == 8) ?
                                    activeImage()->width : activeImage()->height;

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
                activeImage()->setMinMax();
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
            mousePosInRotatedImage.x = (mousePos.x - imagePos.x) / dispScale;
            mousePosInRotatedImage.y = (mousePos.y - imagePos.y) / dispScale;

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
            if (ctrlShiftPressed && ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !isSelecting && !dragging) {
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
            if ((isSelecting && ctrlShiftPressed) || sampleVisible) {
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
                selStartScreen.x = imagePos.x + rotStX * dispScale;
                selStartScreen.y = imagePos.y + rotStY * dispScale;
                selEndScreen.x = imagePos.x + rotEdX * dispScale;
                selEndScreen.y = imagePos.y + rotEdY * dispScale;

                // Draw selection rectangle
                ImU32 selectionColor = IM_COL32(0, 255, 255, 128); // Cyan with transparency
                ImU32 selectionBorderColor = IM_COL32(0, 255, 255, 255); // Solid cyan for border

                drawList->AddRectFilled(selStartScreen, selEndScreen, selectionColor);
                drawList->AddRect(selStartScreen, selEndScreen, selectionBorderColor, 0.0f, 0, 2.0f);
            }
        }


        // Display the current selection coordinates if a selection exists
        /*if (sampleX[0] != sampleX[1] || sampleY[0] != sampleY[1]) {
            ImVec2 textPos = ImVec2(10, 10); // Position in top-left of window
            char selText[128];
            sprintf(selText, "Selection: (%.1f, %.1f) to (%.1f, %.1f)",
                    sampleX[0], sampleY[0], sampleX[1], sampleY[1]);
            drawList->AddText(ImGui::GetWindowPos() + textPos, IM_COL32(255, 255, 255, 255), selText);
        }*/

        // Controls for panning and zooming the image
        if (ImGui::IsItemHovered()) {
            ImVec2 mousePosInImage;
            mousePosInImage.x =
                (ImGui::GetIO().MousePos.x - ImGui::GetWindowPos().x -
                 cursorPos.x + ImGui::GetScrollX()) /
                dispScale;
            mousePosInImage.y =
                (ImGui::GetIO().MousePos.y - ImGui::GetWindowPos().y -
                 cursorPos.y + ImGui::GetScrollY()) /
                dispScale;

            // Inside the mouse wheel condition:
            if (ImGui::GetIO().MouseWheel != 0 && (ImGui::GetIO().KeyShift || ImGui::GetIO().KeyAlt || !appPrefs.prefs.trackpadMode)) {
                // Store the mouse position relative to the image before zooming
                float mouseXRatio = mousePosInImage.x / activeImage()->width;
                float mouseYRatio = mousePosInImage.y / activeImage()->height;

                // Adjust scale factor
                float prevScale = dispScale;
                dispScale = dispScale * pow(1.05f, ImGui::GetIO().MouseWheel);

                // Clamp the scale
                if (dispScale < 0.1f) dispScale = 0.1f;
                if (dispScale > 30.0f) dispScale = 30.0f;

                // Calculate new display size
                dispSize = ImVec2(dispScale * activeImage()->width,
                                  dispScale * activeImage()->height);

                // Recalculate cursor position correctly
                cursorPos.x = (ImGui::GetWindowSize().x - dispSize.x) * 0.5f;
                cursorPos.y = (ImGui::GetWindowSize().y - dispSize.y) * 0.5f;

                if (cursorPos.x < 0) cursorPos.x = 0;
                if (cursorPos.y < 0) cursorPos.y = 0;

                // Calculate new scroll position to keep mouse point fixed during zoom
                float newMouseImageX = mouseXRatio * activeImage()->width;
                float newMouseImageY = mouseYRatio * activeImage()->height;

                scroll.x = (newMouseImageX * dispScale) - (ImGui::GetIO().MousePos.x - ImGui::GetWindowPos().x) + cursorPos.x;
                scroll.y = (newMouseImageY * dispScale) - (ImGui::GetIO().MousePos.y - ImGui::GetWindowPos().y) + cursorPos.y;

                ImGui::SetScrollX(scroll.x);
                ImGui::SetScrollY(scroll.y);
                currentlyInteracting = true;
            }

            if ((ImGui::GetIO().MouseWheel != 0 || ImGui::GetIO().MouseWheelH != 0) && !ImGui::GetIO().KeyShift && !ImGui::GetIO().KeyAlt && appPrefs.prefs.trackpadMode) {
                scroll.x = ImGui::GetScrollX() - (ImGui::GetIO().MouseWheelH * 12);
                scroll.y = ImGui::GetScrollY() - (ImGui::GetIO().MouseWheel * 12);
                ImGui::SetScrollX(scroll.x);
                ImGui::SetScrollY(scroll.y);
                currentlyInteracting = true;
            }

            if ((ImGui::IsKeyPressed(ImGuiKey_H) && !ImGui::IsKeyPressed(ImGuiMod_Ctrl)) || firstImage) {
                int iWidth = activeImage()->imgParam.rotation == 6 || activeImage()->imgParam.rotation == 8 ? activeImage()->height : activeImage()->width;
                int iHeight = activeImage()->imgParam.rotation == 6 || activeImage()->imgParam.rotation == 8 ? activeImage()->width : activeImage()->height;
                // Pressed the z key, reset zoom
                float scaleX = ImGui::GetWindowSize().x / (iWidth + ((float)iWidth * 0.01f));
                float scaleY = ImGui::GetWindowSize().y / (iHeight + ((float)iHeight * 0.01f));

                dispScale = scaleX > scaleY ? scaleY : scaleX;
                currentlyInteracting = true;
                firstImage = false;
            }

            // Only allow panning if we're not dragging a corner and not making a selection
            if ((ImGui::IsMouseDragging(ImGuiMouseButton_Left, 0.0f) || ImGui::IsMouseDragging(ImGuiMouseButton_Middle, 0.0f)) &&
                    !dragging && !isSelecting && !minDrag && !maxDrag) {
                scroll.x = ImGui::GetScrollX() - ImGui::GetIO().MouseDelta.x;
                scroll.y = ImGui::GetScrollY() - ImGui::GetIO().MouseDelta.y;
                ImGui::SetScrollX(scroll.x);
                ImGui::SetScrollY(scroll.y);
                currentlyInteracting = true;
            }
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
        ImVec2 textPos = ImVec2(windowPos.x + 5, windowPos.y + 5); // 10px from left, 30px from top

        // Draw the text directly to the draw list so it's always visible
        ImU32 textColor = IM_COL32(255, 0, 0, 255); // Red color
        drawList->AddText(textPos, textColor, "GRADE BYPASS");
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

    // Calculate base color if needed:
    if (calcBaseColor) {
        if (validIm()) {
            activeImage()->processBaseColor();
            activeRoll()->rollUpState();
        }
    }
}
