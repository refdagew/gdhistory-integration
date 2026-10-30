#include <Geode/Geode.hpp>
#include <Geode/modify/CreatorLayer.hpp>

#include "../ui/DailyLevelsLayer.hpp"

using namespace geode::prelude;

class $modify(GDHistoryCreatorLayer, CreatorLayer) {
    void openGDHistoryDailyArchive() {
        auto scene = GDHistoryDailyLevelsLayer::scene();
        if (scene) {
            CCDirector::sharedDirector()->pushScene(scene);
        }
    }

    void showUnsupportedFeature(char const* feature) {
        FLAlertLayer::create(
            "GDHistory",
            fmt::format("{} are unavailable because GDHistory does not provide this feature.", feature).c_str(),
            "OK"
        )->show();
    }

    void onDailyLevel(CCObject*) {
        this->openGDHistoryDailyArchive();
    }

    void onWeeklyLevel(CCObject*) {
        this->openGDHistoryDailyArchive();
    }

    void onMapPacks(CCObject*) {
        this->showUnsupportedFeature("Map Packs");
    }

    void onGauntlets(CCObject*) {
        this->showUnsupportedFeature("Gauntlets");
    }
};
