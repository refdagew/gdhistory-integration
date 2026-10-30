#include <Geode/Geode.hpp>
#include <Geode/modify/CreatorLayer.hpp>
#include <Geode/binding/LevelSearchLayer.hpp>

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

    void onEventLevel(CCObject*) {
        this->openGDHistoryDailyArchive();
    }

    void onOnlineLevels(CCObject*) {
        auto scene = LevelSearchLayer::scene(0);
        if (scene) {
            CCDirector::sharedDirector()->pushScene(scene);
        }
    }

    void onMapPacks(CCObject*) {
        this->showUnsupportedFeature("Map Packs");
    }

    void onGauntlets(CCObject*) {
        this->showUnsupportedFeature("Gauntlets");
    }

    void onFeaturedLevels(CCObject*) {
        this->showUnsupportedFeature("Featured levels");
    }

    void onLeaderboards(CCObject*) {
        this->showUnsupportedFeature("Leaderboards");
    }

    void onTopLists(CCObject*) {
        this->showUnsupportedFeature("Top lists");
    }

    void onChallenge(CCObject*) {
        this->showUnsupportedFeature("Challenges");
    }

    void onMultiplayer(CCObject*) {
        this->showUnsupportedFeature("Multiplayer");
    }

    void onPaths(CCObject*) {
        this->showUnsupportedFeature("Paths");
    }
};
