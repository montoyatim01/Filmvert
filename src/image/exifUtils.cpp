
#include "exifUtils.h"


// Remove the faff from the front of a string
std::string saniJsonString(const std::string& input) {
    // Find the first occurrence of '{' or '['
    auto jsonStart = std::find_if(input.begin(), input.end(), [](char c) {
        return c == '{' || c == '[';
    });

    // If we found a valid JSON start character, return the substring from that point
    if (jsonStart != input.end()) {
        return std::string(jsonStart, input.end());
    }

    // If no valid JSON start character was found, return an empty string
    return "";
}
