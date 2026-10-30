#pragma once

#include <Geode/utils/web.hpp>

#include <algorithm>
#include <cctype>
#include <string>
#include <string_view>

namespace gdhistory::api {

inline bool isHttpSuccess(geode::utils::web::WebResponse const& response) {
    return response.ok() && response.code() >= 200 && response.code() < 300;
}

/// Copies the response bytes without assuming that the payload is UTF-8.
/// Use this for GMD downloads, which must remain byte-for-byte intact.
inline std::string responseBytes(geode::utils::web::WebResponse const& response) {
    auto const& bytes = response.data();
    if (bytes.empty()) {
        return {};
    }

    return std::string(
        reinterpret_cast<char const*>(bytes.data()),
        bytes.size()
    );
}

inline std::string lowercase(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return value;
}

/// The extension identifies this exact GDHistory message as the one condition
/// that permits trying the next older record. Other errors must not fall back.
inline bool isRecordWithoutLevelData(std::string_view body) {
    return lowercase(std::string(body)).find(
        "this record does not contain any level data"
    ) != std::string::npos;
}

} // namespace gdhistory::api
