#include "roll.h"
#include "image.h"
#include "imageMeta.h"
#include "nlohmann/json_fwd.hpp"
#include "preferences.h"
#include "threadPool.h"
#include "logger.h"
#include <cstring>
#include <fstream>

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

//--- Clear Buffers ---//
/*
    Loop through all images and unload their buffers.
    Only happens if the roll is fully loaded, and if
    the user has performance mode enabled, or is removing
    the roll
*/
void filmRoll::clearBuffers(bool remove) {
    if (imagesLoading)
        return; // User is jumping back and forth between rolls
    if (!appPrefs.prefs.perfMode && !remove)
        return; // User does not have performance mode enabled
    for (int i = 0; i < images.size(); i++) {
        images[i].clearBuffers();
    }
    rollLoaded = false;
}

//--- Load Buffers ---//
/*
    Loop through all valid images, and re-load
    the image data back from disk
*/
void filmRoll::loadBuffers() {
    imagesLoading = true;
    std::thread([this]() {
        auto start = std::chrono::steady_clock::now();
        ThreadPool pool(std::thread::hardware_concurrency());
        std::vector<std::future<void>> futures;

        for (image& img : images) {
            futures.push_back(pool.submit([&img]() {
                img.loadBuffers();
            }));
        }

        for (auto& f : futures) {
            f.get();  // Wait for job to finish
        }
        auto end = std::chrono::steady_clock::now();
        auto dur = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        LOG_INFO("-----------Roll Load Time-----------");
        LOG_INFO("{:*>8}μs | {:*>8}ms", dur.count(), dur.count()/1000);
        imagesLoading = false; // Set only after all are done
        rollLoaded = true;
    }).detach();
}

//--- Check Buffers ---//
/*
    Check for if any images need to be
    re-loaded after save
*/
void filmRoll::checkBuffers() {
    std::thread([this]() {
        ThreadPool pool(std::thread::hardware_concurrency());
        std::vector<std::future<void>> futures;

        for (image& img : images) {
            if (!img.imageLoaded) {
                futures.push_back(pool.submit([&img]() {
                    img.loadBuffers();
                }));
            }

        }

        for (auto& f : futures) {
            f.get();  // Wait for job to finish
        }
        imagesLoading = false; // Set only after all are done
        rollLoaded = true;
    }).detach();
}

//--- Close Selected ---//
/*
    Close the selected images and
    remove them from the roll
*/
void filmRoll::closeSelected() {
    for (auto it = images.begin(); it != images.end();) {
        if (it->selected) {
            it->clearBuffers();
            it = images.erase(it);  // erase() returns iterator to next element
        } else {
            ++it;
        }
    }
}

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

//--- Export Roll Metadata JSON ---//
/*
    Export the full JSON file with metadata and params
    from each image in the roll.
*/
bool filmRoll::exportRollMetaJSON() {
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
        std::string jDump = j.dump(4);
        std::string outPath = rollPath + "/" + rollName + ".json";
        std::ofstream jsonFile(outPath, std::ios::out | std::ios::trunc);
        if (!jsonFile) {
            LOG_ERROR("Unable to open JSON file for roll metadata export!");
            return false;
        }
        jsonFile << jDump;
        jsonFile.close();
        return true;
    } catch (const std::exception& e) {
        LOG_WARN("Unable to format roll JSON: {}", e.what());
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
    csvFile << "DevelopmentNotes,Scanner,ScanNotes\n";

    for (int i = 0; i < images.size(); i++) {
        csvFile << images[i].imMeta.frameNumber << ",";
        csvFile << images[i].srcFilename << ",";
        csvFile << images[i].imMeta.cameraMake << ",";
        csvFile << images[i].imMeta.cameraModel << ",";
        csvFile << images[i].imMeta.lens << ",";
        csvFile << images[i].imMeta.filmStock << ",";
        csvFile << images[i].imMeta.focalLength << ",";
        csvFile << images[i].imMeta.fNumber << ",";
        csvFile << images[i].imMeta.exposureTime << ",";
        csvFile << images[i].imMeta.rollName << ",";
        csvFile << images[i].imMeta.dateTime << ",";
        csvFile << images[i].imMeta.location << ",";
        csvFile << images[i].imMeta.gps << ",";
        csvFile << images[i].imMeta.notes << ",";
        csvFile << images[i].imMeta.devProcess << ",";
        csvFile << images[i].imMeta.chemMfg << ",";
        csvFile << images[i].imMeta.devNotes << ",";
        csvFile << images[i].imMeta.scanner << ",";
        csvFile << images[i].imMeta.scanNotes << "\n";
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
     try {
        nlohmann::json jIn;
        std::ifstream f(jsonFile);
        if (!f)
            return false;
        jIn = nlohmann::json::parse(f);
        auto first_item = jIn.begin();
        rollName = first_item.key();
        nlohmann::json jRoll = first_item.value();
        for (int i = 0; i < images.size(); i++) {
            if (jRoll.contains(images[i].srcFilename)) {
                nlohmann::json obj = jRoll[images[i].srcFilename].get<nlohmann::json>();
                images[i].loadMetaFromStr(obj.dump(-1));
            }
        }
        return true;

    } catch (const std::exception& e) {
        LOG_WARN("Unable to import roll from JSON file: {}", e.what());
        return false;
    }

}

//--- Unsaved Images ---//
/*
    Determine whether there are any images
    with unsaved changes
*/
bool filmRoll::unsavedImages() {
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
            return obj.imMeta.cameraMake == images.front().imMeta.cameraMake;
        });
    if (!meta->dif_camMake)
        std::strcpy(meta->camMake, images[0].imMeta.cameraMake.c_str());
    //------
    meta->dif_camModel = !std::all_of(images.begin() + 1, images.end(),
        [&](const image& obj) {
            return obj.imMeta.cameraModel == images.front().imMeta.cameraModel;
        });
    if (!meta->dif_camModel)
        std::strcpy(meta->camModel, images[0].imMeta.cameraModel.c_str());
    //------
    meta->dif_lens = !std::all_of(images.begin() + 1, images.end(),
        [&](const image& obj) {
            return obj.imMeta.lens == images.front().imMeta.lens;
        });
    if (!meta->dif_lens)
        std::strcpy(meta->lens, images[0].imMeta.lens.c_str());
    //------
    meta->dif_film = !std::all_of(images.begin() + 1, images.end(),
        [&](const image& obj) {
            return obj.imMeta.filmStock == images.front().imMeta.filmStock;
        });
    if (!meta->dif_film)
        std::strcpy(meta->film, images[0].imMeta.filmStock.c_str());
    //------
    meta->dif_focal = !std::all_of(images.begin() + 1, images.end(),
        [&](const image& obj) {
            return obj.imMeta.focalLength == images.front().imMeta.focalLength;
        });
    if (!meta->dif_focal)
        std::strcpy(meta->focal, images[0].imMeta.focalLength.c_str());
    //------
    meta->dif_date = !std::all_of(images.begin() + 1, images.end(),
        [&](const image& obj) {
            return obj.imMeta.dateTime == images.front().imMeta.dateTime;
        });
    if (!meta->dif_date)
        std::strcpy(meta->date, images[0].imMeta.dateTime.c_str());
    //------
    meta->dif_loc = !std::all_of(images.begin() + 1, images.end(),
        [&](const image& obj) {
            return obj.imMeta.location == images.front().imMeta.location;
        });
    if (!meta->dif_loc)
        std::strcpy(meta->loc, images[0].imMeta.location.c_str());
    //------
    meta->dif_gps = !std::all_of(images.begin() + 1, images.end(),
        [&](const image& obj) {
            return obj.imMeta.gps == images.front().imMeta.gps;
        });
    if (!meta->dif_gps)
        std::strcpy(meta->gps, images[0].imMeta.gps.c_str());
    //------
    meta->dif_notes = !std::all_of(images.begin() + 1, images.end(),
        [&](const image& obj) {
            return obj.imMeta.notes == images.front().imMeta.notes;
        });
    if (!meta->dif_notes)
        std::strcpy(meta->notes, images[0].imMeta.notes.c_str());
    //------
    meta->dif_dev = !std::all_of(images.begin() + 1, images.end(),
        [&](const image& obj) {
            return obj.imMeta.devProcess == images.front().imMeta.devProcess;
        });
    if (!meta->dif_dev)
        std::strcpy(meta->dev, images[0].imMeta.devProcess.c_str());
    //------
    meta->dif_chem = !std::all_of(images.begin() + 1, images.end(),
        [&](const image& obj) {
            return obj.imMeta.chemMfg == images.front().imMeta.chemMfg;
        });
    if (!meta->dif_chem)
        std::strcpy(meta->chem, images[0].imMeta.chemMfg.c_str());
    //------
    meta->dif_devnotes = !std::all_of(images.begin() + 1, images.end(),
        [&](const image& obj) {
            return obj.imMeta.devNotes == images.front().imMeta.devNotes;
        });
    if (!meta->dif_devnotes)
        std::strcpy(meta->devnotes, images[0].imMeta.devNotes.c_str());
    //------
    meta->dif_scanner = !std::all_of(images.begin() + 1, images.end(),
        [&](const image& obj) {
            return obj.imMeta.scanner == images.front().imMeta.scanner;
        });
    if (!meta->dif_scanner)
        std::strcpy(meta->scanner, images[0].imMeta.scanner.c_str());
    //------
    meta->dif_scannotes = !std::all_of(images.begin() + 1, images.end(),
        [&](const image& obj) {
            return obj.imMeta.scanNotes == images.front().imMeta.scanNotes;
        });
    if (!meta->dif_scannotes)
        std::strcpy(meta->scannotes, images[0].imMeta.scanNotes.c_str());
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
    if (meta->a_rollPath)
        rollPath = meta->rollPath;
    if (meta->a_rollname)
        rollName = meta->rollname;

    if (images.size() < 1)
        return;

    if (meta->a_rollname)
        for (auto &img : images){
            img.imMeta.rollName = meta->rollname;
            img.needMetaWrite = true;
        }

    //------
    if (meta->a_camMake)
        for (auto &img : images) {
            img.imMeta.cameraMake = meta->camMake;
            img.needMetaWrite = true;
        }
    //------
    if (meta->a_camModel)
        for (auto &img : images) {
            img.imMeta.cameraModel = meta->camModel;
            img.needMetaWrite = true;
        }
    //------
    if (meta->a_lens)
        for (auto &img : images) {
            img.imMeta.lens = meta->lens;
            img.needMetaWrite = true;
        }
    //------
    if (meta->a_film)
        for (auto &img : images) {
            img.imMeta.filmStock = meta->film;
            img.needMetaWrite = true;
        }
    //------
    if (meta->a_focal)
        for (auto &img : images) {
            img.imMeta.focalLength = meta->focal;
            img.needMetaWrite = true;
        }
    //------
    if (meta->a_date)
        for (auto &img : images) {
            img.imMeta.dateTime = meta->date;
            img.needMetaWrite = true;
        }
    //------
    if (meta->a_loc)
        for (auto &img : images) {
            img.imMeta.location = meta->loc;
            img.needMetaWrite = true;
        }
    //------
    if (meta->a_gps)
        for (auto &img : images) {
            img.imMeta.gps = meta->gps;
            img.needMetaWrite = true;
        }
    //------
    if (meta->a_notes)
        for (auto &img : images) {
            img.imMeta.notes = meta-> notes;
            img.needMetaWrite = true;
        }
    //------
    if (meta->a_dev)
        for (auto &img : images) {
            img.imMeta.devProcess = meta->dev;
            img.needMetaWrite = true;
        }
    //------
    if (meta->a_chem)
        for (auto &img : images) {
            img.imMeta.chemMfg = meta->chem;
            img.needMetaWrite = true;
        }
    //------
    if (meta->a_devnotes)
        for (auto &img : images) {
            img.imMeta.devNotes = meta->devnotes;
            img.needMetaWrite = true;
        }
    //------
    if (meta->a_scanner)
        for (auto &img : images) {
            img.imMeta.scanner = meta->scanner;
            img.needMetaWrite = true;
        }
    //------
    if (meta->a_scannotes)
        for (auto &img : images) {
            img.imMeta.scanNotes = meta->scannotes;
            img.needMetaWrite = true;
        }
    //------

    std::memset(meta, 0, sizeof(metaBuff));

}
