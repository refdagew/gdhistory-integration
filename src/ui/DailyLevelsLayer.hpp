#pragma once

#include <Geode/Geode.hpp>

#include <vector>

#include "../api/Client.hpp"
#include "../api/DailyModels.hpp"
#include "../api/LevelDownloader.hpp"

class GDHistoryDailyLevelsLayer final : public cocos2d::CCLayerColor {
public:
    static cocos2d::CCScene* scene();
    static GDHistoryDailyLevelsLayer* create();
    bool init();
    void onExit() override;

private:
    gdhistory::api::Client m_client;
    gdhistory::api::LevelDownloader m_downloader;
    cocos2d::CCLabelBMFont* m_status = nullptr;
    cocos2d::CCMenu* m_resultsMenu = nullptr;
    cocos2d::CCMenu* m_navigationMenu = nullptr;
    cocos2d::CCMenuItemLabel* m_previousItem = nullptr;
    cocos2d::CCMenuItemLabel* m_nextItem = nullptr;
    std::size_t m_offset = 0;
    std::vector<gdhistory::api::TimedLevelResult> m_levels;

    void onDailyResponse(gdhistory::api::JsonResponse response);
    void renderPage();
    void onPreviousPage(cocos2d::CCObject* sender);
    void onNextPage(cocos2d::CCObject* sender);
    void onSelectLevel(cocos2d::CCObject* sender);
    void onBack(cocos2d::CCObject* sender);
};
