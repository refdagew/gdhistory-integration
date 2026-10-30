#include "HistoryClient.hpp"
#include "Endpoints.hpp"

#include <Geode/utils/async.hpp>
#include <chrono>
#include <sstream>

using namespace geode::prelude;

namespace gdhistory::api {

namespace {
std::string encodeQuery(std::string const& input) {
    static constexpr char hex[] = "0123456789ABCDEF";
    std::string out;
    out.reserve(input.size());

    for (unsigned char c : input) {
        if ((c >= 'a' && c <= 'z') ||
            (c >= 'A' && c <= 'Z') ||
            (c >= '0' && c <= '9') ||
            c == '-' || c == '_' || c == '.' || c == '~') {
            out.push_back(static_cast<char>(c));
        } else {
            out.push_back('%');
            out.push_back(hex[c >> 4]);
            out.push_back(hex[c & 0x0F]);
        }
    }

    return out;
}
}

void HistoryClient::get(std::string url, HttpCallback callback) {
    auto future = web::WebRequest()
        .header("Accept", "application/json, application/octet-stream")
        .userAgent("gdhistory-integration/0.1.0 (Geode mod)")
        .timeout(std::chrono::seconds(30))
        .followRedirects(true)
        .get(std::move(url));

    async::spawn(std::move(future), [callback = std::move(callback)](web::WebResponse response) mutable {
        HttpResult result;
        result.statusCode = response.code();

        if (response.error() || response.cancelled()) {
            result.error = std::string(response.errorMessage());
            if (result.error.empty()) {
                result.error = response.cancelled() ? "Request cancelled." : "Network request failed.";
            }
        } else {
            auto const& bytes = response.data();
            result.body.assign(bytes.begin(), bytes.end());
            result.success = response.ok();
            if (!result.success) {
                result.error = "GDHistory returned HTTP " + std::to_string(result.statusCode) + ".";
            }
        }

        if (callback) {
            callback(std::move(result));
        }
    });
}

void HistoryClient::searchLevels(
    std::string query,
    std::string sort,
    int limit,
    int offset,
    HttpCallback callback
) {
    std::ostringstream url;
    url << ::gdhistory::api::searchLevels() << "?limit=" << limit << "&offset=" << offset;

    if (!query.empty()) {
        url << "&query=" << encodeQuery(query);
    }
    if (!sort.empty()) {
        url << "&sort=" << encodeQuery(sort);
    }

    get(url.str(), std::move(callback));
}

void HistoryClient::getLevelRecords(std::uint64_t levelID, HttpCallback callback) {
    get(levelRecords(levelID), std::move(callback));
}

void HistoryClient::getDailyLevels(HttpCallback callback) {
    get(dailyLevels(), std::move(callback));
}

void HistoryClient::downloadGMD(
    std::uint64_t levelID,
    std::uint64_t recordID,
    HttpCallback callback
) {
    // GMD is intentionally hard-coded for Geometry Dash 2.2081.
    get(gdhistory::api::downloadGMD(levelID, recordID), std::move(callback));
}

} // namespace gdhistory::api
