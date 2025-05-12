
#include <chrono>
#include <spdlog/details/log_msg_buffer.h>
#include <thread>
#define NS_PRIVATE_IMPLEMENTATION
#define CA_PRIVATE_IMPLEMENTATION
#define MTL_PRIVATE_IMPLEMENTATION
#include <Foundation/Foundation.hpp>
#include <Metal/Metal.hpp>
#include <QuartzCore/QuartzCore.hpp>
//#include "Metal/MTLResource.hpp"
//#include "Metal/MTLTypes.hpp"
#include "imageIO.h"
#include "renderParams.h"


#include "Foundation/NSString.hpp"
#include "logger.h"
#include "metalGPU.h"
#include "utils.h"
#include "ocioProcessor.h"

#include <cmrc/cmrc.hpp> //read embedded stuff
CMRC_DECLARE(assets);



metalGPU::metalGPU() {

  ocioKernelText1 = "#include <metal_math>\n #include <metal_common>\n #include <metal_compute>\n #include <metal_stdlib>\n using namespace metal;\n\n";
  ocioKernelText2 = "kernel void ocioProcess(device float4 *imgIn [[buffer(0)]], device uchar4 *imgOut [[buffer(1)]], constant int& width [[buffer(2)]], uint2 pos [[thread_position_in_grid]]) { unsigned int index = (pos.y * width) + pos.x; imgOut[index] = (uchar4)(clamp(OCIOMain(imgIn[index]) * 255.0f, 0.0f, 255.0f));}";
  ocioKernelText3 = "kernel void ocioProcessFull(device float4 *imgIn [[buffer(0)]], device float4 *imgOut [[buffer(1)]], constant int& width [[buffer(2)]], uint2 pos [[thread_position_in_grid]]) { unsigned int index = (pos.y * width) + pos.x; imgOut[index] = OCIOMain(imgIn[index]);}";

  auto fs = cmrc::assets::get_filesystem();
  auto mtllib = fs.open("assets/default.metallib");
  if (!mtllib) {
    LOG_ERROR("Failed to load Metal library");
    return;
  }

  m_device = MTL::CreateSystemDefaultDevice();
  if (!m_device) {
    LOG_ERROR("Unable to create Metal Device");
    return;
  }

  m_command_queue = m_device->newCommandQueue();
  if (!m_command_queue) {
    LOG_ERROR("Unable to create Metal command queue");
    return;
  }

  NS::Error *error;
  // Create a dispatch data object from the raw pointer and size
  dispatch_data_t dispatchData = dispatch_data_create(
      (void *)mtllib->begin(), int(mtllib->size()), dispatch_get_main_queue(),
      DISPATCH_DATA_DESTRUCTOR_DEFAULT);
  default_library = m_device->newLibrary(dispatchData, &error);
  if (!default_library) {
    LOG_ERROR("Unable to load Metal Library: {}",
              error->localizedDescription()->cString(NS::UTF8StringEncoding));
  }

  initPipelineState();
  initBuffers();


  // Read in libraries and create buffers?
  LOG_INFO("Metal Active, GPU Device: {}", m_device->name()->cString(NS::UTF8StringEncoding));
  enableQueue = true;
  queueThread = std::thread(&metalGPU::processQueue, this);
}

metalGPU::~metalGPU() {
    enableQueue = false;
    queueThread.join();
}

void metalGPU::initPipelineState() {

  auto m_recursiveGaussian_rgba = NS::String::string("m_recursiveGaussian_rgba", NS::ASCIIStringEncoding);
  MTL::Function *recurFunc = default_library->newFunction(m_recursiveGaussian_rgba);

  auto m_transpose = NS::String::string("m_transpose", NS::ASCIIStringEncoding);
  MTL::Function *transFunc = default_library->newFunction(m_transpose);

  auto m_baseColor = NS::String::string("baseColorProcess", NS::ASCIIStringEncoding);
  MTL::Function *baseColorFunc = default_library->newFunction(m_baseColor);

  auto mainF = NS::String::string("mainProcess", NS::ASCIIStringEncoding);
  MTL::Function *mainFunc = default_library->newFunction(mainF);

  if (!recurFunc || !transFunc || !mainFunc || !baseColorFunc) {
          LOG_ERROR("Failed to initiailze Metal pipeline states");
  }
  NS::Error *error;

  _recursive = m_device->newComputePipelineState(recurFunc, &error);
  if (!_recursive)
    LOG_ERROR("[metal] Failed to init recursive Pipeline State");

  _transpose = m_device->newComputePipelineState(transFunc, &error);
  if (!_transpose)
    LOG_ERROR("[metal] Failed to init transpose Pipeline State");

  _baseColor = m_device->newComputePipelineState(baseColorFunc, &error);
  if (!_baseColor)
    LOG_ERROR("[metal] Failed to init base color Pipeline State");

  _mainProcess = m_device->newComputePipelineState(mainFunc, &error);
  if (!_mainProcess)
    LOG_ERROR("[metal] Failed to init main Pipeline State");

}

bool metalGPU::initOCIOKernels(int csSetting, int csDisp, int view) {

    std::string ocioKernel;

    ocioKernel = ocioKernelText1;
    ocioKernel += ocioProc.getMetalKernel(csSetting, csDisp, view);
    ocioKernel += ocioKernelText2;
    ocioKernel += ocioKernelText3;

    MTL::Library    *metalLibrary;     // Metal library
    MTL::Function   *kernelFunction;   // Compute kernel
    NS::Error* err;

    MTL::CompileOptions* options = MTL::CompileOptions::alloc()->init();
    options->setFastMathEnabled(true);
    options->setLanguageVersion(MTL::LanguageVersion2_4);
    auto nsOCIO = NS::String::string(ocioKernel.c_str(), NS::ASCIIStringEncoding);
    metalLibrary = m_device->newLibrary(nsOCIO, options, &err);
    if (!metalLibrary) {
        LOG_ERROR("Error creating OCIO Metal Library: {}", err->description()->cString(NS::ASCIIStringEncoding));
        return false;
    }
    options->release();

    auto ocioName = NS::String::string("ocioProcess", NS::ASCIIStringEncoding);
    kernelFunction = metalLibrary->newFunction(ocioName);

    auto ocioNameFull = NS::String::string("ocioProcessFull", NS::ASCIIStringEncoding);
    auto kernelFuncFull = metalLibrary->newFunction(ocioNameFull);

    if (!kernelFunction || !kernelFuncFull) {
        LOG_ERROR("Error initializing the Metal OCIO pipeline");
        return false;
    }

    _ocioProcess = m_device->newComputePipelineState(kernelFunction, &err);
    _ocioProcessFull = m_device->newComputePipelineState(kernelFuncFull, &err);
    if (!_ocioProcess || !_ocioProcessFull) {
        LOG_ERROR("Failed to init OCIO Pipeline State");
        return false;
    }
    return true;


}

void metalGPU::initBuffers() {
    // Set intial buffer size to something reasonable for image processing

    bufferSize = 2000 * 3000 * sizeof(float) * 4;
    m_src = m_device->newBuffer(bufferSize, MTL::ResourceStorageModeManaged);
    m_dst = m_device->newBuffer(bufferSize, MTL::ResourceStorageModeManaged);
    m_disp = m_device->newBuffer(bufferSize, MTL::ResourceStorageModeManaged);
    workingA = m_device->newBuffer(bufferSize, MTL::ResourceStorageModePrivate);
    workingB = m_device->newBuffer(bufferSize, MTL::ResourceStorageModePrivate);

    // Render Parameters
        renderParamBuf = m_device->newBuffer(sizeof(renderParams), MTL::ResourceStorageModeManaged);
        //memcpy(renderParamBuf.contents, &_renderParams, sizeof(_renderParams));
        //[renderParamBuf didModifyRange:NSMakeRange(0, sizeof(_renderParams))];

}

void metalGPU::bufferCheck(unsigned int width, unsigned int height) {
    if ((width * height * 4 * sizeof(float)) > bufferSize) {
        // Image to be processed is larger than the previous buffer size, resize the buffers to fit
        bufferSize = width * height * 4 * sizeof(float);
        LOG_INFO("[metal] Resizing metal buffers to accomidate image size: {}", bufferSize);

        m_src->release();
        m_dst->release();
        m_disp->release();
        workingA->release();
        workingB->release();

        m_src = m_device->newBuffer(bufferSize, MTL::ResourceStorageModeManaged);
        m_dst = m_device->newBuffer(bufferSize, MTL::ResourceStorageModeManaged);
        m_disp = m_device->newBuffer(bufferSize, MTL::ResourceStorageModeManaged);
        workingA = m_device->newBuffer(bufferSize, MTL::ResourceStorageModePrivate);
        workingB = m_device->newBuffer(bufferSize, MTL::ResourceStorageModePrivate);
    }
}

void metalGPU::computeKernels(float strength, float* kernels)
{
  int kernelSize = (int)(strength * KERNELSIZE) + 1;
  kernelSize = kernelSize > 1 ? kernelSize : 2;
  int posI = 0;
  float kernelSum = 0.0f;
//safetyMutex.lock();
  // Loop through the kernel and apply the Gaussian blur
  for (int i = -kernelSize / 2; i <= kernelSize / 2; i++)
  {
          float weight = exp(-0.5f * pow(i / strength, 2)) / (strength * sqrt(2 * M_PI));
          kernels[posI] = weight;
          kernelSum += weight;
          posI++;
  }

  // Normalize the kernel
  for (int i = 0; i < posI; i++)
  {
      kernels[i] /= kernelSum;
  }
  //safetyMutex.unlock();
}

void metalGPU::addToRender(image *_image, renderType type) {

    renderQueue.push(gpuQueue(_image, type));
}

bool metalGPU::isInQueue(image* _image) {
    if (!_image)
        return false;
    queueLock.lock();
    std::queue<gpuQueue> searchQueue = renderQueue;
    queueLock.unlock();
    while (!searchQueue.empty()) {
        if (_image == searchQueue.front()._img)
            return true;
        searchQueue.pop();
    }
    return false;
}

void metalGPU::processQueue() {

    while (enableQueue) {
        // While we've enabled the queue
        if (renderQueue.size() > 0) {
            rendering = true;
            // If there are items in the queue
            queueLock.lock();
            image* img = renderQueue.front()._img;
            renderType _type = renderQueue.front()._type;
            while (renderQueue.size() > 0 && renderQueue.front()._img == img) {
                // Removing duplicates to only run a single
                // instance of this image through the render
                renderQueue.pop();
            }
            queueLock.unlock();
            switch (_type) {
                case r_sdt:
                    renderImage(img);
                    break;
                case r_blr:
                    renderBlurPass(img);
                    break;
            }

        }
        else {
            rendering = false;
        }

        // Sleep for a few ms until we have a new image in the queue
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

    }


}


void metalGPU::renderImage(image* _image) {
if (!_image)
    return;
if (!_image->imageLoaded)
    return; //We don't have our buffers yet
auto start = std::chrono::steady_clock::now();
    // Generate RenderParams struct
    renderParams _renderParams = img_to_param(_image);

    MTL::CommandBuffer *commandBuffer = m_command_queue->commandBuffer();
    //commandBuffer->label = NsSt"tGrainKernel";

    MTL::ComputeCommandEncoder *computeEncoder = commandBuffer->computeCommandEncoder();
    computeEncoder->setComputePipelineState(_mainProcess);

    // Execution Sizes
    int exeWidth = _mainProcess->threadExecutionWidth();
    MTL::Size threadGroupCount = MTL::Size(exeWidth, 1, 1);
    MTL::Size threadGroups = MTL::Size((_renderParams.width + exeWidth - 1)/exeWidth, _renderParams.height, 1);

    MTL::Size transposeGrid = MTL::Size(iDivUp(_renderParams.width, BLOCK_DIM), iDivUp(_renderParams.height, BLOCK_DIM), 1);
    MTL::Size transposeGrid2 = MTL::Size(iDivUp(_renderParams.height, BLOCK_DIM), iDivUp(_renderParams.width, BLOCK_DIM), 1);
    MTL::Size transposeThreads = MTL::Size(BLOCK_DIM, BLOCK_DIM, 1);

    MTL::Size blurGridH = MTL::Size(iDivUp(_renderParams.width, exeWidth), 1, 1);
    MTL::Size blurGridV = MTL::Size(iDivUp(_renderParams.height, exeWidth), 1, 1);

    // Buffers
    bufferCheck(_renderParams.width, _renderParams.height);

    // OCIO Kernels
    if (ocioProc.cspOp != prevCSOpt || ocioProc.displayOp != prevDisp || ocioProc.viewOp != prevView || !_ocioProcess) {
        // Compile new kernels
        if (!initOCIOKernels(ocioProc.cspOp, ocioProc.displayOp, ocioProc.viewOp)) {
            LOG_ERROR("Failure to build OCIO kernels, exiting render");
            return;
        }
        prevCSOpt = ocioProc.cspOp;
        prevDisp = ocioProc.displayOp;
        prevView = ocioProc.viewOp;
    }



    // Src img
    unsigned int imgSize = _renderParams.width * _renderParams.height * 4 * sizeof(float);
    unsigned int dispSize = _renderParams.width * _renderParams.height * 4 * sizeof(uint8_t);
    if (prevIm != _image)
    {
        prevIm = _image;
        memcpy(m_src->contents(), _image->rawImgData, imgSize);
        m_src->didModifyRange(NS::Range(0, imgSize));
    }

    // Render params
    memcpy(renderParamBuf->contents(), &_renderParams, sizeof(_renderParams));
    renderParamBuf->didModifyRange(NS::Range(0, sizeof(_renderParams)));


        // Main process
        computeEncoder->setComputePipelineState(_mainProcess);
        computeEncoder->setBuffer(m_src, 0, 0);
        computeEncoder->setBuffer(workingA, 0, 1);
        //computeEncoder->setBuffer(m_disp, 0, 2);
        computeEncoder->setBuffer(renderParamBuf, 0, 2);
        computeEncoder->dispatchThreadgroups(threadGroups, threadGroupCount);

        if (_image->fullIm) {
            computeEncoder->setComputePipelineState(_ocioProcessFull);
            computeEncoder->setBuffer(workingA, 0, 0);
            computeEncoder->setBuffer(m_dst, 0, 1);
            computeEncoder->setBytes(&_image->width, sizeof(unsigned int), 2);
            computeEncoder->dispatchThreadgroups(threadGroups, threadGroupCount);
        } else {
            computeEncoder->setComputePipelineState(_ocioProcess);
            computeEncoder->setBuffer(workingA, 0, 0);
            computeEncoder->setBuffer(m_disp, 0, 1);
            computeEncoder->setBytes(&_image->width, sizeof(unsigned int), 2);
            computeEncoder->dispatchThreadgroups(threadGroups, threadGroupCount);
        }


        computeEncoder->endEncoding();
        commandBuffer->commit();
        commandBuffer->waitUntilCompleted();

        // Copy completed image
        _image->allocDispBuf();

        if (_image->fullIm) {
            _image->allocProcBuf();
            memcpy(_image->procImgData, m_dst->contents(), imgSize);
        } else {
            memcpy(_image->dispImgData, m_disp->contents(), dispSize);
            _image->sdlUpdate = true;
        }


        auto end = std::chrono::steady_clock::now();
        auto dur = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

        rdTimer.renderTime = dur.count();
        rdTimer.fps = 1000.0f / (float)dur.count();

}

void metalGPU::renderBlurPass(image* _image) {
    auto start = std::chrono::steady_clock::now();
        // Generate RenderParams struct
        renderParams _renderParams = img_to_param(_image);

        MTL::CommandBuffer *commandBuffer = m_command_queue->commandBuffer();
        //commandBuffer->label = NsSt"tGrainKernel";

        MTL::ComputeCommandEncoder *computeEncoder = commandBuffer->computeCommandEncoder();
        computeEncoder->setComputePipelineState(_mainProcess);

        // Execution Sizes
        int exeWidth = _mainProcess->threadExecutionWidth();
        MTL::Size threadGroupCount = MTL::Size(exeWidth, 1, 1);
        MTL::Size threadGroups = MTL::Size((_renderParams.width + exeWidth - 1)/exeWidth, _renderParams.height, 1);

        MTL::Size transposeGrid = MTL::Size(iDivUp(_renderParams.width, BLOCK_DIM), iDivUp(_renderParams.height, BLOCK_DIM), 1);
        MTL::Size transposeGrid2 = MTL::Size(iDivUp(_renderParams.height, BLOCK_DIM), iDivUp(_renderParams.width, BLOCK_DIM), 1);
        MTL::Size transposeThreads = MTL::Size(BLOCK_DIM, BLOCK_DIM, 1);

        MTL::Size blurGridH = MTL::Size(iDivUp(_renderParams.width, exeWidth), 1, 1);
        MTL::Size blurGridV = MTL::Size(iDivUp(_renderParams.height, exeWidth), 1, 1);

        // Buffers
        bufferCheck(_renderParams.width, _renderParams.height);

        memcpy(renderParamBuf->contents(), &_renderParams, sizeof(_renderParams));
        renderParamBuf->didModifyRange(NS::Range(0, sizeof(_renderParams)));


        // Src img
        unsigned int imgSize = _renderParams.width * _renderParams.height * 4 * sizeof(float);
        if (prevIm != _image)
        {
            prevIm = _image;
            memcpy(m_src->contents(), _image->rawImgData, imgSize);
            m_src->didModifyRange(NS::Range(0, imgSize));
        }


        //---RENDER---//

        // Process Base Color
        /*computeEncoder->setComputePipelineState(_baseColor);
        computeEncoder->setBuffer(m_src, 0, 0);
        computeEncoder->setBuffer(m_src, 0, 1);
        computeEncoder->setBuffer(renderParamBuf, 0, 2);
        computeEncoder->dispatchThreadgroups(threadGroups, threadGroupCount);
*/
             float sigma = _renderParams.sigmaFilter;
            //Recursive Paramaters
            const float nsigma = sigma < 0.1f ? 0.1f : sigma, alpha = 1.695f / nsigma,
                    ema = (float)std::exp(-alpha), ema2 = (float)std::exp(-2 * alpha),
                    b1 = -2 * ema, b2 = ema2;

            float a0 = 0, a1 = 0, a2 = 0, a3 = 0, coefp = 0, coefn = 0;
            const float k = (1 - ema) * (1 - ema) / (1 + 2 * alpha * ema - ema2);
            a0 = k;
            a1 = k * (alpha - 1) * ema;
            a2 = k * (alpha + 1) * ema;
            a3 = -k * ema2;
            coefp = (a0 + a1) / (1 + b1 + b2);
            coefn = (a2 + a3) / (1 + b1 + b2);

            // Blur H
            computeEncoder->setComputePipelineState(_recursive);
            computeEncoder->setBuffer(m_src, 0, 0);
            computeEncoder->setBuffer(workingA, 0, 1);
            computeEncoder->setBytes(&_renderParams.width, sizeof(int), 2);
            computeEncoder->setBytes(&_renderParams.height, sizeof(int), 3);
            computeEncoder->setBytes(&a0, sizeof(float), 4);
            computeEncoder->setBytes(&a1, sizeof(float), 5);
            computeEncoder->setBytes(&a2, sizeof(float), 6);
            computeEncoder->setBytes(&a3, sizeof(float), 7);
            computeEncoder->setBytes(&b1, sizeof(float), 8);
            computeEncoder->setBytes(&b2, sizeof(float), 9);
            computeEncoder->setBytes(&coefp, sizeof(float), 10);
            computeEncoder->setBytes(&coefn, sizeof(float), 11);
            computeEncoder->dispatchThreadgroups(blurGridH, threadGroupCount);

            // Transpose
            computeEncoder->setComputePipelineState(_transpose);
            computeEncoder->setBuffer(workingB, 0, 0);
            computeEncoder->setBuffer(workingA, 0, 1);
            computeEncoder->setBytes(&_renderParams.width, sizeof(int), 2);
            computeEncoder->setBytes(&_renderParams.height, sizeof(int), 3);
            computeEncoder->dispatchThreadgroups(transposeGrid, transposeThreads);

            // Blur V
            computeEncoder->setComputePipelineState(_recursive);
            computeEncoder->setBuffer(workingB, 0, 0);
            computeEncoder->setBuffer(workingA, 0, 1);
            computeEncoder->setBytes(&_renderParams.height, sizeof(int), 2);
            computeEncoder->setBytes(&_renderParams.width, sizeof(int), 3);
            computeEncoder->setBytes(&a0, sizeof(float), 4);
            computeEncoder->setBytes(&a1, sizeof(float), 5);
            computeEncoder->setBytes(&a2, sizeof(float), 6);
            computeEncoder->setBytes(&a3, sizeof(float), 7);
            computeEncoder->setBytes(&b1, sizeof(float), 8);
            computeEncoder->setBytes(&b2, sizeof(float), 9);
            computeEncoder->setBytes(&coefp, sizeof(float), 10);
            computeEncoder->setBytes(&coefn, sizeof(float), 11);
            computeEncoder->dispatchThreadgroups(blurGridV, threadGroupCount);

            // Transpose
            computeEncoder->setComputePipelineState(_transpose);
            computeEncoder->setBuffer(workingB, 0, 0);
            computeEncoder->setBuffer(workingA, 0, 1);
            computeEncoder->setBytes(&_renderParams.height, sizeof(int), 2);
            computeEncoder->setBytes(&_renderParams.width, sizeof(int), 3);
            computeEncoder->dispatchThreadgroups(transposeGrid2, transposeThreads);

            computeEncoder->setComputePipelineState(_baseColor);
            computeEncoder->setBuffer(workingB, 0, 0);
            computeEncoder->setBuffer(m_dst, 0, 1);
            computeEncoder->setBuffer(renderParamBuf, 0, 2);
            computeEncoder->dispatchThreadgroups(threadGroups, threadGroupCount);

            computeEncoder->endEncoding();
            commandBuffer->commit();
            commandBuffer->waitUntilCompleted();

            // Copy completed image
            memcpy(_image->blurImgData, m_dst->contents(), imgSize);
            _image->blurReady = true;
            auto end = std::chrono::steady_clock::now();
            auto dur = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

            rdTimer.renderTime = dur.count();
            rdTimer.fps = 1000.0f / (float)dur.count();
}
