#pragma once

#include <Geode/Geode.hpp>
#include <Geode/binding/GJGameLevel.hpp>
#include <hjfod.gmd-api/include/GMD.hpp>

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <string>

#include "Response.hpp"

namespace gdhistory::api {

struct GMDImportResult {
    GJGameLevel* level = nullptr;
    std::string error;

    bool ok() const {
        return level != nullptr && error.empty();
    }
};

/// Import the downloaded GMD bytes through HJfod's maintained GMD API.
/// The temporary file is kept inside this mod's save directory and removed
/// after import; no GDLVL conversion or browser download is involved.
inline GMDImportResult importGMD(
    geode::utils::web::WebResponse const& response,
    std::uint64_t levelID,
    std::uint64_t recordID
) {
    auto bytes = responseBytes(response);
    if (bytes.empty()) {
        return {nullptr, "The GMD download was empty."};
    }

    auto path = geode::Mod::get()->getSaveDir() /
        fmt::format("level_{}_{}.gmd", levelID, recordID);

    std::ofstream output(path, std::ios::binary | std::ios::trunc);
    if (!output) {
        return {nullptr, "Could not create a temporary GMD file."};
    }

    output.write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
    output.close();

    if (!output) {
        std::error_code ignored;
        std::filesystem::remove(path, ignored);
        return {nullptr, "Could not write the downloaded GMD file."};
    }

    auto imported = gmd::importGmdAsLevel(path);

    std::error_code ignored;
    std::filesystem::remove(path, ignored);

    if (!imported) {
        return {nullptr, fmt::format("Could not import GMD: {}", imported.unwrapErr())};
    }

    auto level = imported.unwrap();
    if (!level) {
        return {nullptr, "GMD import returned an empty level."};
    }

    level->setLevelID(static_cast<int>(levelID));
    return {level, {}};
}

} // namespace gdhistory::api
