#include "imageIO.h"
#include "imageMeta.h"
#include "exifUtils.h"
#include "exiv2/xmp_exiv2.hpp"
#include "imageParams.h"
#include "logger.h"
#include "nlohmann/json_fwd.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <ostream>

// Metadata workflow:
// Import:
// - Read in all exif data from camera raw
// - Read in xmp data from file, if not available from camera raw
// - If parameters available from xmp, load parameters
// - Save all metadata alongside image struct (xmp + exif)
// - When user makes changes, periodically update the xmp sidecar file
//
// - Make a timer in the main window loop. Only after interactions have
// stopped for say 2-5 seconds, then write out the active file if necessary
//
// - Add a menu item to sync/save all roll changes to disk
//
// Metadata editing:
// - Custom fields for tagging?
// - What tags are available, and where do they get stored? (exif?)
// - Check what's available in LR and make interactions easy there
// - Potential CSV output?
//
// Export:
// - When user exports images, save the sidecar file once more
// - Export all metadata to output file
// - Metadata specific output window (for special formats?)



// TODO: What happens when there is xmp sidecar but
// Exiv2 can't load the image? Load xmp first?
void image::readMetaFromFile() {
    try {
        Exiv2::enableBMFF();
        auto image = Exiv2::ImageFactory::open(fullPath);
        if (!image) {
            throw Exiv2::Error(Exiv2::ErrorCode::kerErrorMessage, "Could not open the image");
        }

        image->readMetadata();

        // Read EXIF Data into Image
        exifData = image->exifData();

        // Read XMP Data into Image
        intxmpData = image->xmpData();

        // Read sidecar XMP Data into Image
        std::filesystem::path imPath(fullPath);
        imPath.replace_extension(".xmp");
        std::string sidecarPath = imPath.string();

        std::string xmpPacket;
        std::ifstream sidecarFile(sidecarPath, std::ios::in);
        if (!sidecarFile) {
            // Do nothing
        } else {
            std::ostringstream sidecarContents;
            sidecarContents << sidecarFile.rdbuf();
            xmpPacket = sidecarContents.str();
            sidecarFile.close();
            Exiv2::XmpParser::decode(scxmpData, xmpPacket);
            hasSCXMP = true;
        }

    } catch (const Exiv2::Error& e) {
        LOG_ERROR("Exiv2 exception: {}", e.what());
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

    // Any cleanup?
}
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
        metaParams["gpsLocation"] = imMeta.gpsLocation;
        metaParams["notes"] = imMeta.notes;
        metaParams["devProcess"] = imMeta.devProcess;
        metaParams["chemMfg"] = imMeta.chemMfg;
        metaParams["devNotes"] = imMeta.devNotes;

        j["filmvert"] = fvParams;
        j["metadata"] = metaParams;
        return j;

    } catch (const std::exception& e) {
        LOG_WARN("Unable to format image metadata: {}", e.what());
        return nullptr;
    }
}
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
                if (metaJson.contains("gpsLocation")) {
                    imMeta.gpsLocation = metaJson["gpsLocation"].get<std::string>();
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
/*
// Read all metadata from an image file (EXIF, XMP, and IPTC)
void readAllMetadata(const std::string& imagePath) {
    Exiv2::enableBMFF();
    try {
        // Open the image file (using newer unique_ptr API)
        auto image = Exiv2::ImageFactory::open(imagePath);
        if (!image) {
            throw Exiv2::Error(Exiv2::ErrorCode::kerErrorMessage, "Could not open the image");
        }

        // Read the metadata from the image
        image->readMetadata();

        // Read EXIF data
        Exiv2::ExifData &exifData = image->exifData();
        if (!exifData.empty()) {
            LOG_INFO("--- EXIF Metadata ---");
            Exiv2::ExifData::const_iterator end = exifData.end();
            for (Exiv2::ExifData::const_iterator i = exifData.begin(); i != end; ++i) {
                const char* typeName = i->typeName();
                LOG_INFO("{}  {}  {}",i->key(), (typeName ? typeName : "Unknown"),i->value().toString() );
            }
        } else {
            LOG_INFO("No EXIF data found in the image");
        }

        // Read XMP data
        Exiv2::XmpData &xmpData = image->xmpData();
        if (!xmpData.empty()) {
            LOG_INFO("\n--- XMP Metadata ---");
            Exiv2::XmpData::const_iterator end = xmpData.end();
            for (Exiv2::XmpData::const_iterator i = xmpData.begin(); i != end; ++i) {
                const char* typeName = i->typeName();
                LOG_INFO("{} {} {}", i->key(), (typeName ? typeName : "Unknown"), i->value().toString());
            }
        } else {
            LOG_INFO("No XMP data found in the image");
        }

        // Read IPTC data
        Exiv2::IptcData &iptcData = image->iptcData();
        if (!iptcData.empty()) {
            std::cout << "\n--- IPTC Metadata ---" << std::endl;
            Exiv2::IptcData::const_iterator end = iptcData.end();
            for (Exiv2::IptcData::const_iterator i = iptcData.begin(); i != end; ++i) {
                const char* typeName = i->typeName();
                std::cout << std::setw(44) << std::left
                          << i->key() << " "
                          << std::setw(9) << std::setfill(' ') << std::left
                          << (typeName ? typeName : "Unknown") << " "
                          << std::dec << i->value()
                          << std::endl;
            }
        } else {
            std::cout << "No IPTC data found in the image" << std::endl;
        }
    }
    catch (const Exiv2::Error& e) {
        LOG_ERROR("Exiv2 exception: {}", e.what());
    }
}

// Read EXIF metadata from an image file
void readExifMetadata(const std::string& imagePath) {
    try {
        // Open the image file
        auto image = Exiv2::ImageFactory::open(imagePath);
        if (!image) {
            throw Exiv2::Error(Exiv2::ErrorCode::kerErrorMessage, "Could not open the image");
        }

        // Read the metadata from the image
        image->readMetadata();

        // Get the EXIF data
        Exiv2::ExifData &exifData = image->exifData();

        // Check if the image has EXIF metadata
        if (exifData.empty()) {
            std::cout << "No EXIF data found in the image" << std::endl;
            return;
        }

        // Iterate through all EXIF metadata and print them
        Exiv2::ExifData::const_iterator end = exifData.end();
        for (Exiv2::ExifData::const_iterator i = exifData.begin(); i != end; ++i) {
            const char* typeName = i->typeName();
            std::cout << std::setw(44) << std::left
                      << i->key() << " "
                      << "0x" << std::setw(4) << std::setfill('0') << std::right
                      << std::hex << i->tag() << " "
                      << std::setw(9) << std::setfill(' ') << std::left
                      << (typeName ? typeName : "Unknown") << " "
                      << std::dec << std::setw(3) << std::right
                      << i->count() << " "
                      << std::dec << i->value()
                      << std::endl;
        }
    }
    catch (const Exiv2::Error& e) {
        LOG_ERROR("Exiv2 exception: {}", e.what());
    }
}

// Write or update EXIF metadata in an image file
void writeExifMetadata(const std::string& imagePath,
                      const std::string& key,
                      const std::string& value) {
    try {
        // Open the image file
        auto image = Exiv2::ImageFactory::open(imagePath);
        if (!image) {
            throw Exiv2::Error(Exiv2::ErrorCode::kerErrorMessage, "Could not open the image");
        }

        // Read the existing metadata
        image->readMetadata();

        // Get the EXIF data
        Exiv2::ExifData &exifData = image->exifData();

        // Set the EXIF data with the provided key and value
        exifData[key] = value;

        // Write the updated EXIF data back to the image
        image->setExifData(exifData);
        image->writeMetadata();

        std::cout << "Successfully updated EXIF metadata: " << key << " = " << value << std::endl;
    }
    catch (const Exiv2::Error& e) {
        LOG_ERROR("Exiv2 exception: {}", e.what());
    }
}

// Write XMP metadata (better for custom data)
void writeXmpMetadata(const std::string& imagePath,
                    const std::string& key,
                    const std::string& value) {
    try {
        // Open the image file
        auto image = Exiv2::ImageFactory::open(imagePath);
        if (!image) {
            throw Exiv2::Error(Exiv2::ErrorCode::kerErrorMessage, "Could not open the image");
        }

        // Read the existing metadata
        image->readMetadata();

        // Get the XMP data
        Exiv2::XmpData &xmpData = image->xmpData();

        // Set the XMP data with the provided key and value
        xmpData[key] = value;

        // Write the updated XMP data back to the image
        image->setXmpData(xmpData);
        image->writeMetadata();

        std::cout << "Successfully updated XMP metadata: " << key << " = " << value << std::endl;
    }
    catch (const Exiv2::Error& e) {
        LOG_ERROR("Exiv2 exception: {}", e.what());
    }
}

// Function to create custom metadata with appropriate namespaces
void writeCustomMetadata(const std::string& imagePath,
                        const std::string& customName,
                        const std::string& value) {
    try {
        // Open the image file
        auto image = Exiv2::ImageFactory::open(imagePath);
        if (!image) {
            throw Exiv2::Error(Exiv2::ErrorCode::kerErrorMessage, "Could not open the image");
        }

        // Read the existing metadata
        image->readMetadata();
        Exiv2::XmpData &xmpData = image->xmpData();
        // Use a standard namespace like dc (Dublin Core)
        xmpData["Xmp.dc.source"] = value;
        std::filesystem::path imPath(imagePath);
        bool useSidecar = imPath.extension().string() == ".CR3";

        if (useSidecar) {
            // Write to .xmp sidecar file
            imPath.replace_extension(".xmp");
            std::string sidecarPath = imPath.string();

            std::string xmpPacket;
            Exiv2::XmpParser::encode(xmpPacket, xmpData);
            std::ofstream sidecarFile(sidecarPath, std::ios::out | std::ios::trunc);
            if (!sidecarFile) {
                throw std::runtime_error("Failed to create sidecar file: " + sidecarPath);
            }
            sidecarFile << xmpPacket;
            sidecarFile.close();
        } else {
            // Read existing metadata
            //image->readMetadata();
            image->setXmpData(xmpData);
            image->writeMetadata();
        }

        image->setXmpData(xmpData);
        image->writeMetadata();
    }
    catch (const Exiv2::Error& e) {
        LOG_ERROR("Exiv2 exception: {}", e.what());
    }
}
*/
