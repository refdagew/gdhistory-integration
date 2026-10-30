#pragma once

#include <Geode/Geode.hpp>

#include <string>
#include <vector>

#include "../api/Client.hpp"
#include "../api/LevelDownloader.hpp"
#include "../api/Models.hpp"

class GDHistorySearchResultsLayer final : public cocos2d::CCLayerColor {
public:
    static cocos2d::CCScene* scene(std::string query);
    static GDHistorySearchResultsLayer* create(std::string query);
    bool init(std::string query);

private:
    gdhistory::api::Client m_client;
    gdhistory::api::LevelDownloader m_downloader;
    cocos2d::CCLabelBMFont* m_status = nullptr;
    cocos2d::CCMenu* m_resultsMenu = nullptr;
    std::vector<gdhistory::api::LevelSearchResult> m_results;

    void onSearchResponse(gdhistory::api::JsonResponse response);
    void onSelectLevel(cocos2d::CCObject* sender);
    void onBack(cocos2d::CCObject* sender);
};
