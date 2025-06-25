#include "roll.h"
#include "structs.h"
#include "exifUtils.h"

//--- Get Roll Metadata String ---//
/*
    Get the string with the full JSON
    metadata/params for saving or embedding
    in a contact sheet
*/
std::string filmRoll::getRollMetaString(bool pretty) {
    try {
        nlohmann::json j;
        nlohmann::json jRoll = nlohmann::json::object();

        for (int i = 0; i < images.size(); i++) {
            image* img = getImage(i);
            if (img) {
                std::optional<nlohmann::json> met = img->getJSONMeta();
                if (met.has_value())
                    jRoll[img->srcFilename.c_str()] = met.value();
            }
        }
        j[rollName.c_str()] = jRoll;
        std::string jDump = j.dump(pretty ? 4 : -1);
        return jDump;
    } catch (const std::exception& e) {
        LOG_WARN("Unable to format roll JSON: {}", e.what());
        return "";
    }
}

//--- Export Roll Metadata JSON ---//
/*
    Export the full JSON file with metadata and params
    from each image in the roll.
*/
bool filmRoll::exportRollMetaJSON() {
    try {
        std::string jDump = getRollMetaString(true);
        std::string outPath = rollPath + "/" + rollName + ".fvi";
        std::ofstream jsonFile(outPath, std::ios::out | std::ios::trunc);
        if (!jsonFile) {
            LOG_ERROR("Unable to open JSON file for roll metadata export!");
            return false;
        }
        jsonFile << jDump;
        jsonFile.close();
        return true;
    } catch (const std::exception& e) {
        LOG_WARN("Unable to save roll JSON: {}", e.what());
        return false;
    }
}

//--- Export Roll CSV ---//
/*
    Export an CSV file with the metadata (but not params)
    for every image in the roll
*/
bool filmRoll::exportRollCSV() {
    std::string csvFileName = rollPath + "/" + rollName + ".csv";
    std::ofstream csvFile(csvFileName, std::ios::trunc);
    if (!csvFile) {
        LOG_ERROR("Could not open CSV file for writing! {}", csvFileName);
        return false;
    }
    csvFile << "FrameNumber,FileName,CameraMake,CameraModel,Lens,FilmStock,";
    csvFile << "FocalLength,fNumber,ExposureTime,RollName,Date/Time,Location,";
    csvFile << "GPS,Notes,DevelopmentProcess,ChemicalManufacturer,";
    csvFile << "DevelopmentNotes,Scanner,ScanNotes,Rating\n";

    for (int i = 0; i < images.size(); i++) {
        csvFile << images[i].imgMeta.frameNumber << ",";
        csvFile << images[i].srcFilename << ",";
        csvFile << images[i].imgMeta.cameraMake << ",";
        csvFile << images[i].imgMeta.cameraModel << ",";
        csvFile << images[i].imgMeta.lens << ",";
        csvFile << images[i].imgMeta.filmStock << ",";
        csvFile << images[i].imgMeta.focalLength << ",";
        csvFile << images[i].imgMeta.fNumber << ",";
        csvFile << images[i].imgMeta.exposureTime << ",";
        csvFile << images[i].imgMeta.rollName << ",";
        csvFile << images[i].imgMeta.dateTime << ",";
        csvFile << images[i].imgMeta.location << ",";
        csvFile << images[i].imgMeta.gps << ",";
        csvFile << images[i].imgMeta.notes << ",";
        csvFile << images[i].imgMeta.devProcess << ",";
        csvFile << images[i].imgMeta.chemMfg << ",";
        csvFile << images[i].imgMeta.devNotes << ",";
        csvFile << images[i].imgMeta.scanner << ",";
        csvFile << images[i].imgMeta.scanNotes << ",";
        csvFile << images[i].imgMeta.rating << "\n";
    }

    csvFile.close();
    return true;
}

//--- Import Roll Metdata JSON ---//
/*
    Import a JSON file, and attempt to match all params
    and settings to the images in the JSON. Rename
    the roll based on the JSON roll's name.
*/
bool filmRoll::importRollMetaJSON(const std::string& jsonFile) {
    metaImp.clear();
     try {
        nlohmann::json jIn;
        std::filesystem::path imPath(jsonFile);

        if (imPath.extension().string() == ".fvi" ||
            imPath.extension().string() == ".FVI") {
            // Selected json file for import
            std::ifstream f(jsonFile);
            if (!f)
                return false;
            jIn = nlohmann::json::parse(f);
        } else {
            // Selected image file for import
            Exiv2::enableBMFF();
            auto image = Exiv2::ImageFactory::open(jsonFile);
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
                        jIn = nlohmann::json::parse(customMeta);
                    } else {
                        LOG_WARN("No metadata in selected image!");
                        return false;
                    }
                } else {
                    LOG_WARN("No metadata in selected image!");
                    return false;
                }
            } else {
                LOG_WARN("No metadata in selected image!");
                return false;
            }
        } // Image selection
        auto first_item = jIn.begin();
        rollName = first_item.key();
        nlohmann::json jRoll = first_item.value();
        for (int i = 0; i < images.size(); i++) {
            if (jRoll.contains(images[i].srcFilename)) {
                nlohmann::json obj = jRoll[images[i].srcFilename].get<nlohmann::json>();
                metaImp.emplace_back(MetaImpSet(&images[i], true, images[i].srcFilename, obj.dump(-1)));
                //images[i].loadMetaFromStr(obj.dump(-1), true);
            }
        }
        //if (appPrefs.prefs.autoSort)
        //    sortRoll();
        return true;

    } catch (const std::exception& e) {
        LOG_WARN("Unable to import roll from JSON file: {}", e.what());
        return false;
    } catch (const Exiv2::Error& e) {
        LOG_ERROR("Exiv2 image exception: {}", e.what());
        return false;
    }

}

void filmRoll::applyRollMetaJSON(bool params, copyPaste impOpt) {
    for (int i = 0; i < metaImp.size(); i++) {
        if (metaImp[i].selected) {
            if (metaImp[i].img) {
                metaImp[i].img->loadMetaFromStr(metaImp[i].json, &impOpt);
            }
        }
    }
    sortRoll();
}

bool filmRoll::applySelMetaJSON(std::string inFile, copyPaste impOpt) {
    bool goodSet = true;
    for (auto& img : images) {
        if (img.selected) {
            goodSet &= img.importImageMeta(inFile, &impOpt);
        }
    }
    return goodSet;
}


//--- Roll Metadata Pre-edit ---//
/*
    Pre-populate the roll path and roll name in the
    metadata edit struct. Loop through all fields
    in all images to determine if there are image-specific
    differences that need to be flagged.
*/
void filmRoll::rollMetaPreEdit(metaBuff *meta) {
    if (!meta)
        return;
    if (images.size() < 1)
        return;

    std::strcpy(meta->rollPath, rollPath.c_str());

    std::strcpy(meta->rollname, rollName.c_str());


    meta->dif_camMake = !std::all_of(images.begin() + 1, images.end(),
        [&](const image& obj) {
            return obj.imgMeta.cameraMake == images.front().imgMeta.cameraMake;
        });
    if (!meta->dif_camMake)
        std::strcpy(meta->camMake, images[0].imgMeta.cameraMake.c_str());
    //------
    meta->dif_camModel = !std::all_of(images.begin() + 1, images.end(),
        [&](const image& obj) {
            return obj.imgMeta.cameraModel == images.front().imgMeta.cameraModel;
        });
    if (!meta->dif_camModel)
        std::strcpy(meta->camModel, images[0].imgMeta.cameraModel.c_str());
    //------
    meta->dif_lens = !std::all_of(images.begin() + 1, images.end(),
        [&](const image& obj) {
            return obj.imgMeta.lens == images.front().imgMeta.lens;
        });
    if (!meta->dif_lens)
        std::strcpy(meta->lens, images[0].imgMeta.lens.c_str());
    //------
    meta->dif_film = !std::all_of(images.begin() + 1, images.end(),
        [&](const image& obj) {
            return obj.imgMeta.filmStock == images.front().imgMeta.filmStock;
        });
    if (!meta->dif_film)
        std::strcpy(meta->film, images[0].imgMeta.filmStock.c_str());
    //------
    meta->dif_focal = !std::all_of(images.begin() + 1, images.end(),
        [&](const image& obj) {
            return obj.imgMeta.focalLength == images.front().imgMeta.focalLength;
        });
    if (!meta->dif_focal)
        std::strcpy(meta->focal, images[0].imgMeta.focalLength.c_str());
    //------
    meta->dif_date = !std::all_of(images.begin() + 1, images.end(),
        [&](const image& obj) {
            return obj.imgMeta.dateTime == images.front().imgMeta.dateTime;
        });
    if (!meta->dif_date)
        std::strcpy(meta->date, images[0].imgMeta.dateTime.c_str());
    //------
    meta->dif_loc = !std::all_of(images.begin() + 1, images.end(),
        [&](const image& obj) {
            return obj.imgMeta.location == images.front().imgMeta.location;
        });
    if (!meta->dif_loc)
        std::strcpy(meta->loc, images[0].imgMeta.location.c_str());
    //------
    meta->dif_gps = !std::all_of(images.begin() + 1, images.end(),
        [&](const image& obj) {
            return obj.imgMeta.gps == images.front().imgMeta.gps;
        });
    if (!meta->dif_gps)
        std::strcpy(meta->gps, images[0].imgMeta.gps.c_str());
    //------
    meta->dif_notes = !std::all_of(images.begin() + 1, images.end(),
        [&](const image& obj) {
            return obj.imgMeta.notes == images.front().imgMeta.notes;
        });
    if (!meta->dif_notes)
        std::strcpy(meta->notes, images[0].imgMeta.notes.c_str());
    //------
    meta->dif_dev = !std::all_of(images.begin() + 1, images.end(),
        [&](const image& obj) {
            return obj.imgMeta.devProcess == images.front().imgMeta.devProcess;
        });
    if (!meta->dif_dev)
        std::strcpy(meta->dev, images[0].imgMeta.devProcess.c_str());
    //------
    meta->dif_chem = !std::all_of(images.begin() + 1, images.end(),
        [&](const image& obj) {
            return obj.imgMeta.chemMfg == images.front().imgMeta.chemMfg;
        });
    if (!meta->dif_chem)
        std::strcpy(meta->chem, images[0].imgMeta.chemMfg.c_str());
    //------
    meta->dif_devnotes = !std::all_of(images.begin() + 1, images.end(),
        [&](const image& obj) {
            return obj.imgMeta.devNotes == images.front().imgMeta.devNotes;
        });
    if (!meta->dif_devnotes)
        std::strcpy(meta->devnotes, images[0].imgMeta.devNotes.c_str());
    //------
    meta->dif_scanner = !std::all_of(images.begin() + 1, images.end(),
        [&](const image& obj) {
            return obj.imgMeta.scanner == images.front().imgMeta.scanner;
        });
    if (!meta->dif_scanner)
        std::strcpy(meta->scanner, images[0].imgMeta.scanner.c_str());
    //------
    meta->dif_scannotes = !std::all_of(images.begin() + 1, images.end(),
        [&](const image& obj) {
            return obj.imgMeta.scanNotes == images.front().imgMeta.scanNotes;
        });
    if (!meta->dif_scannotes)
        std::strcpy(meta->scannotes, images[0].imgMeta.scanNotes.c_str());
    //------
}

//--- Roll Metadata Post-Edit ---//
/*
    For every metadata field, if the apply button
    was checked, apply the field to all images
    in the roll
*/
void filmRoll::rollMetaPostEdit(metaBuff *meta) {
    if (!meta)
        return;
    if (meta->a_rollPath) {
        rollPath = meta->rollPath;
        for (auto& img : images)
            img.rollPath = rollPath;
    }

    if (meta->a_rollname)
        rollName = meta->rollname;

    if (images.size() < 1)
        return;

    if (meta->a_rollname)
        for (auto &img : images){
            img.imgMeta.rollName = meta->rollname;
            img.needMetaWrite = true;
        }

    //------
    if (meta->a_camMake)
        for (auto &img : images) {
            img.imgMeta.cameraMake = meta->camMake;
            img.needMetaWrite = true;
        }
    //------
    if (meta->a_camModel)
        for (auto &img : images) {
            img.imgMeta.cameraModel = meta->camModel;
            img.needMetaWrite = true;
        }
    //------
    if (meta->a_lens)
        for (auto &img : images) {
            img.imgMeta.lens = meta->lens;
            img.needMetaWrite = true;
        }
    //------
    if (meta->a_film)
        for (auto &img : images) {
            img.imgMeta.filmStock = meta->film;
            img.needMetaWrite = true;
        }
    //------
    if (meta->a_focal)
        for (auto &img : images) {
            img.imgMeta.focalLength = meta->focal;
            img.needMetaWrite = true;
        }
    //------
    if (meta->a_date)
        for (auto &img : images) {
            img.imgMeta.dateTime = meta->date;
            img.needMetaWrite = true;
        }
    //------
    if (meta->a_loc)
        for (auto &img : images) {
            img.imgMeta.location = meta->loc;
            img.needMetaWrite = true;
        }
    //------
    if (meta->a_gps)
        for (auto &img : images) {
            img.imgMeta.gps = meta->gps;
            img.needMetaWrite = true;
        }
    //------
    if (meta->a_notes)
        for (auto &img : images) {
            img.imgMeta.notes = meta-> notes;
            img.needMetaWrite = true;
        }
    //------
    if (meta->a_dev)
        for (auto &img : images) {
            img.imgMeta.devProcess = meta->dev;
            img.needMetaWrite = true;
        }
    //------
    if (meta->a_chem)
        for (auto &img : images) {
            img.imgMeta.chemMfg = meta->chem;
            img.needMetaWrite = true;
        }
    //------
    if (meta->a_devnotes)
        for (auto &img : images) {
            img.imgMeta.devNotes = meta->devnotes;
            img.needMetaWrite = true;
        }
    //------
    if (meta->a_scanner)
        for (auto &img : images) {
            img.imgMeta.scanner = meta->scanner;
            img.needMetaWrite = true;
        }
    //------
    if (meta->a_scannotes)
        for (auto &img : images) {
            img.imgMeta.scanNotes = meta->scannotes;
            img.needMetaWrite = true;
        }
    //------
    for (auto &img : images)
        img.needMetaWrite = true;

    std::memset(meta, 0, sizeof(metaBuff));

}
