#pragma once

#include <Geode/utils/web.hpp>
#include <matjson.hpp>

#include <algorithm>
#include <optional>
#include <string>
#include <utility>

#include "Response.hpp"

namespace gdhistory::api {

struct JsonResponse {
    std::optional<matjson::Value> value;
    std::string error;

    bool ok() const {
        return value.has_value() && error.empty();
    }
};

inline JsonResponse parseJsonResponse(
    geode::utils::web::WebResponse const& response
) {
    if (!isHttpSuccess(response)) {
        auto const errorMessage = response.errorMessage();
        auto const verboseLogs = response.verboseLogs();

        // Negative response codes are libcurl/Geode transport errors, not HTTP
        // status codes. Preserve the underlying diagnostic so it can be shown
        // in the in-game status label even when external interceptors only
        // expose the numeric code.
        std::string error = response.code() < 0
            ? fmt::format("GDHistory network request failed (cURL {}): {}",
                response.code(),
                errorMessage.empty() ? "No additional error message." : errorMessage)
            : fmt::format("GDHistory returned HTTP {}: {}",
                response.code(),
                errorMessage);

        if (!verboseLogs.empty()) {
            error += fmt::format("\nNetwork details: {}", verboseLogs);
        }

        constexpr std::size_t MAX_ERROR_LENGTH = 900;
        if (error.size() > MAX_ERROR_LENGTH) {
            error.resize(MAX_ERROR_LENGTH);
            error += "...";
        }

        return {
            .value = std::nullopt,
            .error = std::move(error)
        };
    }

    auto body = responseBytes(response);
    if (body.empty()) {
        return {
            .value = std::nullopt,
            .error = "GDHistory returned an empty JSON response."
        };
    }

    auto parsed = matjson::Value::parse(body);
    if (!parsed) {
        return {
            .value = std::nullopt,
            .error = fmt::format(
                "Could not parse GDHistory JSON: {}",
                parsed.unwrapErr().message
            )
        };
    }

    return {
        .value = parsed.unwrap(),
        .error = {}
    };
}

} // namespace gdhistory::api
