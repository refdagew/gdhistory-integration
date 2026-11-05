#pragma once

#include <matjson.hpp>

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

inline JsonResponse parseJsonResponse(ApiResponse const& response) {
    if (!isHttpSuccess(response)) {
        std::string error = response.code() < 0
            ? fmt::format("GDHistory network request failed (cURL/WinHTTP {}): {}",
                response.code(),
                response.error.empty() ? "No additional error message." : response.error)
            : fmt::format("GDHistory returned HTTP {}: {}",
                response.code(),
                response.error);

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

    auto const& body = response.body;
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
