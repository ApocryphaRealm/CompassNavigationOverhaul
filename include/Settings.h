#pragma once

namespace SKSE::log
{
	using level = spdlog::level::level_enum;
}
namespace logger = SKSE::log;

namespace settings
{
	// Reads the INI into the variables below. The values the variables hold when this is
	// called are remembered as the built-in defaults, so RestoreDefaults() can put them back.
	void Init(const std::string& a_iniFileName);

	// Writes every setting below back to the INI that Init() read, leaving the comments and
	// any unrelated keys in that file alone. Returns false if the file could not be written.
	bool Save();

	// Puts every setting back to its built-in default. This only touches the variables;
	// follow it with Save() to persist, and with UI::ApplyLiveSettings() to show it in game.
	void RestoreDefaults();

	// Re-reads the INI that Init() read, discarding any unsaved change made since. Returns
	// false if the file could not be read, leaving the current values alone. Follow it with
	// UI::ApplyLiveSettings() to show the reloaded values in game.
	bool Reload();

	// Full path of the INI Init() read, or an empty string before Init() has run.
	const std::string& GetIniPath();

	// Default values

	namespace debug
	{
		inline logger::level logLevel = logger::level::info;
	}

	namespace display
	{
		inline bool useMetricUnits = false;
		inline bool showUndiscoveredLocationMarkers = false;
		inline bool undiscoveredMeansUnknownMarkers = true;
		inline bool undiscoveredMeansUnknownInfo = true;
		inline bool showEnemyMarkers = true;
		inline bool showEnemyNameUnderMarker = true;
		inline bool showObjectiveAsTarget = true;
		inline bool showOtherObjectivesCount = true;
		inline bool showInteriorMarkers = true;
		inline float angleToShowMarkerDetails = 10.0F;
		inline float angleToKeepMarkerDetailsShown = 35.0F;
		inline float focusingDelayToShow = 0.07F;
		// The quest marker cap the game enforces inside HUDMarkerManager::UpdateQuests (vanilla: 16). Read once at load
		// by hooks::QuestMarkerLimit::Install() and written into the game's code, so a change needs a restart. Clamped
		// to 16..48 when read: 16 is vanilla, 48 is all the marker array holds.
		// 1.1.7: default 32, not 48 (makihaze, 2026-10-08: no location markers on 1.1.6 with many quests active; "limit in
		// 16, The map markers appear again"). The 48 slots are shared - quests are added first and locations take what is
		// left - so a 48 cap let quest markers take every slot. 32 always leaves 16 for locations, enemies and the
		// player's own marker.
		inline std::uint32_t questMarkerLimit = 32;
	}

	namespace questlist
	{
		inline float positionX = 0.008F;
		inline float positionY = 0.125F;
		inline float maxHeight = 0.675F;
		inline bool showInExteriors = true;
		inline bool showInInteriors = true;
		inline float walkingDelayToShow = 0.0F;
		inline float joggingDelayToShow = 1.0F;
		inline float sprintingDelayToShow = 1.5F;
		inline bool hideInCombat = true;
	}
}
