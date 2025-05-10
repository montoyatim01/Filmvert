#include "roll.h"
#include "imageIO.h"
#include "nlohmann/json_fwd.hpp"
#include "threadPool.h"
#include "logger.h"

image* filmRoll::selImage() {
    if (validIm())
        return &images[selIm];
    return nullptr;
}

image* filmRoll::getImage(int index) {
    if (index >= 0 && index < images.size()) {
        return &images[index];
    }
    //LOG_WARN("Roll: Index out of range: {}, {}", index, images.size());
    return nullptr;
}

void filmRoll::clearBuffers() {
    if (imagesLoading)
        return; // User is jumping back and forth between rolls
    for (int i = 0; i < images.size(); i++) {
        images[i].clearBuffers();
    }
    rollLoaded = false;
}

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

void filmRoll::saveAll() {
    for (int i = 0; i < images.size(); i++) {
        images[i].writeXMPFile();
    }
}
//TODO IMPORT ROLL JSON
//TODO DEFAULT ROLL SAVE DIRECTORY
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

bool filmRoll::sdlUpdating() {
    bool updating = false;
    for (auto &img : images) {
        updating |= img.sdlUpdate;
    }
    return updating;
}

bool filmRoll::validIm() {
    return images.size() > 0 && selIm >= 0 && selIm < images.size();
}
