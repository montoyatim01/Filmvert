#include "roll.h"


//--- Clear Buffers ---//
/*
    Loop through all images and unload their buffers.
    Only happens if the roll is fully loaded, and if
    the user has performance mode enabled, or is removing
    the roll
*/
bool filmRoll::clearBuffers(bool remove) {
    if (imagesLoading) {
        rollDumpTimer = std::chrono::steady_clock::now();
        return false; // User is jumping back and forth between rolls
    }
    if (!remove) {
        // Only check the time if we're not trying to remove
        if (!rollDumpCall) {
            // We have not previously called for a roll dump
            // Start the timer and set our flag
            rollDumpCall = true;
            rollDumpTimer = std::chrono::steady_clock::now();
            return false;
        } else {
            // Check the time, return if too soon
            auto now = std::chrono::steady_clock::now();
            auto dur = std::chrono::duration_cast<std::chrono::seconds>(now - rollDumpTimer);
            if (dur.count() < appPrefs.prefs.rollTimeout) {
                // We haven't hit the limit, keep the buffers for now
                return false;
            }
            LOG_INFO("Clearing the {} buffers!", rollName);
        }
    }

    if (!appPrefs.prefs.perfMode && !remove)
        return false; // User does not have performance mode enabled
    for (int i = 0; i < images.size(); i++) {
        images[i].clearBuffers();
    }
    rollLoaded = false;
    return true;
}

//--- Load Buffers ---//
/*
    Loop through all valid images, and re-load
    the image data back from disk
*/
void filmRoll::loadBuffers() {
    rollDumpTimer = std::chrono::steady_clock::now();
    rollDumpCall = false;
    if (rollLoaded) {
        return;
    }
    imagesLoading = true;
    std::thread([this]() {
        auto start = std::chrono::steady_clock::now();
        std::vector<std::future<void>> futures;

        for (image& img : images) {
            futures.push_back(tPool->submit([&img]() {
                img.loadBuffers();
            }));
        }

        for (auto& f : futures) {
            f.get();  // Wait for job to finish
        }
        auto end = std::chrono::steady_clock::now();
        auto dur = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        LOG_INFO("-----------{} Roll Load Time-----------", rollName);
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
        std::vector<std::future<void>> futures;

        for (image& img : images) {
            if (!img.imageLoaded) {
                futures.push_back(tPool->submit([&img]() {
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
