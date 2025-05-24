#include "roll.h"


//--- Save All ---//
/*
    Loop through all images and save their XMP
    sidecar files. Also save the full roll JSON.
*/
void filmRoll::saveAll() {
    for (int i = 0; i < images.size(); i++) {
        images[i].writeXMPFile();
    }
    exportRollMetaJSON();
}

//--- Save Selected ---//
/*
    Loop through all images and save their XMP
    if they are currently selected.
*/
void filmRoll::saveSelected() {
    for (int i = 0; i < images.size(); i++){
        if (images[i].selected)
            images[i].writeXMPFile();
    }
}
