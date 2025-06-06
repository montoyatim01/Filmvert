#include "preferences.h"
#include "window.h"
#include <chrono>

//--- Check Metadata ---//
/*
    If there's an state change, update the roll state
    if we've elapsed enough time

    If auto save is enabled, and if we've met the
    minimum duration since the last save, loop
    through all images in all rolls and check
    if a save is needed, if so, save the individual image

    Currently does not save a whole roll JSON
*/
void mainWindow::checkMeta() {
    auto now = std::chrono::steady_clock::now();
    auto dur = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastChange);

    if (dur.count() > 250) {
        // 250ms since the last state change
        // Update the roll
        if (needStateUp) {
            if (validRoll())
                activeRoll()->rollUpState();
            needStateUp = false;
        }
    }

    if (!metaRefresh || !appPrefs.prefs.autoSave)
        return;
    if (dur.count() * 1000 > appPrefs.prefs.autoSFreq) {
        for (int r = 0; r < activeRolls.size(); r++) {
            for (int i = 0; i < activeRolls[r].rollSize(); i++) {
                image* img = getImage(r, i);
                if (!img) {
                   LOG_WARN("Cannot update meta on nullptr");
                }

                if (img->needMetaWrite) {
                    img->writeXMPFile();
                }
            }
        }
        metaRefresh = false;
    }
    return;
}

//--- Save Meta ---//
/*
    Periodically save the state of some of the
    UI elements.

    Currently histogram settings
*/
void mainWindow::saveUI() {
    auto now = std::chrono::steady_clock::now();
    auto dur = std::chrono::duration_cast<std::chrono::seconds>(now - lastUISave);
    if (dur.count() > 10) {
        appPrefs.saveToFile();
        lastUISave = std::chrono::steady_clock::now();
    }
}

//--- Image Metadata Pre-Edit ---//
/*
    Load up the metadata edit struct with the image's
    current metadata fields. This allows non-destructive
    changes that only take effect on a "save" or "apply"
    action.
*/
void mainWindow::imageMetaPreEdit() {
    if (!validIm())
        return;
    metaEdit.frameNum = activeImage()->imgMeta.frameNumber;
    std::strcpy(metaEdit.camMake, activeImage()->imgMeta.cameraMake.c_str());
    std::strcpy(metaEdit.camModel, activeImage()->imgMeta.cameraModel.c_str());
    std::strcpy(metaEdit.lens, activeImage()->imgMeta.lens.c_str());
    std::strcpy(metaEdit.film, activeImage()->imgMeta.filmStock.c_str());
    std::strcpy(metaEdit.focal, activeImage()->imgMeta.focalLength.c_str());
    std::strcpy(metaEdit.fnum, activeImage()->imgMeta.fNumber.c_str());
    std::strcpy(metaEdit.exp, activeImage()->imgMeta.exposureTime.c_str());

    std::strcpy(metaEdit.date, activeImage()->imgMeta.dateTime.c_str());
    std::strcpy(metaEdit.loc, activeImage()->imgMeta.location.c_str());
    std::strcpy(metaEdit.gps, activeImage()->imgMeta.gps.c_str());
    std::strcpy(metaEdit.notes, activeImage()->imgMeta.notes.c_str());
    std::strcpy(metaEdit.dev, activeImage()->imgMeta.devProcess.c_str());
    std::strcpy(metaEdit.chem, activeImage()->imgMeta.chemMfg.c_str());
    std::strcpy(metaEdit.devnotes, activeImage()->imgMeta.devNotes.c_str());
    std::strcpy(metaEdit.scanner, activeImage()->imgMeta.scanner.c_str());
    std::strcpy(metaEdit.scannotes, activeImage()->imgMeta.scanNotes.c_str());
}

//--- Image Metadata Post-Edit ---//
/*
    After the user has clicked "Save" or "Apply"
    Copy the metadata fields with valid changes from
    the struct into the image.
*/
void mainWindow::imageMetaPostEdit() {
    if (!validIm())
        return;
    activeImage()->imgMeta.frameNumber = metaEdit.frameNum;
    if (metaEdit.a_camMake)
        activeImage()->imgMeta.cameraMake = metaEdit.camMake;
    if (metaEdit.a_camModel)
        activeImage()->imgMeta.cameraModel = metaEdit.camModel;
    if (metaEdit.a_lens)
        activeImage()->imgMeta.lens = metaEdit.lens;
    if (metaEdit.a_film)
        activeImage()->imgMeta.filmStock = metaEdit.film;
    if (metaEdit.a_focal)
        activeImage()->imgMeta.focalLength = metaEdit.focal;
    if (metaEdit.a_fnum)
        activeImage()->imgMeta.fNumber = metaEdit.fnum;
    if (metaEdit.a_exp)
        activeImage()->imgMeta.exposureTime = metaEdit.exp;

    if (metaEdit.a_date)
        activeImage()->imgMeta.dateTime = metaEdit.date;
    if (metaEdit.a_loc)
        activeImage()->imgMeta.location = metaEdit.loc;
    if (metaEdit.a_gps)
        activeImage()->imgMeta.gps = metaEdit.gps;
    if (metaEdit.a_notes)
        activeImage()->imgMeta.notes = metaEdit.notes;
    if (metaEdit.a_dev)
        activeImage()->imgMeta.devProcess = metaEdit.dev;
    if (metaEdit.a_chem)
        activeImage()->imgMeta.chemMfg = metaEdit.chem;
    if (metaEdit.a_devnotes)
        activeImage()->imgMeta.devNotes = metaEdit.devnotes;
    if (metaEdit.a_scanner)
        activeImage()->imgMeta.scanner = metaEdit.scanner;
    if (metaEdit.a_scannotes)
        activeImage()->imgMeta.scanNotes = metaEdit.scannotes;

    activeImage()->needMetaWrite = true;

}
