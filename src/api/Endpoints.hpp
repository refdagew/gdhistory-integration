#pragma once

#include <cstdint>
#include <string>

namespace gdhistory::api {

inline constexpr char kBaseUrl[] = "https://history.geometrydash.eu";

inline std::string searchLevels() {
    return std::string(kBaseUrl) + "/api/v1/search/level/advanced/";
}

inline std::string levelRecords(std::uint64_t levelID) {
    return std::string(kBaseUrl) + "/api/v1/level/" + std::to_string(levelID) + "/";
}

inline std::string dailyLevels() {
    return std::string(kBaseUrl) + "/api/v1/daily/";
}

// Deliberately fixed to GMD. Do not change this to gdlvl.
inline std::string downloadGMD(std::uint64_t levelID, std::uint64_t recordID) {
    return std::string(kBaseUrl) + "/level/" +
        std::to_string(levelID) + "/" +
        std::to_string(recordID) + "/download/gmd/";
}

} // namespace gdhistory::api
