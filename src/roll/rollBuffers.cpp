#include "roll.h"


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
