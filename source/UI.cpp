#include "UI.h"

#include "SKSEMenuFramework.h"

#include "Compass.h"
#include "Settings.h"

#include "utils/Logger.h"
#include "utils/Strings.h"
#include "utils/Toggle.h"

#include <algorithm>
#include <string>
#include <vector>

namespace UI
{
	namespace
	{
		std::string statusMessage;

		// The slider the arrow keys currently drive. Set by clicking one.
		std::string selectedSlider;

		constexpr const char* kLogLevelNames[] = { "Trace", "Debug", "Info", "Warning", "Error", "Critical", "Off" };
		constexpr const char* kLogLevelKeys[] = { "CNO_LogLevel_Trace", "CNO_LogLevel_Debug", "CNO_LogLevel_Info",
													"CNO_LogLevel_Warning", "CNO_LogLevel_Error", "CNO_LogLevel_Critical", "CNO_LogLevel_Off" };
		constexpr int kLogLevelCount = 7;

		// The framework renders from the renderer's present hook, which is not the thread
		// Scaleform and the rest of the game expect to be talked to. Anything that reaches into
		// the compass or the rest of the HUD has to be handed to the main thread first.
		void OnMainThread(std::function<void()> a_task)
		{
			if (auto* taskInterface = SKSE::GetTaskInterface())
			{
				taskInterface->AddTask(std::move(a_task));
			}
		}

		// The bundled header reaches ImGui through the framework's exported cimgui entry points.
		// Older builds of SKSE Menu Framework do not export them, and every widget call in
		// Render() would then call through a null function pointer, so refuse to register
		// unless the ones this panel needs are all there. Varargs widgets resolve to a
		// "...V"-suffixed export (TextDisabled resolves igTextDisabledV, not igTextDisabled) -
		// probing the wrong name lets Register() succeed and then crashes on the first draw.
		bool HasRequiredExports()
		{
			constexpr const char* required[] = {
				"AddSectionItem",
				"igTextV",
				"igTextDisabledV",
				"igTextWrappedV",
				"igSetTooltipV",
				"igSeparatorText",
				"igCheckbox",
				"igCombo_Str_arr",
				"igSliderFloat",
				// Needed by NudgeableSlider's arrow-key nudge (ported from Dragon's Eye
				// Minimap's UI.cpp - CLAUDE.md rule 24).
				"igIsKeyPressed_Bool",
				"igIsItemClicked",
				"igIsItemActive",
				"igIsItemHovered",
				"igButton",
				"igSameLine",
				"igSpacing",
				"igPushItemWidth",
				"igPopItemWidth",
				// Toggle() - the on/off switch every boolean setting now renders as
				// instead of a tick-box (utils/Toggle.h, CLAUDE.md rule 32).
				"igGetCursorScreenPos",
				"igGetWindowDrawList",
				"igGetFrameHeight",
				"igInvisibleButton",
				"igPushID_Str",
				"igPopID",
				"ImDrawList_AddRectFilled",
				"ImDrawList_AddCircleFilled"
			};

			for (const char* name : required)
			{
				if (!GetMenuFrameworkFunction<void*>(name))
				{
					logger::warn("SKSE Menu Framework does not export \"{}\"", name);

					return false;
				}
			}

			return true;
		}

		// A slider that the arrow keys can also nudge, once it has been clicked. Dragging is
		// hopeless for the last decimal place, and the framework does not turn on ImGui's own
		// keyboard navigation, so this tracks the selection itself rather than changing a
		// setting shared with every other mod's page. Ported verbatim from Dragon's Eye
		// Minimap's UI.cpp, which already had this working - see CLAUDE.md rule 24.
		bool NudgeableSlider(const char* a_label, float* a_value, float a_min, float a_max,
							 const char* a_format, float a_step)
		{
			bool changed = ImGuiMCP::SliderFloat(a_label, a_value, a_min, a_max, a_format);

			if (ImGuiMCP::IsItemClicked() || ImGuiMCP::IsItemActive())
			{
				selectedSlider = a_label;
			}

			if (selectedSlider == a_label)
			{
				float nudge = 0.0F;

				if (ImGuiMCP::IsKeyPressed(ImGuiMCP::ImGuiKey_LeftArrow) || ImGuiMCP::IsKeyPressed(ImGuiMCP::ImGuiKey_DownArrow))
				{
					nudge -= a_step;
				}
				if (ImGuiMCP::IsKeyPressed(ImGuiMCP::ImGuiKey_RightArrow) || ImGuiMCP::IsKeyPressed(ImGuiMCP::ImGuiKey_UpArrow))
				{
					nudge += a_step;
				}

				if (nudge != 0.0F)
				{
					*a_value = std::clamp(*a_value + nudge, a_min, a_max);
					changed = true;
				}

				ImGuiMCP::SameLine();
				ImGuiMCP::TextDisabled("%s", strings::TR("CNO_SliderNudge", "<-->"));
			}

			return changed;
		}

		void HelpMarker(const char* a_description)
		{
			ImGuiMCP::SameLine();
			ImGuiMCP::TextDisabled("%s", strings::TR("CNO_HelpMark", "(?)"));

			if (ImGuiMCP::IsItemHovered())
			{
				ImGuiMCP::SetTooltip("%s", a_description);
			}
		}

		void ApplyUseMetricUnits()
		{
			OnMainThread([]() {
				if (auto* compass = CNO::Compass::GetSingleton())
				{
					compass->SetUnits(settings::display::useMetricUnits);
				}
			});
		}

		void RenderDisplaySection()
		{
			using namespace settings::display;

			ImGuiMCP::SeparatorText(strings::TR("CNO_Title", "Compass and markers"));

			if (ImGuiMCP::Toggle(strings::TR("CNO_UseMetricUnits", "Use metric units"), &useMetricUnits))
			{
				ApplyUseMetricUnits();
			}
			HelpMarker(strings::TR("CNO_HelpUseMetricUnits", "Shows distances to markers in meters instead of the vanilla feet."));

			ImGuiMCP::Toggle(strings::TR("CNO_ShowUndiscoveredLocationMarkers", "Show undiscovered location markers"), &showUndiscoveredLocationMarkers);
			ImGuiMCP::Toggle(strings::TR("CNO_UndiscoveredMeansUnknownMarkers", "Undiscovered means unknown marker"), &undiscoveredMeansUnknownMarkers);
			HelpMarker(strings::TR("CNO_HelpUndiscoveredMeansUnknownMarkers", "Shows a generic marker instead of the location's real icon until you discover it."));
			ImGuiMCP::Toggle(strings::TR("CNO_UndiscoveredMeansUnknownInfo", "Undiscovered means unknown info"), &undiscoveredMeansUnknownInfo);
			HelpMarker(strings::TR("CNO_HelpUndiscoveredMeansUnknownInfo", "Hides the location's name and distance on the compass until you discover it."));

			ImGuiMCP::Toggle(strings::TR("CNO_ShowEnemyMarkers", "Show enemy markers"), &showEnemyMarkers);
			ImGuiMCP::Toggle(strings::TR("CNO_ShowEnemyNameUnderMarker", "Show enemy name under marker"), &showEnemyNameUnderMarker);
			ImGuiMCP::Toggle(strings::TR("CNO_ShowInteriorMarkers", "Show interior markers"), &showInteriorMarkers);

			ImGuiMCP::Toggle(strings::TR("CNO_ShowObjectiveAsTarget", "Show objective as target"), &showObjectiveAsTarget);
			ImGuiMCP::Toggle(strings::TR("CNO_ShowOtherObjectivesCount", "Show other objectives count"), &showOtherObjectivesCount);

			NudgeableSlider(strings::TR("CNO_AngleToShowMarkerDetails", "Angle to show marker details"), &angleToShowMarkerDetails, 0.0F, 90.0F, "%.0f", 1.0F);
			HelpMarker(strings::TR("CNO_HelpAngleToShowMarkerDetails", "How close to the center of the compass a marker has to be before its name and distance appear."));
			NudgeableSlider(strings::TR("CNO_AngleToKeepMarkerDetailsShown", "Angle to keep marker details shown"), &angleToKeepMarkerDetailsShown, 0.0F, 90.0F, "%.0f", 1.0F);
			HelpMarker(strings::TR("CNO_HelpAngleToKeepMarkerDetailsShown", "Once shown, a marker's details stay visible until it drifts past this wider angle - keeps the text from flickering right at the threshold."));
			NudgeableSlider(strings::TR("CNO_FocusingDelayToShow", "Focusing delay to show"), &focusingDelayToShow, 0.0F, 2.0F, "%.2f", 0.01F);
			HelpMarker(strings::TR("CNO_HelpFocusingDelayToShow", "How long a marker has to stay within the angle above before its details appear."));
		}

		void RenderQuestListSection()
		{
			using namespace settings::questlist;

			ImGuiMCP::SeparatorText(strings::TR("CNO_QuestListTitle", "Quest list"));

			NudgeableSlider(strings::TR("CNO_PositionX", "Position X"), &positionX, 0.0F, 1.0F, "%.3f", 0.01F);
			NudgeableSlider(strings::TR("CNO_PositionY", "Position Y"), &positionY, 0.0F, 1.0F, "%.3f", 0.01F);
			NudgeableSlider(strings::TR("CNO_MaxHeight", "Max height"), &maxHeight, 0.0F, 1.0F, "%.2f", 0.01F);

			ImGuiMCP::Toggle(strings::TR("CNO_ShowInExteriors", "Show in exteriors"), &showInExteriors);
			ImGuiMCP::Toggle(strings::TR("CNO_ShowInInteriors", "Show in interiors"), &showInInteriors);
			ImGuiMCP::Toggle(strings::TR("CNO_HideInCombat", "Hide in combat"), &hideInCombat);
			HelpMarker(strings::TR("CNO_HelpHideInCombat", "Hides the quest list entirely while a weapon or spell is drawn."));

			NudgeableSlider(strings::TR("CNO_WalkingDelayToShow", "Walking delay to show"), &walkingDelayToShow, 0.0F, 3.0F, "%.2f", 0.01F);
			NudgeableSlider(strings::TR("CNO_JoggingDelayToShow", "Jogging delay to show"), &joggingDelayToShow, 0.0F, 3.0F, "%.2f", 0.01F);
			NudgeableSlider(strings::TR("CNO_SprintingDelayToShow", "Sprinting delay to show"), &sprintingDelayToShow, 0.0F, 3.0F, "%.2f", 0.01F);
			HelpMarker(strings::TR("CNO_HelpPaceDelays", "How long you have to move at each pace before the quest list fades in."));
		}

		void RenderDebugSection()
		{
			using namespace settings;

			ImGuiMCP::SeparatorText(strings::TR("CNO_Debug", "Debug"));

			int level = static_cast<int>(debug::logLevel);
			// Rebuilt from TR'd entries every frame (plan 2.2); labelStore owns the translated
			// bytes for this call so the const char* pointers handed to Combo stay valid.
			std::vector<std::string> logLevelLabelStore;
			logLevelLabelStore.reserve(kLogLevelCount);
			for (int i = 0; i < kLogLevelCount; ++i)
			{
				logLevelLabelStore.push_back(strings::TR(kLogLevelKeys[i], kLogLevelNames[i]));
			}
			std::vector<const char*> logLevelLabels;
			logLevelLabels.reserve(logLevelLabelStore.size());
			for (const auto& s : logLevelLabelStore) { logLevelLabels.push_back(s.c_str()); }
			if (ImGuiMCP::Combo(strings::TR("CNO_LogLevel", "Log level"), &level, logLevelLabels.data(), kLogLevelCount))
			{
				debug::logLevel = static_cast<logger::level>(level);

				OnMainThread([]() { logger::set_level(settings::debug::logLevel, settings::debug::logLevel); });
			}
			HelpMarker(strings::TR("CNO_HelpLogLevel", "Applies to the log immediately."));
		}

		void RenderButtons()
		{
			if (ImGuiMCP::Button(strings::TR("CNO_SaveBtn", "Save")))
			{
				OnMainThread([]() {
					statusMessage = settings::Save() ? strings::TR("CNO_StatusSaved", "Settings saved.")
													   : strings::TR("CNO_StatusSaveFail", "Could not save the INI. See the log for why.");
				});
			}
			HelpMarker(strings::TR("CNO_HelpSave", "Writes every setting above back to the INI. Comments and unrelated keys are left alone."));

			ImGuiMCP::SameLine();

			if (ImGuiMCP::Button(strings::TR("CNO_ReloadBtn", "Reload from INI")))
			{
				OnMainThread([]() {
					if (settings::Reload())
					{
						ApplyLiveSettings();

						statusMessage = strings::TR("CNO_StatusReloaded", "Settings reloaded from the INI.");
					}
					else
					{
						statusMessage = strings::TR("CNO_StatusReloadFail", "Could not read the INI. See the log for why.");
					}
				});
			}
			HelpMarker(strings::TR("CNO_HelpReload", "Throws away any change made here since the last save and re-reads the INI from disk. Also picks up edits made to the file by hand."));

			ImGuiMCP::SameLine();

			if (ImGuiMCP::Button(strings::TR("CNO_RestoreBtn", "Restore defaults")))
			{
				OnMainThread([]() {
					settings::RestoreDefaults();
					ApplyLiveSettings();
				});

				statusMessage = strings::TR("CNO_StatusRestored", "Defaults restored. Press Save to keep them.");
			}
			HelpMarker(strings::TR("CNO_HelpRestore", "Puts every setting back to the value it has on a fresh install. Nothing is written until you press Save."));

			if (!statusMessage.empty())
			{
				ImGuiMCP::TextWrapped("%s", statusMessage.c_str());
			}

			ImGuiMCP::Spacing();
			ImGuiMCP::TextDisabled("%s", settings::GetIniPath().c_str());
		}
	}

	void Register()
	{
		if (!SKSEMenuFramework::IsInstalled())
		{
			logger::info("SKSE Menu Framework is not installed; settings will be read from the INI only");

			return;
		}

		if (!HasRequiredExports())
		{
			logger::warn("The installed SKSE Menu Framework is older than this plugin's settings "
						 "menu needs. Update it to version 3 or newer to configure the compass and "
						 "quest list in game.");

			return;
		}

		SKSEMenuFramework::SetSection("Compass Navigation Overhaul");
		SKSEMenuFramework::AddSectionItem("Settings", SettingsPanel::Render);

		logger::info("Registered the settings page with SKSE Menu Framework");
	}

	void ApplyLiveSettings()
	{
		logger::set_level(settings::debug::logLevel, settings::debug::logLevel);

		ApplyUseMetricUnits();
	}

	void __stdcall SettingsPanel::Render()
	{
		strings::Tick();

		ImGuiMCP::TextWrapped("%s", strings::TR("CNO_Intro", "Most settings apply as soon as you make them. Press Save to keep them for the next time you play."));
		ImGuiMCP::Spacing();

		ImGuiMCP::PushItemWidth(260.0F);

		RenderDisplaySection();
		ImGuiMCP::Spacing();

		RenderQuestListSection();
		ImGuiMCP::Spacing();

		RenderDebugSection();
		ImGuiMCP::Spacing();

		ImGuiMCP::PopItemWidth();

		RenderButtons();
	}
}
