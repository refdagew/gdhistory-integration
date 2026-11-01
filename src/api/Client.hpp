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

using namespace geode::prelude;

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
        ResponseCallback callback,
        std::string sort = "cache_downloads:desc"
    ) {
        auto request = baseRequest();
        request
            .param("limit", std::max(1, std::min(limit, 1000)))
            .param("offset", std::max(0, offset))
            .param("sort", std::move(sort));

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
        JsonCallback callback,
        std::string sort = "cache_downloads:desc"
    ) {
        this->searchLevels(
            std::move(query), limit, offset,
            [callback = std::move(callback)](geode::utils::web::WebResponse response) mutable {
                callback(parseJsonResponse(response));
            },
            std::move(sort)
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
            // Do not leave keep-alive sockets around; this avoids retrying over
            // a stale connection after the server or network has closed it.
            .header("Connection", "close")
            .userAgent("gdhistory-integration/0.1.0 (Geode mod)")
            .timeout(std::chrono::seconds(30))
            .followRedirects(true)
            // Avoid intermittent Windows libcurl failures seen with HTTP/2
            // connections (CURLE_SSL_CONNECT_ERROR / CURLE_SEND_ERROR).
            // GDHistory supports HTTP/1.1, so use the more compatible protocol.
            .version(geode::utils::web::HttpVersion::VERSION_1_1);
        return request;
    }

    void get(std::string url, ResponseCallback callback) {
        auto request = baseRequest();
        this->send(std::move(request), std::move(url), std::move(callback));
    }

    static bool isTransientNetworkError(int code) {
        // libcurl reports these before receiving an HTTP response. Both can be
        // transient when a connection is interrupted or a TLS handshake fails.
        return code == 35 || code == 55;
    }

    void send(
        geode::utils::web::WebRequest request,
        std::string url,
        ResponseCallback callback,
        int retriesRemaining = 2
    ) {
        // Keep a copy of the configured request so a retry preserves headers,
        // query parameters, and the selected HTTP version.
        auto retryRequest = request;
        // Start the transfer before moving url into the completion callback.
        // Function-argument evaluation order is unspecified; doing both inside
        // spawn(request.get(url), [url = std::move(url)]...) can empty the URL
        // before request.get() reads it, producing cURL error 3.
        // Geode 5.11.2 enables TLS early data for the exact uppercase
        // method "GET". Sending the same safe, read-only method in lowercase
        // bypasses that code path; Django normalizes REQUEST_METHOD to
        // uppercase before routing, so GDHistory still receives a GET.
        auto future = request.send("get", url);
        m_request.spawn(
            std::move(future),
            [this,
             retryRequest = std::move(retryRequest),
             url = std::move(url),
             callback = std::move(callback),
             retriesRemaining](geode::utils::web::WebResponse response) mutable {
                if (retriesRemaining > 0 && isTransientNetworkError(response.code())) {
                    log::warn(
                        "GDHistory request failed with transient cURL error {}; retrying ({} attempt(s) left)",
                        response.code(),
                        retriesRemaining
                    );
                    this->send(
                        std::move(retryRequest),
                        std::move(url),
                        std::move(callback),
                        retriesRemaining - 1
                    );
                    return;
                }

                callback(std::move(response));
            }
        );
    }
};

} // namespace gdhistory::api
