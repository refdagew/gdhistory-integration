#include <Geode/Geode.hpp>
#include <Geode/modify/MenuLayer.hpp>

#include "../ui/DailyLevelsLayer.hpp"

using namespace geode::prelude;

class $modify(GDHistoryMenuLayer, MenuLayer) {
    void onDaily(CCObject*) {
        auto scene = GDHistoryDailyLevelsLayer::scene();
        if (scene) {
            CCDirector::sharedDirector()->pushScene(scene);
        }
    }
};
