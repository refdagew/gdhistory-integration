#include <Geode/Geode.hpp>
#include <Geode/modify/LevelSearchLayer.hpp>

#include "../ui/SearchResultsLayer.hpp"

#include <string>
#include <utility>

using namespace geode::prelude;

class $modify(GDHistoryLevelSearchLayer, LevelSearchLayer) {
    void openGDHistorySearch(std::string query, std::string sort = "cache_downloads:desc") {
        auto scene = GDHistorySearchResultsLayer::scene(std::move(query), std::move(sort));
        if (scene) {
            CCDirector::sharedDirector()->pushScene(scene);
        }
    }

    void showUnsupportedSearchMode(char const* mode) {
        FLAlertLayer::create(
            "GDHistory",
            fmt::format("{} search is not available through GDHistory.", mode).c_str(),
            "OK"
        )->show();
    }

    void onSearch(CCObject*) {
        auto query = m_searchInput
            ? std::string(m_searchInput->getString().c_str())
            : std::string{};
        this->openGDHistorySearch(std::move(query));
    }

    void onMostDownloaded(CCObject*) {
        this->openGDHistorySearch({}, "cache_downloads:desc");
    }

    void onMostLikes(CCObject*) {
        this->openGDHistorySearch({}, "cache_likes:desc");
    }

    void onMostRecent(CCObject*) {
        this->openGDHistorySearch({}, "cache_submitted:desc");
    }

    void onTrending(CCObject*) {
        this->showUnsupportedSearchMode("Trending");
    }

    void onSuggested(CCObject*) {
        this->showUnsupportedSearchMode("Suggested");
    }

    void onMagic(CCObject*) {
        this->showUnsupportedSearchMode("Magic");
    }

    void onLatestStars(CCObject*) {
        this->showUnsupportedSearchMode("Latest Stars");
    }

    void onFollowed(CCObject*) {
        this->showUnsupportedSearchMode("Followed creators");
    }

    void onFriends(CCObject*) {
        this->showUnsupportedSearchMode("Friends");
    }

    void onSearchUser(CCObject*) {
        this->showUnsupportedSearchMode("User");
    }

    void onSpecialDemon(CCObject*) {
        this->showUnsupportedSearchMode("Special Demon");
    }
};
