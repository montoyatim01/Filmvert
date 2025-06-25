#ifndef _roll_h_
#define _roll_h_

#include "image.h"
#include "imageMeta.h"
#include "state.h"

#include "logger.h"
#include "preferences.h"
#include "structs.h"
#include "threadPool.h"

#include "nlohmann/json.hpp"

#include <vector>
#include <deque>
#include <cstring>
#include <fstream>

struct MetaImpSet {
    image* img;
    bool selected;
    std::string imName;
    std::string json;

    MetaImpSet(image* _img, bool sel, std::string _imName, std::string _json)
    : img(_img),
    selected(sel),
    imName(_imName),
    json(_json){}
};


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

    bool unsavedImages() const;
    bool unsavedIndividual();

    bool sortRoll();

    // rollBuffers.cpp
    bool clearBuffers(bool remove = false);
    void loadBuffers();
    void checkBuffers();
    void closeSelected();

    // rollIO.cpp
    void saveAll();
    void saveSelected();

    // rollMeta.cpp
    std::string getRollMetaString(bool pretty = false);
    bool exportRollMetaJSON();
    bool exportRollCSV();
    bool importRollMetaJSON(const std::string& jsonFile);
    void applyRollMetaJSON(bool params, copyPaste impOpt);
    bool applySelMetaJSON(std::string inFile, copyPaste impOpt);
    void rollMetaPreEdit(metaBuff* meta);
    void rollMetaPostEdit(metaBuff* meta);

    // rollState.cpp
    void rollUpState();
    void rollUndo();
    void rollRedo();
    bool rollUAvail();
    bool rollRAvil();

    // rollContactSheet.cpp
    void generateContactSheet(int imageWidth, exportParam expParam);



    // vector for importing JSON metadata
    // and linking to images
    std::vector<MetaImpSet> metaImp;

    // Timer for checking if it's been
    // enough time since request for roll-dump
    // for us to actually dump it (prevents rapid dump/reload)
    std::chrono::time_point<std::chrono::steady_clock> rollDumpTimer;
    bool rollDumpCall = false;

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
