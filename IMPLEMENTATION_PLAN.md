# Implementation plan

## Target
- Geometry Dash 2.2081
- Geode SDK 5.11.2
- Windows x64 only
- C++23 / CMake
- Mod ID and name: gdhistory-integration

## Product decisions
- Replace the existing online Search entry point with an in-game GDHistory search experience.
- Use GDHistory as the only source for online level discovery and level data. Never silently fall back to RobTop.
- Do not open a browser.
- Use GDHistory's native website download route with the `gmd` format only:
  `/level/{level_id}/{record_id}/download/gmd/`
- Do not request or emit `.gdlvl` files. The target game is 2.2081 and this mod deliberately uses GMD.
- For each level, request its GDHistory record list, order record IDs newest-first, and try each GMD download in order. Advance to the next record only when the response explicitly indicates that the record contains no level data. Stop and report other failures.
- Disable unsupported online features (comments, likes, ratings, leaderboards, Map Packs, Gauntlets, etc.) so they cannot contact original game services.
- Preserve unrelated offline/editor functionality.

## GDHistory endpoints
Base: https://history.geometrydash.eu

Documented JSON endpoints:
- `GET /api/v1/search/level/advanced/` — advanced level search; query, limit, offset, sort, filter, matching_strategy.
- `GET /api/v1/level/{level_id}/` — level metadata and records; default mode returns normal record set. `brief` omits records.
- `GET /api/v1/daily/` — timely levels grouped by type and/or year.

GMD download route (confirmed by the supplied gdhistorydl 1.7.0 extension):
- `GET /level/{level_id}/{record_id}/download/gmd/`
- The extension's previous configurable format switch also supported `gdlvl`; this project hard-codes `gmd` as requested.
- The route is a website download endpoint, distinct from the documented JSON API. The mod must request it directly using native Geode networking and consume the returned GMD payload; it must not navigate a browser.

Important: GDHistory's API documentation warns that the API is unfinished and may have breaking changes. The repository backend source has now been inspected for the current search and daily response shapes, but live endpoint behavior still needs in-game verification.

## Architecture
1. Native HTTP client
   - HTTPS GET requests using Geode networking.
   - Central endpoint builder, JSON parsing, status/error handling, cancellation and request lifetime safety.
   - No browser, tab, DOM, or external navigation dependencies.

2. API/data layer
   - Typed parsing for search hits, level metadata, record IDs, and daily groups.
   - Isolate API JSON from UI and game hooks.
   - Validate required IDs and fields.

3. Search UI
   - Hook the existing Search entry point and open an in-game GDHistory search layer.
   - Support text query, pagination, and filters that map to documented GDHistory search parameters.
   - Convert results into Geometry Dash-compatible level objects.
   - Never call RobTop search endpoints.

4. Historical downloads
   - Fetch `/api/v1/level/{id}/`.
   - Extract record IDs from the actual response schema and sort descending numerically.
   - Request each candidate from `/level/{id}/{record_id}/download/gmd/`.
   - Detect the exact no-level-data response from GDHistory's backend source; only that condition triggers fallback.
   - Parse the GMD payload into the level object expected by Geometry Dash 2.2081.
   - Do not treat arbitrary HTTP errors, malformed payloads, or network failures as missing level data.

5. Daily levels
   - Fetch `/api/v1/daily/`.
   - Backend source currently constructs top-level year collections for daily levels plus separate `Weekly` and `Event` collections. Each level entry includes `online_id`, `cache_level_name`, `cache_username`, and `cache_daily_id`.
   - The adapter maps year buckets to Daily, preserves Weekly/Event categories, deduplicates by level ID, and sorts newest-first by the archive ID.
   - Reuse the same result conversion and GMD downloader.

6. Unsupported features / server isolation
   - Disable comments, likes, ratings, leaderboards, Map Packs, Gauntlets, and other unsupported online actions.
   - Audit hooks and UI entry points to prevent accidental requests to RobTop.
   - Do not disable unrelated offline or editor features.

## Milestones
1. Bootstrap Geode project metadata, CMake, source layout, and Windows-only support.
2. Implement native GDHistory HTTP client and inspect backend response schemas.
3. Implement in-game search and result conversion.
4. Implement GMD-only historical downloader and no-data fallback.
5. Implement daily data adapter from the backend's current response schema.
6. Disable unsupported online surfaces and audit server isolation.
7. Build and test against Geometry Dash 2.2081 / Geode 5.11.2 on Windows.

## Acceptance criteria
- Search and level downloads use GDHistory exclusively.
- Every level download uses the GMD route, never GDLVL.
- Records are tried newest-first; fallback occurs only for confirmed missing level data.
- Search, daily, and downloads work entirely in-game without opening a browser.
- Unsupported online features are disabled and cannot contact RobTop.
- API/network failures are visible and never trigger a RobTop fallback.
- The project builds for Windows x64 against Geode 5.11.2 and Geometry Dash 2.2081.

## Implementation status

The repository currently includes:
- Native asynchronous Geode WebRequest client for search, level records, daily data, and GMD downloads.
- JSON response validation and tolerant search/record parsing against the backend's Meilisearch response structure (`hits`).
- Newest-first historical record downloader. It retries only when the response contains GDHistory's explicit no-level-data message.
- GMD import through the `hjfod.gmd-api` dependency (version 1.5.0 or newer).
- In-game search results layer wired to `LevelSearchLayer::onSearch`, with pagination.
- In-game daily archive layer wired to `DailyLevelPage::show`, with pagination and GMD download/play.
- Daily archive parsing now correctly classifies year buckets as Daily, keeps Weekly/Event categories, displays archive IDs, and sorts newest-first.

Not yet complete or verified:
- Actual HTTP payloads and GMD imports still need runtime validation in Geometry Dash 2.2081.
- Search pagination and filters need UI/runtime testing.
- Complete Map Pack, Gauntlet, comments, likes, ratings, and leaderboard disablement needs a binding audit.
- Audit of all remaining RobTop network entry points.
- Windows build after the latest daily archive changes and runtime test against the target game version.

The current UI is an initial usable path, not a finished replacement. The repository should not be considered release-ready until the remaining items are implemented and tested.

## Latest implementation notes

- Search results request nine results per page and expose Previous/Next navigation.
- Daily archive uses GDHistory's current year/Weekly/Event JSON grouping; entries now show archive number, category, and year.
- Daily archive entries are sorted by GDHistory's archive ID descending so recent entries appear first.
- The Creator menu routes Online Levels to the GDHistory-backed search layer, and Daily/Weekly/Event entry points to the GDHistory archive.
- Map Packs, Gauntlets, leaderboards, Top Lists, Hall of Fame, challenges, multiplayer, and Paths are blocked with an in-game unsupported-feature message.
- Level-info actions for likes, ratings, moderator ratings, leaderboards, updates, featured requests, creator profiles, and online level information are blocked.
- The normal MenuLayer daily entry point opens the GDHistory archive.
- A Windows x64 GitHub Actions build is configured using the SDK version declared in mod.json.

These changes are implementation progress only. A successful CI build and in-game test are still required before claiming compatibility or release readiness.
