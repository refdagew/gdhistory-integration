#include "SearchResultsLayer.hpp"

#include <algorithm>
#include <utility>

using namespace geode::prelude;

CCScene* GDHistorySearchResultsLayer::scene(std::string query) {
    auto scene = CCScene::create();
    auto layer = GDHistorySearchResultsLayer::create(std::move(query));
    if (!scene || !layer) return nullptr;
    scene->addChild(layer);
    return scene;
}

GDHistorySearchResultsLayer* GDHistorySearchResultsLayer::create(std::string query) {
    auto ret = new GDHistorySearchResultsLayer();
    if (ret && ret->init(std::move(query))) {
        ret->autorelease();
        return ret;
    }
    CC_SAFE_DELETE(ret);
    return nullptr;
}

bool GDHistorySearchResultsLayer::init(std::string query) {
    if (!CCLayerColor::initWithColor({18, 19, 30, 255})) {
        return false;
    }

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

    auto backLabel = CCLabelBMFont::create("Back", "bigFont.fnt");
    auto backItem = CCMenuItemLabel::create(
        backLabel,
        this,
        menu_selector(GDHistorySearchResultsLayer::onBack)
    );
    auto backMenu = CCMenu::createWithItem(backItem);
    backMenu->setPosition({winSize.width / 2.f, 28.f});
    this->addChild(backMenu);

    m_client.searchLevelsJSON(
        std::move(query),
        100,
        0,
        [this](gdhistory::api::JsonResponse response) {
            this->onSearchResponse(std::move(response));
        }
    );

    return true;
}

void GDHistorySearchResultsLayer::onSearchResponse(gdhistory::api::JsonResponse response) {
    if (!response.ok()) {
        m_status->setString(response.error.c_str());
        return;
    }

    m_results = gdhistory::api::parseSearchResults(*response.value);
    if (m_results.empty()) {
        m_status->setString("No matching levels found.");
        return;
    }

    m_status->setString(
        fmt::format("{} GDHistory results (select a level to fetch its GMD)", m_results.size()).c_str()
    );

    auto winSize = CCDirector::sharedDirector()->getWinSize();
    auto const visibleCount = std::min<std::size_t>(m_results.size(), 12);

    for (std::size_t i = 0; i < visibleCount; ++i) {
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
        [this](gdhistory::api::HistoricalDownloadResult result) {
            if (!result.hasData()) {
                m_status->setString(result.error.c_str());
                return;
            }

            auto bytes = gdhistory::api::responseBytes(*result.response);
            m_status->setString(
                fmt::format(
                    "GMD record {} downloaded ({} bytes). Game loading is the next integration step.",
                    result.recordID,
                    bytes.size()
                ).c_str()
            );
        }
    );
}

void GDHistorySearchResultsLayer::onBack(CCObject*) {
    CCDirector::sharedDirector()->popScene();
}
