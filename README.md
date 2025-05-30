<div align="center">

<!-- Put Logo Here -->

# Filmvert

A roll-based film inversion software with a simple repeatable workflow.

Features:
- Lossless 32-bit floating point operation
- Highly customizeable inversion parameters
- Replicatable inversion parameters
- Wide support for input image formats (Camera RAW, Pakon, TIFF/DNG)

</div>

## ⚡️ Quick start
Download the latest release from the [Releases Page](https://github.com/montoyatim01/Filmvert/releases) and install to your Applications folder.

### Loading Images/Rolls
Load in images by selecting a few individual images, or an entire directory's worth at once. Filmvert operates per-roll, so directories will import as rolls. Individual images will need to be added to a roll.

### Analysis
Set the four corners of the analysis region to encompass the image. Exclude any sprocket holes or any scanning equipment as this will throw off the inversion. The analysis only looks at pixels within the set region, so portions of the frame can be included/excluded to influence the analysis.

Set the base color by holding Cmd + Shift, and clicking and dragging a clear space in the image where the film base is visible.

Click "Analyze"

The engine will process the image, setting the white point and black point based on the lightest and darkest areas of the image. These points will be displayed on screen.

The "Analysis Bias" slider can be adjusted, and the image re-analyzed to tweak the results of the analysis.

### Image Adjustments
Depending upon the results of the analysis, the **Analyzed White Point** and **Analyzed Black Point** can be adjusted to taste.

Further, the **Grade** options can be used to set a quick grade on an image.


## 📖 Project Wiki

For full documentation on all features and workflows, take a look at the project wiki.

## 🚚 Building from source
-- Conan 1.66.0
-- Cmake 3.28?
-- metal-cpp /usr/local/include


## ⚠️ License
GNU GPL
