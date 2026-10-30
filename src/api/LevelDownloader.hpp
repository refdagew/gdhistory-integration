#pragma once

#include <Geode/Geode.hpp>

#include <cstdint>
#include <functional>
#include <string>

#include "Client.hpp"
#include "HistoricalDownloader.hpp"
#include "Models.hpp"

namespace gdhistory::api {

/// Resolves a level's record list and downloads the newest usable GMD record.
class LevelDownloader {
public:
    using Callback = std::function<void(HistoricalDownloadResult)>;

    LevelDownloader() = default;
    LevelDownloader(LevelDownloader const&) = delete;
    LevelDownloader& operator=(LevelDownloader const&) = delete;

    void cancel() {
        m_recordsRequest.cancel();
        m_history.cancel();
        m_callback = nullptr;
    }

    void download(std::uint64_t levelID, Callback callback) {
        this->cancel();
        m_callback = std::move(callback);

        if (levelID == 0) {
            this->fail("A valid Geometry Dash level ID is required.");
            return;
        }

        m_recordsRequest.getLevelRecordsJSON(
            levelID,
            [this, levelID](JsonResponse result) mutable {
                if (!result.ok()) {
                    this->fail(std::move(result.error));
                    return;
                }

                auto const& payload = *result.value;
                if (payload["success"].isBool()) {
                    auto success = payload["success"].asBool();
                    if (success && !success.unwrap()) {
                        this->fail("GDHistory reports that this level has no historical records.");
                        return;
                    }
                }

                auto recordIDs = parseRecordIDs(payload);
                if (recordIDs.empty()) {
                    this->fail("GDHistory returned no usable historical record IDs.");
                    return;
                }

                m_history.start(
                    levelID,
                    std::move(recordIDs),
                    [this](HistoricalDownloadResult outcome) mutable {
                        if (!m_callback) return;
                        auto callback = std::move(m_callback);
                        callback(std::move(outcome));
                    }
                );
            }
        );
    }

private:
    Client m_recordsRequest;
    HistoricalDownloader m_history;
    Callback m_callback;

    void fail(std::string message) {
        if (!m_callback) return;

        auto callback = std::move(m_callback);
        callback(HistoricalDownloadResult {
            .recordID = 0,
            .response = std::nullopt,
            .error = std::move(message)
        });
    }
};

} // namespace gdhistory::api
