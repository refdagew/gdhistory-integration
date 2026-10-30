#include <Geode/Geode.hpp>
#include <Geode/modify/LevelSearchLayer.hpp>

#include "../ui/SearchResultsLayer.hpp"

#include <string>
#include <utility>

using namespace geode::prelude;

class $modify(GDHistoryLevelSearchLayer, LevelSearchLayer) {
    void onSearch(CCObject*) {
        auto query = m_searchInput
            ? std::string(m_searchInput->getString().c_str())
            : std::string{};

        auto scene = GDHistorySearchResultsLayer::scene(std::move(query));
        if (scene) {
            CCDirector::sharedDirector()->pushScene(scene);
        }
    }
};
