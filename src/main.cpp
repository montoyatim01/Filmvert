#include <stdint.h>

#include "logger.h"
//#include "metalGPU.h"
#include "window.h"

#if defined (WIN32)
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
#else
int main(void)
#endif
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
// -* Add in import from image? (import the metadata or inversion settings)
//
// -* Bug when closing multiple rolls (maybe roll is loading?)
// -- Get arrow keys to always switch images?
// -- Hotkeys for BP/WP RGB?
//
// -* Make the min/max points adjustable??
// -* Grade disabled notification? (Text on image view)
// -* Undo/redo triggers zoom
// --* Change zoom key?
// -* BP/WP grade to happen in linear and not jplog
// -* Import image metadata
// -* Select images to sync metadata (import roll)
// -* Select debayer method (preferences)
// -* Disallow certain operations if the image is still loading (or queue it up?)
// -* Add in scroll speed preferences (for mouse)
// -* Mouse vs trackpad mode? - Preferences
// -* Fix the damn histogram
// -* remove legacy SDL code/variables
// -* Metadata paste doesn't trigger save required?
// -* Use same import window for image import (select meta or params)
// -* Cycling quickly between rolls (with a lot loaded) slows it way down
// -* Cycling quickly between rolls that are loading (while closing them) causes a crash, resizeproxy thread
// -* Close roll/Image shortcut (Cmd + W?)
// -* Check esc hotkey in popups to close them!
// -* Rotation doesn't rollup

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
