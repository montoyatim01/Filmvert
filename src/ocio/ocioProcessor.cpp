#include "ocioProcessor.h"
#include "OpenColorIO/OpenColorIO.h"
#include "OpenColorIO/OpenColorTransforms.h"
#include "OpenColorIO/OpenColorTypes.h"
#include "logger.h"
#include "structs.h"
#include <cstring>
#include <istream>

ocioProcessor ocioProc;

//--- Initialize ---//
/*
    Initialize the OCIO class with the
    built-in OCIO config. Load up the vectors
    with the colorspace and display/view names
*/
bool ocioProcessor::initialize(std::string &configFile) {
    try {
        // Load config from the memory stream
        std::istringstream is(configFile);
        OCIOconfig = OCIO::Config::CreateFromStream(is);

        // Sanity check the config
        OCIOconfig->validate();

        // Set as current config
        OCIO::SetCurrentConfig(OCIOconfig);

        LOG_INFO("Successfully loaded OCIO config from memory");

    } catch (const OCIO::Exception& e) {
        LOG_ERROR("Error loading OCIO config: {}", e.what());
        return false;
    }

    if (!OCIOconfig) {
        LOG_ERROR("No OCIO config loaded");
        return false;
    }

    // Get all available color spaces
    for (int i = 0; i < OCIOconfig->getNumColorSpaces(); i++) {
        std::string colorspace = OCIOconfig->getColorSpaceNameByIndex(i);
        colorspaces.resize(colorspaces.size() + 1);
        colorspaces[i] = new char[colorspace.length() + 1];
        std::strcpy(colorspaces[i], colorspace.c_str());
    }

    // Get all available displays
    for (int i = 0; i < OCIOconfig->getNumDisplays(); i++) {
        std::string displayName = OCIOconfig->getDisplay(i);
        displays.resize(displays.size() + 1);
        displays[i] = new char[displayName.length() + 1];
        std::strcpy(displays[i], displayName.c_str());

        std::vector<char*> curView;
        // Get views for this display
        for (int j = 0; j < OCIOconfig->getNumViews(displayName.c_str()); j++) {
            std::string view = OCIOconfig->getView(displayName.c_str(), j);
            curView.resize(curView.size() + 1);
            curView[j] = new char[view.length() + 1];
            std::strcpy(curView[j], view.c_str());
        }
        views.push_back(curView);
    }
    return true;
}

//--- Initialize Alternate Config ---//
/*
    Attempt to initialize the config at the provided
    path. Don't yet set it active.
*/
bool ocioProcessor::initAltConfig(std::string path) {
    try {
        // Load config from file
        extOCIOconfig = OCIO::Config::CreateFromFile(path.c_str());

        // Sanity check the config
        extOCIOconfig->validate();

        LOG_INFO("Successfully loaded OCIO config from file");
        externalConfigPath = path;
        validExternal = true;
        return true;

    } catch (const OCIO::Exception& e) {
        LOG_ERROR("Error loading OCIO config: {}", e.what());
        return false;
    }
}

//--- Set External Active---//
/*
    If a valid external config exists, set it
    as the active config.
*/
void ocioProcessor::setExtActive() {
    if (!validExternal)
        return;

    try {
        OCIO::SetCurrentConfig(extOCIOconfig);
        colorspaces.clear();
        for (int i = 0; i < extOCIOconfig->getNumColorSpaces(); i++) {
            std::string colorspace = extOCIOconfig->getColorSpaceNameByIndex(i);
            colorspaces.resize(colorspaces.size() + 1);
            colorspaces[i] = new char[colorspace.length() + 1];
            std::strcpy(colorspaces[i], colorspace.c_str());
        }

        displays.clear();
        views.clear();
        for (int i = 0; i < extOCIOconfig->getNumDisplays(); i++) {
            std::string displayName = extOCIOconfig->getDisplay(i);

            displays.resize(displays.size() + 1);
            displays[i] = new char[displayName.length() + 1];
            std::strcpy(displays[i], displayName.c_str());

            std::vector<char*> curView;
            // Get views for this display
            for (int j = 0; j < extOCIOconfig->getNumViews(displayName.c_str()); j++) {
                std::string view = extOCIOconfig->getView(displayName.c_str(), j);
                curView.resize(curView.size() + 1);
                curView[j] = new char[view.length() + 1];
                std::strcpy(curView[j], view.c_str());
            }
            views.push_back(curView);
        }
        useExt = true;

    } catch (const OCIO::Exception& e) {
        LOG_ERROR("Error setting ext OCIO config: {}", e.what());
        return;
    }
}

//--- Set Internal Active---//
/*
    Switch from the external OCIO config back
    to the internal config
*/
void ocioProcessor::setIntActive() {
  try {
    OCIO::SetCurrentConfig(OCIOconfig);
    colorspaces.clear();
    for (int i = 0; i < OCIOconfig->getNumColorSpaces(); i++) {
      std::string colorspace = OCIOconfig->getColorSpaceNameByIndex(i);
      colorspaces.resize(colorspaces.size() + 1);
      colorspaces[i] = new char[colorspace.length() + 1];
      std::strcpy(colorspaces[i], colorspace.c_str());
    }

    displays.clear();
    views.clear();
    for (int i = 0; i < OCIOconfig->getNumDisplays(); i++) {
      std::string displayName = OCIOconfig->getDisplay(i);

      displays.resize(displays.size() + 1);
      displays[i] = new char[displayName.length() + 1];
      std::strcpy(displays[i], displayName.c_str());

      std::vector<char *> curView;
      // Get views for this display
      for (int j = 0; j < OCIOconfig->getNumViews(displayName.c_str()); j++) {
        std::string view = OCIOconfig->getView(displayName.c_str(), j);
        curView.resize(curView.size() + 1);
        curView[j] = new char[view.length() + 1];
        std::strcpy(curView[j], view.c_str());
      }
      views.push_back(curView);
    }

    useExt = false;

  } catch (const OCIO::Exception &e) {
    LOG_ERROR("Error setting ext OCIO config: {}", e.what());
    return;
  }
}

//--- Process Image ---//
/*
    CPU process an image with the given OCIO Settings
*/
void ocioProcessor::processImage(float *img, unsigned int width,
                                 unsigned int height, ocioSetting &ocioSet) {

  try {
      bool ext = ocioSet.ext && validExternal;
      const char *colorspace =
          !ext ? OCIOconfig->getColorSpaceNameByIndex(ocioSet.colorspace)
                  : extOCIOconfig->getColorSpaceNameByIndex(ocioSet.colorspace);

      const char *display = !ext ? OCIOconfig->getDisplay(ocioSet.display)
                                    : extOCIOconfig->getDisplay(ocioSet.display);
      const char *view = !ext ? OCIOconfig->getView(display, ocioSet.view)
                                 : extOCIOconfig->getView(display, ocioSet.view);

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
      processor = !ext ? OCIOconfig->getProcessor(transform)
                          : extOCIOconfig->getProcessor(transform);
    } else {
      OCIO::DisplayViewTransformRcPtr transform =
          OCIO::DisplayViewTransform::Create();
      transform->setSrc("ACEScg");
      transform->setDisplay(display);
      transform->setView(view);
      if (ocioSet.inverse)
        transform->setDirection(OCIO::TRANSFORM_DIR_INVERSE);
      processor = !ext ? OCIOconfig->getProcessor(transform)
                          : extOCIOconfig->getProcessor(transform);
    }

    OCIO::ConstCPUProcessorRcPtr cpu =
        processor->getOptimizedCPUProcessor(OCIO::OPTIMIZATION_DEFAULT);

    // Get number of hardware threads
    const unsigned int numThreads = std::thread::hardware_concurrency();
    const unsigned int rowsPerThread = (height + numThreads - 1) / numThreads;

    std::vector<std::thread> threads;

    for (unsigned int t = 0; t < numThreads; ++t) {
      unsigned int yStart = t * rowsPerThread;
      unsigned int yEnd = std::min(height, yStart + rowsPerThread);

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
    ocioSet.ext = useExt;

  } catch (OCIO::Exception &e) {
    LOG_ERROR("Error processing OIIO image!");
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
        const char* colorspace = !useExt ? OCIOconfig->getColorSpaceNameByIndex(ocioSet.colorspace) : extOCIOconfig->getColorSpaceNameByIndex(ocioSet.colorspace);

        const char* display = !useExt ? OCIOconfig->getDisplay(ocioSet.display) : extOCIOconfig->getDisplay(ocioSet.display);
        const char* view = !useExt ? OCIOconfig->getView(display, ocioSet.view) : extOCIOconfig->getView(display, ocioSet.view);

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
            processor = !useExt ? OCIOconfig->getProcessor(transform) : extOCIOconfig->getProcessor(transform);
        } else {
            OCIO::DisplayViewTransformRcPtr transform = OCIO::DisplayViewTransform::Create();
            transform->setSrc("ACEScg");
            transform->setDisplay(display);
            transform->setView(view);
            if (ocioSet.inverse)
              transform->setDirection(OCIO::TRANSFORM_DIR_INVERSE);
            processor = !useExt ? OCIOconfig->getProcessor(transform) : extOCIOconfig->getProcessor(transform);
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

//--- Get Metal Kernel ---//
/*
    With the given OCIO Settings, generate the requisite Metal
    kerenel string. If a 1D LUT is needed for the kernel,
    assign the pointer to the 1D and the dimentions to the
    OCIO Settings for copying in the GPU rendering stage
*/
std::string ocioProcessor::getMetalKernel(ocioSetting& ocioSet) {


    try {
        const char* colorspace = !useExt ? OCIOconfig->getColorSpaceNameByIndex(ocioSet.colorspace) : extOCIOconfig->getColorSpaceNameByIndex(ocioSet.colorspace);

        const char* display = !useExt ? OCIOconfig->getDisplay(ocioSet.display) : extOCIOconfig->getDisplay(ocioSet.display);
        const char* view = !useExt ? OCIOconfig->getView(display, ocioSet.view) : extOCIOconfig->getView(display, ocioSet.view);

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
            processor = !useExt ? OCIOconfig->getProcessor(transform) : extOCIOconfig->getProcessor(transform);
        } else {
            OCIO::DisplayViewTransformRcPtr transform = OCIO::DisplayViewTransform::Create();
            transform->setSrc("ACEScg");
            transform->setDisplay(display);
            transform->setView(view);
            if (ocioSet.inverse)
              transform->setDirection(OCIO::TRANSFORM_DIR_INVERSE);
            processor = !useExt ? OCIOconfig->getProcessor(transform) : extOCIOconfig->getProcessor(transform);
        }
        // Step 5: Create GPU shader description
        OCIO::GpuShaderDescRcPtr shaderDesc = OCIO::GpuShaderDesc::CreateShaderDesc();
        shaderDesc->setLanguage(OCIO::GPU_LANGUAGE_GLSL_4_0);

        OCIO::OptimizationFlags flags = OCIO::OPTIMIZATION_NONE;
        // Step 6: Get the shader program info
        OCIO::ConstGPUProcessorRcPtr gpu = processor->getOptimizedGPUProcessor(flags);
        gpu->extractGpuShaderInfo(shaderDesc);

        LOG_INFO("Texture Count: {}", shaderDesc->getNumTextures());
        LOG_INFO("3D Texture Count: {}", shaderDesc->getNum3DTextures());
        ocioSet.texCount = shaderDesc->getNumTextures();
        if (shaderDesc->getNumTextures() > 0) {
            ocioSet.texCount = shaderDesc->getNumTextures();
            //LOG_INFO("Texture Needed! {}", shaderDesc->getNumTextures());
            const char* name;
            const char* sampleName;
            OCIO::GpuShaderCreator::TextureType texType;
            OCIO::GpuShaderCreator::TextureDimensions texDim;
            OCIO::Interpolation interp = OCIO::Interpolation::INTERP_LINEAR;

            for (int i = 0; i < ocioSet.texCount; i++) {
                shaderDesc->getTexture(i, name, sampleName, ocioSet.texWidth[i], ocioSet.texHeight[i], texType, texDim, interp);

                // Get the actual texture data
                shaderDesc->getTextureValues(i, ocioSet.texture[i]);
            }

        }

        LOG_INFO("OpenGL Shaders: {}", shaderDesc->getShaderText());
        return shaderDesc->getShaderText();
    } catch (OCIO::Exception& e) {
        LOG_ERROR("OCIO processing error: {}", e.what());
        return "No Shader!";
    }

}
