#ifndef _imageMeta_h
#define _imageMeta_h

#include "nlohmann/json.hpp"

#include <iostream>
#include <string>
#include <iomanip>


struct imageMetadata {
    std::string fileName;
    std::string rollName;
    int frameNumber = 9999;
    std::string cameraMake;
    std::string cameraModel;
    std::string lens;
    std::string filmStock;
    std::string focalLength;
    std::string fNumber;
    std::string exposureTime;
    std::string dateTime;
    std::string location;
    std::string gps;
    std::string notes;
    std::string devProcess;
    std::string chemMfg;
    std::string devNotes;

    std::string scanner;
    std::string scanNotes;

    NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(imageMetadata, fileName, rollName,
        frameNumber, cameraMake, cameraModel, lens, filmStock,
        focalLength, fNumber, exposureTime, dateTime, location,
        gps, notes, devProcess, chemMfg, devNotes, scanner, scanNotes);

    bool operator==(const imageMetadata& other) const {
        return fileName == other.fileName &&
                rollName == other.rollName &&
                frameNumber == other.frameNumber &&
                cameraMake == other.cameraMake &&
                cameraModel == other.cameraModel &&
                lens == other.lens &&
                filmStock == other.filmStock &&
                focalLength == other.focalLength &&
                fNumber == other.fNumber &&
                exposureTime == other.exposureTime &&
                dateTime == other.dateTime &&
                location == other.location &&
                gps == other.gps &&
                notes == other.notes &&
                devProcess == other.devProcess &&
                chemMfg == other.chemMfg &&
                devNotes == other.devNotes &&
                scanner == other.scanner &&
                scanNotes == other.scanNotes;
    }

    bool operator!=(const imageMetadata& other) const {
        return !(*this == other);
    }
};

struct metaBuff {
    int frameNum;

    char rollPath[1024];
    uint8_t a_rollPath;
    uint8_t dif_rollPath;

    char rollname[64];
    uint8_t a_rollname;
    uint8_t dif_rollname;

    char camMake[128];
    uint8_t a_camMake;
    uint8_t dif_camMake;

    char camModel[128];
    uint8_t a_camModel;
    uint8_t dif_camModel;

    char lens[256];
    uint8_t a_lens;
    uint8_t dif_lens;

    char film[128];
    uint8_t a_film;
    uint8_t dif_film;

    char focal[32];
    uint8_t a_focal;
    uint8_t dif_focal;

    char fnum[32];
    uint8_t a_fnum;
    uint8_t dif_fnum;

    char exp[32];
    uint8_t a_exp;
    uint8_t dif_exp;

    char date[128];
    uint8_t a_date;
    uint8_t dif_date;

    char loc[256];
    uint8_t a_loc;
    uint8_t dif_loc;

    char gps[256];
    uint8_t a_gps;
    uint8_t dif_gps;

    char notes[4096];
    uint8_t a_notes;
    uint8_t dif_notes;

    char dev[128];
    uint8_t a_dev;
    uint8_t dif_dev;

    char chem[128];
    uint8_t a_chem;
    uint8_t dif_chem;

    char devnotes[4096];
    uint8_t a_devnotes;
    uint8_t dif_devnotes;

    char scanner[256];
    uint8_t a_scanner;
    uint8_t dif_scanner;

    char scannotes[4096];
    uint8_t a_scannotes;
    uint8_t dif_scannotes;

};



#endif
