#ifndef _imageMeta_h
#define _imageMeta_h

#include <iostream>
#include <string>
#include <iomanip>
// Metadata Fields
// Roll (inherit/modify internal roll name)
// Frame number (inherit from image order?)
// Camera Make
// Camera Model
// Lens
// Film Stock
// Focal Length
// F number
// Exposure time
// Date/time shot
// Location
// GPS Location
// Notes
// Development Process
// Chemistry Manufacturer
// Development notes

struct imageMetadata {
    std::string fileName;
    std::string rollName;
    int frameNumber;
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

};



#endif
