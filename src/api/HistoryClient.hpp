#pragma once

#include <Geode/Geode.hpp>
#include <Geode/utils/web.hpp>
#include <functional>
#include <string>

namespace gdhistory::api {

struct HttpResult {
    bool success = false;
    int statusCode = 0;
    std::string body;
    std::string error;
};

using HttpCallback = std::function<void(HttpResult)>;

class HistoryClient {
public:
    static void get(std::string url, HttpCallback callback);
    static void searchLevels(std::string query, std::string sort, int limit, int offset, HttpCallback callback);
    static void getLevelRecords(std::uint64_t levelID, HttpCallback callback);
    static void getDailyLevels(HttpCallback callback);
    static void downloadGMD(std::uint64_t levelID, std::uint64_t recordID, HttpCallback callback);
};

} // namespace gdhistory::api
