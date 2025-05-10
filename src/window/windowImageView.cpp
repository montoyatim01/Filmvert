#include "window.h"
#include "windowUtils.h"

void mainWindow::imageView() {
    bool calcBaseColor = false;
    ImGui::SetNextWindowPos(ImVec2(0,25));
    ImGui::SetNextWindowSize(ImVec2(winWidth * 0.65,winHeight - (25 + 280)));
    ImGui::Begin("Image Display", 0, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysVerticalScrollbar | ImGuiWindowFlags_AlwaysHorizontalScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

    if (validIm()) {
        // Pre-calc
        int displayWidth, displayHeight;
        if (activeImage()->imRot == 6 || activeImage()->imRot == 8) {
            // For 90-degree rotations, swap width and height
            displayWidth = activeImage()->height;
            displayHeight = activeImage()->width;
        } else {
            displayWidth = activeImage()->width;
            displayHeight = activeImage()->height;
        }
        dispSize = ImVec2(dispScale * displayWidth,
                        dispScale * displayHeight);

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

        ImGui::Image(
            reinterpret_cast<ImTextureID>(activeImage()->texture),
            dispSize);

        ImGui::SetItemKeyOwner(ImGuiKey_MouseWheelY);
        ImGui::SetItemKeyOwner(ImGuiKey_MouseWheelX);

        scroll.x = ImGui::GetScrollX();
        scroll.y = ImGui::GetScrollY();

        ImGuiWindow *win = ImGui::GetCurrentWindow();
        bool currentlyInteracting = false;

        // Variables for drag handling
        static int draggedCorner = -1;
        static bool dragging = false;

        // Convert crop box coordinates to screen space, accounting for rotation
        ImVec2 cropBoxScreen[4];
        for (int i = 0; i < 4; i++) {
            int x = activeImage()->imgParam.cropBoxX[i];
            int y = activeImage()->imgParam.cropBoxY[i];

            // Transform coordinates based on rotation
            int transformedX = x;
            int transformedY = y;
            transformCoordinates(transformedX, transformedY, activeImage()->imRot,
                                 activeImage()->width, activeImage()->height);

            // Calculate screen positions with proper scroll offset
            cropBoxScreen[i].x = imagePos.x + transformedX * dispScale;
            cropBoxScreen[i].y = imagePos.y + transformedY * dispScale;


            // Log coordinates for debugging
            //printf("Corner %d: Image coords (%i, %i), Screen coords (%.1f, %.1f)\n",
                   //i, activeImage()->cropBoxX[i], activeImage()->cropBoxY[i],
                   //cropBoxScreen[i].x, cropBoxScreen[i].y);
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
                drawList->AddText(cropBoxScreen[i], IM_COL32(255, 255, 255, 255), cornerText);
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
                if (activeImage()->imgParam.minX != 0 && activeImage()->imgParam.maxY != 0) {
                    // Transform coordinates for minPoint
                    int minX = activeImage()->imgParam.minX;
                    int minY = activeImage()->imgParam.minY;
                    transformCoordinates(minX, minY, activeImage()->imRot,
                                         activeImage()->width, activeImage()->height);

                    // Transform coordinates for maxPoint
                    int maxX = activeImage()->imgParam.maxX;
                    int maxY = activeImage()->imgParam.maxY;
                    transformCoordinates(maxX, maxY, activeImage()->imRot,
                                         activeImage()->width, activeImage()->height);

                    ImVec2 minPoint;
                    minPoint.x = imagePos.x + (float)minX * dispScale;
                    minPoint.y = imagePos.y + (float)minY * dispScale;
                    ImVec2 maxPoint;
                    maxPoint.x = imagePos.x + (float)maxX * dispScale;
                    maxPoint.y = imagePos.y + (float)maxY * dispScale;
                    float pointRadius = 8.0f;
                    ImU32 handleColor = IM_COL32(255, 0, 0, 255);
                    char minText[4] = "Min";
                    char maxText[4] = "Max";
                    drawList->AddText(minPoint, IM_COL32(255, 255, 255, 255), minText);
                    drawList->AddText(maxPoint, IM_COL32(255, 255, 255, 255), maxText);
                    drawList->AddCircleFilled(minPoint, pointRadius, handleColor);
                    drawList->AddCircleFilled(maxPoint, pointRadius, handleColor);
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
                int rotatedWidth = (activeImage()->imRot == 6 || activeImage()->imRot == 8) ?
                                    activeImage()->height : activeImage()->width;
                int rotatedHeight = (activeImage()->imRot == 6 || activeImage()->imRot == 8) ?
                                    activeImage()->width : activeImage()->height;

                newPosRotated.x = ImClamp(newPosRotated.x, 0.0f, (float)rotatedWidth);
                newPosRotated.y = ImClamp(newPosRotated.y, 0.0f, (float)rotatedHeight);

                // Convert from rotated to original image coordinates
                int origX = newPosRotated.x;
                int origY = newPosRotated.y;
                inverseTransformCoordinates(origX, origY, activeImage()->imRot,
                                            activeImage()->width, activeImage()->height);

                // Update the corner position
                activeImage()->imgParam.cropBoxX[draggedCorner] = origX;
                activeImage()->imgParam.cropBoxY[draggedCorner] = origY;

                currentlyInteracting = true;
            } else {
                // Mouse released, stop dragging
                dragging = false;
                draggedCorner = -1;
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
            if (activeImage()->imRot != 1) {
                int origX = mousePosInRotatedImage.x;
                int origY = mousePosInRotatedImage.y;
                inverseTransformCoordinates(origX, origY, activeImage()->imRot,
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
                activeImage()->imgParam.sampleX[0] = selectionStart.x;
                activeImage()->imgParam.sampleY[0] = selectionStart.y;
                activeImage()->imgParam.sampleX[1] = selectionEnd.x;
                activeImage()->imgParam.sampleY[1] = selectionEnd.y;

                currentlyInteracting = true;
            }

            // Update selection while dragging
            if (isSelecting && ctrlShiftPressed && ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
                selectionEnd = mousePosInImage;  // This is in original image coordinates

                // Store in sample arrays (in original image coordinates)
                activeImage()->imgParam.sampleX[1] = selectionEnd.x;
                activeImage()->imgParam.sampleY[1] = selectionEnd.y;

                currentlyInteracting = true;
            }

            // End selection when mouse is released
            if (isSelecting && !ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
                isSelecting = false;

                // Coordinates are already in original image space
                // Final update of sample arrays
                activeImage()->imgParam.sampleX[0] = selectionStart.x;
                activeImage()->imgParam.sampleY[0] = selectionStart.y;
                activeImage()->imgParam.sampleX[1] = selectionEnd.x;
                activeImage()->imgParam.sampleY[1] = selectionEnd.y;

                calcBaseColor = true;
                currentlyInteracting = true;
            }
        }

        // Draw the selection rectangle if actively selecting
        if (validIm()){
            if ((isSelecting && ctrlShiftPressed) || sampleVisible) {
                // Get selection coordinates in original image space
                int stX = activeImage()->imgParam.sampleX[0];
                int stY = activeImage()->imgParam.sampleY[0];
                int edX = activeImage()->imgParam.sampleX[1];
                int edY = activeImage()->imgParam.sampleY[1];

                // Transform to rotated image coordinates
                int rotStX = stX;
                int rotStY = stY;
                transformCoordinates(rotStX, rotStY, activeImage()->imRot,
                                    activeImage()->width, activeImage()->height);

                int rotEdX = edX;
                int rotEdY = edY;
                transformCoordinates(rotEdX, rotEdY, activeImage()->imRot,
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

        // Controls for panning and zooming the image (your existing code)
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
            if (ImGui::GetIO().MouseWheel != 0 && ImGui::GetIO().KeyShift) {
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

            if ((ImGui::GetIO().MouseWheel != 0 || ImGui::GetIO().MouseWheelH != 0) && !ImGui::GetIO().KeyShift) {
                scroll.x = ImGui::GetScrollX() - (ImGui::GetIO().MouseWheelH * 12);
                scroll.y = ImGui::GetScrollY() - (ImGui::GetIO().MouseWheel * 12);
                ImGui::SetScrollX(scroll.x);
                ImGui::SetScrollY(scroll.y);
                currentlyInteracting = true;
            }

            if (ImGui::IsKeyPressed(ImGuiKey_Z)) {
                int iWidth = activeImage()->imRot == 6 || activeImage()->imRot == 8 ? activeImage()->height : activeImage()->width;
                int iHeight = activeImage()->imRot == 6 || activeImage()->imRot == 8 ? activeImage()->width : activeImage()->height;
                // Pressed the z key, reset zoom
                float scaleX = ImGui::GetWindowSize().x / (iWidth + ((float)iWidth * 0.1f));
                float scaleY = ImGui::GetWindowSize().y / (iHeight + ((float)iHeight * 0.1f));

                dispScale = scaleX > scaleY ? scaleY : scaleX;
                currentlyInteracting = true;
            }

            // Only allow panning if we're not dragging a corner and not making a selection
            if (ImGui::IsMouseDragging(ImGuiMouseButton_Left, 0.0f) && !dragging && !isSelecting) {
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
    }
    ImGui::End();

    // Calculate base color if needed:
    if (calcBaseColor) {
        if (validIm()) {
            activeImage()->processBaseColor();
        }
    }
}
