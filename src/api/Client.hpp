#pragma once

#include <Geode/Geode.hpp>
#include <Geode/utils/async.hpp>
#include <Geode/utils/web.hpp>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <thread>
#include <utility>

#include "Endpoints.hpp"
#include "Json.hpp"
#include "WinHttpTransport.hpp"

using namespace geode::prelude;

namespace gdhistory::api {

/// Native GDHistory client. It uses Geode's WebRequest first and falls back to
/// Windows WinHTTP for TLS/send failures. Both transports retain certificate
/// verification; the fallback uses the operating system's TLS trust store.
class Client {
public:
    using ResponseCallback = std::function<void(ApiResponse)>;
    using JsonCallback = std::function<void(JsonResponse)>;

    Client() = default;
    Client(Client const&) = delete;
    Client& operator=(Client const&) = delete;

    ~Client() {
        this->cancel();
    }

    void cancel() {
        m_request.cancel();
        if (m_cancelToken) {
            m_cancelToken->store(true, std::memory_order_relaxed);
        }
    }

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

    void getLevelRecords(std::uint64_t levelID, ResponseCallback callback) {
        this->get(levelRecords(levelID), std::move(callback));
    }

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
            [callback = std::move(callback)](ApiResponse response) mutable {
                callback(parseJsonResponse(response));
            },
            std::move(sort)
        );
    }

    void getLevelRecordsJSON(std::uint64_t levelID, JsonCallback callback) {
        this->getLevelRecords(
            levelID,
            [callback = std::move(callback)](ApiResponse response) mutable {
                callback(parseJsonResponse(response));
            }
        );
    }

    void getDailyLevelsJSON(JsonCallback callback) {
        this->getDailyLevels(
            [callback = std::move(callback)](ApiResponse response) mutable {
                callback(parseJsonResponse(response));
            }
        );
    }

    /// Historical downloads are always GMD. GDLVL is intentionally unsupported.
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
    std::shared_ptr<std::atomic_bool> m_cancelToken;

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

    static ApiResponse fromGeode(geode::utils::web::WebResponse const& response) {
        return ApiResponse {
            .statusCode = response.code(),
            .body = responseBytes(response),
            .error = std::string(response.errorMessage())
        };
    }

    static std::string requestHeaders(geode::utils::web::WebRequest const& request) {
        std::string headers;
        for (auto const& [name, values] : request.getHeaders()) {
            for (auto const& value : values) {
                headers += name;
                headers += ": ";
                headers += value;
                headers += "\r\n";
            }
        }
        return headers;
    }

    void send(
        geode::utils::web::WebRequest request,
        std::string url,
        ResponseCallback callback
    ) {
        this->cancel();
        auto token = std::make_shared<std::atomic_bool>(false);
        m_cancelToken = token;

        auto retryRequest = request;
        auto future = request.get(url);
        m_request.spawn(
            std::move(future),
            [retryRequest = std::move(retryRequest),
             url = std::move(url),
             callback = std::move(callback),
             token](geode::utils::web::WebResponse response) mutable {
                if (token->load(std::memory_order_relaxed)) return;

                // Geode stores transport errors as NEGATIVE values in
                // WebResponse::code(). GDIntercept reports these as -35
                // (CURLE_SSL_CONNECT_ERROR) and -55 (CURLE_SEND_ERROR).
                // The old positive comparisons meant this fallback was
                // never entered, even though the code was present.
                if (response.code() == -35 || response.code() == -55) {
                    auto fullUrl = winhttp_transport::makeUrl(retryRequest, url);
                    auto headers = requestHeaders(retryRequest);

                    try {
                        std::thread(
                            [token,
                             fullUrl = std::move(fullUrl),
                             headers = std::move(headers),
                             callback = std::move(callback)]() mutable {
                                auto result = winhttp_transport::perform(fullUrl, headers);
                                geode::queueInMainThread(
                                    [token, callback = std::move(callback),
                                     result = std::move(result)]() mutable {
                                        if (token->load(std::memory_order_relaxed)) return;
                                        callback(std::move(result));
                                    }
                                );
                            }
                        ).detach();
                    } catch (...) {
                        ApiResponse fallbackError {
                            .statusCode = -10002,
                            .body = {},
                            .error = "Could not start the native Windows network fallback."
                        };
                        callback(std::move(fallbackError));
                    }
                    return;
                }

                callback(fromGeode(response));
            }
        );
    }
};

} // namespace gdhistory::api
