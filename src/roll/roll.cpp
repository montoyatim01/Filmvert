#include "roll.h"


//--- Selected Image ---//
/*
    Return the pointer to the current
    selected image, otherwise a nullptr
*/
image* filmRoll::selImage() {
    if (validIm())
        return &images[selIm];
    return nullptr;
}

//--- Valid Image ---//
/*
    Return whether the user has selected a valid image
*/
bool filmRoll::validIm() {
    return images.size() > 0 && selIm >= 0 && selIm < images.size();
}


//--- Get Image ---//
/*
    Get image at specified index,
    otherwise nullptr if no valid image
    at that index.
*/
image* filmRoll::getImage(int index) {
    if (index >= 0 && index < images.size()) {
        return &images[index];
    }
    //LOG_WARN("Roll: Index out of range: {}, {}", index, images.size());
    return nullptr;
}

//--- Roll Size ---//
/*
    Return the number of images
    in the roll
*/
int filmRoll::rollSize() {
    return images.size();
}

//--- Select All ---//
/*
    Set selection state to all images
*/
void filmRoll::selectAll() {
    for (int i = 0; i < images.size(); i++) {
        images[i].selected = true;
    }
}

//--- Clear Selection ---//
/*
    Clear all selected image flags
*/
void filmRoll::clearSelection() {
    for (int i = 0; i < images.size(); i++) {
        images[i].selected = false;
    }
}


//--- Unsaved Images ---//
/*
    Determine whether there are any images
    with unsaved changes
*/
bool filmRoll::unsavedImages() const {
    for (int i = 0; i < images.size(); i++) {
        if (images[i].needMetaWrite)
            return true;
    }
    return false;
}

//--- Unsaved Individual ---//
/*
    Deteremine whether there are any images
    that are selected, and have unsaved changes
*/
bool filmRoll::unsavedIndividual() {
    for (int i = 0; i < images.size(); i++) {
        if (images[i].selected && images[i].needMetaWrite)
            return true;
    }
    return false;
}

//--- Sort Roll ---//
/*
    Sort the images in a roll based on
    the metadata index. If the same
    index occurs, sort based on source
    filename.
*/

bool filmRoll::sortRoll() {
    for (int i = 0; i < images.size(); i++) {
        if (images[i].inRndQueue)
            return false;
    }
    std::sort(images.begin(), images.end(),
            [](const image& a, const image& b) {
                if (a.imgMeta.frameNumber == b.imgMeta.frameNumber)
                    return a.srcFilename < b.srcFilename;
                else
                    return a.imgMeta.frameNumber < b.imgMeta.frameNumber;
            });
    return true;
}
