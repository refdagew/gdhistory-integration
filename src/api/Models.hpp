#pragma once

#include <matjson.hpp>

#include <cstdint>
#include <cstdlib>
#include <string>
#include <vector>

namespace gdhistory::api {

struct LevelSearchResult {
    std::uint64_t levelID = 0;
    std::string name;
    std::string creator;
    std::uint64_t downloads = 0;
    int difficulty = 0;
};

inline std::string jsonString(matjson::Value const& value) {
    if (!value.isString()) return {};
    auto parsed = value.asString();
    return parsed ? parsed.unwrap() : std::string{};
}

inline std::uint64_t jsonID(matjson::Value const& value) {
    if (value.isNumber()) {
        auto parsed = value.asUInt();
        return parsed ? parsed.unwrap() : 0;
    }

    if (value.isString()) {
        auto text = jsonString(value);
        if (text.empty()) return 0;

        char* end = nullptr;
        auto parsed = std::strtoull(text.c_str(), &end, 10);
        return end != text.c_str() && *end == '\0' ? parsed : 0;
    }

    return 0;
}

inline std::uint64_t firstID(
    matjson::Value const& value,
    std::initializer_list<char const*> keys
) {
    for (auto key : keys) {
        auto id = jsonID(value[key]);
        if (id != 0) return id;
    }
    return 0;
}

inline std::string firstString(
    matjson::Value const& value,
    std::initializer_list<char const*> keys
) {
    for (auto key : keys) {
        auto text = jsonString(value[key]);
        if (!text.empty()) return text;
    }
    return {};
}

inline matjson::Value const* findArray(
    matjson::Value const& value,
    std::initializer_list<char const*> keys
) {
    if (value.isArray()) return &value;

    for (auto key : keys) {
        auto const& candidate = value[key];
        if (candidate.isArray()) return &candidate;
    }

    return nullptr;
}

/// Parse the response variants used by GDHistory's search endpoint.
inline std::vector<LevelSearchResult> parseSearchResults(matjson::Value const& payload) {
    std::vector<LevelSearchResult> results;
    auto array = findArray(payload, {"hits", "results", "data"});
    if (!array) return results;

    for (auto const& entry : array->asArray().unwrap()) {
        if (!entry.isObject()) continue;

        LevelSearchResult result;
        result.levelID = firstID(entry, {"online_id", "id"});
        if (result.levelID == 0) continue;

        result.name = firstString(entry, {"name", "level_name", "cache_level_name"});
        result.creator = firstString(entry, {"creator", "creator_name", "username", "cache_username"});
        result.downloads = firstID(entry, {"cache_downloads", "downloads"});
        result.difficulty = static_cast<int>(
            firstID(entry, {"cache_filter_difficulty", "difficulty"})
        );

        results.push_back(std::move(result));
    }

    return results;
}

/// Extract record IDs from the common historical-record collection keys.
inline std::vector<std::uint64_t> parseRecordIDs(matjson::Value const& payload) {
    std::vector<std::uint64_t> ids;
    auto array = findArray(
        payload,
        {"records", "level_records", "levelRecords", "versions", "history", "data", "hits", "history_records"}
    );
    if (!array) return ids;

    for (auto const& entry : array->asArray().unwrap()) {
        auto id = entry.isObject()
            ? firstID(entry, {"id", "record_id", "download_id", "level_record_id", "online_id"})
            : jsonID(entry);

        if (id != 0) ids.push_back(id);
    }

    return ids;
}

} // namespace gdhistory::api
