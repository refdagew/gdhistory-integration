# gdhistory-integration

A Windows-only Geode mod for Geometry Dash 2.2081 / Geode 5.11.2 that replaces online level discovery with GDHistory.

## Project status

This repository is being bootstrapped. The current commit contains the project scaffold and implementation plan; the in-game integration and API client are not implemented yet.

## Confirmed design

- The existing Search entry point will open an in-game GDHistory search UI.
- GDHistory is the only source for online search and level data. There is no RobTop fallback.
- The mod never opens a browser.
- Historical records are tried newest-first.
- A record fallback happens only when GDHistory confirms that the selected record contains no level data.
- Downloads use the GMD route only:
  `/level/{level_id}/{record_id}/download/gmd/`
- The mod will not use the newer `.gdlvl` format.
- Unsupported online features (comments, likes, ratings, leaderboards, Map Packs, Gauntlets, etc.) will be disabled.
- Daily data comes from `/api/v1/daily/`; categories will be mapped only after validating the live response schema.

## GDHistory endpoints

Base URL: `https://history.geometrydash.eu`

- `GET /api/v1/search/level/advanced/`
- `GET /api/v1/level/{level_id}/`
- `GET /api/v1/daily/`
- `GET /level/{level_id}/{record_id}/download/gmd/` (GMD payload download)

The first three are documented JSON API routes. The GMD download route is the site's file-download route, confirmed by the supplied gdhistorydl 1.7.0 extension. The API documentation warns that its schema may change.

## Build

Use the Geode SDK v5.11.2 checkout and set `GEODE_SDK` to its path.

From a Windows developer command prompt with CMake and the Geode CLI installed:

```powershell
$env:GEODE_SDK = "C:\\path\\to\\geode"
geode build
```

The initial project is intentionally restricted to Windows.

## Documentation

See [IMPLEMENTATION_PLAN.md](IMPLEMENTATION_PLAN.md) for architecture, milestones, endpoint notes, and acceptance criteria.

Unofficial community project; not affiliated with RobTop Games, Geometry Dash, Geode, or GDHistory.
