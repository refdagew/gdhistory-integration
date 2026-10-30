#pragma once

#include <matjson.hpp>

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

#include "Models.hpp"

namespace gdhistory::api {

struct TimedLevelResult {
    std::uint64_t levelID = 0;
    std::string category;
    std::string year;
    std::string name;
    std::string creator;
    std::uint64_t dailyNumber = 0;
};

inline std::string lowercaseKey(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return value;
}

inline bool isYearKey(std::string const& key) {
    return key.size() == 4 &&
        std::all_of(key.begin(), key.end(), [](unsigned char ch) {
            return std::isdigit(ch) != 0;
        });
}

inline void collectTimedLevels(
    matjson::Value const& value,
    std::string category,
    std::string year,
    std::vector<TimedLevelResult>& output
) {
    if (value.isArray()) {
        for (auto const& child : value.asArray().unwrap()) {
            collectTimedLevels(child, category, year, output);
        }
        return;
    }

    if (!value.isObject()) return;

    auto levelID = firstID(value, {"online_id", "level_id", "id"});
    if (levelID != 0) {
        auto ownCategory = firstString(value, {"type", "category", "level_type"});
        auto ownYear = firstString(value, {"year"});
        output.push_back({
            .levelID = levelID,
            .category = ownCategory.empty() ? category : lowercaseKey(ownCategory),
            .year = ownYear.empty() ? year : ownYear,
            .name = firstString(value, {"name", "level_name", "cache_level_name"}),
            .creator = firstString(value, {"creator", "creator_name", "username", "cache_username"}),
            .dailyNumber = firstID(value, {"daily_id", "daily_number", "number", "cache_daily_id"})
        });
        return;
    }

    for (auto const& [key, child] : value) {
        auto nextCategory = category;
        auto nextYear = year;
        auto normalized = lowercaseKey(key);

        if (normalized.find("weekly") != std::string::npos) {
            nextCategory = "weekly";
        } else if (normalized.find("daily") != std::string::npos) {
            nextCategory = "daily";
        } else if (normalized.find("event") != std::string::npos) {
            nextCategory = "event";
        }

        if (isYearKey(key)) {
            nextYear = key;
        }

        collectTimedLevels(child, std::move(nextCategory), std::move(nextYear), output);
    }
}

/// The daily endpoint is documented as grouping timely levels by type/year.
/// Walk the returned tree rather than assuming a fixed nesting depth.
inline std::vector<TimedLevelResult> parseDailyLevels(matjson::Value const& payload) {
    std::vector<TimedLevelResult> levels;
    collectTimedLevels(payload, {}, {}, levels);

    std::vector<TimedLevelResult> unique;
    unique.reserve(levels.size());
    for (auto& level : levels) {
        auto found = std::find_if(unique.begin(), unique.end(), [&](auto const& existing) {
            return existing.levelID == level.levelID;
        });
        if (found == unique.end()) {
            unique.push_back(std::move(level));
        }
    }

    return unique;
}

} // namespace gdhistory::api
