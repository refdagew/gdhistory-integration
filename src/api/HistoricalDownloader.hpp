#pragma once

#include <Geode/Geode.hpp>

#include <algorithm>
#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <vector>

#include "Client.hpp"
#include "Response.hpp"

namespace gdhistory::api {

struct HistoricalDownloadResult {
    std::uint64_t recordID = 0;
    std::optional<geode::utils::web::WebResponse> response;
    std::string error;

    bool hasData() const {
        return response.has_value() && error.empty();
    }
};

/// Downloads the newest available historical record first. The next record is
/// attempted only when GDHistory explicitly says the current record has no data.
class HistoricalDownloader {
public:
    using Callback = std::function<void(HistoricalDownloadResult)>;

    HistoricalDownloader() = default;
    HistoricalDownloader(HistoricalDownloader const&) = delete;
    HistoricalDownloader& operator=(HistoricalDownloader const&) = delete;

    void cancel() {
        m_client.cancel();
        m_records.clear();
        m_callback = nullptr;
        m_levelID = 0;
    }

    void start(
        std::uint64_t levelID,
        std::vector<std::uint64_t> recordIDs,
        Callback callback
    ) {
        this->cancel();

        m_levelID = levelID;
        m_records = std::move(recordIDs);
        m_callback = std::move(callback);

        std::sort(m_records.begin(), m_records.end(), std::greater<>());
        m_records.erase(
            std::unique(m_records.begin(), m_records.end()),
            m_records.end()
        );

        if (m_levelID == 0 || m_records.empty()) {
            this->finishFailure("GDHistory returned no usable historical records.");
            return;
        }

        this->tryNextRecord();
    }

private:
    Client m_client;
    std::uint64_t m_levelID = 0;
    std::vector<std::uint64_t> m_records;
    std::size_t m_nextIndex = 0;
    Callback m_callback;

    void tryNextRecord() {
        if (m_nextIndex >= m_records.size()) {
            this->finishFailure("No historical record contained level data.");
            return;
        }

        auto const recordID = m_records[m_nextIndex++];
        m_client.downloadGMD(
            m_levelID,
            recordID,
            [this, recordID](geode::utils::web::WebResponse response) mutable {
                auto body = responseBytes(response);

                if (isRecordWithoutLevelData(body)) {
                    log::debug(
                        "GDHistory record {} for level {} has no level data; trying older record.",
                        recordID,
                        m_levelID
                    );
                    this->tryNextRecord();
                    return;
                }

                if (!isHttpSuccess(response)) {
                    this->finishFailure(
                        fmt::format(
                            "GDHistory GMD download failed (HTTP {}): {}",
                            response.code(),
                            response.errorMessage()
                        )
                    );
                    return;
                }

                if (body.empty()) {
                    this->finishFailure("GDHistory returned an empty GMD download.");
                    return;
                }

                this->finishSuccess(recordID, std::move(response));
            }
        );
    }

    void finishSuccess(
        std::uint64_t recordID,
        geode::utils::web::WebResponse response
    ) {
        if (!m_callback) return;

        auto callback = std::move(m_callback);
        m_records.clear();
        m_levelID = 0;

        callback(HistoricalDownloadResult {
            .recordID = recordID,
            .response = std::move(response),
            .error = {}
        });
    }

    void finishFailure(std::string error) {
        if (!m_callback) return;

        auto callback = std::move(m_callback);
        m_records.clear();
        m_levelID = 0;

        callback(HistoricalDownloadResult {
            .recordID = 0,
            .response = std::nullopt,
            .error = std::move(error)
        });
    }
};

} // namespace gdhistory::api
