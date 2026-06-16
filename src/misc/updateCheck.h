#ifndef _UPDATECHECK_H
#define _UPDATECHECK_H

#include "structs.h"          // ReleaseStage, VERSTAGE, version macros + validation
#include "nlohmann/json.hpp"
#include <spdlog/fmt/fmt.h>
#include <string>
#include <optional>
#include <stdexcept>
#include <tuple>
#include <chrono>

struct Version {
    uint16_t major = 0;
    uint16_t minor = 0;
    uint16_t patch = 0;
    ReleaseStage stage = ReleaseStage::Release;

    bool operator>(const Version& b) const {
        auto stageRank = [](ReleaseStage s) -> int {
            switch (s) {
                case ReleaseStage::Alpha:   return 0;
                case ReleaseStage::Beta:    return 1;
                case ReleaseStage::Release: return 2;
                default: throw std::invalid_argument("Unknown ReleaseStage");
            }
        };

        auto lhs = std::tie(major,    minor,    patch);
        auto rhs = std::tie(b.major,  b.minor,  b.patch);

        return lhs > rhs
            || (lhs == rhs && stageRank(stage) > stageRank(b.stage));
    }

    std::string versionString() const {
        const char* stageStr = stage == ReleaseStage::Alpha ? "a"
                             : stage == ReleaseStage::Beta  ? "b" : "";
        return fmt::format("{}.{}.{}{}", major, minor, patch, stageStr);
    }

    NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(Version, major, minor, patch, stage);


};

std::string fetchLatestReleaseJson();
std::optional<Version> checkForUpdates(std::string& version, std::string& body, uint64_t& lastCheckTime);
Version parseVersion(const std::string& tag);


constexpr const char* queryURL    = "https://filmvert.app/update";
constexpr const char* downloadURL = "https://filmvert.app/download";

#endif
