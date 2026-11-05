#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <winhttp.h>

#include <string>
#include <string_view>
#include <utility>

#include <Geode/utils/web.hpp>
#include "Response.hpp"

namespace gdhistory::api::winhttp_transport {

class Handle {
public:
    explicit Handle(HINTERNET value = nullptr) : m_value(value) {}
    ~Handle() { if (m_value) WinHttpCloseHandle(m_value); }
    Handle(Handle const&) = delete;
    Handle& operator=(Handle const&) = delete;
    Handle(Handle&& other) noexcept : m_value(std::exchange(other.m_value, nullptr)) {}
    HINTERNET get() const { return m_value; }
    explicit operator bool() const { return m_value != nullptr; }
private:
    HINTERNET m_value;
};

inline std::wstring widen(std::string_view value) {
    if (value.empty()) return {};
    int count = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, value.data(),
        static_cast<int>(value.size()), nullptr, 0);
    if (count <= 0) return {};
    std::wstring out(static_cast<std::size_t>(count), L'\\0');
    MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, value.data(),
        static_cast<int>(value.size()), out.data(), count);
    return out;
}

inline std::string encode(std::string_view value) {
    static constexpr char hex[] = "0123456789ABCDEF";
    std::string out;
    for (unsigned char c : value) {
        if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
            (c >= '0' && c <= '9') || c == '-' || c == '_' || c == '.' || c == '~') {
            out.push_back(static_cast<char>(c));
        } else {
            out.push_back('%');
            out.push_back(hex[c >> 4]);
            out.push_back(hex[c & 15]);
        }
    }
    return out;
}

inline std::string makeUrl(
    geode::utils::web::WebRequest const& request,
    std::string url
) {
    bool first = url.find('?') == std::string::npos;
    for (auto const& [key, value] : request.getUrlParams()) {
        url.push_back(first ? '?' : '&');
        first = false;
        url += encode(key);
        url.push_back('=');
        url += encode(value);
    }
    return url;
}

inline ApiResponse perform(std::string const& url, std::string const& headers) {
    ApiResponse out;
    auto fail = [&out](char const* operation) {
        auto code = GetLastError();
        out.statusCode = -10000 - static_cast<int>(code);
        out.error = std::string("WinHTTP ") + operation +
            " failed (Windows error " + std::to_string(code) + ").";
        return out;
    };

    auto wideUrl = widen(url);
    if (wideUrl.empty()) {
        out.statusCode = -10001;
        out.error = "WinHTTP could not convert the GDHistory URL to UTF-16.";
        return out;
    }

    URL_COMPONENTS parts{};
    parts.dwStructSize = sizeof(parts);
    parts.dwSchemeLength = static_cast<DWORD>(-1);
    parts.dwHostNameLength = static_cast<DWORD>(-1);
    parts.dwUrlPathLength = static_cast<DWORD>(-1);
    parts.dwExtraInfoLength = static_cast<DWORD>(-1);
    if (!WinHttpCrackUrl(wideUrl.c_str(), 0, 0, &parts)) return fail("URL parsing");

    std::wstring host(parts.lpszHostName, parts.dwHostNameLength);
    std::wstring path = parts.dwUrlPathLength
        ? std::wstring(parts.lpszUrlPath, parts.dwUrlPathLength) : L"/";
    if (parts.dwExtraInfoLength) path.append(parts.lpszExtraInfo, parts.dwExtraInfoLength);

    Handle session(WinHttpOpen(L"gdhistory-integration/0.1.0",
        WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME,
        WINHTTP_NO_PROXY_BYPASS, 0));
    if (!session) return fail("session open");
    WinHttpSetTimeouts(session.get(), 5000, 5000, 30000, 30000);

    Handle connection(WinHttpConnect(session.get(), host.c_str(), parts.nPort, 0));
    if (!connection) return fail("connect");

    Handle requestHandle(WinHttpOpenRequest(connection.get(), L"GET", path.c_str(),
        nullptr, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES,
        parts.nScheme == INTERNET_SCHEME_HTTPS ? WINHTTP_FLAG_SECURE : 0));
    if (!requestHandle) return fail("request open");

    auto wideHeaders = widen(headers);
    auto headerPtr = wideHeaders.empty() ? WINHTTP_NO_ADDITIONAL_HEADERS : wideHeaders.c_str();
    auto headerLen = wideHeaders.empty() ? 0 : static_cast<DWORD>(wideHeaders.size());
    if (!WinHttpSendRequest(requestHandle.get(), headerPtr, headerLen,
        WINHTTP_NO_REQUEST_DATA, 0, 0, 0)) return fail("send");
    if (!WinHttpReceiveResponse(requestHandle.get(), nullptr)) return fail("receive");

    DWORD status = 0;
    DWORD statusSize = sizeof(status);
    if (!WinHttpQueryHeaders(requestHandle.get(),
        WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
        WINHTTP_HEADER_NAME_BY_INDEX, &status, &statusSize, WINHTTP_NO_HEADER_INDEX)) {
        return fail("status query");
    }
    out.statusCode = static_cast<int>(status);

    for (;;) {
        DWORD available = 0;
        if (!WinHttpQueryDataAvailable(requestHandle.get(), &available)) return fail("body size query");
        if (available == 0) break;
        std::string chunk(static_cast<std::size_t>(available), '\\0');
        DWORD received = 0;
        if (!WinHttpReadData(requestHandle.get(), chunk.data(), available, &received)) return fail("body read");
        chunk.resize(received);
        out.body += chunk;
        if (received == 0) break;
    }
    return out;
}

} // namespace gdhistory::api::winhttp_transport
