#include "tether.h"
#include <gphoto2-widget.h>

// Initialize our param based on its key
void fvTether::setupParam(fvCamParam* param, std::string paramName) {
    if (!param)
        return;
    // Set our name
    param->paramName = paramName;
    // Block this in a try/catch
    // If any step fails, the param isn't good
    try {
        param->paramType = getParamType(paramName.c_str());
        param->paramVal = getParamVal(paramName.c_str());
        listChoiceValues(param);
        matchParam(param);
    } catch(std::exception &e) {
        LOG_WARN("Error initializing parameter: {}", e.what());
        param->validParam = false;
        return;
    }
    param->validParam = true;
}

CameraWidgetType fvTether::getParamType(const char *key) {
    CameraWidget *config, *widget;
    CameraWidgetType type;
    int ret = gp_camera_get_config(camera, &config, context);
    if (ret < GP_OK) {
        throw std::runtime_error(fmt::format("Unable to get camera config: {}", gp_result_as_string(ret)));
    }

    ret = gp_widget_get_child_by_name(config, key, &widget);
    if (ret < GP_OK) {
        gp_widget_free(config);
        throw std::runtime_error(fmt::format("Failed to get widget: {}", gp_result_as_string(ret)));
    }
    ret = gp_widget_get_type(widget, &type);
    if (ret < GP_OK) {
        gp_widget_free(config);
        throw std::runtime_error(fmt::format("Failed to get widget type: {}", gp_result_as_string(ret)));
    }
    return type;
}

// Get a param's value based on it's key
const char* fvTether::getParamVal(const char* paramName) {
    CameraWidget* paramWid;
    int ret = gp_camera_get_single_config(camera, paramName, &paramWid, context);
    if (ret < GP_OK) {
        throw std::runtime_error(fmt::format("Failed to get param: {}", gp_result_as_string(ret)));
    } else {
        char* param;
        gp_widget_get_value(paramWid, &param);
        return param;
    }
}

// Get the index of the param in the array of valid values
void fvTether::matchParam(fvCamParam* param) {
    if (param->paramType == GP_WIDGET_RADIO) {
        std::string paramS = param->paramVal;
        for (size_t i = 0; i < param->validValues.size(); i++) {
            std::string arrayS = param->validValues[i];
            if (arrayS == paramS) {
                param->selVal = i;
            }
        }
    }
}

void fvTether::listChoiceValues(fvCamParam* param) {
    if (param->paramType == GP_WIDGET_RADIO) {
        CameraWidget *config, *widget;
        int ret = gp_camera_get_config(camera, &config, context);
        if (ret < GP_OK) {
            throw (fmt::format("Failed to get config: {}", gp_result_as_string(ret)));
        }

        ret = gp_widget_get_child_by_name(config, param->paramName.c_str(), &widget);
        if (ret < GP_OK) {
            gp_widget_free(config);
            throw (fmt::format("Failed to get widget: {}", gp_result_as_string(ret)));
        }

        int choice_count = gp_widget_count_choices(widget);

        param->validValues.resize(choice_count);
        for (int i = 0; i < choice_count; i++) {
            const char *choice;
            int ret = gp_widget_get_choice(widget, i, &choice);
            if (ret >= GP_OK) {
                param->validValues[i] = choice;
            }
        }
        return;
    }
}
