
#include <chrono>
#include <spdlog/details/log_msg_buffer.h>
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

#include <cmrc/cmrc.hpp> //read embedded stuff
CMRC_DECLARE(assets);



metalGPU::metalGPU() {

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

void metalGPU::initBuffers() {
    // Set intial buffer size to something reasonable for image processing

    bufferSize = 2000 * 3000 * sizeof(float) * 4;
    m_src = m_device->newBuffer(bufferSize, MTL::ResourceStorageModeManaged);
    m_dst = m_device->newBuffer(bufferSize, MTL::ResourceStorageModeManaged);
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
        workingA->release();
        workingB->release();

        m_src = m_device->newBuffer(bufferSize, MTL::ResourceStorageModeManaged);
        m_dst = m_device->newBuffer(bufferSize, MTL::ResourceStorageModeManaged);
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




void metalGPU::renderImage(image* _image) {

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


    // Src img
    unsigned int imgSize = _renderParams.width * _renderParams.height * 4 * sizeof(float);
    if (prevIm != _image)
    {
        prevIm = _image;
        memcpy(m_src->contents(), _image->rawImgData, imgSize);
        m_src->didModifyRange(NS::Range(0, imgSize));
    }

    // Render params
    memcpy(renderParamBuf->contents(), &_renderParams, sizeof(_renderParams));
    renderParamBuf->didModifyRange(NS::Range(0, sizeof(_renderParams)));


        // Transpose
        computeEncoder->setComputePipelineState(_mainProcess);
        computeEncoder->setBuffer(m_src, 0, 0);
        computeEncoder->setBuffer(m_dst, 0, 1);
        computeEncoder->setBuffer(renderParamBuf, 0, 2);
        computeEncoder->dispatchThreadgroups(threadGroups, threadGroupCount);


        computeEncoder->endEncoding();
        commandBuffer->commit();
        commandBuffer->waitUntilCompleted();

        // Copy completed image
        memcpy(_image->procImgData, m_dst->contents(), imgSize);

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

            auto end = std::chrono::steady_clock::now();
            auto dur = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

            rdTimer.renderTime = dur.count();
            rdTimer.fps = 1000.0f / (float)dur.count();
}
