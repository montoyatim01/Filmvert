#ifndef _grainparams_h
#define _grainparams_h
#include <stdint.h>
#include <nlohmann/json.hpp>
#include <fstream>
#include <string>

class grainParams {
    public:
    grainParams(){};
    ~grainParams(){};

    bool formatMeta();
    bool restoreMeta();
    bool loadMeta(const nlohmann::json& j);
    bool loadMeta(const std::string& filename);
    bool saveMeta(const std::string& filename);
    float grainRadius = 0.2f;
    float slowRadius = 0.0f;
    float slowPivot = 0.35f;
    float grainScale = 1.25f;
    float grainSat = 0.75f;
    float grainSigma = 0.8f;

    float highlights = 1.0f;
    float midtones = 1.0f;
    float shadows = 1.0f;
    float midPivot = 0.5f;
    float midSlope = 2.5f;

    float redMix = 1.0f;
    float greenMix = 1.0f;
    float blueMix = 1.0f;
    float grainBlend = 0.7f;

    int monteCarlo = 300;
    int colorspace = 100;
    std::string presetName = "";

    std::string metadata = "";
    private:

};


#endif
