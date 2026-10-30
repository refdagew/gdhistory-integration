#pragma once

#include <Geode/Geode.hpp>
#include <Geode/utils/async.hpp>
#include <Geode/utils/web.hpp>

#include <cstdint>
#include <string>
#include <utility>

#include "Endpoints.hpp"

namespace gdhistory::api {

/// Native GDHistory HTTP client. Requests are asynchronous and their callbacks
/// are dispatched by Geode on the main thread.
class Client {
public:
    using ResponseCallback = geode::Function<void(geode::utils::web::WebResponse)>;

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

        this->send(std::move(request), searchLevels(), std::move(callback));
    }

    /// Retrieve a level and its historical records.
    void getLevelRecords(std::uint64_t levelID, ResponseCallback callback) {
        this->get(levelRecords(levelID), std::move(callback));
    }

    /// Retrieve GDHistory's daily-level collection.
    void getDailyLevels(ResponseCallback callback) {
        this->get(dailyLevels(), std::move(callback));
    }

    /// Download a historical record in GMD format. GDLVL is intentionally
    /// unsupported by this project.
    void downloadGMD(
        std::uint64_t levelID,
        std::uint64_t recordID,
        ResponseCallback callback
    ) {
        this->get(api::downloadGMD(levelID, recordID), std::move(callback));
    }

private:
    geode::async::TaskHolder<geode::utils::web::WebResponse> m_request;

    static geode::utils::web::WebRequest baseRequest() {
        auto request = geode::utils::web::WebRequest();
        request
            .header("Accept", "application/json")
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
