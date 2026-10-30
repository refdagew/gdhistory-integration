#pragma once

#include <Geode/Geode.hpp>
#include <Geode/utils/async.hpp>
#include <Geode/utils/web.hpp>

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <functional>
#include <string>
#include <utility>

#include "Endpoints.hpp"
#include "Json.hpp"

namespace gdhistory::api {

/// Native GDHistory HTTP client. Requests are asynchronous and their callbacks
/// are dispatched by Geode on the main thread.
class Client {
public:
    using ResponseCallback = std::function<void(geode::utils::web::WebResponse)>;
    using JsonCallback = std::function<void(JsonResponse)>;

    Client() = default;
    Client(Client const&) = delete;
    Client& operator=(Client const&) = delete;

    /// Cancels the currently active request, if any.
    void cancel() {
        m_request.cancel();
    }

    /// Search levels using GDHistory's advanced search endpoint.
    void searchLevels(
        std::string query,
        int limit,
        int offset,
        ResponseCallback callback
    ) {
        auto request = baseRequest();
        request
            .param("limit", std::max(1, std::min(limit, 1000)))
            .param("offset", std::max(0, offset))
            .param("sort", "cache_downloads:desc");

        if (!query.empty()) {
            request.param("query", std::move(query));
        }

        this->send(std::move(request), ::gdhistory::api::searchLevels(), std::move(callback));
    }

    /// Retrieve a level and its historical records.
    void getLevelRecords(std::uint64_t levelID, ResponseCallback callback) {
        this->get(levelRecords(levelID), std::move(callback));
    }

    /// Retrieve GDHistory's daily-level collection.
    void getDailyLevels(ResponseCallback callback) {
        this->get(dailyLevels(), std::move(callback));
    }

    void searchLevelsJSON(
        std::string query,
        int limit,
        int offset,
        JsonCallback callback
    ) {
        this->searchLevels(
            std::move(query), limit, offset,
            [callback = std::move(callback)](geode::utils::web::WebResponse response) mutable {
                callback(parseJsonResponse(response));
            }
        );
    }

    void getLevelRecordsJSON(std::uint64_t levelID, JsonCallback callback) {
        this->getLevelRecords(
            levelID,
            [callback = std::move(callback)](geode::utils::web::WebResponse response) mutable {
                callback(parseJsonResponse(response));
            }
        );
    }

    void getDailyLevelsJSON(JsonCallback callback) {
        this->getDailyLevels(
            [callback = std::move(callback)](geode::utils::web::WebResponse response) mutable {
                callback(parseJsonResponse(response));
            }
        );
    }

    /// Download a historical record in GMD format. GDLVL is intentionally
    /// unsupported by this project.
    void downloadGMD(
        std::uint64_t levelID,
        std::uint64_t recordID,
        ResponseCallback callback
    ) {
        auto request = baseRequest("*/*");
        this->send(
            std::move(request),
            ::gdhistory::api::downloadGMD(levelID, recordID),
            std::move(callback)
        );
    }

private:
    geode::async::TaskHolder<geode::utils::web::WebResponse> m_request;

    static geode::utils::web::WebRequest baseRequest(std::string accept = "application/json") {
        auto request = geode::utils::web::WebRequest();
        request
            .header("Accept", std::move(accept))
            .userAgent("gdhistory-integration/0.1.0 (Geode mod)")
            .timeout(std::chrono::seconds(30))
            .followRedirects(true);
        return request;
    }

    void get(std::string url, ResponseCallback callback) {
        auto request = baseRequest();
        this->send(std::move(request), std::move(url), std::move(callback));
    }

    void send(
        geode::utils::web::WebRequest request,
        std::string url,
        ResponseCallback callback
    ) {
        m_request.spawn(
            request.get(std::move(url)),
            [callback = std::move(callback)](geode::utils::web::WebResponse response) mutable {
                callback(std::move(response));
            }
        );
    }
};

} // namespace gdhistory::api
