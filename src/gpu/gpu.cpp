#include "gpu.h"
#include "OpenColorIO/OpenColorTypes.h"
#include "glslKernels.h"

#include "ocioProcessor.h"
#include "structs.h"

#include <GLFW/glfw3.h>
#include <cmath>

#include <csignal>
#include <ostream>


openglGPU::openglGPU() {
    memset(&m_uniforms, -1, sizeof(m_uniforms));
}


openglGPU::~openglGPU() {
    glDeleteTextures(1, &inputTex);
    glDeleteTextures(1, &outputTex);
    glDeleteFramebuffers(1, &fbo);
    glDeleteVertexArrays(1, &quadVAO);
    glDeleteProgram(m_shaderProgram);
    if (histObj.histData)
        delete [] histObj.histData;
}

bool openglGPU::initialize(ocioSetting &ocioSet) {
    if (m_initialized) {
            return true;
    }

    // Initialize GLEW if not already done
    if (glewInit() != GLEW_OK) {
        LOG_ERROR("Failed to initialize GLEW");
        return false;
    }

    // Create shaders
    if (!createShaders(ocioSet)) {
        LOG_ERROR("Failed to create shaders");
        return false;
    }

    // Setup geometry
    setupGeometry();

    // Setup Histogram Texture
    glGenTextures(1, &m_histoTex);
    glBindTexture(GL_TEXTURE_2D, m_histoTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, HISTWIDTH, HISTHEIGHT,
                 0, GL_RGBA, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // Set up out histogram buffer
    histObj.histData = new float[HISTWIDTH * HISTHEIGHT * 4];
    m_initialized = true;
    return true;
}

void openglGPU::checkError(std::string location) {
    if (!status.error) {
        GLint err = glGetError();
        if (err != GL_NO_ERROR) {
            status.error = true;
            LOG_ERROR("Error after {}: 0x{:x}, {}", location, err, (char*)gluErrorString(err));
            status.errorCode = err;
            status.errString = "Error with " + location + ": " + (char*)gluErrorString(err);
        }
    }
}

bool openglGPU::getStatus() {
    return status.error;
}

void openglGPU::clearError() {
    status.error = false;
    status.errorCode = 0;
    status.errString = "";
}

std::string openglGPU::getError() {
    return status.errString;
}

void openglGPU::cacheUniformLocations() {
    m_uniforms.inputTexture = glGetUniformLocation(m_shaderProgram, "inputTexture");
    m_uniforms.baseColor = glGetUniformLocation(m_shaderProgram, "baseColor");
    m_uniforms.blackPoint = glGetUniformLocation(m_shaderProgram, "blackPoint");
    m_uniforms.whitePoint = glGetUniformLocation(m_shaderProgram, "whitePoint");
    m_uniforms.G_blackpoint = glGetUniformLocation(m_shaderProgram, "G_blackpoint");
    m_uniforms.G_whitepoint = glGetUniformLocation(m_shaderProgram, "G_whitepoint");
    m_uniforms.G_lift = glGetUniformLocation(m_shaderProgram, "G_lift");
    m_uniforms.G_gain = glGetUniformLocation(m_shaderProgram, "G_gain");
    m_uniforms.G_mult = glGetUniformLocation(m_shaderProgram, "G_mult");
    m_uniforms.G_offset = glGetUniformLocation(m_shaderProgram, "G_offset");
    m_uniforms.G_gamma = glGetUniformLocation(m_shaderProgram, "G_gamma");
    m_uniforms.G_temp = glGetUniformLocation(m_shaderProgram, "G_temp");
    m_uniforms.G_tint = glGetUniformLocation(m_shaderProgram, "G_tint");
    m_uniforms.bypass = glGetUniformLocation(m_shaderProgram, "bypass");
    m_uniforms.gradeBypass = glGetUniformLocation(m_shaderProgram, "gradeBypass");
}

void openglGPU::setupGeometry()
{
    // Full-screen quad vertices
    float vertices[] = {
        // positions   // texture coords
        -1.0f, -1.0f, 0.0f,  0.0f, 0.0f,
         1.0f, -1.0f, 0.0f,  1.0f, 0.0f,
         1.0f,  1.0f, 0.0f,  1.0f, 1.0f,
        -1.0f,  1.0f, 0.0f,  0.0f, 1.0f
    };

    unsigned int indices[] = {
        0, 1, 2,
        2, 3, 0
    };

    glGenVertexArrays(1, &m_vertexArray);
    glGenBuffers(1, &m_vertexBuffer);
    glGenBuffers(1, &m_indexBuffer);

    glBindVertexArray(m_vertexArray);

    glBindBuffer(GL_ARRAY_BUFFER, m_vertexBuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_indexBuffer);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    // Position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // Texture coordinate attribute
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);

    return;
}

GLuint openglGPU::compileShader(const std::string& source, GLenum type) {
    GLuint shader = glCreateShader(type);
    const char* sourceCStr = source.c_str();
    glShaderSource(shader, 1, &sourceCStr, nullptr);
    glCompileShader(shader);

    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(shader, 512, nullptr, infoLog);
        LOG_ERROR("Shader compilation failed: {}", infoLog);
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}

bool openglGPU::createShaders(ocioSetting& ocioSet) {

    // Get OCIO Context
    OCIO::GpuShaderDescRcPtr gpuDesc = ocioProc.getGLDesc(ocioSet);

    if (!gpuDesc) {
        LOG_ERROR("Unable to get OCIO GPU Context!");
        checkError("OCIO GPU Description");
        return false;
    }

    if (ocioBuilder && prevOCIO != ocioSet) {
        // Clear cached uniform locations
        memset(&m_uniforms, -1, sizeof(m_uniforms));
    }

    ocioBuilder = OCIO::OpenGLBuilder::Create(gpuDesc);

    checkError("OCIO Processor Creation");

    ocioBuilder->allocateAllTextures(1);
    checkError("OCIO Texture Allocation");

    std::string ocioKernText = glsl_process;
    auto nPos = ocioKernText.find("OCIOFUNC");
    if (nPos != std::string::npos) {
        auto nSize = std::string("OCIOFUNC").size();
        std::string kernFunc = gpuDesc->getFunctionName();
        ocioKernText.erase(ocioKernText.begin() + nPos, ocioKernText.begin() + nPos + nSize);
        ocioKernText.insert(nPos, kernFunc);
    } else {
        LOG_ERROR("Corrupted GLSL shaders!");
        return false;
    }

    GLuint vertexShader = compileShader(glsl_vertex, GL_VERTEX_SHADER);
    checkError("Vertex Shader Compilation");

    try {
        // Build the fragment shader program.
        ocioBuilder->buildProgram(ocioKernText.c_str(), false, vertexShader);
    } catch (OCIO::Exception& e) {
        LOG_ERROR("Unable to build shaders: {}", e.what());
    }

    m_shaderProgram = ocioBuilder->getProgramHandle();
    cacheUniformLocations();
    prevOCIO = ocioSet;

    return true;
}

void openglGPU::bufferCheck(image* _image)
{
    if (!_image)
        return;

    unsigned int nWidth = _image->fullIm ? _image->rawWidth : _image->width;
    unsigned int nHeight = _image->fullIm ? _image->rawHeight : _image->height;

    if (nWidth > m_width || nHeight > m_height) {
        LOG_INFO("Resizing Input Buffer");

        // Create input texture
        if (m_inputTexture == 0 || glIsTexture(m_inputTexture) == GL_FALSE) {
            glGenTextures(1, &m_inputTexture);
            checkError("Generating Input Texture");
        }
        glBindTexture(GL_TEXTURE_2D, m_inputTexture);
        while( glGetError() != GL_NO_ERROR){} // Ignore errors for binding to input
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, nWidth, nHeight,
                     0, GL_RGBA, GL_FLOAT, nullptr);
        checkError("Allocating Input Texture");
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        checkError("Setting Modes Input Texture");
        m_width = nWidth;
        m_height = nHeight;
    }

        // Create output texture
        if (_image->glTexture == 0 || !glIsTexture(_image->glTexture)) {
            glGenTextures(1, (GLuint*)&_image->glTexture);
            glBindTexture(GL_TEXTURE_2D, _image->glTexture);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, nWidth, nHeight,
                         0, GL_RGBA, GL_FLOAT, nullptr);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            checkError("Generating Initial Output Texture");
        } else {
            glBindTexture(GL_TEXTURE_2D, _image->glTexture);
            int oWidth, oHeight;
            glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &oWidth);
            glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &oHeight);
            checkError("Querying Output Texture");
            if (oWidth < nWidth || oHeight < nHeight) {
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, nWidth, nHeight,
                             0, GL_RGBA, GL_FLOAT, nullptr);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
                checkError("Generating Fullsize Output Texture");
            }
        }
        glBindTexture(GL_TEXTURE_2D, 0);

        if (m_framebuffer == 0) {
            // Create framebuffer
            glGenFramebuffers(1, &m_framebuffer);
        }

    glBindFramebuffer(GL_FRAMEBUFFER, m_framebuffer);
    checkError("Binding to Framebuffer for resize");

    // Attach output texture to framebuffer
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                               GL_TEXTURE_2D, _image->glTexture, 0);

    checkError("Attaching Output to Framebuffer");

    // Check framebuffer completeness
    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE) {
        LOG_ERROR("Framebuffer not complete! Status: 0x{:x}", status);
    }

    // Unbind
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void openglGPU::updateUniforms(renderParams params) {
    glUniform1i(m_uniforms.inputTexture, 0);
    glUniform4fv(m_uniforms.baseColor, 1, params.baseColor);
    glUniform4fv(m_uniforms.blackPoint, 1, params.blackPoint);
    glUniform4fv(m_uniforms.whitePoint, 1, params.whitePoint);
    glUniform4fv(m_uniforms.G_blackpoint, 1, params.G_blackpoint);
    glUniform4fv(m_uniforms.G_whitepoint, 1, params.G_whitepoint);
    glUniform4fv(m_uniforms.G_lift, 1, params.G_lift);
    glUniform4fv(m_uniforms.G_gain, 1, params.G_gain);
    glUniform4fv(m_uniforms.G_mult, 1, params.G_mult);
    glUniform4fv(m_uniforms.G_offset, 1, params.G_offset);
    glUniform4fv(m_uniforms.G_gamma, 1, params.G_gamma);
    glUniform1f(m_uniforms.G_temp, params.temp);
    glUniform1f(m_uniforms.G_tint, params.tint);
    glUniform1i(m_uniforms.bypass, params.bypass);
    glUniform1i(m_uniforms.gradeBypass, params.gradeBypass);
}

//--- Add To Render Queue ---//
/*
    Add an image to the queue to be rendered
*/
void openglGPU::addToRender(image *_image, renderType type, ocioSetting ocioSet) {

    if (_image) {
        queueLock.lock();
        _image->inRndQueue = true;
        if (type == r_sdt || type == r_blr)
            renderQueue.push_front(gpuQueue(_image, type, ocioSet));
        else
            renderQueue.push_back(gpuQueue(_image, type, ocioSet));
        queueLock.unlock();
    }

}

//--- Is In Queue ---//
/*
    Determine whether a given image is in the render queue
*/
bool openglGPU::isInQueue(image* _image) {
    if (!_image)
        return false;
    queueLock.lock();
    std::deque<gpuQueue> searchQueue = renderQueue;
    queueLock.unlock();
    while (!searchQueue.empty()) {
        if (_image == searchQueue.front()._img)
            return true;
        searchQueue.pop_front();
    }
    return false;
}

//--- Process Queue ---//
/*
    Loop through checking to see if there is a new item in the
    queue to be rendered. If a single image/type/OCIO Set
    is sent to the queue multiple times in a row, ignore
    all but the last instance (before a new image). This
    prevents the rendering of the same image multiple times
    when unneccessary.
*/
void openglGPU::processQueue() {


        // While we've enabled the queue
        if (renderQueue.size() > 0) {
            rendering = true;
            // If there are items in the queue
            queueLock.lock();
            image* img = renderQueue.front()._img;
            renderType type = renderQueue.front()._type;
            ocioSetting ocioSet = renderQueue.front()._ocioSet;
            while (renderQueue.size() > 0 &&
                renderQueue.front()._img == img &&
                renderQueue.front()._type == type &&
                renderQueue.front()._ocioSet == ocioSet) {
                // Removing duplicates to only run a single
                // instance of this image through the render
                renderQueue.pop_front();
            }
            queueLock.unlock();
            switch (type) {
                case r_sdt:
                case r_full:
                case r_bg:
                    renderImage(img, ocioSet);
                    break;
                case r_blr:
                    //renderBlurPass(img);
                    break;
            }

        }
        else {
            rendering = false;
        }



}

void openglGPU::renderImage(image* _image, ocioSetting ocioSet) {

    if (!_image)
        return;
    if (!_image->imageLoaded)
        return; //We don't have our buffers yet
    auto start = std::chrono::steady_clock::now();
    while( glGetError() != GL_NO_ERROR){} // Clear any errors from previous
    // Generate RenderParams struct
    renderParams _renderParams = img_to_param(_image);

    // Resize framebuffer if needed
    bufferCheck(_image);
    checkError("Buffer Validation");

    // Setup Geometry
    setupGeometry();
    checkError("Geometry Setup");

    // Create Shaders
    //createShaders(ocioSet);
    //checkError("Shader Compilation");

    // OCIO Check
    if (prevOCIO != ocioSet || prevIm != _image || _image->fullIm || _image->imgRst) {
        if (!createShaders(ocioSet)){
            LOG_ERROR("Could not compile shaders!");
            return;
        }
        copyToTex(m_inputTexture, _renderParams.width, _renderParams.height, _image->rawImgData);
        checkError("Copying Image to Input Texture");
        prevIm = _image;
        _image->imgRst = false;
    }

    // Bind framebuffer for rendering to output texture
    glBindFramebuffer(GL_FRAMEBUFFER, m_framebuffer);
    glViewport(0, 0, _renderParams.width, _renderParams.height);
    checkError("Setting Viewport");

    // Clear the framebuffer
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    checkError("Clearing Framebuffer");

    // Use shader program
    glUseProgram(0);
    glUseProgram(m_shaderProgram);
    checkError("Setting Program");

    // Update uniforms
    updateUniforms(_renderParams);
    checkError("Updating Uniforms");

    // Bind input texture to texture unit 0
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_inputTexture);
    glUniform1i(m_uniforms.inputTexture, 0);
    checkError("Binding Input Texture");

    // OCIO textures should be on units 1, 2, 3
    // The OCIO builder should handle this with useAllTextures()
    ocioBuilder->useAllTextures();
    ocioBuilder->useAllUniforms();
    checkError("Setting OCIO Textures");

    // Render full-screen quad
    glBindVertexArray(m_vertexArray);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    checkError("Render");

    //while(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE){}

    // Unbind
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, 0, 0);
    glBindVertexArray(0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, 0);
    glUseProgram(0);
    checkError("Unbind");

    // Update MipMaps
    //glBindTexture(GL_TEXTURE_2D, _image->glTexture);
    //glGenerateMipmap(GL_TEXTURE_2D);
    //checkError("Mipmap");

    // Copy Completed Image
    if (_image->fullIm) {
        _image->allocProcBuf();
        copyFromTexFull(_image->glTexture, _renderParams.width, _renderParams.height, _image->procImgData);
        checkError("Copying Full Image for write");
        _image->renderReady = true;
    } else {
        // Change to OpenGL image buffer
        //_image->allocDispBuf();
        //memcpy(_image->dispImgData, m_disp->contents(), dispSize);
        //_image->glUpdate = true;
    }

    if (!isInQueue(_image))
        _image->inRndQueue = false;

    _image->needHist = true;
    auto end = std::chrono::steady_clock::now();
    auto dur = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    rdTimer.renderTime = dur.count();
    rdTimer.fps = 1000.0f / (float)dur.count();



    // Disable depth testing for 2D rendering
    glDisable(GL_DEPTH_TEST);
    checkError("Depth Test Disable");

    // Set clear color and clear
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    checkError("Clearing post-render");

    return;
}


void openglGPU::copyToTex(GLuint textureID, int width, int height, float* rgbaData) {
    glBindTexture(GL_TEXTURE_2D, textureID);

    // Upload the data to the existing texture
    glTexSubImage2D(GL_TEXTURE_2D, 0,          // target, level
                    0, 0,                      // xoffset, yoffset
                    width, height,             // width, height
                    GL_RGBA,                   // format
                    GL_FLOAT,                  // type
                    rgbaData);                 // data pointer

    checkError("Copying Texture");

    glBindTexture(GL_TEXTURE_2D, 0);
}
void openglGPU::copyFromTexFull(GLuint textureID, int width, int height, float* rgbaData) {
    glBindTexture(GL_TEXTURE_2D, textureID);

    // Read texture data directly
    glGetTexImage(GL_TEXTURE_2D,    // target
                  0,                // level
                  GL_RGBA,          // format
                  GL_FLOAT,         // type
                  rgbaData);        // pixels

    glBindTexture(GL_TEXTURE_2D, 0);
}

void openglGPU::getMipMapTexture(image* _img, float*& pixels, int &width, int &height) {
    if (!_img) {
        LOG_WARN("No image for histogram processing!");
        return;
    }
    if (glIsTexture(_img->glTexture) == GL_FALSE) {
        LOG_WARN("Image doesn't have an active GL Texture yet!");
        return;
    }
    int gWidth = 0;
    int gHeight = 0;
    glBindTexture(GL_TEXTURE_2D, _img->glTexture);
    // Use MipMap 0 (1x res)
    glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &gWidth);
    glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &gHeight);

    pixels = new float[gWidth * gHeight * 4];

    // Read texture data directly
    glGetTexImage(GL_TEXTURE_2D,    // target
                  0,                // level
                  GL_RGBA,          // format
                  GL_FLOAT,         // type
                  pixels);          // pixels

    width = gWidth;
    height = gHeight;

    glBindTexture(GL_TEXTURE_2D, 0);
}

void openglGPU::setHistTexture(float* pixels) {
    if (!pixels) {
        LOG_WARN("No Histogram Pixels!");
        return;
    }
    glBindTexture(GL_TEXTURE_2D, m_histoTex);

    // Upload the data to the existing texture
    glTexSubImage2D(GL_TEXTURE_2D, 0,          // target, level
                    0, 0,                      // xoffset, yoffset
                    HISTWIDTH, HISTHEIGHT,             // width, height
                    GL_RGBA,                   // format
                    GL_FLOAT,                  // type
                    pixels);                 // data pointer

    checkError("Copying Histogram");

    glBindTexture(GL_TEXTURE_2D, 0);
}

void openglGPU::histoCheck() {
    if (!histLock.try_lock()) // Don't get caught up waiting
        return;
    if (histObj.get) {
        getMipMapTexture(histObj.imgPtr, histObj.imgData, histObj.imgW, histObj.imgH);
        histObj.get = false;
    }
    if (histObj.set) {
        setHistTexture(histObj.histData);
        if (histObj.imgData)
            delete [] histObj.imgData;
        histObj.set = false;
    }
    histLock.unlock();
}
