#include "updateCheck.h"
#include "logger.h"
#include "structs.h"
#include "utils.h"
#include "curl/curl.h"
#include "nlohmann/json.hpp"
#include <optional>
#include <stdexcept>
#include <string>
#include <spdlog/fmt/fmt.h>

// libcurl write callback — appends received data into a std::string
static size_t WriteCallback(void* contents, size_t size,
                            size_t nmemb, std::string* output) {
    constexpr size_t kMaxBytes = 1 * 1024 * 1024; // 1 MB ceiling
    size_t incoming = size * nmemb;
    if (output->size() + incoming > kMaxBytes)
        return 0; // returning 0 causes curl to abort with CURLE_WRITE_ERROR
    output->append(static_cast<char*>(contents), incoming);
    return incoming;
}

// Returns the raw JSON string, or empty string on failure
std::string fetchLatestReleaseJson() {
    std::string response;
    CURL* curl = curl_easy_init();
    if (!curl) return {};
    std::string userAgent = fmt::format("Filmvert/{}.{}.{}", VERMAJOR, VERMINOR, VERPATCH);
    curl_easy_setopt(curl, CURLOPT_URL, queryURL);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, userAgent.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 3L);
    curl_easy_setopt(curl, CURLOPT_REDIR_PROTOCOLS_STR, "https");

    CURLcode res = curl_easy_perform(curl);
    long httpCode = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK || httpCode != 200) {
        LOG_WARN("Unable to check for updates: {}", curl_easy_strerror(res));
        return std::string{};
    } else {
        return response;
    }
}


std::optional<Version> checkForUpdates(std::string& version, std::string& body, uint64_t& lastCheckTime) {
    // Only check once a day
    if (currentEpoch() - lastCheckTime < 86400)
        return std::nullopt;

    // Perform online check
    LOG_INFO("Checking for new releases...");
    std::string releaseJSON = fetchLatestReleaseJson();
    if (releaseJSON.empty()) {
        LOG_ERROR("Invalid web request");
        return std::nullopt;
    }

    try {
        nlohmann::json j;
        j = nlohmann::json::parse(releaseJSON);

        if (j.contains("tag_name")) {
            std::string tagName = j["tag_name"].get<std::string>();
            if (j.contains("body"))
                body = j["body"].get<std::string>();

            // Current version assembled entirely from compile-time macros.
            // Change VERSTAGE in structs.h for alpha/beta builds.
            Version curVer;
            curVer.major = VERMAJOR;
            curVer.minor = VERMINOR;
            curVer.patch = VERPATCH;
            curVer.stage = VERSTAGE;

            Version newVersion = parseVersion(tagName);

            if (newVersion > curVer) {
                version = newVersion.versionString();
                lastCheckTime = currentEpoch();
                LOG_INFO("New version found: v{}", version);
                return newVersion;
            } else {
                // We've gotten a successful response from the server
                // But the version is the same or older. Set the current
                // check time to now
                LOG_INFO("No new version found");
                lastCheckTime = currentEpoch();
                return std::nullopt;
            }

        } else {
            // We've gotten a response from the server but the json doesn't
            // have what we're looking for
            throw std::runtime_error("Update query returned no valid result");
        }
    } catch (const std::exception& e) {
        // Something has gone wrong decoding the JSON. Update the last check
        // to currentepoch minus 22 hours so that it won't check again for
        // another 2 hours
        lastCheckTime = currentEpoch() - 79200;
        LOG_ERROR("Error checking release version: {}", e.what());
        return std::nullopt;
    }
}


Version parseVersion(const std::string& tag) {
    // Expect a leading 'v'
    if (tag.empty() || tag[0] != 'v')
        throw std::invalid_argument(fmt::format("Version tag must start with 'v': {}", tag));

    Version v{};
    std::size_t pos = 1; // skip the 'v'

    auto parseNumber = [&]() -> uint16_t {
        if (pos >= tag.size() || !std::isdigit(tag[pos]))
            throw std::invalid_argument(fmt::format("Expected digit at position {} in: {}", pos, tag));
        uint16_t num = 0;
        while (pos < tag.size() && std::isdigit(tag[pos]))
            num = num * 10 + (tag[pos++] - '0');
        return num;
    };

    v.major = parseNumber();

    if (pos >= tag.size() || tag[pos++] != '.')
        throw std::invalid_argument(fmt::format("Expected '.' after major version in: {}", tag));

    v.minor = parseNumber();

    if (pos >= tag.size() || tag[pos++] != '.')
        throw std::invalid_argument(fmt::format("Expected '.' after minor version in: {}", tag));

    v.patch = parseNumber();

    // Optional stage suffix
    v.stage = ReleaseStage::Release;
    if (pos < tag.size()) {
        if      (tag[pos] == 'a') v.stage = ReleaseStage::Alpha;
        else if (tag[pos] == 'b') v.stage = ReleaseStage::Beta;
        else throw std::invalid_argument(fmt::format("Unexpected suffix '{}' in: {}", tag[pos], tag));
        ++pos;
    }

    if (pos != tag.size())
        throw std::invalid_argument(fmt::format("Unexpected trailing characters in: {}", tag));

    return v;
}
