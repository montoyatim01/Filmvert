#ifndef _roll_h_
#define _roll_h_

#include "image.h"
#include "imageMeta.h"
#include "state.h"

#include "logger.h"
#include "preferences.h"
#include "threadPool.h"

#include "nlohmann/json.hpp"

#include <vector>
#include <deque>
#include <cstring>
#include <fstream>


class filmRoll {

  public:
    filmRoll(){};
    filmRoll(std::string name) : rollName(name) {}
    ~filmRoll(){};

    image* selImage();
    image* getImage(int index);
    bool validIm();

    int rollSize();

    void selectAll();
    void clearSelection();

    bool unsavedImages();
    bool unsavedIndividual();

    bool sortRoll();

    // rollBuffers.cpp
    void clearBuffers(bool remove = false);
    void loadBuffers();
    void checkBuffers();
    void closeSelected();

    // rollIO.cpp
    void saveAll();
    void saveSelected();

    // rollMeta.cpp
    bool exportRollMetaJSON();
    bool exportRollCSV();
    bool importRollMetaJSON(const std::string& jsonFile);
    void rollMetaPreEdit(metaBuff* meta);
    void rollMetaPostEdit(metaBuff* meta);

    // rollState.cpp
    void rollUpState();
    void rollUndo();
    void rollRedo();
    bool rollUAvail();
    bool rollRAvil();






    bool imagesLoading = false;
    bool rollLoaded = false;
    bool selected = false;

    imageMetadata rollMeta;


    std::string rollName;
    std::string rollPath;

    std::deque<image> images;
    int selIm = -1;

    private:


};


#endif
