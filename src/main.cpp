#include <stdint.h>

#include "logger.h"
#include "metalGPU.h"
#include "window.h"

int main(void)
{
    // Start Logger
    LOG_INFO("Logging started");
    LOG_INFO("Version: {}.{}.{}", 3, 1, 0);
    LOG_INFO("Build {:.8}-{}", GIT_COMMIT_HASH, BUILD_DATE);

    // Initialize Metal/CUDA Subsystem
    metalGPU metalSubsystem;

    // Start Window
    mainWindow window;
    window.setGPU(&metalSubsystem);
    return window.openWindow();

}
