#pragma once

namespace UI
{
	// Adds this mod's page to the SKSE Menu Framework's Mod Control Panel. Safe to call when
	// the framework is missing or too old to drive: it logs why and does nothing else.
	void Register();

	// Pushes the current value of settings::display::useMetricUnits into the running compass,
	// the quest list position (Position X/Y, Max height) into the running quest list, and the
	// log level into spdlog. Everything else under settings::display and settings::questlist is
	// read live wherever it's used, so it needs nothing doing here.
	void ApplyLiveSettings();

	// 1.2.0: sets the compass text size (clamped to the slider's range) and applies it on the main thread, as the
	// slider does - the DevBench op=textscale drives it.
	void SetTextScale(float a_scale);

	// 1.2.1: serves the settings page's pending Text size / quest-list re-applies, at most once per 120 ms with the
	// latest values. HUD thread only - hooks::UpdateCompass calls it every frame from the compass's own Update.
	void ServiceLiveApplies();

	namespace SettingsPanel
	{
		void __stdcall Render();
	}
}
