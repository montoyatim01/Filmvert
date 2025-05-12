#ifndef _roll_h_
#define _roll_h_

#include <vector>
#include <deque>
#include "imageIO.h"
#include "imageMeta.h"


class filmRoll {

  public:
    filmRoll(){};
    filmRoll(std::string name) : rollName(name) {}
    ~filmRoll(){};

    image* selImage();
    image* getImage(int index);

    void clearBuffers();
    void loadBuffers();

    void saveAll();
    bool exportRollMetaJSON();
    bool exportRollCSV();
    bool importRollMetaJSON(const std::string& jsonFile);

    bool unsavedImages();

    void rollMetaPreEdit(metaBuff* meta);
    void rollMetaPostEdit(metaBuff* meta);

    bool imagesLoading = false;
    bool rollLoaded = false;
    bool selected = false;

    imageMetadata rollMeta;

    bool sdlUpdating();

    std::string rollName;
    std::string rollPath;

    std::deque<image> images;
    int selIm = -1;

    bool validIm();

    private:


};


#endif
