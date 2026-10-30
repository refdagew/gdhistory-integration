#pragma once

#include <Geode/utils/web.hpp>
#include <matjson.hpp>

#include <optional>
#include <string>

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
        return {
            .value = std::nullopt,
            .error = fmt::format(
                "GDHistory returned HTTP {}: {}",
                response.code(),
                response.errorMessage()
            )
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
