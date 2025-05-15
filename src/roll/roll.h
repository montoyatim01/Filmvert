#ifndef _roll_h_
#define _roll_h_

#include <vector>
#include <deque>
#include "image.h"
#include "imageMeta.h"


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

    void clearBuffers(bool remove = false);
    void loadBuffers();
    void checkBuffers();
    void closeSelected();

    void saveAll();
    void saveSelected();
    bool exportRollMetaJSON();
    bool exportRollCSV();
    bool importRollMetaJSON(const std::string& jsonFile);

    bool unsavedImages();
    bool unsavedIndividual();

    void rollMetaPreEdit(metaBuff* meta);
    void rollMetaPostEdit(metaBuff* meta);



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
