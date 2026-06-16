#include "ocioProcessor.h"
#include "OpenColorIO/OpenColorIO.h"
#include "OpenColorIO/OpenColorTransforms.h"
#include "OpenColorIO/OpenColorTypes.h"
#include "logger.h"
#include "structs.h"
#include <cstring>
#include <istream>

ocioProcessor ocioProc;

//--- Add Config ---//
/*
    Verify a configuration provided as a string
    and add it to the valid congfigs vector
*/
bool ocioProcessor::addConfig(std::string &configFile, std::string configName) {
    ocioConfig newConfig;
    try {
        // Load config from the memory stream
        std::istringstream is(configFile);
        newConfig.config = OCIO::Config::CreateFromStream(is);

        // Sanity check the config
        newConfig.config->validate();
        newConfig.configName = configName;

        LOG_INFO("Successfully loaded OCIO config {}", configName);

    } catch (const OCIO::Exception& e) {
        LOG_ERROR("Error loading OCIO config: {}", e.what());
        return false;
    }

    if (!newConfig.config) {
        LOG_ERROR("No OCIO config loaded");
        return false;
    }
    loadNames(newConfig);
    m_configs.push_back(newConfig);
    return true;
}

//--- Initialize External Config ---//
/*
    Attempt to add the config at the provided
    path to the vector
*/
bool ocioProcessor::initExtConfig(std::string path) {
    ocioConfig newConfig;
    try {
        // Load config from file
        newConfig.config = OCIO::Config::CreateFromFile(path.c_str());

        // Sanity check the config
        newConfig.config->validate();

        LOG_INFO("Successfully loaded OCIO config from file");
        externalConfigPath = path;
        validExternal = true;
        newConfig.configName = newConfig.config->getName();

    } catch (const OCIO::Exception& e) {
        LOG_ERROR("Error loading OCIO config: {}", e.what());
        return false;
    }
    loadNames(newConfig);
    m_configs.push_back(newConfig);
    return true;
}

//--- Load Names ---//
/*
    Load in the colorspace/display/view names
    from the provided valid config
*/
void ocioProcessor::loadNames(ocioConfig& config) {
    // Get all available color spaces
    config.colorspaces.clear();
    for (int i = 0; i < config.config->getNumColorSpaces(); i++) {
        config.colorspaces.push_back(config.config->getColorSpaceNameByIndex(i));
    }

    // Get all available displays
    config.displays.clear();
    config.views.clear();
    for (int i = 0; i < config.config->getNumDisplays(); i++) {
        std::string displayName = config.config->getDisplay(i);
        config.displays.push_back(displayName);

        std::vector<std::string> curView;
        // Get views for this display
        for (int j = 0; j < config.config->getNumViews(displayName.c_str()); j++) {
            curView.push_back(config.config->getView(displayName.c_str(), j));
        }
        config.views.push_back(curView);
    }

    // Get all available looks
    config.looks.clear();
    for (int i = 0; i < config.config->getNumLooks(); i++) {
        config.looks.push_back(config.config->getLookNameByIndex(i));
    }
}

//--- Set Active Config ---//
/*
    Set the active config based on the index
*/
void ocioProcessor::setActiveConfig(int id) {
    if (id < m_configs.size() && id >= 0) {
        // We have a valid id selection
        OCIO::SetCurrentConfig(m_configs[id].config);
        selectedConfig = id;
    }

    if (id == -1) {
        // Startup instance where we want to
        // set the external config (last config
        // in the vector) active
        OCIO::SetCurrentConfig(m_configs.back().config);
        selectedConfig = m_configs.size() - 1;
    }
}

//--- Get Config List ---//
/*
    Get list of configs for display
*/
std::vector<std::string> ocioProcessor::getConfigList() {
    std::vector<std::string> list;
    list.reserve(m_configs.size());
    for (auto& config : m_configs)
        list.push_back(config.configName);
    return list;
}

//--- Get Config Names ---//
/*
    Get list of full config names
*/
std::vector<std::string> ocioProcessor::getConfigNames() {
    std::vector<std::string> list;
    for (auto &config : m_configs) {
        list.push_back(config.config->getName());
    }
    return list;
}

//--- Process Image ---//
/*
    CPU process an image with the given OCIO Settings
*/
void ocioProcessor::processImage(float *img, unsigned int width,
                                 unsigned int height, ocioSetting &ocioSet) {

  try {
      int configIt = selectedConfig;
      if (ocioSet.ocioConfig >= 0 && ocioSet.ocioConfig < m_configs.size())
        configIt = ocioSet.ocioConfig;

      const char *colorspace = m_configs[configIt].config->getColorSpaceNameByIndex(ocioSet.colorspace);
      if (!colorspace && !ocioSet.useDisplay){
          colorspace = m_configs[configIt].config->getColorSpaceNameByIndex(0);
          LOG_WARN("Invald Input Colorspace setting, using default!");
      }

      const char *display = m_configs[configIt].config->getDisplay(ocioSet.display);
      const char *view;
      if (!display && ocioSet.useDisplay){
          display = m_configs[configIt].config->getDisplay(0);
          view = m_configs[configIt].config->getView(display, 0);
          LOG_WARN("Invalid input colorspace setting, using default!");
      } else {
          view = m_configs[configIt].config->getView(display, ocioSet.view);
          if (!view) {
              view = m_configs[configIt].config->getView(display, 0);
              LOG_WARN("Invalid view option, using default!");
          }
      }



    OCIO::ConstProcessorRcPtr processor;
    if (!ocioSet.useDisplay) {
      OCIO::ColorSpaceTransformRcPtr transform =
          OCIO::ColorSpaceTransform::Create();
        if (ocioSet.inverse) {
            transform->setSrc(colorspace);
            transform->setDst("ACEScg");
        } else {
            transform->setSrc("ACEScg");
            transform->setDst(colorspace);
        }
      processor = m_configs[configIt].config->getProcessor(transform);
    } else {
      OCIO::DisplayViewTransformRcPtr transform =
          OCIO::DisplayViewTransform::Create();
      transform->setSrc("ACEScg");
      transform->setDisplay(display);
      transform->setView(view);
      if (ocioSet.inverse)
        transform->setDirection(OCIO::TRANSFORM_DIR_INVERSE);
      processor = m_configs[configIt].config->getProcessor(transform);
    }

    OCIO::ConstCPUProcessorRcPtr cpu =
        processor->getOptimizedCPUProcessor(OCIO::OPTIMIZATION_DEFAULT);

    // Get number of hardware threads
    const unsigned int numThreads = std::thread::hardware_concurrency();
    const unsigned int rowsPerThread = (height + numThreads - 1) / numThreads;

    std::vector<std::thread> threads;

    for (unsigned int t = 0; t < numThreads; ++t) {
      unsigned int yStart = t * rowsPerThread;
      unsigned int yEnd = height > yStart + rowsPerThread ? yStart + rowsPerThread : height;

      if (yStart >= yEnd)
        continue;

      threads.emplace_back([=]() {
        float *threadImg = img + yStart * width * 4; // 4 channels per pixel
        unsigned int threadHeight = yEnd - yStart;

        OCIO::PackedImageDesc threadDesc(threadImg, width, threadHeight, 4);
        cpu->apply(threadDesc);
      });
    }

    for (auto &t : threads) {
      t.join();
    }

  } catch (OCIO::Exception &e) {
    LOG_ERROR("Error processing OIIO image!");
    return;
  }
}

void ocioProcessor::refGamutCompress(float* img, unsigned int width, unsigned int height) {
    try {
      OCIO::ConstProcessorRcPtr processor;
      OCIO::LookTransformRcPtr lookTransform = OCIO::LookTransform::Create();
      lookTransform->setLooks("ACES 1.3 Reference Gamut Compression");
      lookTransform->setSrc("ACEScg");
      lookTransform->setDst("ACEScg");
      processor = m_configs[selectedConfig].config->getProcessor(lookTransform);

      OCIO::ConstCPUProcessorRcPtr cpu =
          processor->getOptimizedCPUProcessor(OCIO::OPTIMIZATION_DEFAULT);

      // Get number of hardware threads
      const unsigned int numThreads = std::thread::hardware_concurrency();
      const unsigned int rowsPerThread = (height + numThreads - 1) / numThreads;

      std::vector<std::thread> threads;

      for (unsigned int t = 0; t < numThreads; ++t) {
        unsigned int yStart = t * rowsPerThread;
        unsigned int yEnd = height > yStart + rowsPerThread ? yStart + rowsPerThread : height;

        if (yStart >= yEnd)
          continue;

        threads.emplace_back([=]() {
          float *threadImg = img + yStart * width * 4; // 4 channels per pixel
          unsigned int threadHeight = yEnd - yStart;

          OCIO::PackedImageDesc threadDesc(threadImg, width, threadHeight, 4);
          cpu->apply(threadDesc);
        });
      }

      for (auto &t : threads) {
        t.join();
      }

    } catch (OCIO::Exception &e) {
      LOG_ERROR("Error processing OIIO Gamut Compression! {}", e.what());
      return;
    }
}

//--- Get GL Desc ---//
/*
    With the given OCIO Settings, create the requisite
    GpuShaderDescRcPtr, and return it
*/
OCIO::GpuShaderDescRcPtr ocioProcessor::getGLDesc(ocioSetting& ocioSet) {
    try {
        const char* colorspace = m_configs[selectedConfig].config->getColorSpaceNameByIndex(ocioSet.colorspace);
        const char* display = m_configs[selectedConfig].config->getDisplay(ocioSet.display);
        const char* view = m_configs[selectedConfig].config->getView(display, ocioSet.view);

        OCIO::ConstProcessorRcPtr processor;
        if (!ocioSet.useDisplay){
            OCIO::ColorSpaceTransformRcPtr transform = OCIO::ColorSpaceTransform::Create();
            if (ocioSet.inverse) {
                transform->setSrc(colorspace);
                transform->setDst("ACEScg");
            } else {
                transform->setSrc("ACEScg");
                transform->setDst(colorspace);
            }
            processor = m_configs[selectedConfig].config->getProcessor(transform);
        } else {
            OCIO::DisplayViewTransformRcPtr transform = OCIO::DisplayViewTransform::Create();
            transform->setSrc("ACEScg");
            transform->setDisplay(display);
            transform->setView(view);
            if (ocioSet.inverse)
              transform->setDirection(OCIO::TRANSFORM_DIR_INVERSE);
            processor = m_configs[selectedConfig].config->getProcessor(transform);
        }

        // Step 5: Create GPU shader description
        OCIO::GpuShaderDescRcPtr shaderDesc = OCIO::GpuShaderDesc::CreateShaderDesc();
        shaderDesc->setLanguage(OCIO::GPU_LANGUAGE_GLSL_4_0);

        OCIO::OptimizationFlags flags = OCIO::OPTIMIZATION_LOSSLESS;
        // Step 6: Get the shader program info
        OCIO::ConstGPUProcessorRcPtr gpu = processor->getOptimizedGPUProcessor(flags);
        gpu->extractGpuShaderInfo(shaderDesc);

        //LOG_INFO("OpenGL Shaders: {}", shaderDesc->getShaderText());
        return shaderDesc;
    } catch (OCIO::Exception& e) {
        LOG_ERROR("OCIO processing error: {}", e.what());
        return nullptr;
    }
}
