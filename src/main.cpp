#include <stdint.h>

#include "logger.h"
#include "metalGPU.h"
#include "window.h"

int main(void)
{
    // Start Logger
    LOG_INFO("Logging started");
    LOG_INFO("Version: {}.{}.{}", 1, 0, 0);
    LOG_INFO("Build {:.8}-{}", GIT_COMMIT_HASH, BUILD_DATE);

    // Initialize Metal/CUDA Subsystem
    metalGPU metalSubsystem;

    // Start Window
    mainWindow window;
    window.setGPU(&metalSubsystem);
    return window.openWindow();

}


// TODO/Wishlist:
//
// - ImageIO
// -* Reincorporate OIIO for non-raw formats
// -* Single import image function, raw first?
// --* How to handle when image has preview file (dng)
// -* Import Pakon RAW files (Colorspace?)
// -- Perf mode off enables full res?
//
// - Export Window:
// -* Single/Multi-export
// -* Roll export
// -* Verify metadata
// -* Roll export asks for directory (sub-directories made based on roll name)
// -* Debayering (how that works with the half res)
// -* Fixing the SDL flags in the metal render function
// -* Proper channel handling
// -* Remember to write the metadata (with exiv2)
// -* Remember to set the OCIO kernels correctly
// -* Global image "export" flag?
// -* Utilize threadpools
//
// - Colorspaces
// -* OCIO Colorspaces in Export
// -* Regular spaces as well (ADX10)?
// -* Handling custom OCIO Configs?
//
// - Preferences/Settings
// -* Auto-save/frequency
// -* OCIO Config setting
// -- Default meta options (cameras, lenses)
// -* Roll load/unload setting (turn off the unloading?)
//
// - Menu Bar
// -* Roll based settings
// --- Roll Analyze No
// --* Roll Metadata
// --* Roll export
// --* Saving
// --* Save one
// --* Save all
// --* Metadata
// --* Edit Roll Meta
// --* Export Roll Meta
//
// - Hotkeys
// -* Copy/Paste meta **
// -* Copy/Paste params **
// -* Save state
// -* Save roll - Add Roll JSON
// -* Save all - Add Roll JSON
// -* Rot left
// -* Rot right
// -* Local Meta edit? (cmd + e?)
// -* Global Meta edit? (cmd + g)
// -- Printer lights? Other adjustments
// -* Make a popup with key shortcuts displayed
//
// - Metadata
// -- Cache of common options (user-pref) Probably not
// -- Button in imParam for edit/export No
//
// - Cleanup
// -- Cleanup windows.h header
// -- Break out the rolls.cpp file?
// -* Break out the imageio file (rename to image? make class?)
// --- Continue work on this
// -- Add additional functions to get active roll, etc in window
//
// - Bugs
// -- Test exporting and OCIO configs
// -* Rework entire OCIO system (try and get rid of the global variable nonsense)
// -* How does the Metal GPU know what CS settings to use (image based?)
// -- Now that OCIO image render is CPU based, are there thread-safety issues?
// -* Metadata for scanning methods/variables? (Two fields?)
// -- Add in import from image (import the metadata or inversion settings)
// -- How does performance mode affect OIIO opened images and their IDTs?
// --- Someone opens a bunch of images, changes their config, and then it can't re-read?
// -- Program in OIIO/Raw-data dump and reload.
