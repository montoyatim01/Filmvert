#include "ocioProcessor.h"
#include "OpenColorIO/OpenColorIO.h"
#include "OpenColorIO/OpenColorTypes.h"
#include <cstring>

ocioProcessor ocioProc;
/*
// Helper function to set up OpenGL context on macOS
bool setupOpenGLContext(int width, int height) {
    // Initialize GLUT
    int argc = 1;
    char *argv[1] = {(char*)"Something"};
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE);
    glutInitWindowSize(width, height);

    // Create a hidden window for OpenGL context
    int windowId = glutCreateWindow("OCIO Context");

    // Check if context was created successfully
    if (windowId <= 0) {
        std::cerr << "Failed to create OpenGL context" << std::endl;
        return false;
    }

#ifndef __APPLE__
    // Initialize GLEW (not needed on macOS)
    glewExperimental = GL_TRUE;
    GLenum err = glewInit();
    if (err != GLEW_OK) {
        std::cerr << "GLEW initialization failed: " << glewGetErrorString(err) << std::endl;
        glutDestroyWindow(windowId);
        return false;
    }
#endif

    return true;
}
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

    // Get all available color spaces for debug info
    LOG_INFO("Available color spaces:" );
    for (int i = 0; i < OCIOconfig->getNumColorSpaces(); i++) {
        LOG_INFO("{}", OCIOconfig->getColorSpaceNameByIndex(i));
    }

    // Get all available displays
    LOG_INFO("Available displays: ");
    for (int i = 0; i < OCIOconfig->getNumDisplays(); i++) {
        std::string displayName = OCIOconfig->getDisplay(i);

        displays.resize(displays.size() + 1);
        displays[i] = new char[displayName.length() + 1];
        std::strcpy(displays[i], displayName.c_str());

        LOG_INFO("{}", displayName);
        std::vector<char*> curView;
        // Get views for this display
        for (int j = 0; j < OCIOconfig->getNumViews(displayName.c_str()); j++) {
            std::string view = OCIOconfig->getView(displayName.c_str(), j);
            curView.resize(curView.size() + 1);
            curView[j] = new char[view.length() + 1];
            std::strcpy(curView[j], view.c_str());
            LOG_INFO("{}", OCIOconfig->getView(displayName.c_str(), j));
        }
        views.push_back(curView);
    }
    return true;
}

void ocioProcessor::processImage(float *img, unsigned int width, unsigned int height) {

    // Step 2: Lookup the display ColorSpace
    const char * display = OCIOconfig->getDefaultDisplay();
    const char * view = OCIOconfig->getDefaultView(display);

    // Step 3: Create a DisplayViewTransform, and set the input and display ColorSpaces
    // (This example assumes the input is a role. Adapt as needed.)
    OCIO::DisplayViewTransformRcPtr transform = OCIO::DisplayViewTransform::Create();
    //transform->setSrc( OCIO::ROLE_SCENE_LINEAR );
    transform->setSrc("ACEScg");
    transform->setDisplay( "sRGB - Display" );
    transform->setView( "ACES 1.0 - SDR Video" );

    // Step 4: Create the processor
    OCIO::ConstProcessorRcPtr processor = OCIOconfig->getProcessor(transform);
    //OCIO::ConstCPUProcessorRcPtr cpu = processor->getOptimizedCPUProcessor()
    OCIO::ConstCPUProcessorRcPtr cpu = processor->getOptimizedCPUProcessor(OCIO::OPTIMIZATION_DEFAULT);

    // Get number of hardware threads
    const unsigned int numThreads = std::thread::hardware_concurrency();
    const unsigned int rowsPerThread = (height + numThreads - 1) / numThreads;

    std::vector<std::thread> threads;

    for (unsigned int t = 0; t < numThreads; ++t) {
        unsigned int yStart = t * rowsPerThread;
        unsigned int yEnd = std::min(height, yStart + rowsPerThread);

        if (yStart >= yEnd) continue;

        threads.emplace_back([=]() {
            float* threadImg = img + yStart * width * 4; // 4 channels per pixel
            unsigned int threadHeight = yEnd - yStart;

            OCIO::PackedImageDesc threadDesc(threadImg, width, threadHeight, 4);
            cpu->apply(threadDesc);
        });
    }

    for (auto& t : threads) {
        t.join();
    }
}

std::string ocioProcessor::getMetalKernel() {
    const char* display = OCIOconfig->getDisplay(displayOp);
    const char* view = OCIOconfig->getView(display, viewOp);

    OCIO::DisplayViewTransformRcPtr transform = OCIO::DisplayViewTransform::Create();
    transform->setSrc("ACEScg");
    transform->setDisplay(display);
    transform->setView(view);

    OCIO::ConstProcessorRcPtr processor = OCIOconfig->getProcessor(transform);
    try {
        // Step 5: Create GPU shader description
        OCIO::GpuShaderDescRcPtr shaderDesc = OCIO::GpuShaderDesc::CreateShaderDesc();
        shaderDesc->setLanguage(OCIO::GPU_LANGUAGE_MSL_2_0);  // macOS compatible GLSL version

        // Step 6: Get the shader program info
        processor->getDefaultGPUProcessor()->extractGpuShaderInfo(shaderDesc);
        //LOG_INFO("GPU Shader: {}", shaderDesc->getShaderText());
        return shaderDesc->getShaderText();
    } catch (OCIO::Exception& e) {
        LOG_ERROR("OCIO processing error: {}", e.what());
        return "No Shader!";
    }

}

void ocioProcessor::processImageGPU(float *img, unsigned int width, unsigned int height) {
    // Step 1: Set up OpenGL context
        /*if (!setupOpenGLContext(width, height)) {
            LOG_ERROR("Failed to set up OpenGL context, falling back to CPU");
            processImage(img, width, height);
            return;
        }*/

        // Step 3: Create the display transform
        const char * display = OCIOconfig->getDefaultDisplay();
        const char * view = OCIOconfig->getDefaultView(display);

        OCIO::DisplayViewTransformRcPtr transform = OCIO::DisplayViewTransform::Create();
        transform->setSrc("ACEScg");
        transform->setDisplay("sRGB - Display");
        transform->setView("ACES 1.0 - SDR Video");

        // Step 4: Create the processor
        OCIO::ConstProcessorRcPtr processor = OCIOconfig->getProcessor(transform);

        try {
            // Step 5: Create GPU shader description
            OCIO::GpuShaderDescRcPtr shaderDesc = OCIO::GpuShaderDesc::CreateShaderDesc();
            shaderDesc->setLanguage(OCIO::GPU_LANGUAGE_MSL_2_0);  // macOS compatible GLSL version

            // Step 6: Get the shader program info
            processor->getDefaultGPUProcessor()->extractGpuShaderInfo(shaderDesc);
            LOG_INFO("GPU Shader: {}", shaderDesc->getShaderText());
/*
            // Step 7: Create input texture
            GLuint inputTexture;
            glGenTextures(1, &inputTexture);
            glBindTexture(GL_TEXTURE_2D, inputTexture);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width, height, 0, GL_RGBA, GL_FLOAT, img);

            // Step 8: Create output texture/framebuffer
            GLuint outputTexture;
            glGenTextures(1, &outputTexture);
            glBindTexture(GL_TEXTURE_2D, outputTexture);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width, height, 0, GL_RGBA, GL_FLOAT, nullptr);

            GLuint framebuffer;
            glGenFramebuffers(1, &framebuffer);
            glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, outputTexture, 0);

            if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
                LOG_ERROR("Framebuffer not complete, falling back to CPU");
                processImage(img, width, height);
                return;
            }

            // Step 9: Create and compile shaders
            // Create vertex shader
            const GLchar* vertShaderSrc =
                "#version 330 core\n"
                "layout(location = 0) in vec2 position;\n"
                "layout(location = 1) in vec2 texCoord;\n"
                "out vec2 TexCoord;\n"
                "void main() {\n"
                "    gl_Position = vec4(position, 0.0, 1.0);\n"
                "    TexCoord = texCoord;\n"
                "}\n";

            GLuint vertShader = glCreateShader(GL_VERTEX_SHADER);
            glShaderSource(vertShader, 1, &vertShaderSrc, nullptr);
            glCompileShader(vertShader);

            // Check vertex shader compilation
            GLint success;
            GLchar infoLog[512];
            glGetShaderiv(vertShader, GL_COMPILE_STATUS, &success);
            if (!success) {
                glGetShaderInfoLog(vertShader, 512, nullptr, infoLog);
                LOG_ERROR("Vertex shader compilation failed: {}", infoLog);
                processImage(img, width, height);
                return;
            }

            // Create fragment shader using OCIO shader program text
            GLuint fragShader = glCreateShader(GL_FRAGMENT_SHADER);
            const char* fragShaderSrc = shaderDesc->getShaderText();
            glShaderSource(fragShader, 1, &fragShaderSrc, nullptr);
            glCompileShader(fragShader);

            // Check fragment shader compilation
            glGetShaderiv(fragShader, GL_COMPILE_STATUS, &success);
            if (!success) {
                glGetShaderInfoLog(fragShader, 512, nullptr, infoLog);
                LOG_ERROR("Fragment shader compilation failed: {}", infoLog);
                processImage(img, width, height);
                return;
            }

            // Create shader program
            GLuint shaderProgram = glCreateProgram();
            glAttachShader(shaderProgram, vertShader);
            glAttachShader(shaderProgram, fragShader);
            glLinkProgram(shaderProgram);

            // Check program linking
            glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
            if (!success) {
                glGetProgramInfoLog(shaderProgram, 512, nullptr, infoLog);
                LOG_ERROR("Shader program linking failed: {}" , infoLog);
                processImage(img, width, height);
                return;
            }

            // Step 10: Set up texture coordinates and vertex positions
            GLfloat vertices[] = {
                // positions      // texture coords
                -1.0f,  1.0f,    0.0f, 1.0f,   // top left
                -1.0f, -1.0f,    0.0f, 0.0f,   // bottom left
                 1.0f, -1.0f,    1.0f, 0.0f,   // bottom right
                 1.0f,  1.0f,    1.0f, 1.0f    // top right
            };

            GLuint indices[] = {
                0, 1, 2,  // first triangle
                0, 2, 3   // second triangle
            };

            // Create VAO, VBO, EBO
            GLuint VAO, VBO, EBO;
            glGenVertexArrays(1, &VAO);
            glGenBuffers(1, &VBO);
            glGenBuffers(1, &EBO);

            glBindVertexArray(VAO);

            glBindBuffer(GL_ARRAY_BUFFER, VBO);
            glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

            // Position attribute
            glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (void*)0);
            glEnableVertexAttribArray(0);

            // Texture coords attribute
            glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (void*)(2 * sizeof(GLfloat)));
            glEnableVertexAttribArray(1);

            // Step 11: Render with OCIO shaders
            glUseProgram(shaderProgram);

            // Set input texture
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, inputTexture);

            // Set OCIO shader uniforms
            // This is where we connect OCIO's 3D LUTs and uniforms to our shader
            int textureUnit = 1;  // Start from texture unit 1 (0 is our input)

            // Set OCIO's uniforms based on the shader description
            for (unsigned int i = 0; i < shaderDesc->getNumUniforms(); ++i) {
                const char* name = shaderDesc->getUniform(i).getUniformName();
                const OCIO::GpuShaderDesc::UniformData& data = shaderDesc->getUniform(i).getUniformData();

                GLint location = glGetUniformLocation(shaderProgram, name);

                if (location >= 0) {
                    if (data.getType() == OCIO::UNIFORM_FLOAT) {
                        glUniform1f(location, data.getFloat());
                    }
                    else if (data.getType() == OCIO::UNIFORM_FLOAT_VECTOR3) {
                        glUniform3fv(location, 1, data.getFloat3());
                    }
                    else if (data.getType() == OCIO::UNIFORM_FLOAT_VECTOR4) {
                        glUniform4fv(location, 1, data.getFloat4());
                    }
                }
            }

            // Set OCIO's textures
            for (unsigned int i = 0; i < shaderDesc->getNumTextures(); ++i) {
                const char* name = shaderDesc->getTexture(i).getUniformName();
                const char* textureName = shaderDesc->getTexture(i).getTextureName();
                const OCIO::GpuShaderDesc::TextureData& data = shaderDesc->getTexture(i).getTextureData();

                GLint location = glGetUniformLocation(shaderProgram, name);

                if (location >= 0) {
                    GLuint textureID;
                    glGenTextures(1, &textureID);

                    GLenum internalFormat = GL_RGB32F;  // Default for 3D LUTs
                    GLenum format = GL_RGB;

                    if (data.getType() == OCIO::TEXTURE_RED_CHANNEL) {
                        internalFormat = GL_R32F;
                        format = GL_RED;
                    }

                    if (data.getType() == OCIO::TEXTURE_RGB_CHANNEL) {
                        glActiveTexture(GL_TEXTURE0 + textureUnit);
                        glBindTexture(GL_TEXTURE_1D, textureID);

                        glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                        glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                        glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);

                        glTexImage1D(GL_TEXTURE_1D, 0, internalFormat, data.getWidth(), 0, format, GL_FLOAT, data.getValues());
                    }
                    else if (data.getType() == OCIO::TEXTURE_3D) {
                        glActiveTexture(GL_TEXTURE0 + textureUnit);
                        glBindTexture(GL_TEXTURE_3D, textureID);

                        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
                        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

                        glTexImage3D(GL_TEXTURE_3D, 0, internalFormat, data.getWidth(), data.getHeight(), data.getDepth(), 0, format, GL_FLOAT, data.getValues());
                    }

                    glUniform1i(location, textureUnit);
                    textureUnit++;
                }
            }

            // Step 12: Set viewport and render
            glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
            glViewport(0, 0, width, height);
            glClear(GL_COLOR_BUFFER_BIT);

            glBindVertexArray(VAO);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

            // Step 13: Read back the processed image
            glReadBuffer(GL_COLOR_ATTACHMENT0);
            glReadPixels(0, 0, width, height, GL_RGBA, GL_FLOAT, img);

            // Step 14: Clean up resources
            glDeleteVertexArrays(1, &VAO);
            glDeleteBuffers(1, &VBO);
            glDeleteBuffers(1, &EBO);
            glDeleteTextures(1, &inputTexture);
            glDeleteTextures(1, &outputTexture);
            glDeleteFramebuffers(1, &framebuffer);
            glDeleteProgram(shaderProgram);
            glDeleteShader(vertShader);
            glDeleteShader(fragShader);

            // Destroy the window/context
            glutDestroyWindow(glutGetWindow());*/

        } catch (OCIO::Exception& e) {
            LOG_ERROR("OCIO GPU processing error: {}", e.what());
            LOG_ERROR("Falling back to CPU processing");
            processImage(img, width, height);
            //glutDestroyWindow(glutGetWindow());
        } catch (std::exception& e) {
            LOG_ERROR("Error during GPU processing: {}", e.what());
            LOG_ERROR("Falling back to CPU processing");
            processImage(img, width, height);
            //glutDestroyWindow(glutGetWindow());
        }
}
