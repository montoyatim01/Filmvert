#include "image.h"
#include "imageMeta.h"
#include "exifUtils.h"
#include "exiv2/xmp_exiv2.hpp"
#include "imageParams.h"
#include "logger.h"
#include "nlohmann/json.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <ostream>

// Metadata workflow:
// Import:
// - Read in all exif/xmp data from image
// - Read in xmp data from file, if not available from image
// - If parameters available from xmp, load parameters
// - Save all metadata alongside image struct (xmp + exif)
// - When user makes changes, periodically update the xmp sidecar file


//--- Read Metadata From File---//
/*
    Attempt to open the image file and read the EXIF and XMP
    data from it. Also attempt to open the XMP Sidecar file.

    Load in all available fv metadata from EXIF, then XMP (internal)
    then from XMP (sidecar).
*/
void image::readMetaFromFile() {
    try {
        Exiv2::enableBMFF();
        auto image = Exiv2::ImageFactory::open(fullPath);
        if (!image) {
            throw Exiv2::Error(Exiv2::ErrorCode::kerErrorMessage, "Could not open the image for metadata reading");
        }

        image->readMetadata();

        // Read EXIF Data into Image
        exifData = image->exifData();

        // Read XMP Data into Image
        intxmpData = image->xmpData();

    } catch (const Exiv2::Error& e) {
        LOG_ERROR("Exiv2 image exception: {}", e.what());
    }

    try {
        // Read sidecar XMP Data into Image
        std::filesystem::path imPath(fullPath);
        imPath.replace_extension(".xmp");
        std::string sidecarPath = imPath.string();

        std::string xmpPacket;
        std::ifstream sidecarFile(sidecarPath, std::ios::in);
        if (!sidecarFile) {
            hasSCXMP = false;
        } else {
            std::ostringstream sidecarContents;
            sidecarContents << sidecarFile.rdbuf();
            xmpPacket = sidecarContents.str();
            sidecarFile.close();
            Exiv2::XmpParser::decode(scxmpData, xmpPacket);
            hasSCXMP = true;
        }
    } catch (const Exiv2::Error& e) {
        LOG_ERROR("Exiv2 sidecar exception: {}", e.what());
    }

    // Attempt to load in the film values based on the metadata in the exif
    if (!exifData.empty()) {
        if (auto rotOpt = getExifValue<int>(exifData, "Exif.Image.Orientation"); rotOpt.has_value()) {
            imRot = rotOpt.value();
        }
        // Try the internal EXIF data
        if (auto fvMetaOpt = getExifValue<std::string>(exifData, "Exif.Image.ImageDescription"); fvMetaOpt.has_value()) {
            std::string customMeta = saniJsonString(fvMetaOpt.value());
            if (!customMeta.empty()) {
                // Run the meta loding function
                if (loadMetaFromStr(customMeta)) {
                    return;
                }
            }
        }
    }

    // Then try the internal XMP data
    if (!intxmpData.empty()) {
        if (auto rotOpt = getXmpValue<int>(intxmpData, "Xmp.tiff.Orientation"); rotOpt.has_value()) {
            imRot = rotOpt.value();
        }
        if(auto fvMetaOpt = getXmpValue<std::string>(intxmpData, "Xmp.dc.description"); fvMetaOpt.has_value()) {
            std::string customMeta = saniJsonString(fvMetaOpt.value());
            if (!customMeta.empty()) {
                // Run the meta loding function
                if (loadMetaFromStr(customMeta)) {
                    return;
                }
            }
        }

    }

    // Try rotation from xmp sidecar
    // Lastly try the sidecar file
    if (!scxmpData.empty()) {
        if (auto rotOpt = getXmpValue<int>(scxmpData, "Xmp.tiff.Orientation"); rotOpt.has_value()) {
            imRot = rotOpt.value();
        }

        if(auto fvMetaOpt = getXmpValue<std::string>(scxmpData, "Xmp.dc.description"); fvMetaOpt.has_value()) {
            std::string customMeta = saniJsonString(fvMetaOpt.value());
            if (!customMeta.empty()) {
                // Run the meta loding function
                if (loadMetaFromStr(customMeta)) {
                    return;
                }
            }
        }
    }
}


//--- Write Export Metadata---//
/*
    Update the metadata string variable with
    the current parameters and metadata values.
    Fill in the ImageDescription EXIF/XMP values
    in the loaded metadata, and write all metadata
    out to the file specified.

    Used in image export, all metadata is written to
    the output files after OIIO has completed writes.
*/
bool image::writeExpMeta(std::string filename) {
    updateMetaStr();
    exifData["Exif.Image.ImageDescription"] = jsonMeta;
    exifData["Exif.Image.Orientation"] = imRot;
    if (hasSCXMP) {
        scxmpData["Xmp.dc.description"] = jsonMeta;
        scxmpData["Xmp.tiff.Orientation"] = imRot;
    }
    else {
        intxmpData["Xmp.dc.description"] = jsonMeta;
        intxmpData["Xmp.tiff.Orientation"] = imRot;
    }

    try {
        // Open the image file
        Exiv2::Image::UniquePtr image = Exiv2::ImageFactory::open(filename);

        if (image.get() == 0) {
            throw Exiv2::Error(Exiv2::ErrorCode::kerErrorMessage, "Failed to open image for metadata writing");
        }

        // Read existing metadata (this is important to preserve any other metadata)
        image->readMetadata();

        // Set the new Exif data
        image->setExifData(exifData);

        // Set the new XMP data
        if (hasSCXMP) {
            image->setXmpData(scxmpData);
        } else {
            image->setXmpData(intxmpData);
        }

        // Write the metadata back to the image file
        image->writeMetadata();

        return true;

    } catch (const Exiv2::Error& e) {
        LOG_ERROR("Exiv2 exception writing metadata: {}", e.what());
        return false;
    }
}

//---Get Json Metadata---//
/*
    Fill out a json object with all parameters
    and metadata present in the image
*/
std::optional<nlohmann::json> image::getJSONMeta() {
    try {
        nlohmann::json j;
        nlohmann::json fvParams;
        nlohmann::json metaParams;

        fvParams["sampleX"] = nlohmann::json::array();
        fvParams["sampleY"] = nlohmann::json::array();
        fvParams["baseColor"] = nlohmann::json::array();
        fvParams["whitePoint"] = nlohmann::json::array();
        fvParams["blackPoint"] = nlohmann::json::array();
        fvParams["g_blackpoint"] = nlohmann::json::array();
        fvParams["g_whitepoint"] = nlohmann::json::array();
        fvParams["g_lift"] = nlohmann::json::array();
        fvParams["g_gain"] = nlohmann::json::array();
        fvParams["g_mult"] = nlohmann::json::array();
        fvParams["g_offset"] = nlohmann::json::array();
        fvParams["g_gamma"] = nlohmann::json::array();
        fvParams["cropBoxX"] = nlohmann::json::array();
        fvParams["cropBoxY"] = nlohmann::json::array();
        for (int i = 0; i < 4; i++) {
            if (i < 2) {
                fvParams["sampleX"].push_back(imgParam.sampleX[i]);
                fvParams["sampleY"].push_back(imgParam.sampleY[i]);
            }
            if (i < 3)
                fvParams["baseColor"].push_back(imgParam.baseColor[i]);

            fvParams["whitePoint"].push_back(imgParam.whitePoint[i]);
            fvParams["blackPoint"].push_back(imgParam.blackPoint[i]);
            fvParams["g_blackpoint"].push_back(imgParam.g_blackpoint[i]);
            fvParams["g_whitepoint"].push_back(imgParam.g_whitepoint[i]);
            fvParams["g_lift"].push_back(imgParam.g_lift[i]);
            fvParams["g_gain"].push_back(imgParam.g_gain[i]);
            fvParams["g_mult"].push_back(imgParam.g_mult[i]);
            fvParams["g_offset"].push_back(imgParam.g_offset[i]);
            fvParams["g_gamma"].push_back(imgParam.g_gamma[i]);
            fvParams["cropBoxX"].push_back(imgParam.cropBoxX[i]);
            fvParams["cropBoxY"].push_back(imgParam.cropBoxY[i]);
        }
        fvParams["blurAmount"] = imgParam.blurAmount;
        fvParams["minX"] = imgParam.minX;
        fvParams["minY"] = imgParam.minY;

        fvParams["maxX"] = imgParam.maxX;
        fvParams["maxY"] = imgParam.maxY;

        fvParams["temp"] = imgParam.temp;
        fvParams["tint"] = imgParam.tint;
        fvParams["rotation"] = imRot;

        metaParams["fileName"] = imMeta.fileName;
        metaParams["rollName"] = imMeta.rollName;
        metaParams["frameNumber"] = imMeta.frameNumber;
        metaParams["cameraMake"] = imMeta.cameraMake;
        metaParams["cameraModel"] = imMeta.cameraModel;
        metaParams["lens"] = imMeta.lens;
        metaParams["filmStock"] = imMeta.filmStock;
        metaParams["focalLength"] = imMeta.focalLength;
        metaParams["fNumber"] = imMeta.fNumber;
        metaParams["exposureTime"] = imMeta.exposureTime;
        metaParams["dateTime"] = imMeta.dateTime;
        metaParams["location"] = imMeta.location;
        metaParams["gps"] = imMeta.gps;
        metaParams["notes"] = imMeta.notes;
        metaParams["devProcess"] = imMeta.devProcess;
        metaParams["chemMfg"] = imMeta.chemMfg;
        metaParams["devNotes"] = imMeta.devNotes;
        metaParams["scanner"] = imMeta.scanner;
        metaParams["scanNotes"] = imMeta.scanNotes;

        j["filmvert"] = fvParams;
        j["metadata"] = metaParams;
        return j;

    } catch (const std::exception& e) {
        LOG_WARN("Unable to format image metadata: {}", e.what());
        return nullptr;
    }
}

//---Update Metadata String Variable---//
/*
    Simple helper function to dump the json object
    with all params and metadata to the jsonMeta
    string variable.
*/
void image::updateMetaStr() {
    try {
        std::optional<nlohmann::json> j = getJSONMeta();

        if (j.has_value())
            jsonMeta = j.value().dump(-1);
        else
            LOG_WARN("Could not format metadata");
        return;
    } catch (const std::exception& e) {
        LOG_WARN("Unable to format image metadata: {}", e.what());
        return;
    }
}

//---Write XMP File---//
/*
    Flush all params and metadata to the metadata
    string variable, then attempt to write out the
    XMP data to disk (sidecar file). If only source
    XMP data was present on image load in (no sidecar),
    That existing data is updated and written to sidecar.

    Otherwise the data present in the sidecar upon load
    will be the data written back to the sidecar
*/

void image::writeXMPFile() {
    updateMetaStr();
    if (hasSCXMP) {
        scxmpData["Xmp.dc.description"] = jsonMeta;
        scxmpData["Xmp.tiff.Orientation"] = imRot;
    }
    else {
        intxmpData["Xmp.dc.description"] = jsonMeta;
        intxmpData["Xmp.tiff.Orientation"] = imRot;
    }


    try {
        std::filesystem::path imPath(fullPath);
        imPath.replace_extension(".xmp");
        std::string sidecarPath = imPath.string();

        std::string xmpPacket;
        if (hasSCXMP)
            Exiv2::XmpParser::encode(xmpPacket, scxmpData);
        else
            Exiv2::XmpParser::encode(xmpPacket, intxmpData);
        std::ofstream sidecarFile(sidecarPath, std::ios::out | std::ios::trunc);
        if (!sidecarFile) {
            throw Exiv2::Error(Exiv2::ErrorCode::kerErrorMessage, "Could not open the sidecar");
        }
        sidecarFile << xmpPacket;
        sidecarFile.close();
        needMetaWrite = false;
    } catch (const Exiv2::Error& e) {
        LOG_ERROR("Exiv2 exception writing sidecar: {}", e.what());
    }
}

//--- Load Metadata from String---//
/*
    Attempt to load in and populate the inversion
    params and metadata from a json string (usually
    from within the xmp or exif data in a file).
    Will only update the params if a full set is
    found and matched. Otherwise will match available
    metadata values (incomplete match possible)
*/

bool image::loadMetaFromStr(const std::string& j) {
    try {
            // Parse JSON into class members
            nlohmann::json jsonObject = nlohmann::json::parse(j);
            imageParams tmpParam;
            // Toss into temp param first so that if there's a failure
            // nothing actually sticks (no partial params)
            bool goodImgParm = true;
            if (jsonObject.contains("filmvert")) {
                nlohmann::json imgJson = jsonObject["filmvert"].get<nlohmann::json>();

                if (imgJson.contains("sampleX") && imgJson["sampleX"].is_array()) {
                    const auto& arr = imgJson["sampleX"];
                    for (size_t i = 0; i < std::min(arr.size(), size_t(2)); ++i) {
                        tmpParam.sampleX[i] = arr[i].get<unsigned int>();
                    }
                } else {
                    goodImgParm = false;
                }

                if (imgJson.contains("sampleY") && imgJson["sampleY"].is_array()) {
                    const auto& arr = imgJson["sampleY"];
                    for (size_t i = 0; i < std::min(arr.size(), size_t(2)); ++i) {
                        tmpParam.sampleY[i] = arr[i].get<unsigned int>();
                    }
                } else {
                    goodImgParm = false;
                }

                if (imgJson.contains("baseColor") && imgJson["baseColor"].is_array()) {
                    const auto& arr = imgJson["baseColor"];
                    for (size_t i = 0; i < std::min(arr.size(), size_t(3)); ++i) {
                        tmpParam.baseColor[i] = arr[i].get<float>();
                    }
                } else {
                    goodImgParm = false;
                }

                if (imgJson.contains("whitePoint") && imgJson["whitePoint"].is_array()) {
                    const auto& arr = imgJson["whitePoint"];
                    for (size_t i = 0; i < std::min(arr.size(), size_t(4)); ++i) {
                        tmpParam.whitePoint[i] = arr[i].get<float>();
                    }
                } else {
                    goodImgParm = false;
                }

                if (imgJson.contains("blackPoint") && imgJson["blackPoint"].is_array()) {
                    const auto& arr = imgJson["blackPoint"];
                    for (size_t i = 0; i < std::min(arr.size(), size_t(4)); ++i) {
                        tmpParam.blackPoint[i] = arr[i].get<float>();
                    }
                } else {
                    goodImgParm = false;
                }

                if (imgJson.contains("g_blackpoint") && imgJson["g_blackpoint"].is_array()) {
                    const auto& arr = imgJson["g_blackpoint"];
                    for (size_t i = 0; i < std::min(arr.size(), size_t(4)); ++i) {
                        tmpParam.g_blackpoint[i] = arr[i].get<float>();
                    }
                } else {
                    goodImgParm = false;
                }

                if (imgJson.contains("g_whitepoint") && imgJson["g_whitepoint"].is_array()) {
                    const auto& arr = imgJson["g_whitepoint"];
                    for (size_t i = 0; i < std::min(arr.size(), size_t(4)); ++i) {
                        tmpParam.g_whitepoint[i] = arr[i].get<float>();
                    }
                } else {
                    goodImgParm = false;
                }

                if (imgJson.contains("g_lift") && imgJson["g_lift"].is_array()) {
                    const auto& arr = imgJson["g_lift"];
                    for (size_t i = 0; i < std::min(arr.size(), size_t(4)); ++i) {
                        tmpParam.g_lift[i] = arr[i].get<float>();
                    }
                } else {
                    goodImgParm = false;
                }

                if (imgJson.contains("g_gain") && imgJson["g_gain"].is_array()) {
                    const auto& arr = imgJson["g_gain"];
                    for (size_t i = 0; i < std::min(arr.size(), size_t(4)); ++i) {
                        tmpParam.g_gain[i] = arr[i].get<float>();
                    }
                } else {
                    goodImgParm = false;
                }

                if (imgJson.contains("g_mult") && imgJson["g_mult"].is_array()) {
                    const auto& arr = imgJson["g_mult"];
                    for (size_t i = 0; i < std::min(arr.size(), size_t(4)); ++i) {
                        tmpParam.g_mult[i] = arr[i].get<float>();
                    }
                } else {
                    goodImgParm = false;
                }

                if (imgJson.contains("g_offset") && imgJson["g_offset"].is_array()) {
                    const auto& arr = imgJson["g_offset"];
                    for (size_t i = 0; i < std::min(arr.size(), size_t(4)); ++i) {
                        tmpParam.g_offset[i] = arr[i].get<float>();
                    }
                } else {
                    goodImgParm = false;
                }

                if (imgJson.contains("g_gamma") && imgJson["g_gamma"].is_array()) {
                    const auto& arr = imgJson["g_gamma"];
                    for (size_t i = 0; i < std::min(arr.size(), size_t(4)); ++i) {
                        tmpParam.g_gamma[i] = arr[i].get<float>();
                    }
                } else {
                    goodImgParm = false;
                }

                if (imgJson.contains("cropBoxX") && imgJson["cropBoxX"].is_array()) {
                    const auto& arr = imgJson["cropBoxX"];
                    for (size_t i = 0; i < std::min(arr.size(), size_t(4)); ++i) {
                        tmpParam.cropBoxX[i] = arr[i].get<unsigned int>();
                    }
                } else {
                    goodImgParm = false;
                }

                if (imgJson.contains("cropBoxY") && imgJson["cropBoxY"].is_array()) {
                    const auto& arr = imgJson["cropBoxY"];
                    for (size_t i = 0; i < std::min(arr.size(), size_t(4)); ++i) {
                        tmpParam.cropBoxY[i] = arr[i].get<unsigned int>();
                    }
                } else {
                    goodImgParm = false;
                }

                if (imgJson.contains("blurAmount")){
                    tmpParam.blurAmount = imgJson["blurAmount"].get<float>();
                } else {
                    goodImgParm = false;
                }

                if (imgJson.contains("minX")){
                    tmpParam.minX = imgJson["minX"].get<unsigned int>();
                } else {
                    goodImgParm = false;
                }

                if (imgJson.contains("minY")){
                    tmpParam.minY = imgJson["minY"].get<unsigned int>();
                } else {
                    goodImgParm = false;
                }

                if (imgJson.contains("maxX")){
                    tmpParam.maxX = imgJson["maxX"].get<unsigned int>();
                } else {
                    goodImgParm = false;
                }

                if (imgJson.contains("maxY")){
                    tmpParam.maxY = imgJson["maxY"].get<unsigned int>();
                } else {
                    goodImgParm = false;
                }

                if (imgJson.contains("temp")){
                    tmpParam.temp = imgJson["temp"].get<float>();
                } else {
                    goodImgParm = false;
                }

                if (imgJson.contains("tint")){
                    tmpParam.tint = imgJson["tint"].get<float>();
                } else {
                    goodImgParm = false;
                }

                if (imgJson.contains("rotation")){
                    imRot = imgJson["rotation"].get<int>();
                } else {
                    //goodImgParm = false;
                }

            } else {
                LOG_WARN("No parameters found in metadata!");
                goodImgParm = false;
            }

            if (jsonObject.contains("metadata")) {
                nlohmann::json metaJson = jsonObject["metadata"].get<nlohmann::json>();

                if (metaJson.contains("fileName")) {
                    imMeta.fileName = metaJson["fileName"].get<std::string>();
                }

                if (metaJson.contains("rollName")) {
                    imMeta.rollName = metaJson["rollName"].get<std::string>();
                }
                if (metaJson.contains("frameNumber") && metaJson["frameNumber"].is_number()) {
                    imMeta.frameNumber = metaJson["frameNumber"].get<int>();
                }
                if (metaJson.contains("cameraMake")) {
                    imMeta.cameraMake = metaJson["cameraMake"].get<std::string>();
                }
                if (metaJson.contains("cameraModel")) {
                    imMeta.cameraModel = metaJson["cameraModel"].get<std::string>();
                }
                if (metaJson.contains("lens")) {
                    imMeta.lens = metaJson["lens"].get<std::string>();
                }
                if (metaJson.contains("filmStock")) {
                    imMeta.filmStock = metaJson["filmStock"].get<std::string>();
                }
                if (metaJson.contains("focalLength")) {
                    imMeta.focalLength = metaJson["focalLength"].get<std::string>();
                }
                if (metaJson.contains("fNumber")) {
                    imMeta.fNumber = metaJson["fNumber"].get<std::string>();
                }
                if (metaJson.contains("exposureTime")) {
                    imMeta.exposureTime = metaJson["exposureTime"].get<std::string>();
                }
                if (metaJson.contains("dateTime")) {
                    imMeta.dateTime = metaJson["dateTime"].get<std::string>();
                }
                if (metaJson.contains("location")) {
                    imMeta.location = metaJson["location"].get<std::string>();
                }
                if (metaJson.contains("gps")) {
                    imMeta.gps = metaJson["gps"].get<std::string>();
                }
                if (metaJson.contains("notes")) {
                    imMeta.notes = metaJson["notes"].get<std::string>();
                }
                if (metaJson.contains("devProcess")) {
                    imMeta.devProcess = metaJson["devProcess"].get<std::string>();
                }
                if (metaJson.contains("chemMfg")) {
                    imMeta.chemMfg = metaJson["chemMfg"].get<std::string>();
                }
                if (metaJson.contains("devNotes")) {
                    imMeta.devNotes = metaJson["devNotes"].get<std::string>();
                }
                if (metaJson.contains("scanner")) {
                    imMeta.scanner = metaJson["scanner"].get<std::string>();
                }
                if (metaJson.contains("scanNotes")) {
                    imMeta.scanNotes = metaJson["scanNotes"].get<std::string>();
                }
            } else {
                LOG_WARN("No metadata found!");
            }

            if (goodImgParm) {
                imgParam = tmpParam;
                renderBypass = false;
            }

            return true;
        } catch (const std::exception& e) {
            LOG_WARN("Exception parsing metadata: {}", e.what());
            return false;
        }
}
