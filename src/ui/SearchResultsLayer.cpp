#include "SearchResultsLayer.hpp"

#include <Geode/binding/PlayLayer.hpp>

#include "../api/GMDLoader.hpp"

#include <algorithm>
#include <utility>

using namespace geode::prelude;

CCScene* GDHistorySearchResultsLayer::scene(std::string query, std::string sort) {
    auto scene = CCScene::create();
    auto layer = GDHistorySearchResultsLayer::create(std::move(query), std::move(sort));
    if (!scene || !layer) return nullptr;
    scene->addChild(layer);
    return scene;
}

GDHistorySearchResultsLayer* GDHistorySearchResultsLayer::create(std::string query, std::string sort) {
    auto ret = new GDHistorySearchResultsLayer();
    if (ret && ret->init(std::move(query), std::move(sort))) {
        ret->autorelease();
        return ret;
    }
    CC_SAFE_DELETE(ret);
    return nullptr;
}

bool GDHistorySearchResultsLayer::init(std::string query, std::string sort) {
    if (!CCLayerColor::initWithColor({18, 19, 30, 255})) {
        return false;
    }

    m_query = std::move(query);
    m_sort = std::move(sort);

    auto winSize = CCDirector::sharedDirector()->getWinSize();

    auto title = CCLabelBMFont::create("GDHistory Search", "bigFont.fnt");
    title->setPosition({winSize.width / 2.f, winSize.height - 34.f});
    title->setScale(0.8f);
    this->addChild(title);

    m_status = CCLabelBMFont::create("Searching GDHistory...", "goldFont.fnt");
    m_status->setPosition({winSize.width / 2.f, winSize.height - 76.f});
    m_status->setScale(0.45f);
    this->addChild(m_status);

    m_resultsMenu = CCMenu::create();
    m_resultsMenu->setPosition({0.f, 0.f});
    this->addChild(m_resultsMenu);

    auto previousLabel = CCLabelBMFont::create("Previous", "bigFont.fnt");
    previousLabel->setScale(0.45f);
    m_previousItem = CCMenuItemLabel::create(
        previousLabel, this, menu_selector(GDHistorySearchResultsLayer::onPreviousPage)
    );
    m_previousItem->setPosition({winSize.width / 2.f - 75.f, 65.f});
    m_previousItem->setEnabled(false);
    m_resultsMenu->addChild(m_previousItem);

    auto nextLabel = CCLabelBMFont::create("Next", "bigFont.fnt");
    nextLabel->setScale(0.45f);
    m_nextItem = CCMenuItemLabel::create(
        nextLabel, this, menu_selector(GDHistorySearchResultsLayer::onNextPage)
    );
    m_nextItem->setPosition({winSize.width / 2.f + 75.f, 65.f});
    m_nextItem->setEnabled(false);
    m_resultsMenu->addChild(m_nextItem);

    auto backLabel = CCLabelBMFont::create("Back", "bigFont.fnt");
    auto backItem = CCMenuItemLabel::create(
        backLabel,
        this,
        menu_selector(GDHistorySearchResultsLayer::onBack)
    );
    auto backMenu = CCMenu::createWithItem(backItem);
    backMenu->setPosition({winSize.width / 2.f, 28.f});
    this->addChild(backMenu);

    this->requestPage();
    return true;
}

void GDHistorySearchResultsLayer::requestPage() {
    m_results.clear();
    m_resultsMenu->removeAllChildrenWithCleanup(true);
    m_resultsMenu->addChild(m_previousItem);
    m_resultsMenu->addChild(m_nextItem);
    m_previousItem->setEnabled(m_offset > 0);
    m_nextItem->setEnabled(false);
    m_status->setString("Searching GDHistory...");

    m_client.searchLevelsJSON(
        m_query,
        PAGE_SIZE,
        m_offset,
        [this](gdhistory::api::JsonResponse response) {
            this->onSearchResponse(std::move(response));
        },
        m_sort
    );
}

void GDHistorySearchResultsLayer::onSearchResponse(gdhistory::api::JsonResponse response) {
    if (!response.ok()) {
        m_status->setString(response.error.c_str());
        return;
    }

    m_results = gdhistory::api::parseSearchResults(*response.value);
    if (m_results.empty()) {
        m_status->setString(m_offset == 0 ? "No matching levels found." : "No more results.");
        m_nextItem->setEnabled(false);
        return;
    }

    m_status->setString(
        fmt::format("Showing {} results (offset {})", m_results.size(), m_offset).c_str()
    );
    m_previousItem->setEnabled(m_offset > 0);
    m_nextItem->setEnabled(m_results.size() == PAGE_SIZE);

    auto winSize = CCDirector::sharedDirector()->getWinSize();

    for (std::size_t i = 0; i < m_results.size(); ++i) {
        auto const& result = m_results[i];
        auto rowText = fmt::format(
            "{}  |  {}  |  {} downloads",
            result.name.empty() ? fmt::format("Level {}", result.levelID) : result.name,
            result.creator.empty() ? "Unknown creator" : result.creator,
            result.downloads
        );

        auto label = CCLabelBMFont::create(rowText.c_str(), "bigFont.fnt");
        label->setScale(0.38f);

        auto item = CCMenuItemLabel::create(
            label,
            this,
            menu_selector(GDHistorySearchResultsLayer::onSelectLevel)
        );
        item->setTag(static_cast<int>(i));
        item->setPosition({
            winSize.width / 2.f,
            winSize.height - 118.f - static_cast<float>(i) * 31.f
        });
        m_resultsMenu->addChild(item);
    }
}

void GDHistorySearchResultsLayer::onSelectLevel(CCObject* sender) {
    auto index = static_cast<std::size_t>(sender->getTag());
    if (index >= m_results.size()) return;

    auto const selected = m_results[index];
    m_status->setString(
        fmt::format("Fetching history for level {}...", selected.levelID).c_str()
    );

    m_downloader.download(
        selected.levelID,
        [this, levelID = selected.levelID](gdhistory::api::HistoricalDownloadResult result) {
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

void GDHistorySearchResultsLayer::onPreviousPage(CCObject*) {
    if (m_offset < PAGE_SIZE) return;
    m_offset -= PAGE_SIZE;
    this->requestPage();
}

void GDHistorySearchResultsLayer::onNextPage(CCObject*) {
    if (m_results.size() != PAGE_SIZE) return;
    m_offset += PAGE_SIZE;
    this->requestPage();
}

void GDHistorySearchResultsLayer::onBack(CCObject*) {
    CCDirector::sharedDirector()->popScene();
}
