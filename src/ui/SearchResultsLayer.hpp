#pragma once

#include <Geode/Geode.hpp>

#include <string>
#include <vector>

#include "../api/Client.hpp"
#include "../api/LevelDownloader.hpp"
#include "../api/Models.hpp"

class GDHistorySearchResultsLayer final : public cocos2d::CCLayerColor {
public:
    static cocos2d::CCScene* scene(std::string query, std::string sort = "cache_downloads:desc");
    static GDHistorySearchResultsLayer* create(std::string query, std::string sort);
    bool init(std::string query, std::string sort);

private:
    static constexpr int PAGE_SIZE = 9;

    gdhistory::api::Client m_client;
    gdhistory::api::LevelDownloader m_downloader;
    cocos2d::CCLabelBMFont* m_status = nullptr;
    cocos2d::CCMenu* m_resultsMenu = nullptr;
    cocos2d::CCMenu* m_navigationMenu = nullptr;
    cocos2d::CCMenuItemLabel* m_previousItem = nullptr;
    cocos2d::CCMenuItemLabel* m_nextItem = nullptr;
    std::vector<gdhistory::api::LevelSearchResult> m_results;
    std::string m_query;
    std::string m_sort;
    int m_offset = 0;

    void requestPage();
    void onSearchResponse(gdhistory::api::JsonResponse response);
    void onSelectLevel(cocos2d::CCObject* sender);
    void onPreviousPage(cocos2d::CCObject* sender);
    void onNextPage(cocos2d::CCObject* sender);
    void onBack(cocos2d::CCObject* sender);
};
