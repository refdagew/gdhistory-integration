#include <Geode/Geode.hpp>
#include <Geode/modify/MenuLayer.hpp>

#include "../ui/DailyLevelsLayer.hpp"

using namespace geode::prelude;

class $modify(GDHistoryMenuLayer, MenuLayer) {
    void showGDHistoryUnavailable(char const* feature) {
        FLAlertLayer::create(
            "GDHistory",
            fmt::format("{} are unavailable because GDHistory does not provide this feature.", feature).c_str(),
            "OK"
        )->show();
    }

    void openGDHistoryDailyArchive() {
        auto scene = GDHistoryDailyLevelsLayer::scene();
        if (scene) {
            CCDirector::sharedDirector()->pushScene(scene);
        }
    }

    void onDaily(CCObject*) {
        this->openGDHistoryDailyArchive();
    }

    void onWeeklyLevel(CCObject*) {
        this->openGDHistoryDailyArchive();
    }

    void onMapPacks(CCObject*) {
        this->showGDHistoryUnavailable("Map Packs");
    }

    void onGauntlets(CCObject*) {
        this->showGDHistoryUnavailable("Gauntlets");
    }
};
