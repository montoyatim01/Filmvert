#include <stdint.h>

#include "logger.h"
#include "window.h"
#include "tether.h"

#if defined (WIN32)
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
#else
int main(void)
#endif
{
    // Start Logger
    LOG_INFO("Logging started");
    LOG_INFO("Version: {}.{}.{}", VERMAJOR, VERMINOR, VERPATCH);
    LOG_INFO("Build {:.8}-{}", GIT_COMMIT_HASH, BUILD_DATE);

    // Setup Threadpool
    unsigned int numThreads = std::thread::hardware_concurrency();
    LOG_INFO("Starting thread pool with {} threads", numThreads);
    tPool = new ThreadPool(numThreads);
    // Initialize Metal/CUDA Subsystem
    //metalGPU metalSubsystem;

    // Tether testing
    gblTether.initialize();
    gblTether.detectCameras();

    // Start Window
    mainWindow window;
    //window.setGPU(&metalSubsystem);
    return window.openWindow();

}

// - Make it so images scale the same
// -- Want it so that swapping between images maintains
// -- the same zoom level/position

/* 20250703
- Disable base color selection when crop is enabled
- Image viewer maintains scale between images
- Addressed issue where images would import with incorrect colorspace settings
*/

/* 20250625
- Revamped OCIO backend
- Added ACES 1.3 configuration
- Images store which configuration was used, attempt to re-use on import
- Added option to add border to exported images
- Added roll contact sheet feature
- Roll contact sheets can be used to import metadata
- Fixed crop box display on specific rotation settings
- Addressed an issue where existing image metadata wouldn't populate on open
- Addressed issue where crop box wasn't editable
- Updated CMake compatibility
*/

/* 20250617
- Added saturation slider
- Added the ability to copy/paste rotation across images
- Added flip horizontal/vertical
- Added arbitrary crop/rotation
- Added the ability to bake rotations/crops in export
- Color coded the controls
- Hide the IDT/ODT image settings in a tree
- Addressed an issue where images wouldn't import to the selected roll
- Addressed an issue with non-raw images exporting
- Addressed an issue that would allow for setting non-legal preferences.
*/

/*
- Set ImGui ini locations
- Modified macOS preference location
- Preferences only save when modified
- Safegaurd actions with no active images/rolls
- Export window options disable when export in progress
- Addressed issue where roll exports would crash
- Addressed issue where re-imported images would default bypass render
- Addressed issue where image renders would not clear vram buffers
*/

/*
- Image rating possible now
- Cleanup variables
- XMP embedding on export for all but jpg files
- Hotkeys for moving forward/backward images
- Rolls with unsaved images show an asterisk in the roll name
- Fixed an issue with image scaling after export
*/


/*
- Image params/metadata save out as .fvi files
- Images now render 'proxy' buffers that are available regardless of roll load state
- Histogram now renders alongside the image render from the proxy buffer
- Addressed issues with the selection state of the thumbnails
- Image rotation now correctly adds to the undo state
*/


/*
- Rolls now completely unload in the background (saving vram)
- Images will only load metadata if loading in the background (saving resources)
- Addressed an issue where the Roll Timeout setting wouldn't save/take effect
- Addressed an issue where Pakon raw files wouldn't load metadata
*/

/*
- Change from json file extension to fvi
- Histogram fixes
 */

/*
- Added "Roll Timeout". Rolls will wait a set time before unloading
- Addressed an issue where cycling between many rolls would hammer the system
- Addressed crashes related to the histogram
- Addressed an issue where the histogram wouldn't update when changing images
- Set sane defaults for all variables
 */

/*
- Significantly improved histogram performance
- Swapped Temperature slider direction
- Added the ability to export a single image JSON file (to roll directory)
- Added the ability to import a single image JSON file
- Added the ability to select individual options on JSON import.
- Added hotkeys for closing images/rolls
- Roll re-loads now trigger a re-render
- Fixed OpenGL Errors
- Fixed an issue where Dev process wouldn't paste across images
- Fixed an issue where the dev notes, scanner, and scan notes wouldn't apply editing a single image
- Fixed an issue where the hotkeys would mis-trigger while in a pop-up
- Fixed an issue where the "All Metadata" button wouldn't paste scan notes
- Fixed an issue where a metadata-only paste wouldn't mark unsaved changes
*/

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
