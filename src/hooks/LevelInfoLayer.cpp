#include <Geode/Geode.hpp>
#include <Geode/modify/LevelInfoLayer.hpp>

using namespace geode::prelude;

class $modify(GDHistoryLevelInfoLayer, LevelInfoLayer) {
    void showGDHistoryUnavailable(char const* feature) {
        FLAlertLayer::create(
            "GDHistory",
            fmt::format("{} are unavailable for GDHistory-hosted levels.", feature).c_str(),
            "OK"
        )->show();
    }

    void onLike(CCObject*) {
        this->showGDHistoryUnavailable("Likes");
    }

    void onRate(CCObject*) {
        this->showGDHistoryUnavailable("Ratings");
    }

    void onRateDemon(CCObject*) {
        this->showGDHistoryUnavailable("Demon ratings");
    }

    void onRateStars(CCObject*) {
        this->showGDHistoryUnavailable("Star ratings");
    }

    void onRateStarsMod(CCObject*) {
        this->showGDHistoryUnavailable("Moderator star ratings");
    }

    void onLevelLeaderboard(CCObject*) {
        this->showGDHistoryUnavailable("Level leaderboards");
    }

    void onUpdate(CCObject*) {
        this->showGDHistoryUnavailable("RobTop level updates");
    }

    void onFeatured(CCObject*) {
        this->showGDHistoryUnavailable("Featured-level requests");
    }

    void onViewProfile(CCObject*) {
        this->showGDHistoryUnavailable("Creator profiles");
    }

    void onLevelInfo(CCObject*) {
        this->showGDHistoryUnavailable("Level comments and online information");
    }
};
