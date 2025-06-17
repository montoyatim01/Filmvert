#include "window.h"
#include <imgui.h>
#include <string>

// TODO:
// -- Implement application for image EXIF rotation (strip meta)

void mainWindow::windowCrop(ImVec2 &imagePos, bool &dragging, bool &isInteracting, bool &currentlyInteracting) {
    // Variables for rectangular crop handling
    //draggingImageCrop = false;
    static int draggedCropHandle = -1; // 0-3 for corners, 4 for entire rectangle
    static ImVec2 dragStartMouse;
    static float dragStartMinX, dragStartMinY, dragStartMaxX, dragStartMaxY;

    // Only show crop if cropVisible is true
    if (!cropVisible) {
        return;
    }

    // Get actual display dimensions considering rotation
    int displayWidth = activeImage()->width;
    int displayHeight = activeImage()->height;
    if (activeImage()->imgParam.rotation == 6 || activeImage()->imgParam.rotation == 8 ||
        activeImage()->imgParam.rotation == 5 || activeImage()->imgParam.rotation == 7) {
        std::swap(displayWidth, displayHeight);
    }

    // Calculate crop rectangle corners in normalized coordinates (0-1)
    // The crop box is always square and doesn't rotate
    float minX = activeImage()->imgParam.imageCropMinX;
    float minY = activeImage()->imgParam.imageCropMinY;
    float maxX = activeImage()->imgParam.imageCropMaxX;
    float maxY = activeImage()->imgParam.imageCropMaxY;

    // Apply EXIF transformations for display (but crop box stays square)
    ImVec2 exifTransformedCorners[4];
    switch (activeImage()->imgParam.rotation) {
        case 1: // Normal
            exifTransformedCorners[0] = ImVec2(minX, minY); // Top-left
            exifTransformedCorners[1] = ImVec2(maxX, minY); // Top-right
            exifTransformedCorners[2] = ImVec2(maxX, maxY); // Bottom-right
            exifTransformedCorners[3] = ImVec2(minX, maxY); // Bottom-left
            break;
        case 2: // Flip horizontal
            exifTransformedCorners[0] = ImVec2(1.0f - maxX, minY);
            exifTransformedCorners[1] = ImVec2(1.0f - minX, minY);
            exifTransformedCorners[2] = ImVec2(1.0f - minX, maxY);
            exifTransformedCorners[3] = ImVec2(1.0f - maxX, maxY);
            break;
        case 3: // Rotate 180
            exifTransformedCorners[0] = ImVec2(1.0f - maxX, 1.0f - maxY);
            exifTransformedCorners[1] = ImVec2(1.0f - minX, 1.0f - maxY);
            exifTransformedCorners[2] = ImVec2(1.0f - minX, 1.0f - minY);
            exifTransformedCorners[3] = ImVec2(1.0f - maxX, 1.0f - minY);
            break;
        case 4: // Flip vertical
            exifTransformedCorners[0] = ImVec2(minX, 1.0f - maxY);
            exifTransformedCorners[1] = ImVec2(maxX, 1.0f - maxY);
            exifTransformedCorners[2] = ImVec2(maxX, 1.0f - minY);
            exifTransformedCorners[3] = ImVec2(minX, 1.0f - minY);
            break;
        case 5: // Rotate 90 CCW + flip horizontal
            exifTransformedCorners[0] = ImVec2(1.0f - maxY, 1.0f - maxX);
            exifTransformedCorners[1] = ImVec2(1.0f - minY, 1.0f - maxX);
            exifTransformedCorners[2] = ImVec2(1.0f - minY, 1.0f - minX);
            exifTransformedCorners[3] = ImVec2(1.0f - maxY, 1.0f - minX);
            break;
        case 6: // Rotate 90 CW
            exifTransformedCorners[0] = ImVec2(1.0f - maxY, minX);
            exifTransformedCorners[1] = ImVec2(1.0f - minY, minX);
            exifTransformedCorners[2] = ImVec2(1.0f - minY, maxX);
            exifTransformedCorners[3] = ImVec2(1.0f - maxY, maxX);
            break;
        case 7: // Rotate 90 CW + flip horizontal
            exifTransformedCorners[0] = ImVec2(maxY, minX);
            exifTransformedCorners[1] = ImVec2(minY, minX);
            exifTransformedCorners[2] = ImVec2(minY, maxX);
            exifTransformedCorners[3] = ImVec2(maxY, maxX);
            break;
        case 8: // Rotate 90 CCW
            exifTransformedCorners[0] = ImVec2(minY, 1.0f - maxX);
            exifTransformedCorners[1] = ImVec2(maxY, 1.0f - maxX);
            exifTransformedCorners[2] = ImVec2(maxY, 1.0f - minX);
            exifTransformedCorners[3] = ImVec2(minY, 1.0f - minX);
            break;
    }

    // Convert to screen coordinates (no rotation applied to crop box)
    ImVec2 cropCornersScreen[4];
    for (int i = 0; i < 4; i++) {
        cropCornersScreen[i].x = imagePos.x + exifTransformedCorners[i].x * displayWidth * dispScale;
        cropCornersScreen[i].y = imagePos.y + exifTransformedCorners[i].y * displayHeight * dispScale;
    }

    // Draw the crop rectangle
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    ImU32 imageCropLineColor = IM_COL32(0, 255, 255, 255); // Cyan
    ImU32 imageCropFillColor = IM_COL32(0, 255, 255, 30);  // Semi-transparent cyan
    float lineThickness = 2.0f;

    if (!activeImage()->imgParam.cropEnable) {
        // Draw semi-transparent fill as a quad
        drawList->AddQuadFilled(cropCornersScreen[0], cropCornersScreen[1],
                               cropCornersScreen[2], cropCornersScreen[3],
                               imageCropFillColor);

        // Draw rectangle outline
        drawList->AddQuad(cropCornersScreen[0], cropCornersScreen[1],
                         cropCornersScreen[2], cropCornersScreen[3],
                         imageCropLineColor, lineThickness);

        // Draw resize handles on corners
        float handleRadius = 8.0f;
        ImU32 handleColor = IM_COL32(0, 200, 200, 255);
        ImU32 handleHoverColor = IM_COL32(0, 255, 255, 255);

        ImVec2 mousePos = ImGui::GetIO().MousePos;

        for (int i = 0; i < 4; i++) {
            float distSq = (mousePos.x - cropCornersScreen[i].x) * (mousePos.x - cropCornersScreen[i].x) +
                           (mousePos.y - cropCornersScreen[i].y) * (mousePos.y - cropCornersScreen[i].y);
            bool handleHovered = distSq <= (handleRadius * handleRadius);

            drawList->AddCircleFilled(cropCornersScreen[i], handleRadius,
                                     handleHovered ? handleHoverColor : handleColor);

            // Handle corner dragging
            if (handleHovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left) &&
                !draggingImageCrop && !dragging) {
                draggingImageCrop = true;
                draggedCropHandle = i;
                dragStartMouse = mousePos;
                dragStartMinX = activeImage()->imgParam.imageCropMinX;
                dragStartMinY = activeImage()->imgParam.imageCropMinY;
                dragStartMaxX = activeImage()->imgParam.imageCropMaxX;
                dragStartMaxY = activeImage()->imgParam.imageCropMaxY;
                currentlyInteracting = true;
            }
        }

        // Check if mouse is inside crop rectangle for moving entire crop
        bool insideCrop = false;
        {
            // Simple rectangle bounds check (since crop box is always square)
            ImVec2 topLeft = cropCornersScreen[0];
            ImVec2 bottomRight = cropCornersScreen[2];

            // Ensure proper ordering
            if (topLeft.x > bottomRight.x) std::swap(topLeft.x, bottomRight.x);
            if (topLeft.y > bottomRight.y) std::swap(topLeft.y, bottomRight.y);

            insideCrop = (mousePos.x >= topLeft.x && mousePos.x <= bottomRight.x &&
                         mousePos.y >= topLeft.y && mousePos.y <= bottomRight.y);
        }

        if (insideCrop && ImGui::IsMouseClicked(ImGuiMouseButton_Left) &&
            !draggingImageCrop && draggedCropHandle == -1 && !dragging) {
            draggingImageCrop = true;
            draggedCropHandle = 4; // 4 means dragging entire rectangle
            dragStartMouse = mousePos;
            dragStartMinX = activeImage()->imgParam.imageCropMinX;
            dragStartMinY = activeImage()->imgParam.imageCropMinY;
            dragStartMaxX = activeImage()->imgParam.imageCropMaxX;
            dragStartMaxY = activeImage()->imgParam.imageCropMaxY;
            currentlyInteracting = true;
        }

        // Draw crosshair or move cursor when hovering over crop area
        if (insideCrop && !draggingImageCrop && draggedCropHandle == -1) {
            ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
        }
    }

    // Dragging logic
    if (draggingImageCrop && ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
        ImVec2 currentMouse = ImGui::GetIO().MousePos;

        // Calculate delta in screen space
        float screenDeltaX = currentMouse.x - dragStartMouse.x;
        float screenDeltaY = currentMouse.y - dragStartMouse.y;

        // Convert to normalized coordinates
        float normDeltaX = screenDeltaX / (displayWidth * dispScale);
        float normDeltaY = screenDeltaY / (displayHeight * dispScale);

        // Apply inverse EXIF transformation to get delta in original image space
        float deltaX = normDeltaX;
        float deltaY = normDeltaY;

        switch (activeImage()->imgParam.rotation) {
            case 1: // Normal - no change
                break;
            case 2: // Flip horizontal
                deltaX = -normDeltaX;
                break;
            case 3: // Rotate 180
                deltaX = -normDeltaX;
                deltaY = -normDeltaY;
                break;
            case 4: // Flip vertical
                deltaY = -normDeltaY;
                break;
            case 5: // Rotate 90 CCW + flip horizontal
                deltaX = normDeltaY;
                deltaY = normDeltaX;
                break;
            case 6: // Rotate 90 CW
                deltaX = normDeltaY;
                deltaY = -normDeltaX;
                break;
            case 7: // Rotate 90 CW + flip horizontal
                deltaX = -normDeltaY;
                deltaY = -normDeltaX;
                break;
            case 8: // Rotate 90 CCW
                deltaX = -normDeltaY;
                deltaY = normDeltaX;
                break;
        }

        if (draggedCropHandle == 4) {
            // Moving entire rectangle
            float newMinX = dragStartMinX + deltaX;
            float newMinY = dragStartMinY + deltaY;
            float newMaxX = dragStartMaxX + deltaX;
            float newMaxY = dragStartMaxY + deltaY;

            // Constrain to image bounds
            float width = newMaxX - newMinX;
            float height = newMaxY - newMinY;

            if (newMinX < 0) { newMinX = 0; newMaxX = width; }
            if (newMinY < 0) { newMinY = 0; newMaxY = height; }
            if (newMaxX > 1) { newMaxX = 1; newMinX = 1 - width; }
            if (newMaxY > 1) { newMaxY = 1; newMinY = 1 - height; }

            activeImage()->imgParam.imageCropMinX = newMinX;
            activeImage()->imgParam.imageCropMinY = newMinY;
            activeImage()->imgParam.imageCropMaxX = newMaxX;
            activeImage()->imgParam.imageCropMaxY = newMaxY;
        } else {
            // Resizing from corner - map visual corner to logical corner based on EXIF rotation
            float newMinX = dragStartMinX;
            float newMinY = dragStartMinY;
            float newMaxX = dragStartMaxX;
            float newMaxY = dragStartMaxY;

            // Map visual corner index to logical corner based on EXIF rotation
            int logicalCorner = draggedCropHandle;
            switch (activeImage()->imgParam.rotation) {
                case 1: // Normal
                    logicalCorner = draggedCropHandle;
                    break;
                case 2: // Flip horizontal - corners swap horizontally
                    if (draggedCropHandle == 0) logicalCorner = 1;      // Visual TL -> Logical TR
                    else if (draggedCropHandle == 1) logicalCorner = 0; // Visual TR -> Logical TL
                    else if (draggedCropHandle == 2) logicalCorner = 3; // Visual BR -> Logical BL
                    else if (draggedCropHandle == 3) logicalCorner = 2; // Visual BL -> Logical BR
                    break;
                case 3: // Rotate 180 - corners are diagonally opposite
                    logicalCorner = (draggedCropHandle + 2) % 4;
                    break;
                case 4: // Flip vertical - corners swap vertically
                    if (draggedCropHandle == 0) logicalCorner = 3;      // Visual TL -> Logical BL
                    else if (draggedCropHandle == 1) logicalCorner = 2; // Visual TR -> Logical BR
                    else if (draggedCropHandle == 2) logicalCorner = 1; // Visual BR -> Logical TR
                    else if (draggedCropHandle == 3) logicalCorner = 0; // Visual BL -> Logical TL
                    break;
                case 5: // Rotate 90 CCW + flip horizontal
                    if (draggedCropHandle == 0) logicalCorner = 2;      // Visual TL -> Logical BR
                    else if (draggedCropHandle == 1) logicalCorner = 3; // Visual TR -> Logical BL
                    else if (draggedCropHandle == 2) logicalCorner = 0; // Visual BR -> Logical TL
                    else if (draggedCropHandle == 3) logicalCorner = 1; // Visual BL -> Logical TR
                    break;
                case 6: // Rotate 90 CW
                    logicalCorner = (draggedCropHandle + 1) % 4;
                    break;
                case 7: // Rotate 90 CW + flip horizontal
                    if (draggedCropHandle == 0) logicalCorner = 0;      // Visual TL -> Logical TL
                    else if (draggedCropHandle == 1) logicalCorner = 3; // Visual TR -> Logical BL
                    else if (draggedCropHandle == 2) logicalCorner = 2; // Visual BR -> Logical BR
                    else if (draggedCropHandle == 3) logicalCorner = 1; // Visual BL -> Logical TR
                    break;
                case 8: // Rotate 90 CCW
                    logicalCorner = (draggedCropHandle + 3) % 4;
                    break;
            }

            // Apply delta to the correct logical corner in original image coordinates
            switch (logicalCorner) {
                case 0: // Logical top-left in original image
                    newMinX = dragStartMinX + deltaX;
                    newMinY = dragStartMinY + deltaY;
                    break;
                case 1: // Logical top-right in original image
                    newMaxX = dragStartMaxX + deltaX;
                    newMinY = dragStartMinY + deltaY;
                    break;
                case 2: // Logical bottom-right in original image
                    newMaxX = dragStartMaxX + deltaX;
                    newMaxY = dragStartMaxY + deltaY;
                    break;
                case 3: // Logical bottom-left in original image
                    newMinX = dragStartMinX + deltaX;
                    newMaxY = dragStartMaxY + deltaY;
                    break;
            }

            // Ensure min < max and constrain to bounds
            newMinX = ImClamp(newMinX, 0.0f, 1.0f);
            newMinY = ImClamp(newMinY, 0.0f, 1.0f);
            newMaxX = ImClamp(newMaxX, 0.0f, 1.0f);
            newMaxY = ImClamp(newMaxY, 0.0f, 1.0f);

            // Prevent inverting the rectangle
            if (newMinX < newMaxX && newMinY < newMaxY) {
                activeImage()->imgParam.imageCropMinX = newMinX;
                activeImage()->imgParam.imageCropMinY = newMinY;
                activeImage()->imgParam.imageCropMaxX = newMaxX;
                activeImage()->imgParam.imageCropMaxY = newMaxY;
            }
        }

        renderCall = true;
        currentlyInteracting = true;
    } else if (draggingImageCrop) {
        // Mouse released
        draggingImageCrop = false;
        draggedCropHandle = -1;
        activeRoll()->rollUpState();
    }
}
