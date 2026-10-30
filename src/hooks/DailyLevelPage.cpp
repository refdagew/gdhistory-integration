#include <Geode/Geode.hpp>
#include <Geode/modify/DailyLevelPage.hpp>

#include "../ui/DailyLevelsLayer.hpp"

using namespace geode::prelude;

class $modify(GDHistoryDailyLevelPage, DailyLevelPage) {
    void show() {
        auto scene = GDHistoryDailyLevelsLayer::scene();
        if (scene) {
            CCDirector::sharedDirector()->pushScene(scene);
        }
    }
};
