#include "exiv2/xmp_exiv2.hpp"
#include "image.h"
#include <string>
#include <optional>

// Remove the faff from the front of a string
std::string saniJsonString(const std::string& input);

// Templated function to get values from EXIF data for a specific key
template<typename T>
std::optional<T> getExifValue(const Exiv2::ExifData& exifData, const std::string& key) {
    try {
        // Find the key in the EXIF data
        Exiv2::ExifData::const_iterator pos = exifData.findKey(Exiv2::ExifKey(key));

        // Check if the key was found
        if (pos != exifData.end()) {
            // Convert the value to the requested type and return it
            if constexpr (std::is_same_v<T, std::string>) {
                return pos->toString();
            }
            else if constexpr (std::is_integral_v<T> || std::is_same_v<T, long> || std::is_same_v<T, int>) {
                return static_cast<T>(pos->toInt64());
            }
            else if constexpr (std::is_floating_point_v<T> || std::is_same_v<T, double> || std::is_same_v<T, float>) {
                return static_cast<T>(pos->toFloat());
            }
            else if constexpr (std::is_same_v<T, Exiv2::Rational>) {
                return pos->toRational();
            }
            else {
                // If type isn't directly supported, try with string conversion as fallback
                return static_cast<T>(pos->toString());
            }
        }

        // Key not found - return empty optional
        return std::nullopt;
    }
    catch (const Exiv2::Error& e) {
        std::cerr << "Error accessing EXIF key " << key << ": " << e.what() << std::endl;
        return std::nullopt;
    }
}

// Templated function to get values from XMP data for a specific key
template<typename T>
std::optional<T> getXmpValue(const Exiv2::XmpData& xmpData, const std::string& key) {
    try {
        // Find the key in the XMP data
        Exiv2::XmpData::const_iterator pos = xmpData.findKey(Exiv2::XmpKey(key));

        // Check if the key was found
        if (pos != xmpData.end()) {
            // Convert the value to the requested type and return it
            if constexpr (std::is_same_v<T, std::string>) {
                return pos->toString();
            }
            else if constexpr (std::is_integral_v<T> || std::is_same_v<T, long> || std::is_same_v<T, int>) {
                return static_cast<T>(pos->toInt64());
            }
            else if constexpr (std::is_floating_point_v<T> || std::is_same_v<T, double> || std::is_same_v<T, float>) {
                return static_cast<T>(pos->toFloat());
            }
            else if constexpr (std::is_same_v<T, Exiv2::Rational>) {
                return pos->toRational();
            }
            else {
                // If type isn't directly supported, try with string conversion as fallback
                return static_cast<T>(pos->toString());
            }
        }

        // Key not found - return empty optional
        return std::nullopt;
    }
    catch (const Exiv2::Error& e) {
        std::cerr << "Error accessing XMP key " << key << ": " << e.what() << std::endl;
        return std::nullopt;
    }
}
