#include "grainParams.h"
#include "logger.h"
#include "nlohmann/json_fwd.hpp"
#include <filesystem>

bool grainParams::formatMeta() {
        try {
            nlohmann::json j;

            // Convert class members to JSON
            j["grainRadius"] = grainRadius;
            j["slowRadius"] = slowRadius;
            j["slowPivot"] = slowPivot;
            j["grainScale"] = grainScale;
            j["grainSat"] = grainSat;
            j["grainSigma"] = grainSigma;

            j["highlights"] = highlights;
            j["midtones"] = midtones;
            j["shadows"] = shadows;
            j["midPivot"] = midPivot;
            j["midSlope"] = midSlope;

            j["redMix"] = redMix;
            j["greenMix"] = greenMix;
            j["blueMix"] = blueMix;
            j["grainBlend"] = grainBlend;

            j["monteCarlo"] = monteCarlo;
            j["colorspace"] = colorspace;
            j["presetName"] = presetName;

            metadata = j.dump(-1);
            return true;
        } catch (const std::exception& e) {
            LOG_WARN("Unable to format grain metadata: {}", e.what());
            return false;
        }
}

bool grainParams::restoreMeta() {
        try {
            nlohmann::json j = nlohmann::json::parse(metadata);

            // Parse JSON into class members
            grainRadius = j["grainRadius"].get<float>();
            slowRadius = j["slowRadius"].get<float>();
            slowPivot = j["slowPivot"].get<float>();
            grainScale = j["grainScale"].get<float>();
            grainSat = j["grainSat"].get<float>();
            grainSigma = j["grainSigma"].get<float>();
            highlights = j["highlights"].get<float>();
            midtones = j["midtones"].get<float>();
            shadows = j["shadows"].get<float>();
            midPivot = j["midPivot"].get<float>();
            midSlope = j["midSlope"].get<float>();
            redMix = j["redMix"].get<float>();
            greenMix = j["greenMix"].get<float>();
            blueMix = j["blueMix"].get<float>();
            grainBlend = j["grainBlend"].get<float>();
            monteCarlo = j["monteCarlo"].get<int>();
            colorspace = j["colorspace"].get<int>();


            return true;
        } catch (const std::exception& e) {
            LOG_WARN("Unable to load grain metadata: {}", e.what());
            return false;
        }
}

    bool grainParams::loadMeta(const nlohmann::json& j) {
            try {
                // Parse JSON into class members
                grainRadius = j["grainRadius"].get<float>();
                slowRadius = j["slowRadius"].get<float>();
                slowPivot = j["slowPivot"].get<float>();
                grainScale = j["grainScale"].get<float>();
                grainSat = j["grainSat"].get<float>();
                grainSigma = j["grainSigma"].get<float>();
                highlights = j["highlights"].get<float>();
                midtones = j["midtones"].get<float>();
                shadows = j["shadows"].get<float>();
                midPivot = j["midPivot"].get<float>();
                midSlope = j["midSlope"].get<float>();
                redMix = j["redMix"].get<float>();
                greenMix = j["greenMix"].get<float>();
                blueMix = j["blueMix"].get<float>();
                grainBlend = j["grainBlend"].get<float>();
                monteCarlo = j["monteCarlo"].get<int>();
                colorspace = j["colorspace"].get<int>();
                presetName = j["presetName"].get<std::string>();

                return true;
            } catch (const std::exception& e) {
                LOG_WARN("Unable to load grain metadata: {}", e.what());
                return false;
            }
}

bool grainParams::loadMeta(const std::string& filename) {
        try {
            std::ifstream file(filename);
            if (!file.is_open()) {
                LOG_WARN("Unable to open preset file: {}", filename);
                return false;
            }

            nlohmann::json j;
            file >> j;
            file.close();
            // Parse JSON into class members
            grainRadius = j["grainRadius"].get<float>();
            slowRadius = j["slowRadius"].get<float>();
            slowPivot = j["slowPivot"].get<float>();
            grainScale = j["grainScale"].get<float>();
            grainSat = j["grainSat"].get<float>();
            grainSigma = j["grainSigma"].get<float>();
            highlights = j["highlights"].get<float>();
            midtones = j["midtones"].get<float>();
            shadows = j["shadows"].get<float>();
            midPivot = j["midPivot"].get<float>();
            midSlope = j["midSlope"].get<float>();
            redMix = j["redMix"].get<float>();
            greenMix = j["greenMix"].get<float>();
            blueMix = j["blueMix"].get<float>();
            grainBlend = j["grainBlend"].get<float>();
            monteCarlo = j["monteCarlo"].get<int>();
            colorspace = j["colorspace"].get<int>();
            presetName = j["presetName"].get<std::string>();

            return true;
        } catch (const std::exception& e) {
            LOG_WARN("Unable to load grain metadata: {}", e.what());
            return false;
        }
}

bool grainParams::saveMeta(const std::string &filename) {
    try {
                nlohmann::json j;

                // Convert class members to JSON
                j["grainRadius"] = grainRadius;
                j["slowRadius"] = slowRadius;
                j["slowPivot"] = slowPivot;
                j["grainScale"] = grainScale;
                j["grainSat"] = grainSat;
                j["grainSigma"] = grainSigma;

                j["highlights"] = highlights;
                j["midtones"] = midtones;
                j["shadows"] = shadows;
                j["midPivot"] = midPivot;
                j["midSlope"] = midSlope;

                j["redMix"] = redMix;
                j["greenMix"] = greenMix;
                j["blueMix"] = blueMix;
                j["grainBlend"] = grainBlend;

                j["monteCarlo"] = monteCarlo;
                j["colorspace"] = colorspace;
                j["presetName"] = presetName;

                // Write to file
                std::string corrected = filename;
                std::filesystem::path filePath = filename;
                if (!filePath.has_extension()) {
                    //No extension, add tgrain
                    corrected += ".tgrain";
                }
                std::ofstream file(corrected);
                if (!file.is_open()) {
                    return false;
                }

                file << j.dump(4); // Pretty print with indent of 4
                file.close();
                return true;
            } catch (const std::exception& e) {
                return false;
            }
}
