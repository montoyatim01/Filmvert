#include "tether.h"
#include "logger.h"
#include "image.h"
#include "ocioProcessor.h"

#include <chrono>
#include <exception>
#include <gphoto2-abilities-list.h>
#include <gphoto2-camera.h>
#include <gphoto2-context.h>
#include <gphoto2-widget.h>
#include <OpenImageIO/imageio.h>
#include <OpenImageIO/filesystem.h>

#include <string>
#include <iostream>
#include <filesystem>
#include <thread>

fvTether gblTether;

fvTether::~fvTether() {
    if (streaming)
        stopLiveView();
    gp_list_unref(list);
    gp_context_unref(context);
}

bool fvTether::initialize() {
    // Set the requisite environment variables
    // for the libgphoto2 libraries
    #ifdef __APPLE__
    char pathBuffer[1024];
    uint32_t size = sizeof(pathBuffer);
    if (_NSGetExecutablePath(pathBuffer, &size) == 0) {
            std::string executablePath(pathBuffer);
            std::filesystem::path execPath(executablePath);
            std::filesystem::path bundlePath = execPath.parent_path().parent_path().parent_path();

            char iolibs_path[1024];
            char camlibs_path[1024];
            snprintf(iolibs_path, sizeof(iolibs_path), "%s/Contents/Frameworks/libgphoto2_port/0.12.2", bundlePath.string().c_str());
            snprintf(camlibs_path, sizeof(camlibs_path), "%s/Contents/Frameworks/libgphoto2/2.5.31", bundlePath.string().c_str());

            setenv("IOLIBS", iolibs_path, 1);
            setenv("CAMLIBS", camlibs_path, 1);

    } else {
        // Could not accurately get buffer
        init = false;
        return false;
    }

    context = gp_context_new();
    if (!context) {
        LOG_ERROR("Unable to setup tether context!");
        init = false;
        return false;
    }
    gp_list_new(&list);
    return true;

    #endif
}

void fvTether::detectCameras() {
    camList.clear();
    gp_list_reset(list);
    int ret = gp_camera_autodetect(list, context);
    if (ret < GP_OK) {
        LOG_ERROR("Camera autodetect failed: {}", gp_result_as_string(ret));
        return;
    }
    int count = gp_list_count(list);

    LOG_INFO("Found {} camera(s):\n", count);

    // Print detected cameras
    const char *name, *value;
    for (int i = 0; i < count; i++) {
        gp_list_get_name(list, i, &name);
        gp_list_get_value(list, i, &value);
        LOG_INFO("Camera {}: {} on port {}\n", i+1, name, value);
        camList.push_back(name);
    }
}

void fvTether::connectCamera(int idx) {
    int count = gp_list_count(list);
    if (idx < 0 || idx >= count) {
        LOG_WARN("Invalid camera index: {}", idx);
        return;
    }
    connected = false;
    if (camera) {
        gp_camera_exit(camera, context);
        gp_camera_free(camera);
    }
    gp_camera_new(&camera);

    fvCam.rst();
    const char *model, *port;
    int ret;

    // Get camera model and port from the list
    ret = gp_list_get_name(list, idx, &model);
    if (ret < GP_OK) return;
    fvCamInfo.model = model;

    ret = gp_list_get_value(list, idx, &port);
    if (ret < GP_OK) return;

    // Set the camera model and port
    CameraAbilitiesList *abilities_list;
    int model_index;

    // Get abilities list
    ret = gp_abilities_list_new(&abilities_list);
    if (ret < GP_OK) return;

    ret = gp_abilities_list_load(abilities_list, context);
    if (ret < GP_OK) {
        gp_abilities_list_free(abilities_list);
        return;
    }

    // Find model in abilities list
    model_index = gp_abilities_list_lookup_model(abilities_list, model);
    if (model_index < GP_OK) {
        gp_abilities_list_free(abilities_list);
        return;
    }

    // Get abilities for this model
    ret = gp_abilities_list_get_abilities(abilities_list, model_index, &abilities);
    if (ret < GP_OK) {
        gp_abilities_list_free(abilities_list);
        return;
    }

    // Set camera abilities
    ret = gp_camera_set_abilities(camera, abilities);
    if (ret < GP_OK) {
        gp_abilities_list_free(abilities_list);
        return;
    }

    gp_abilities_list_free(abilities_list);

    // Set port
    GPPortInfoList *port_list;
    GPPortInfo port_info;
    int port_index;

    ret = gp_port_info_list_new(&port_list);
    if (ret < GP_OK) return;

    ret = gp_port_info_list_load(port_list);
    if (ret < GP_OK) {
        gp_port_info_list_free(port_list);
        return;
    }

    port_index = gp_port_info_list_lookup_path(port_list, port);
    if (port_index < GP_OK) {
        LOG_ERROR("Port not found in port list");
        gp_port_info_list_free(port_list);
        return;
    }

    ret = gp_port_info_list_get_info(port_list, port_index, &port_info);
    if (ret < GP_OK) {
        gp_port_info_list_free(port_list);
        return;
    }

    ret = gp_camera_set_port_info(camera, port_info);
    if (ret < GP_OK) {
        gp_port_info_list_free(port_list);
        return;
    }

    gp_port_info_list_free(port_list);

    // Finally, initialize the camera
    ret = gp_camera_init(camera, context);
    if (ret < GP_OK) {
        LOG_ERROR("Failed to initialize camera: {}", gp_result_as_string(ret));
        return;
    }
    if (abilities.operations & GP_OPERATION_TRIGGER_CAPTURE)
        fvCam.trigger = true;
    if (abilities.operations & GP_OPERATION_CONFIG)
        fvCam.config = true;
    if (abilities.operations & GP_OPERATION_CAPTURE_PREVIEW)
        fvCam.preview = true;
    if (abilities.operations & GP_OPERATION_CAPTURE_IMAGE)
        fvCam.capture = true;
    if (abilities.operations & GP_OPERATION_CAPTURE_VIDEO)
        fvCam.video = true;
    listSettings();

    // Type 5
    setupParam(&fvCamInfo.p_iso, "iso");
    setupParam(&fvCamInfo.p_aperture, "aperture");
    setupParam(&fvCamInfo.p_shutterSpeed, "shutterspeed");
    setupParam(&fvCamInfo.p_whiteBalance, "whitebalance");
    setupParam(&fvCamInfo.p_colorTemp, "colortemperature");
    setupParam(&fvCamInfo.p_output, "output");
    setupParam(&fvCamInfo.p_evfMode, "evfmode");
    setupParam(&fvCamInfo.p_capturetarget, "capturetarget");
    setupParam(&fvCamInfo.p_imageFormat, "imageformat");
    setupParam(&fvCamInfo.p_wbAdjA, "whitebalanceadjusta");
    setupParam(&fvCamInfo.p_wbAdjB, "whitebalanceadjustb");
    setupParam(&fvCamInfo.p_wbXa, "whitebalancexa");
    setupParam(&fvCamInfo.p_wbXb, "whitebalancexb");
    setupParam(&fvCamInfo.p_colorspace, "colorspace");
    setupParam(&fvCamInfo.p_focusMode, "focusmode");
    setupParam(&fvCamInfo.p_continuousAF, "continuousaf");
    setupParam(&fvCamInfo.p_afmethod, "afmethod");
    setupParam(&fvCamInfo.p_driveMode, "drivemode");
    setupParam(&fvCamInfo.p_meterMode, "meteringmode");
    setupParam(&fvCamInfo.p_lvSize, "liveviewsize");
    setupParam(&fvCamInfo.p_manualFocusDrive, "manualfocusdrive");

    // Type 4
    setupParam(&fvCamInfo.p_autoFocusDrive, "autofocusdrive");
    setupParam(&fvCamInfo.p_viewfinder, "viewfinder");
    setupParam(&fvCamInfo.p_capture, "capture");
    setupParam(&fvCamInfo.p_uiLock, "uilock");

    fvCamInfo.battery = getParamVal("batterylevel");


    connected = true;
}

void fvTether::disconnectCamera() {
    if (streaming)
        stopLiveView();
    if (!camera || !context) {
            return;
    }

    int ret = gp_camera_exit(camera, context);
    if (ret < GP_OK) {
        LOG_WARN("Warning: Camera exit failed: {}", gp_result_as_string(ret));
    }
    gp_camera_free(camera);
    camera = nullptr;
    connected = false;
}


void printWidget(CameraWidget *widget, const std::string &indent) {
    const char *name, *label;
    CameraWidgetType type;

    gp_widget_get_name(widget, &name);
    gp_widget_get_label(widget, &label);
    gp_widget_get_type(widget, &type);

    LOG_INFO("Name: {}, Label: {}, Type: {}",
        (name ? name : "NULL"),
        (label ? label : "NULL"),
        (int)type);

    // Recursively print children
    int children = gp_widget_count_children(widget);
    for (int i = 0; i < children; i++) {
        CameraWidget *child;
        gp_widget_get_child(widget, i, &child);
        printWidget(child, indent + "  ");
    }
}

void fvTether::listSettings() {
    CameraWidget *config;
    int ret = gp_camera_get_config(camera, &config, context);

    if (ret < GP_OK) {
        LOG_WARN("Failed to get config: {}", gp_result_as_string(ret));
        return;
    }

        // Print all available settings
        printWidget(config, "");

        gp_widget_free(config);
}

void fvTether::setValue(const char* key, const char* val) {
    if (!connected) {
        LOG_INFO("Cam Not Connected!");
        return;
    }
    LOG_INFO("Setting Config: {}, Value: {}", key, val);

    CameraWidget *config, *widget;

    int ret = gp_camera_get_config(camera, &config, context);
    if (ret < GP_OK) return;

    ret = gp_widget_get_child_by_name(config, key, &widget);
    if (ret < GP_OK) {
        LOG_WARN("Failed to get widget: {}", gp_result_as_string(ret));
        gp_widget_free(config);
        return;
    }

    ret = gp_widget_set_value(widget, val);
    if (ret < GP_OK) {
        LOG_WARN("Failed to set value: {}", gp_result_as_string(ret));
        gp_widget_free(config);
        return;
    }
    ret = gp_camera_set_config(camera, config, context);
    if (ret < GP_OK) {
        LOG_WARN("Failed to send config: {}", gp_result_as_string(ret));
    }
    gp_widget_free(config);
}

/*
iso
whitebalance
colortemperature
capturesettings
focusmode
continuousaf
afmethod
highisonr
autoexposuremode
autoexposuremodedial
drivemode
aperture
shutterspeed
meteringmode
liveviewsize
batterylevel
*/

void fvTether::startLiveView() {
    CameraWidget *config, *liveview_widget;
    int ret;

    // Get camera configuration
    ret = gp_camera_get_config(camera, &config, context);
    if (ret < GP_OK) return;

    // Try to find and enable live view setting
    // Setting names vary by manufacturer: "eosviewfinder", "liveview", "viewfinder"
    const char* liveview_names[] = {"eosviewfinder", "liveview", "viewfinder", "d7000viewfinder"};

    for (const char* name : liveview_names) {
        ret = gp_widget_get_child_by_name(config, name, &liveview_widget);
        if (ret >= GP_OK) {
            // Enable live view (usually "1" or "On")
            ret = gp_widget_set_value(liveview_widget, "1");
            if (ret >= GP_OK) {
                ret = gp_camera_set_config(camera, config, context);
                if (ret >= GP_OK) {
                    LOG_INFO("Live view enabled using: {}", name);
                    streaming = true;
                    captureThread = std::thread([this]{
                        capThread();
                    });
                    gp_widget_free(config);
                    return;
                }
            }
        }
    }

    gp_widget_free(config);
    LOG_ERROR("Failed to enable live view: {}", gp_result_as_string(ret));
    return;
}

void fvTether::stopLiveView() {
    if (!streaming) return;

    CameraWidget *config, *liveview_widget;
    int ret;

    ret = gp_camera_get_config(camera, &config, context);
    if (ret < GP_OK) return;

    const char* liveview_names[] = {"eosviewfinder", "liveview", "viewfinder", "d7000viewfinder"};

    for (const char* name : liveview_names) {
        ret = gp_widget_get_child_by_name(config, name, &liveview_widget);
        if (ret >= GP_OK) {
            ret = gp_widget_set_value(liveview_widget, "0");
            if (ret >= GP_OK) {
                gp_camera_set_config(camera, config, context);
                LOG_INFO("Live view disabled");
                streaming = false;
                if (captureThread.joinable())
                    captureThread.join();
                break;
            }
        }
    }

    gp_widget_free(config);
}

void fvTether::saveToFile() {
    CameraFile *file;
    int ret;

    if (!streaming) {
        LOG_ERROR("Live view not enabled");
        return;
    }

    // Create camera file object
    ret = gp_file_new(&file);
    if (ret < GP_OK) return;

    // Capture preview frame
    ret = gp_camera_capture_preview(camera, file, context);
    if (ret < GP_OK) {
        gp_file_free(file);
        return;
    }

    // Save to file
    std::string filename = "/Users/Shared/Filmvert/test01";
    ret = gp_file_save(file, filename.c_str());
    gp_file_free(file);

    return;
}

void fvTether::processCapturePreview() {
    capFrame = false;
    if (!lvImage) {
        LOG_ERROR("Invalid display image!");
        stopLiveView();
        return;
    }
    if (!imMutex.try_lock()) {
        // Couldn't lock due to GPU
        // Skip frame and catch the next one
        //LOG_INFO("Skipping because mutex lock");
        return;
    }
    CameraFile *file;
    int ret;

    if (!streaming){
        imMutex.unlock();
        return;
    }

    ret = gp_file_new(&file);
    if (ret < GP_OK) {
        imMutex.unlock();
        return;
    }

    ret = gp_camera_capture_preview(camera, file, context);
    if (ret < GP_OK) {
        LOG_ERROR("Unable to get cap preview");
        gp_file_free(file);
        imMutex.unlock();
        return;
    }
    const char* data;
    unsigned long size;

    // Get data pointer (don't free this manually)
    ret = gp_file_get_data_and_size(file, &data, &size);

    auto in = OIIO::ImageInput::create("jpeg");
    if (! in  ||  ! in->supports ("ioproxy")) {
        LOG_ERROR("OpenImageIO Logo Read Error");
        imMutex.unlock();
        stopLiveView();
        return;
    }
    OIIO::Filesystem::IOMemReader memreader(data, size);  // I/O proxy object

    auto input = OIIO::ImageInput::open("frame.jpeg", nullptr, &memreader);
    if (!input)
    {
        LOG_ERROR("Failed to create ImageInput: " + OIIO::geterror());
        imMutex.unlock();
        stopLiveView();
        return;
    }

    // Get image specification
    const OIIO::ImageSpec& spec = input->spec();
    int width = spec.width;
    int height = spec.height;
    int channels = spec.nchannels;

    if ((lvImage->width * lvImage->height) <
        (width * height)) {
        if (lvImage->rawImgData)
            delete[] lvImage->rawImgData;
        lvImage->rawImgData = new float[width * height * 4];
    }

    lvImage->width = width;
    lvImage->height = height;
    lvImage->rawWidth = width;
    lvImage->rawHeight = height;
    lvImage->nChannels = channels;
    // Read the image data
    if (!input->read_image(OIIO::TypeDesc::FLOAT, lvImage->rawImgData))
    {
        LOG_ERROR("Failed to read image data: " + input->geterror());
        input->close();
        imMutex.unlock();
        return;
    }
    input->close();
    lvImage->padToRGBA();
    //lvImage->setCrop();
    //ocioProc.processImage(lvImage->rawImgData, lvImage->width, lvImage->height, lvImage->intOCIOSet);
    lvImage->imageLoaded = true;
    lvImage->needRndr = true;
    lvImage->imgRst = true;
    lvImage->isTetherLive = true;
    //LOG_INFO("Processed image with width: {}, height: {}, channels: {}", width, height, channels);
    imMutex.unlock();

    gp_file_free(file);

    capFrame = false;

    return;
}

void fvTether::capThread() {
    while (streaming) {
        if (capFrame)
            processCapturePreview();
        else
            std::this_thread::sleep_for(std::chrono::milliseconds(4));
    }
}

void fvTether::capTrigger() {
    if (streaming)
        capFrame = true;
}

bool fvTether::testCapture(const std::string &rollDir) {
    capFrame = false;
    std::string filename = rollDir + "/tempCap";
    CameraFile *file;
    CameraFilePath camera_file_path;
    int ret;

    LOG_INFO("Capturing image...");

    // Trigger capture - this stores the image on camera
    ret = gp_camera_capture(camera, GP_CAPTURE_IMAGE, &camera_file_path, context);
    if (ret < GP_OK) {
        LOG_ERROR("Failed to capture image: {}", gp_result_as_string(ret));
        return false;
    }

    LOG_INFO("Image captured to camera: {}/{}",
        camera_file_path.folder,
        camera_file_path.name);

    // Create file object
    ret = gp_file_new(&file);
    if (ret < GP_OK) return false;

    // Download the image from camera to file object
    ret = gp_camera_file_get(camera, camera_file_path.folder, camera_file_path.name,
                            GP_FILE_TYPE_NORMAL, file, context);
    if (ret < GP_OK) {
        LOG_ERROR("Failed to download image: {}", gp_result_as_string(ret));
        gp_file_free(file);
        return false;
    }

    // Save to disk
    ret = gp_file_save(file, filename.c_str());
    if (ret < GP_OK) {
        LOG_ERROR("Failed to save image: {}", gp_result_as_string(ret));
    } else {
        LOG_INFO("Image saved as: {}", filename);
    }

    // Clean up - delete image from camera (optional)
    gp_camera_file_delete(camera, camera_file_path.folder, camera_file_path.name, context);

    gp_file_free(file);
    return true;
}
