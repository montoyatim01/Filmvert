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
    std::string gpsLocation;
    std::string notes;
    std::string devProcess;
    std::string chemMfg;
    std::string devNotes;
};



#endif
