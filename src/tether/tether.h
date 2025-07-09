#ifndef _tether_h
#define _tether_h
#include "image.h"
#include <gphoto2-widget.h>
#include <gphoto2/gphoto2-camera.h>
#include <thread>
#include <vector>

#ifdef __APPLE__
#include <mach-o/dyld.h>
#endif

struct fvCamAbilities {
    bool config = false;
    bool trigger = false;
    bool preview = false;
    bool capture = false;
    bool video = false;

    void rst() {config = trigger = preview = capture = video = false;}
};

struct fvCamParam {
    CameraWidgetType paramType;
    std::string paramName = "";
    const char* paramVal;
    std::vector<const char*> validValues;
    int selVal = 0;
    bool validParam = false;
};

struct fvCamInfo {
    std::string model;
    const char* battery;

    // Type 5 Radio
    fvCamParam p_iso;
    fvCamParam p_aperture;
    fvCamParam p_shutterSpeed;
    fvCamParam p_whiteBalance;
    fvCamParam p_colorTemp;

    fvCamParam p_output;
    fvCamParam p_evfMode;
    fvCamParam p_capturetarget;
    fvCamParam p_imageFormat;
    fvCamParam p_lvSize;

    fvCamParam p_wbAdjA;
    fvCamParam p_wbAdjB;
    fvCamParam p_wbXa;
    fvCamParam p_wbXb;

    fvCamParam p_colorspace;
    fvCamParam p_focusMode;
    fvCamParam p_continuousAF;
    fvCamParam p_afmethod;
    fvCamParam p_driveMode;
    fvCamParam p_meterMode;
    fvCamParam p_manualFocusDrive;


    // Type 4 Toggle
    fvCamParam p_autoFocusDrive;
    fvCamParam p_viewfinder;
    fvCamParam p_capture;
    fvCamParam p_uiLock;

};

class fvTether {
    public:
        fvTether(){}
        ~fvTether();
        bool initialize();

        void detectCameras();
        void connectCamera(int idx);
        void disconnectCamera();
        void listSettings();



        void setValue(const char* key, const char* val);

        void startLiveView();
        void stopLiveView();
        void saveToFile();
        void setImage(image* im){if (im) lvImage = im;}
        void capTrigger();
        bool testCapture(const std::string &rollDir);

    private:
        bool init = false;
        GPContext *context = nullptr;
        CameraList *list = nullptr;
        Camera *camera = nullptr;
        CameraAbilities abilities;
        fvCamAbilities fvCam;
        image* lvImage = nullptr;
        std::thread captureThread;
        bool capFrame = false;

    private:
        void capThread();
        void processCapturePreview();


        // tetherParam.cpp
        void setupParam(fvCamParam* param, std::string paramName);
        CameraWidgetType getParamType(const char* key);
        const char* getParamVal(const char* paramName);
        void matchParam(fvCamParam* param);
        void listChoiceValues(fvCamParam* param);

    public:
        std::mutex imMutex;
        std::vector<const char*> camList;
        int selCam = 0;
        bool connected = false;
        bool streaming = false;
        fvCamInfo fvCamInfo;
        int mfStepSize = 1;

};

extern fvTether gblTether;

#endif

// TODO:
// - Fix the UI (hide parameters we don't need)
// -* Do complicated setup with child widget (does sameLine work?)
// -* with custom settings (width/height too)
// - Leave the save destination though (change names?)
// - Add in connection checking
// -- If disconnected, force live mode to stop!
// - Full capture vs temp capture
// -- Saving the full files vs the temp
// - Focusing!
// -- How does manual focus work
// -- How do the autofocus settings work
// - Fix the buttons up top
//
// - How does the tethering work across multiple rolls?
// - Make sure we maintain valid state
// - Does live view disable when switching rolls?
// -* Crash from the 'delete [] rawImgData' when re-activating live view
// -* after a camera reconnect

/*
/main/actions/eosremoterelease
Label: Canon EOS Remote Release
Readonly: 0
Type: RADIO
Current: None
Choice: 0 None
Choice: 1 Press Half
Choice: 2 Press Full
Choice: 3 Release Half
Choice: 4 Release Full
Choice: 5 Immediate
Choice: 6 Press 1
Choice: 7 Press 2
Choice: 8 Press 3
Choice: 9 Release 1
Choice: 10 Release 2
Choice: 11 Release 3
END



*/
