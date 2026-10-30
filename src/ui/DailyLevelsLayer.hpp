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

private:
    gdhistory::api::Client m_client;
    gdhistory::api::LevelDownloader m_downloader;
    cocos2d::CCLabelBMFont* m_status = nullptr;
    cocos2d::CCMenu* m_resultsMenu = nullptr;
    std::vector<gdhistory::api::TimedLevelResult> m_levels;

    void onDailyResponse(gdhistory::api::JsonResponse response);
    void onSelectLevel(cocos2d::CCObject* sender);
    void onBack(cocos2d::CCObject* sender);
};
