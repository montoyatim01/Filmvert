#ifndef _releasenotes_h
#define _releasenotes_h

#include <string.h>

const std::string relNotes_1_2_0(R"V0G0N(
New Features:
- Added a curves grade adjustment
- Added a color matrix to the grade controls
- Added CMYK grading controls
- Added the ability to toggle on/off individual grade sections
- Added a preference for adding an analysis offset to prevent clipped shadows
- Added the ability to enable ACES gamut compression
- Importing a roll based on a .fvi file is now possible
- Periodically check for new versions
- Added the ability to scale the images on export
- Added the ability to set custom aspect ratios when cropping images

UI Enhancements:
- Added the ability to view single color channels
- Added the ability to highlight clipped areas in the image
- Image viewer now maintains the cursor position while zooming
- The interface has been reorganized and cleaned up
- Added the ability to selectively show/hide the individual colors in the grade controls
- Added the ability to tweak the background color of the interface panes
- Added the ability to change the color adjustment popup style
- Added the ability to inspect pixel values
- Moved the stats to a preference item and now overlay on the image viewer

Other Enhancements:
- Slight speed improvements when analyzing a photo
- The black point analysis no longer clips the shadows
- Added image filename and file path to metadata
- Images are hashed to prevent clashing metadata in a roll
- Added a hidden preference feature to cache image files in memory
- Inversion data is now saved and readable from EXR files
- GPU ram usage is now significantly reduced
- Unsaved image state is improved
- Updated build scripts
- Updated the build platform to Conan v2

Bug Fixes:
- Addressed an issue where the program would hang/crash on a fresh/new install
- Addressed an issue where saving roll metadata would write an empty file
- Addressed an issue where the program would occasionally crash on export
- Addressed an issue where image rotation wouldn't correctly sync/paste
- Addressed issues with the analysis region behaving incorrectly with certain image orientations
- Addressed an issue where export of Pakon/Data Raw images would distort when performance mode was set to a resolution smaller than the output image
- Base color is now correctly computed over the entire selection
- Histrogram now updates immediately when selecting a new image
- Enabling "Display analysis region" now functions correctly on images with pasted metadata
- Addressed an issue where hotkeys may be triggered when unwarranted (in popups)
- Image rating metadata is now properly saved when metadata is embedded in the output file
- Addressed an issue where the program may crash with auto-save enabled
- Addressed an issue exporting Jpeg contact sheets
- Addressed an issue where rolls would fail to reload after an export
- Other miscelaneous bug fixes

)V0G0N");

const std::string relNotes_1_1_2(R"V0G0N(
New Features:
- Added the ability to export greyscale images
- Added a CPU render mode utilizing a Mitchell sampling algorithm
- Saving roll metadata now merges changes instead of overwriting

UI Enhancements:
- Image viewer scale is image agnostic. Different resolution/orientation images now display at the same scale
- (macOS) The dock icon now bounces when trying to close with unsaved changes
- (macOS) The export window now allows folder creation
- The export dialog will be disabled until a path is selected

Bug Fixes:
- Addressed an issue where the histogram wouldn't update with an image visible, but not selected
- Addressed an issue where the copied saturation value wouldn't paste to other images
- (UNIX) Hidden files are now ignored while importing a roll
- Addressed an issue where some images would import with the wrong library resulting in incorrect colors
- Image exports will fail-over to CPU rendering if GPU rendering fails
- Simultaneous image loading follows max simultaneous exports preference number
- Addressed an issue where images may still export cropped with the bake crop option de-selected
- Addressed an issue where in some cases rotating an image left or right would perform the opposite of intended operation

)V0G0N");

const std::string relNotes_1_1_1(R"V0G0N(
UI Enhancements:
- Added release notes popups for new versions
- Added high-dpi window rendering for supported operating systems

Bug Fixes:
- Addressed an issue where exporting would lock up the system
- Addressed an issue where single-channel RAW files would import blank
- Addressed an issue where images would import with the incorrect IDT set
- Addressed an issue where some images would export with incorrect metadata tags
)V0G0N");

const std::string relNotes_1_1_0(R"V0G0N(
New Features:
- Saturation slider
- Flip horizontal/vertical. All image orientations are now accessible
- Image crop, and rotation
- Bake crop/rotation in exports
- Add a border to exported images
- Roll contact sheets (which can be used to save/import full-roll metadata)
- Added ACES 1.3 configuration (for troublesome images)

UI Enhancements:
- Analysis and grade controls are color coded
- Image input/output colorspace settings are hidden by default

Bug Fixes:
- Addressed an issue where images wouldn't import to the selected roll
- Addressed an issue with exporting non-raw images
- Addressed an issue that would allow for setting non-legal preferences

Under The Hood:
- Revamped OCIO backend
- Images store which configuration was used, attempt to re-use the same config/settings on import
- Updated CMake compatibility
- 'Hidden' preferences for various uncommon settings
)V0G0N");

#endif
