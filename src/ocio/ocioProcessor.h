#ifndef _ocioprocessor_h
#define _ocioprocessor_h

#include "structs.h"
#include <string>
#include <sstream>
#include <thread>
#include <vector>

// OCIO
#include <OpenColorIO/OpenColorIO.h>
namespace OCIO = OCIO_NAMESPACE;

#include "logger.h"

struct ocioConfig {
    std::string configName;
    OCIO::ConstConfigRcPtr config;
    std::vector<char*> colorspaces;
    std::vector<char*> displays;
    std::vector<std::vector<char*>> views;
    std::vector<char*> looks;
};

class ocioProcessor {
    public:
    ocioProcessor(){};
    ~ocioProcessor(){};

    bool initialize(std::string &configFile);
    bool addConfig(std::string &configFile, std::string configName);
    bool initExtConfig(std::string path);

    void setActiveConfig(int id);
    ocioConfig* activeConfig(){return &m_configs[selectedConfig];}
    std::vector<char*> getConfigList();
    std::vector<std::string> getConfigNames();

    void processImage(float* img, unsigned int width, unsigned int height, ocioSetting &ocioSet);
    void refGamutCompress(float* img, unsigned int width, unsigned int height);
    OCIO::GpuShaderDescRcPtr getGLDesc(ocioSetting& ocioSet);

    //std::vector<char*> colorspaces;
    //std::vector<char*> displays;
    //std::vector<std::vector<char*>> views;


    bool validExternal = false;
    int selectedConfig = 0;

    private:
    void loadNames(ocioConfig& config);

    std::vector<ocioConfig> m_configs;

    //bool useExt = false;
    //std::string internalConfig;
    std::string externalConfigPath;
    //OCIO::ConstConfigRcPtr OCIOconfig;
    //OCIO::ConstConfigRcPtr extOCIOconfig;
};

// OCIO
extern ocioProcessor ocioProc;


#endif
