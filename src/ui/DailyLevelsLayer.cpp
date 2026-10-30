#include "DailyLevelsLayer.hpp"

#include <Geode/binding/PlayLayer.hpp>

#include <algorithm>
#include <utility>

#include "../api/GMDLoader.hpp"

using namespace geode::prelude;

void GDHistoryDailyLevelsLayer::onExit() {
    m_client.cancel();
    m_downloader.cancel();
    CCLayerColor::onExit();
}

CCScene* GDHistoryDailyLevelsLayer::scene() {
    auto scene = CCScene::create();
    auto layer = GDHistoryDailyLevelsLayer::create();
    if (!scene || !layer) return nullptr;
    scene->addChild(layer);
    return scene;
}

GDHistoryDailyLevelsLayer* GDHistoryDailyLevelsLayer::create() {
    auto ret = new GDHistoryDailyLevelsLayer();
    if (ret && ret->init()) {
        ret->autorelease();
        return ret;
    }
    CC_SAFE_DELETE(ret);
    return nullptr;
}

bool GDHistoryDailyLevelsLayer::init() {
    if (!CCLayerColor::initWithColor({18, 19, 30, 255})) {
        return false;
    }

    auto winSize = CCDirector::sharedDirector()->getWinSize();

    auto title = CCLabelBMFont::create("GDHistory Timely Levels", "bigFont.fnt");
    title->setPosition({winSize.width / 2.f, winSize.height - 34.f});
    title->setScale(0.8f);
    this->addChild(title);

    m_status = CCLabelBMFont::create("Loading GDHistory daily archive...", "goldFont.fnt");
    m_status->setPosition({winSize.width / 2.f, winSize.height - 76.f});
    m_status->setScale(0.45f);
    this->addChild(m_status);

    m_resultsMenu = CCMenu::create();
    m_resultsMenu->setPosition({0.f, 0.f});
    this->addChild(m_resultsMenu);

    m_navigationMenu = CCMenu::create();
    m_navigationMenu->setPosition({0.f, 0.f});
    this->addChild(m_navigationMenu);

    auto previousLabel = CCLabelBMFont::create("Previous", "bigFont.fnt");
    previousLabel->setScale(0.45f);
    m_previousItem = CCMenuItemLabel::create(
        previousLabel, this, menu_selector(GDHistoryDailyLevelsLayer::onPreviousPage)
    );
    m_previousItem->setPosition({winSize.width / 2.f - 75.f, 65.f});
    m_previousItem->setEnabled(false);
    m_navigationMenu->addChild(m_previousItem);

    auto nextLabel = CCLabelBMFont::create("Next", "bigFont.fnt");
    nextLabel->setScale(0.45f);
    m_nextItem = CCMenuItemLabel::create(
        nextLabel, this, menu_selector(GDHistoryDailyLevelsLayer::onNextPage)
    );
    m_nextItem->setPosition({winSize.width / 2.f + 75.f, 65.f});
    m_nextItem->setEnabled(false);
    m_navigationMenu->addChild(m_nextItem);

    auto backLabel = CCLabelBMFont::create("Back", "bigFont.fnt");
    auto backItem = CCMenuItemLabel::create(
        backLabel,
        this,
        menu_selector(GDHistoryDailyLevelsLayer::onBack)
    );
    auto backMenu = CCMenu::createWithItem(backItem);
    backMenu->setPosition({winSize.width / 2.f, 28.f});
    this->addChild(backMenu);

    m_client.getDailyLevelsJSON(
        [this](gdhistory::api::JsonResponse response) {
            this->onDailyResponse(std::move(response));
        }
    );

    return true;
}

void GDHistoryDailyLevelsLayer::onDailyResponse(gdhistory::api::JsonResponse response) {
    if (!response.ok()) {
        m_status->setString(response.error.c_str());
        return;
    }

    m_levels = gdhistory::api::parseDailyLevels(*response.value);
    if (m_levels.empty()) {
        m_status->setString("GDHistory returned no daily archive entries.");
        return;
    }

    this->renderPage();
}

void GDHistoryDailyLevelsLayer::renderPage() {
    static constexpr std::size_t PAGE_SIZE = 9;
    m_resultsMenu->removeAllChildrenWithCleanup(true);

    auto const end = std::min(m_offset + PAGE_SIZE, m_levels.size());
    m_status->setString(
        fmt::format(
            "Showing {}-{} of {} timely levels",
            m_offset + 1,
            end,
            m_levels.size()
        ).c_str()
    );

    m_previousItem->setEnabled(m_offset > 0);
    m_nextItem->setEnabled(end < m_levels.size());

    auto winSize = CCDirector::sharedDirector()->getWinSize();
    for (std::size_t index = m_offset; index < end; ++index) {
        auto const& level = m_levels[index];
        auto archiveNumber = level.dailyNumber > 0
            ? fmt::format("#{}", level.dailyNumber)
            : fmt::format("ID {}", level.levelID);
        auto labelText = fmt::format(
            "{} {} [{}{}]  |  {}",
            archiveNumber,
            level.name.empty() ? fmt::format("Level {}", level.levelID) : level.name,
            level.category.empty() ? "timed" : level.category,
            level.year.empty() ? "" : " " + level.year,
            level.creator.empty() ? fmt::format("ID {}", level.levelID) : level.creator
        );

        auto label = CCLabelBMFont::create(labelText.c_str(), "bigFont.fnt");
        label->setScale(0.38f);

        auto item = CCMenuItemLabel::create(
            label,
            this,
            menu_selector(GDHistoryDailyLevelsLayer::onSelectLevel)
        );
        item->setTag(static_cast<int>(index));
        item->setPosition({
            winSize.width / 2.f,
            winSize.height - 118.f - static_cast<float>(index - m_offset) * 31.f
        });
        m_resultsMenu->addChild(item);
    }
}

void GDHistoryDailyLevelsLayer::onPreviousPage(CCObject*) {
    static constexpr std::size_t PAGE_SIZE = 9;
    if (m_offset < PAGE_SIZE) return;
    m_offset -= PAGE_SIZE;
    this->renderPage();
}

void GDHistoryDailyLevelsLayer::onNextPage(CCObject*) {
    static constexpr std::size_t PAGE_SIZE = 9;
    if (m_offset + PAGE_SIZE >= m_levels.size()) return;
    m_offset += PAGE_SIZE;
    this->renderPage();
}

void GDHistoryDailyLevelsLayer::onSelectLevel(CCObject* sender) {
    auto index = static_cast<std::size_t>(sender->getTag());
    if (index >= m_levels.size()) return;

    auto const levelID = m_levels[index].levelID;
    m_status->setString(fmt::format("Fetching history for level {}...", levelID).c_str());

    m_downloader.download(
        levelID,
        [this, levelID](gdhistory::api::HistoricalDownloadResult result) {
            if (!result.hasData()) {
                m_status->setString(result.error.c_str());
                return;
            }

            auto imported = gdhistory::api::importGMD(
                *result.response,
                levelID,
                result.recordID
            );
            if (!imported.ok()) {
                m_status->setString(imported.error.c_str());
                return;
            }

            auto playScene = PlayLayer::scene(imported.level, false, false);
            if (!playScene) {
                m_status->setString("Geometry Dash could not create a play scene for this level.");
                return;
            }

            CCDirector::sharedDirector()->pushScene(playScene);
        }
    );
}

void GDHistoryDailyLevelsLayer::onBack(CCObject*) {
    CCDirector::sharedDirector()->popScene();
}
