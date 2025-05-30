#include <stdint.h>

#include "logger.h"
//#include "metalGPU.h"
#include "window.h"

int main(void)
{
    // Start Logger
    LOG_INFO("Logging started");
    LOG_INFO("Version: {}.{}.{}", 1, 0, 0);
    LOG_INFO("Build {:.8}-{}", GIT_COMMIT_HASH, BUILD_DATE);

    // Initialize Metal/CUDA Subsystem
    //metalGPU metalSubsystem;

    // Start Window
    mainWindow window;
    //window.setGPU(&metalSubsystem);
    return window.openWindow();

}


// TODO/Wishlist:
//
//
// - Hotkeys
// -- Printer lights? BP/WP?
//
// - Bugs
// -- Add in import from image? (import the metadata or inversion settings)
//
// -* Bug when closing multiple rolls (maybe roll is loading?)
// -- Get arrow keys to always switch images?
// -- Hotkeys for BP/WP RGB?
//
// -- Make the min/max points adjustable??
// -- Grade disabled notification? (Text on image view)
// -- Undo/redo triggers zoom
// --- Change zoom key?
// -* BP/WP grade to happen in linear and not jplog
// -- Replace the lancir image resizer (only does 8-bit) ugh
// -- Import image metadata
// -- Select images to sync metadata (import roll)
// -* Select debayer method (preferences)
// -* Disallow certain operations if the image is still loading (or queue it up?)

/*
- Changed GPU and ImGui backend to OpenGL
- Intial work for cross-platform builds
- Enabled the moving of the analyzed min/max points to set new whitepoint/blackpoint
- Added the ability to import single-image parameters from exported images
- Added error messaging for GPU anomolies
- Addressed an issue where closing a roll would cause a crash
- Addressed an issue where analyzing an image being loaded would cause a crash
- Fixed bug where single image export was blank
- Fixed bug where roll-based scanner meta wouldn't show on single image edit
*/
