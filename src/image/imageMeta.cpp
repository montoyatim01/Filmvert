#include "image.h"
#include "imageMeta.h"
#include "exifUtils.h"
#include "exiv2/xmp_exiv2.hpp"
#include "imageParams.h"
#include "logger.h"
#include "nlohmann/json.hpp"
#include "structs.h"
#include <cstddef>
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
                if (loadMetaFromStr(customMeta, nullptr, true)) {
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
                if (loadMetaFromStr(customMeta, nullptr, true)) {
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
                if (loadMetaFromStr(customMeta, nullptr, true)) {
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

        Exiv2::ExifData destExif = image->exifData();
        // List of problematic Exif tags to skip
        std::set<std::string> skipExifTags = {
            "Exif.Thumbnail.JPEGInterchangeFormat",
            "Exif.Thumbnail.JPEGInterchangeFormatLength",
            "Exif.Image.ImageWidth",
            "Exif.Image.ImageLength",
            "Exif.Photo.PixelXDimension",
            "Exif.Photo.PixelYDimension"

        };

        // Selective copy of Exif data
        for (Exiv2::ExifData::const_iterator i = exifData.begin(); i != exifData.end(); ++i) {
            std::string key = i->key();
            if (skipExifTags.find(key) == skipExifTags.end()) {
                destExif[key] = i->value();
            }
        }

        // Set the new Exif data
        image->setExifData(destExif);

        // Set the new XMP data
        /*if (hasSCXMP) {
            image->setXmpData(scxmpData);
        } else {
            image->setXmpData(intxmpData);
        }*/

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

        metaParams = imMeta;

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

bool image::writeJSONFile() {
    updateMetaStr();
    try {
        std::optional<nlohmann::json> met = getJSONMeta();
        if (!met.has_value())
            return false;
        std::string jsonPath = rollPath + "/" + srcFilename + ".fvi";
        std::ofstream file(jsonPath);
        if (!file)
            return false;
        file << met.value().dump(4);
        file.close();
    } catch (const std::exception& e) {
        LOG_ERROR("Error writing JSON file: {}", e.what());
        return false;
    }
    return true;
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

bool image::loadMetaFromStr(const std::string& j, copyPaste* impOpt, bool init) {
    imageParams *impParam = nullptr;
    imageMetadata *impMeta = nullptr;
    // Try importing Param JSON Object
    try {
        // Parse JSON into class members
        nlohmann::json jsonObject = nlohmann::json::parse(j);
        if (jsonObject.contains("filmvert")) {
            impParam = new imageParams;
            *impParam = jsonObject["filmvert"].get<imageParams>();
        } else {
            LOG_WARN("No params found!");
        }
    } catch (const std::exception& e) {
        LOG_WARN("Exception parsing metadata: {}", e.what());
    }

    // Try importing Meta JSON Object
    try {
        nlohmann::json jsonObject = nlohmann::json::parse(j);
            if (jsonObject.contains("metadata")) {
                impMeta = new imageMetadata;
                *impMeta = jsonObject["metadata"].get<imageMetadata>();
            } else {
                LOG_WARN("No metadata found!");
            }
    } catch (const std::exception& e) {
        LOG_WARN("Exception parsing metadata: {}", e.what());
    }
    copyPaste opts;
    if (impOpt == nullptr) {
        // Toggle the initial false values to true
        opts.analysisGlobal();
        opts.gradeGlobal();
        opts.metaGlobal();
        //LOG_INFO("Setting full params");
    } else {
        opts = *impOpt;
    }
    metaPaste(opts, impParam, impMeta, init);
    if (impParam == nullptr && impMeta == nullptr)
        return false;
    return true;
}

//--- Import Image Metadata ---//
/*
    Import the params (if available) from the
    selected image
*/
bool image::importImageMeta(std::string filename, copyPaste* impOpt) {
    std::filesystem::path imPath(filename);
    if (imPath.extension().string() == ".fvi" ||
        imPath.extension().string() == ".FVI") {
        // Selected json file for import
        std::ifstream file(filename);
        std::string fileContents((std::istreambuf_iterator<char>(file)),
                            std::istreambuf_iterator<char>());
        return loadMetaFromStr(fileContents, impOpt);
    } else {
        try {
            Exiv2::enableBMFF();
            auto image = Exiv2::ImageFactory::open(filename);
            if (!image) {
                throw Exiv2::Error(Exiv2::ErrorCode::kerErrorMessage, "Could not open the image for metadata reading");
            }

            image->readMetadata();

            // Read EXIF Data into Image
            Exiv2::ExifData exifData = image->exifData();

            if (!exifData.empty()) {
                if (auto fvMetaOpt = getExifValue<std::string>(exifData, "Exif.Image.ImageDescription"); fvMetaOpt.has_value()) {
                    std::string customMeta = saniJsonString(fvMetaOpt.value());
                    if (!customMeta.empty()) {
                        // Run the meta loding function
                        return loadMetaFromStr(customMeta, impOpt);
                    }
                }
            }

        } catch (const Exiv2::Error& e) {
            LOG_ERROR("Exiv2 image exception: {}", e.what());
            return false;
        }
    }
    return true;

}


void image::metaPaste(copyPaste selectons, imageParams* params, imageMetadata* meta, bool init) {
    bool metaChg = false;
    if (params != nullptr) {
        imageParams impParams = *params;
        if (selectons.baseColor) {
            imgParam.baseColor[0] = impParams.baseColor[0];
            imgParam.baseColor[1] = impParams.baseColor[1];
            imgParam.baseColor[2] = impParams.baseColor[2];
            metaChg = true;
        }
        if (selectons.analysisBlur) {
            imgParam.blurAmount = impParams.blurAmount;
            metaChg = true;
        }
        if (selectons.temp) {
            imgParam.temp = impParams.temp;
            metaChg = true;
        }
        if (selectons.tint) {
            imgParam.tint = impParams.tint;
            metaChg = true;
        }
        for (int j = 0; j < 4; j++) {
            if (selectons.cropPoints) {
                imgParam.cropBoxX[j] = impParams.cropBoxX[j];
                imgParam.cropBoxY[j] = impParams.cropBoxY[j];
                metaChg = true;
            }
            if (selectons.analysis) {
                imgParam.blackPoint[j] = impParams.blackPoint[j];
                imgParam.whitePoint[j] = impParams.whitePoint[j];
                metaChg = true;
            }
            if (selectons.bp) {
                imgParam.g_blackpoint[j] = impParams.g_blackpoint[j];
                metaChg = true;
            }
            if (selectons.wp) {
                imgParam.g_whitepoint[j] = impParams.g_whitepoint[j];
                metaChg = true;
            }
            if (selectons.lift) {
                imgParam.g_lift[j] = impParams.g_lift[j];
                metaChg = true;
            }
            if (selectons.gain) {
                imgParam.g_gain[j] = impParams.g_gain[j];
                metaChg = true;
            }
            if (selectons.mult) {
                imgParam.g_mult[j] = impParams.g_mult[j];
                metaChg = true;
            }
            if (selectons.offset) {
                imgParam.g_offset[j] = impParams.g_offset[j];
                metaChg = true;
            }
            if (selectons.gamma) {
                imgParam.g_gamma[j] = impParams.g_gamma[j];
                metaChg = true;
            }
        }
        renderBypass = !metaChg;
    }

    if (meta != nullptr) {
        imageMetadata impMeta = *meta;
        //---Metadata
        if (selectons.make) {
            imMeta.cameraMake = impMeta.cameraMake;
            metaChg = true;
        }
        if (selectons.model) {
            imMeta.cameraModel = impMeta.cameraModel;
            metaChg = true;
        }
        if (selectons.lens) {
            imMeta.lens = impMeta.lens;
            metaChg = true;
        }
        if (selectons.stock) {
            imMeta.filmStock = impMeta.filmStock;
            metaChg = true;
        }
        if (selectons.focal) {
            imMeta.focalLength = impMeta.focalLength;
            metaChg = true;
        }
        if (selectons.fstop) {
            imMeta.fNumber = impMeta.fNumber;
            metaChg = true;
        }
        if (selectons.exposure) {
            imMeta.exposureTime = impMeta.exposureTime;
            metaChg = true;
        }
        if (selectons.date) {
            imMeta.dateTime = impMeta.dateTime;
            metaChg = true;
        }
        if (selectons.location) {
            imMeta.location = impMeta.location;
            metaChg = true;
        }
        if (selectons.gps) {
            imMeta.gps = impMeta.gps;
            metaChg = true;
        }
        if (selectons.notes) {
            imMeta.notes = impMeta.notes;
            metaChg = true;
        }
        if (selectons.dev) {
            imMeta.devProcess = impMeta.devProcess;
            metaChg = true;
        }
        if (selectons.chem) {
            imMeta.chemMfg = impMeta.chemMfg;
            metaChg = true;
        }
        if (selectons.devnote) {
            imMeta.devNotes = impMeta.devNotes;
            metaChg = true;
        }
        if (selectons.scanner) {
            imMeta.scanner = impMeta.scanner;
            metaChg = true;
        }
        if (selectons.scannotes) {
            imMeta.scanNotes = impMeta.scanNotes;
            metaChg = true;
        }
    }
    if (!init)
        needMetaWrite |= metaChg;
}
