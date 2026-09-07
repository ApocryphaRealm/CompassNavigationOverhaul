# -*- coding: utf-8 -*-
"""lang-patch.py - one-shot, re-runnable language-support patch for Compass Navigation Overhaul.

Applies the consumer-side mechanism from D:\\Claude output\\4. plans\\translation-rollout\\plan.md
sections 2 and 4.1: strings::TR() routing for every literal the settings page draws, the
"!ApocryphaMenuFramework" module-name lookup, strings::Configure() at kDataLoaded, and a
"strings" op on the "compassnavigationoverhaul.status" DevBench tool. Every edit below is a
must-match anchor replace: if an anchor is not found EXACTLY ONCE the script raises instead of
silently doing nothing, so a stale run against changed source fails loudly rather than leaving
the code half patched.

Run from anywhere: `python tools/lang-patch.py` (paths are relative to the repo root, taken as
this script's grandparent directory).
"""
import os

REPO = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))


def read(path):
    with open(path, "r", encoding="utf-8", newline=None) as f:
        return f.read()


def write(path, text, crlf=False):
    with open(path, "w", encoding="utf-8", newline="\r\n" if crlf else "\n") as f:
        f.write(text)


def apply_one(text, anchor, replacement, label, done_marker=None):
    if done_marker is not None and done_marker in text:
        return text
    n = text.count(anchor)
    if n != 1:
        raise RuntimeError("[{}] anchor found {} time(s), expected exactly 1:\n{!r}".format(label, n, anchor))
    return text.replace(anchor, replacement, 1)


# ------------------------------------------------------------------------------------------------
# 1) include/SKSEMenuFramework.h - "!ApocryphaMenuFramework" first, ahead of the direct name.
#    This file is CRLF-terminated already, so it is written back with crlf=True.
# ------------------------------------------------------------------------------------------------
def patch_skse_menu_framework_h():
    path = os.path.join(REPO, "include", "SKSEMenuFramework.h")
    text = read(path)
    if 'GetModuleHandleW(L"!ApocryphaMenuFramework")' in text:
        return  # already applied by a previous (partial) run
    anchor = (
        "inline HMODULE GetMenuFrameworkModule() {\n"
        "    static HMODULE menuFramework = nullptr;\n"
        "    if (!menuFramework) {\n"
        "        menuFramework = GetModuleHandleW(L\"ApocryphaMenuFramework\");\n"
        "        if (!menuFramework) {\n"
        "            menuFramework = GetModuleHandleW(L\"SKSEMenuFramework\");\n"
        "        }\n"
        "    }\n"
        "    return menuFramework;\n"
        "}"
    )
    replacement = (
        "inline HMODULE GetMenuFrameworkModule() {\n"
        "    static HMODULE menuFramework = nullptr;\n"
        "    if (!menuFramework) {\n"
        "        menuFramework = GetModuleHandleW(L\"!ApocryphaMenuFramework\");\n"
        "        if (!menuFramework) {\n"
        "            menuFramework = GetModuleHandleW(L\"ApocryphaMenuFramework\");\n"
        "        }\n"
        "        if (!menuFramework) {\n"
        "            menuFramework = GetModuleHandleW(L\"SKSEMenuFramework\");\n"
        "        }\n"
        "    }\n"
        "    return menuFramework;\n"
        "}"
    )
    text = apply_one(text, anchor, replacement, "SKSEMenuFramework.h:GetMenuFrameworkModule")
    write(path, text, crlf=True)


# ------------------------------------------------------------------------------------------------
# 2) source/MessageListeners.cpp - strings::Configure("CompassNavigationOverhaul") at kDataLoaded.
# ------------------------------------------------------------------------------------------------
def patch_message_listeners_cpp():
    path = os.path.join(REPO, "source", "MessageListeners.cpp")
    text = read(path)
    if 'strings::Configure("CompassNavigationOverhaul")' in text:
        return  # already applied by a previous (partial) run

    text = apply_one(
        text,
        "#include \"Hooks.h\"\n#include \"UI.h\"\n\n#undef GetModuleHandle",
        "#include \"Hooks.h\"\n#include \"UI.h\"\n#include \"utils/Strings.h\"\n\n#undef GetModuleHandle",
        "MessageListeners.cpp:include",
    )

    anchor = (
        "\telse if (a_msg->type == SKSE::MessagingInterface::kDataLoaded)\n"
        "\t{\n"
        "\t\t// Second and last attempt at the NND API (CLAUDE.md rule 17). By kDataLoaded every\n"
        "\t\t// plugin has finished loading, so if it is not available now it is not installed.\n"
        "\t\tNND::NPCNameProvider::GetSingleton()->RequestAPI();\n"
    )
    replacement = (
        "\telse if (a_msg->type == SKSE::MessagingInterface::kDataLoaded)\n"
        "\t{\n"
        "\t\tstrings::Configure(\"CompassNavigationOverhaul\");\n"
        "\n"
        "\t\t// Second and last attempt at the NND API (CLAUDE.md rule 17). By kDataLoaded every\n"
        "\t\t// plugin has finished loading, so if it is not available now it is not installed.\n"
        "\t\tNND::NPCNameProvider::GetSingleton()->RequestAPI();\n"
    )
    text = apply_one(text, anchor, replacement, "MessageListeners.cpp:kDataLoaded")
    write(path, text, crlf=False)


# ------------------------------------------------------------------------------------------------
# 3) source/Diagnostics.cpp - an ArgString() helper (this tool had no op parsing before), a
#    "strings" op on "compassnavigationoverhaul.status" returning strings::StatusJson(), and the
#    descriptor's inputSchema/description updated to mention it.
# ------------------------------------------------------------------------------------------------
def patch_diagnostics_cpp():
    path = os.path.join(REPO, "source", "Diagnostics.cpp")
    text = read(path)

    text = apply_one(
        text,
        "#include \"DevBench/DevBenchAPI.h\"\n#include \"Settings.h\"\n#include \"utils/Logger.h\"\n",
        "#include \"DevBench/DevBenchAPI.h\"\n#include \"Settings.h\"\n#include \"utils/Logger.h\"\n#include \"utils/Strings.h\"\n",
        "Diagnostics.cpp:include",
    )

    # ArgString() + op="strings" dispatch, inserted right before the existing StatusTool body
    # starts building the full status JSON. StatusTool's args parameter was previously unnamed
    # (this tool had no op of its own) - it is named a_argsJson here so ArgString can read it.
    anchor = (
        "\t\tvoid StatusTool(void*, const char*, void* a_sink, DevBenchAPI::WriteFn a_write)\n"
        "\t\t{\n"
        "\t\t\tstd::string json;\n"
    )
    replacement = (
        "\t\t// Reads \"op\" out of the args JSON without pulling in a parser: the strings are this\n"
        "\t\t// tool's own, and a driving tool that needed a JSON library to answer one question\n"
        "\t\t// would be worse than the question.\n"
        "\t\tstd::string ArgString(const std::string& a_args, const char* a_key)\n"
        "\t\t{\n"
        "\t\t\tconst std::string needle = std::string(\"\\\"\") + a_key + \"\\\"\";\n"
        "\t\t\tconst auto at = a_args.find(needle);\n"
        "\t\t\tif (at == std::string::npos) { return {}; }\n"
        "\t\t\tconst auto colon = a_args.find(':', at + needle.size());\n"
        "\t\t\tif (colon == std::string::npos) { return {}; }\n"
        "\t\t\tauto start = a_args.find_first_not_of(\" \\t\", colon + 1);\n"
        "\t\t\tif (start == std::string::npos) { return {}; }\n"
        "\t\t\tif (a_args[start] == '\"') {\n"
        "\t\t\t\tconst auto end = a_args.find('\"', start + 1);\n"
        "\t\t\t\treturn end == std::string::npos ? std::string{} : a_args.substr(start + 1, end - start - 1);\n"
        "\t\t\t}\n"
        "\t\t\tconst auto end = a_args.find_first_of(\",}\", start);\n"
        "\t\t\treturn a_args.substr(start, (end == std::string::npos ? a_args.size() : end) - start);\n"
        "\t\t}\n"
        "\n"
        "\t\tvoid StatusTool(void*, const char* a_argsJson, void* a_sink, DevBenchAPI::WriteFn a_write)\n"
        "\t\t{\n"
        "\t\t\tconst std::string args = a_argsJson ? a_argsJson : \"{}\";\n"
        "\t\t\tif (ArgString(args, \"op\") == \"strings\")\n"
        "\t\t\t{\n"
        "\t\t\t\tconst std::string stringsReply = std::format(R\"({{\"ok\":true,\"op\":\"strings\",\"strings\":{}}})\", strings::StatusJson());\n"
        "\t\t\t\ta_write(a_sink, stringsReply.c_str());\n"
        "\t\t\t\treturn;\n"
        "\t\t\t}\n"
        "\n"
        "\t\t\tstd::string json;\n"
    )
    text = apply_one(text, anchor, replacement, "Diagnostics.cpp:StatusTool ops")

    text = apply_one(
        text,
        "\t\t\t\"and why, the Infinity UI HUD patch lifecycle, and per-frame marker/compass-update \"\n"
        "\t\t\t\"counters.\\\",\"\n"
        "\t\t\t\"\\\"inputSchema\\\":{\\\"type\\\":\\\"object\\\",\\\"properties\\\":{}},\"\n"
        "\t\t\t\"\\\"readOnly\\\":true\"\n"
        "\t\t\t\"}\";",
        "\t\t\t\"and why, the Infinity UI HUD patch lifecycle, and per-frame marker/compass-update \"\n"
        "\t\t\t\"counters. op=strings reports the active language, source and loaded translation \"\n"
        "\t\t\t\"count.\\\",\"\n"
        "\t\t\t\"\\\"inputSchema\\\":{\\\"type\\\":\\\"object\\\",\\\"properties\\\":{\\\"op\\\":{\\\"type\\\":\\\"string\\\"}}},\"\n"
        "\t\t\t\"\\\"readOnly\\\":true\"\n"
        "\t\t\t\"}\";",
        "Diagnostics.cpp:descriptor",
    )
    write(path, text, crlf=False)


# ------------------------------------------------------------------------------------------------
# 4) source/UI.cpp - route every drawn literal through strings::TR().
# ------------------------------------------------------------------------------------------------
def patch_ui_cpp():
    path = os.path.join(REPO, "source", "UI.cpp")
    text = read(path)

    text = apply_one(
        text,
        "#include \"utils/Logger.h\"\n#include \"utils/Toggle.h\"",
        "#include \"utils/Logger.h\"\n#include \"utils/Strings.h\"\n#include \"utils/Toggle.h\"",
        "UI.cpp:include",
    )

    text = apply_one(
        text,
        "#include <algorithm>\n",
        "#include <algorithm>\n#include <string>\n#include <vector>\n",
        "UI.cpp:vector include",
    )

    # --- kLogLevelNames block: add the parallel key array right after it --------------------------
    text = apply_one(
        text,
        "\t\tconstexpr const char* kLogLevelNames[] = { \"Trace\", \"Debug\", \"Info\", \"Warning\", \"Error\", \"Critical\", \"Off\" };\n"
        "\t\tconstexpr int kLogLevelCount = 7;",
        "\t\tconstexpr const char* kLogLevelNames[] = { \"Trace\", \"Debug\", \"Info\", \"Warning\", \"Error\", \"Critical\", \"Off\" };\n"
        "\t\tconstexpr const char* kLogLevelKeys[] = { \"CNO_LogLevel_Trace\", \"CNO_LogLevel_Debug\", \"CNO_LogLevel_Info\",\n"
        "\t\t\t\t\t\t\t\t\t\t\t\t\t\"CNO_LogLevel_Warning\", \"CNO_LogLevel_Error\", \"CNO_LogLevel_Critical\", \"CNO_LogLevel_Off\" };\n"
        "\t\tconstexpr int kLogLevelCount = 7;",
        "UI.cpp:kLogLevelKeys",
    )

    # --- HelpMarker: the "(?)" indicator (the tooltip text passed in is TR'd at each call site) ---
    text = apply_one(
        text,
        "\t\tvoid HelpMarker(const char* a_description)\n"
        "\t\t{\n"
        "\t\t\tImGuiMCP::SameLine();\n"
        "\t\t\tImGuiMCP::TextDisabled(\"(?)\");\n"
        "\n"
        "\t\t\tif (ImGuiMCP::IsItemHovered())\n"
        "\t\t\t{\n"
        "\t\t\t\tImGuiMCP::SetTooltip(\"%s\", a_description);\n"
        "\t\t\t}\n"
        "\t\t}",
        "\t\tvoid HelpMarker(const char* a_description)\n"
        "\t\t{\n"
        "\t\t\tImGuiMCP::SameLine();\n"
        "\t\t\tImGuiMCP::TextDisabled(\"%s\", strings::TR(\"CNO_HelpMark\", \"(?)\"));\n"
        "\n"
        "\t\t\tif (ImGuiMCP::IsItemHovered())\n"
        "\t\t\t{\n"
        "\t\t\t\tImGuiMCP::SetTooltip(\"%s\", a_description);\n"
        "\t\t\t}\n"
        "\t\t}",
        "UI.cpp:HelpMarker",
    )

    # --- NudgeableSlider: the "<-->" nudge indicator ------------------------------------------------
    text = apply_one(
        text,
        "\t\t\t\tImGuiMCP::SameLine();\n"
        "\t\t\t\tImGuiMCP::TextDisabled(\"<-->\");",
        "\t\t\t\tImGuiMCP::SameLine();\n"
        "\t\t\t\tImGuiMCP::TextDisabled(\"%s\", strings::TR(\"CNO_SliderNudge\", \"<-->\"));",
        "UI.cpp:NudgeableSlider",
    )

    # --- RenderDisplaySection ------------------------------------------------------------------------
    text = apply_one(
        text,
        "\t\t\tImGuiMCP::SeparatorText(\"Compass and markers\");\n"
        "\n"
        "\t\t\tif (ImGuiMCP::Toggle(\"Use metric units\", &useMetricUnits))\n"
        "\t\t\t{\n"
        "\t\t\t\tApplyUseMetricUnits();\n"
        "\t\t\t}\n"
        "\t\t\tHelpMarker(\"Shows distances to markers in meters instead of the vanilla feet.\");\n"
        "\n"
        "\t\t\tImGuiMCP::Toggle(\"Show undiscovered location markers\", &showUndiscoveredLocationMarkers);\n"
        "\t\t\tImGuiMCP::Toggle(\"Undiscovered means unknown marker\", &undiscoveredMeansUnknownMarkers);\n"
        "\t\t\tHelpMarker(\"Shows a generic marker instead of the location's real icon until you discover it.\");\n"
        "\t\t\tImGuiMCP::Toggle(\"Undiscovered means unknown info\", &undiscoveredMeansUnknownInfo);\n"
        "\t\t\tHelpMarker(\"Hides the location's name and distance on the compass until you discover it.\");\n"
        "\n"
        "\t\t\tImGuiMCP::Toggle(\"Show enemy markers\", &showEnemyMarkers);\n"
        "\t\t\tImGuiMCP::Toggle(\"Show enemy name under marker\", &showEnemyNameUnderMarker);\n"
        "\t\t\tImGuiMCP::Toggle(\"Show interior markers\", &showInteriorMarkers);\n"
        "\n"
        "\t\t\tImGuiMCP::Toggle(\"Show objective as target\", &showObjectiveAsTarget);\n"
        "\t\t\tImGuiMCP::Toggle(\"Show other objectives count\", &showOtherObjectivesCount);\n"
        "\n"
        "\t\t\tNudgeableSlider(\"Angle to show marker details\", &angleToShowMarkerDetails, 0.0F, 90.0F, \"%.0f\", 1.0F);\n"
        "\t\t\tHelpMarker(\"How close to the center of the compass a marker has to be before its name and distance appear.\");\n"
        "\t\t\tNudgeableSlider(\"Angle to keep marker details shown\", &angleToKeepMarkerDetailsShown, 0.0F, 90.0F, \"%.0f\", 1.0F);\n"
        "\t\t\tHelpMarker(\"Once shown, a marker's details stay visible until it drifts past this wider angle - keeps the text from flickering right at the threshold.\");\n"
        "\t\t\tNudgeableSlider(\"Focusing delay to show\", &focusingDelayToShow, 0.0F, 2.0F, \"%.2f\", 0.01F);\n"
        "\t\t\tHelpMarker(\"How long a marker has to stay within the angle above before its details appear.\");",
        "\t\t\tImGuiMCP::SeparatorText(strings::TR(\"CNO_Title\", \"Compass and markers\"));\n"
        "\n"
        "\t\t\tif (ImGuiMCP::Toggle(strings::TR(\"CNO_UseMetricUnits\", \"Use metric units\"), &useMetricUnits))\n"
        "\t\t\t{\n"
        "\t\t\t\tApplyUseMetricUnits();\n"
        "\t\t\t}\n"
        "\t\t\tHelpMarker(strings::TR(\"CNO_HelpUseMetricUnits\", \"Shows distances to markers in meters instead of the vanilla feet.\"));\n"
        "\n"
        "\t\t\tImGuiMCP::Toggle(strings::TR(\"CNO_ShowUndiscoveredLocationMarkers\", \"Show undiscovered location markers\"), &showUndiscoveredLocationMarkers);\n"
        "\t\t\tImGuiMCP::Toggle(strings::TR(\"CNO_UndiscoveredMeansUnknownMarkers\", \"Undiscovered means unknown marker\"), &undiscoveredMeansUnknownMarkers);\n"
        "\t\t\tHelpMarker(strings::TR(\"CNO_HelpUndiscoveredMeansUnknownMarkers\", \"Shows a generic marker instead of the location's real icon until you discover it.\"));\n"
        "\t\t\tImGuiMCP::Toggle(strings::TR(\"CNO_UndiscoveredMeansUnknownInfo\", \"Undiscovered means unknown info\"), &undiscoveredMeansUnknownInfo);\n"
        "\t\t\tHelpMarker(strings::TR(\"CNO_HelpUndiscoveredMeansUnknownInfo\", \"Hides the location's name and distance on the compass until you discover it.\"));\n"
        "\n"
        "\t\t\tImGuiMCP::Toggle(strings::TR(\"CNO_ShowEnemyMarkers\", \"Show enemy markers\"), &showEnemyMarkers);\n"
        "\t\t\tImGuiMCP::Toggle(strings::TR(\"CNO_ShowEnemyNameUnderMarker\", \"Show enemy name under marker\"), &showEnemyNameUnderMarker);\n"
        "\t\t\tImGuiMCP::Toggle(strings::TR(\"CNO_ShowInteriorMarkers\", \"Show interior markers\"), &showInteriorMarkers);\n"
        "\n"
        "\t\t\tImGuiMCP::Toggle(strings::TR(\"CNO_ShowObjectiveAsTarget\", \"Show objective as target\"), &showObjectiveAsTarget);\n"
        "\t\t\tImGuiMCP::Toggle(strings::TR(\"CNO_ShowOtherObjectivesCount\", \"Show other objectives count\"), &showOtherObjectivesCount);\n"
        "\n"
        "\t\t\tNudgeableSlider(strings::TR(\"CNO_AngleToShowMarkerDetails\", \"Angle to show marker details\"), &angleToShowMarkerDetails, 0.0F, 90.0F, \"%.0f\", 1.0F);\n"
        "\t\t\tHelpMarker(strings::TR(\"CNO_HelpAngleToShowMarkerDetails\", \"How close to the center of the compass a marker has to be before its name and distance appear.\"));\n"
        "\t\t\tNudgeableSlider(strings::TR(\"CNO_AngleToKeepMarkerDetailsShown\", \"Angle to keep marker details shown\"), &angleToKeepMarkerDetailsShown, 0.0F, 90.0F, \"%.0f\", 1.0F);\n"
        "\t\t\tHelpMarker(strings::TR(\"CNO_HelpAngleToKeepMarkerDetailsShown\", \"Once shown, a marker's details stay visible until it drifts past this wider angle - keeps the text from flickering right at the threshold.\"));\n"
        "\t\t\tNudgeableSlider(strings::TR(\"CNO_FocusingDelayToShow\", \"Focusing delay to show\"), &focusingDelayToShow, 0.0F, 2.0F, \"%.2f\", 0.01F);\n"
        "\t\t\tHelpMarker(strings::TR(\"CNO_HelpFocusingDelayToShow\", \"How long a marker has to stay within the angle above before its details appear.\"));",
        "UI.cpp:RenderDisplaySection",
    )

    # --- RenderQuestListSection ----------------------------------------------------------------------
    text = apply_one(
        text,
        "\t\t\tImGuiMCP::SeparatorText(\"Quest list\");\n"
        "\n"
        "\t\t\tNudgeableSlider(\"Position X\", &positionX, 0.0F, 1.0F, \"%.3f\", 0.01F);\n"
        "\t\t\tNudgeableSlider(\"Position Y\", &positionY, 0.0F, 1.0F, \"%.3f\", 0.01F);\n"
        "\t\t\tNudgeableSlider(\"Max height\", &maxHeight, 0.0F, 1.0F, \"%.2f\", 0.01F);\n"
        "\n"
        "\t\t\tImGuiMCP::Toggle(\"Show in exteriors\", &showInExteriors);\n"
        "\t\t\tImGuiMCP::Toggle(\"Show in interiors\", &showInInteriors);\n"
        "\t\t\tImGuiMCP::Toggle(\"Hide in combat\", &hideInCombat);\n"
        "\t\t\tHelpMarker(\"Hides the quest list entirely while a weapon or spell is drawn.\");\n"
        "\n"
        "\t\t\tNudgeableSlider(\"Walking delay to show\", &walkingDelayToShow, 0.0F, 3.0F, \"%.2f\", 0.01F);\n"
        "\t\t\tNudgeableSlider(\"Jogging delay to show\", &joggingDelayToShow, 0.0F, 3.0F, \"%.2f\", 0.01F);\n"
        "\t\t\tNudgeableSlider(\"Sprinting delay to show\", &sprintingDelayToShow, 0.0F, 3.0F, \"%.2f\", 0.01F);\n"
        "\t\t\tHelpMarker(\"How long you have to move at each pace before the quest list fades in.\");",
        "\t\t\tImGuiMCP::SeparatorText(strings::TR(\"CNO_QuestListTitle\", \"Quest list\"));\n"
        "\n"
        "\t\t\tNudgeableSlider(strings::TR(\"CNO_PositionX\", \"Position X\"), &positionX, 0.0F, 1.0F, \"%.3f\", 0.01F);\n"
        "\t\t\tNudgeableSlider(strings::TR(\"CNO_PositionY\", \"Position Y\"), &positionY, 0.0F, 1.0F, \"%.3f\", 0.01F);\n"
        "\t\t\tNudgeableSlider(strings::TR(\"CNO_MaxHeight\", \"Max height\"), &maxHeight, 0.0F, 1.0F, \"%.2f\", 0.01F);\n"
        "\n"
        "\t\t\tImGuiMCP::Toggle(strings::TR(\"CNO_ShowInExteriors\", \"Show in exteriors\"), &showInExteriors);\n"
        "\t\t\tImGuiMCP::Toggle(strings::TR(\"CNO_ShowInInteriors\", \"Show in interiors\"), &showInInteriors);\n"
        "\t\t\tImGuiMCP::Toggle(strings::TR(\"CNO_HideInCombat\", \"Hide in combat\"), &hideInCombat);\n"
        "\t\t\tHelpMarker(strings::TR(\"CNO_HelpHideInCombat\", \"Hides the quest list entirely while a weapon or spell is drawn.\"));\n"
        "\n"
        "\t\t\tNudgeableSlider(strings::TR(\"CNO_WalkingDelayToShow\", \"Walking delay to show\"), &walkingDelayToShow, 0.0F, 3.0F, \"%.2f\", 0.01F);\n"
        "\t\t\tNudgeableSlider(strings::TR(\"CNO_JoggingDelayToShow\", \"Jogging delay to show\"), &joggingDelayToShow, 0.0F, 3.0F, \"%.2f\", 0.01F);\n"
        "\t\t\tNudgeableSlider(strings::TR(\"CNO_SprintingDelayToShow\", \"Sprinting delay to show\"), &sprintingDelayToShow, 0.0F, 3.0F, \"%.2f\", 0.01F);\n"
        "\t\t\tHelpMarker(strings::TR(\"CNO_HelpPaceDelays\", \"How long you have to move at each pace before the quest list fades in.\"));",
        "UI.cpp:RenderQuestListSection",
    )

    # --- RenderDebugSection: SeparatorText + Combo label + option list + HelpMarker ---------------
    text = apply_one(
        text,
        "\t\t\tImGuiMCP::SeparatorText(\"Debug\");\n"
        "\n"
        "\t\t\tint level = static_cast<int>(debug::logLevel);\n"
        "\t\t\tif (ImGuiMCP::Combo(\"Log level\", &level, kLogLevelNames, kLogLevelCount))\n"
        "\t\t\t{\n"
        "\t\t\t\tdebug::logLevel = static_cast<logger::level>(level);\n"
        "\n"
        "\t\t\t\tOnMainThread([]() { logger::set_level(settings::debug::logLevel, settings::debug::logLevel); });\n"
        "\t\t\t}\n"
        "\t\t\tHelpMarker(\"Applies to the log immediately.\");",
        "\t\t\tImGuiMCP::SeparatorText(strings::TR(\"CNO_Debug\", \"Debug\"));\n"
        "\n"
        "\t\t\tint level = static_cast<int>(debug::logLevel);\n"
        "\t\t\t// Rebuilt from TR'd entries every frame (plan 2.2); labelStore owns the translated\n"
        "\t\t\t// bytes for this call so the const char* pointers handed to Combo stay valid.\n"
        "\t\t\tstd::vector<std::string> logLevelLabelStore;\n"
        "\t\t\tlogLevelLabelStore.reserve(kLogLevelCount);\n"
        "\t\t\tfor (int i = 0; i < kLogLevelCount; ++i)\n"
        "\t\t\t{\n"
        "\t\t\t\tlogLevelLabelStore.push_back(strings::TR(kLogLevelKeys[i], kLogLevelNames[i]));\n"
        "\t\t\t}\n"
        "\t\t\tstd::vector<const char*> logLevelLabels;\n"
        "\t\t\tlogLevelLabels.reserve(logLevelLabelStore.size());\n"
        "\t\t\tfor (const auto& s : logLevelLabelStore) { logLevelLabels.push_back(s.c_str()); }\n"
        "\t\t\tif (ImGuiMCP::Combo(strings::TR(\"CNO_LogLevel\", \"Log level\"), &level, logLevelLabels.data(), kLogLevelCount))\n"
        "\t\t\t{\n"
        "\t\t\t\tdebug::logLevel = static_cast<logger::level>(level);\n"
        "\n"
        "\t\t\t\tOnMainThread([]() { logger::set_level(settings::debug::logLevel, settings::debug::logLevel); });\n"
        "\t\t\t}\n"
        "\t\t\tHelpMarker(strings::TR(\"CNO_HelpLogLevel\", \"Applies to the log immediately.\"));",
        "UI.cpp:RenderDebugSection",
    )

    # --- RenderButtons: Save / Reload / Restore buttons, their HelpMarkers, status assignments ----
    text = apply_one(
        text,
        "\t\t\tif (ImGuiMCP::Button(\"Save\"))\n"
        "\t\t\t{\n"
        "\t\t\t\tOnMainThread([]() {\n"
        "\t\t\t\t\tstatusMessage = settings::Save() ? \"Settings saved.\" : \"Could not save the INI. See the log for why.\";\n"
        "\t\t\t\t});\n"
        "\t\t\t}\n"
        "\t\t\tHelpMarker(\"Writes every setting above back to the INI. Comments and unrelated keys are left alone.\");\n"
        "\n"
        "\t\t\tImGuiMCP::SameLine();\n"
        "\n"
        "\t\t\tif (ImGuiMCP::Button(\"Reload from INI\"))\n"
        "\t\t\t{\n"
        "\t\t\t\tOnMainThread([]() {\n"
        "\t\t\t\t\tif (settings::Reload())\n"
        "\t\t\t\t\t{\n"
        "\t\t\t\t\t\tApplyLiveSettings();\n"
        "\n"
        "\t\t\t\t\t\tstatusMessage = \"Settings reloaded from the INI.\";\n"
        "\t\t\t\t\t}\n"
        "\t\t\t\t\telse\n"
        "\t\t\t\t\t{\n"
        "\t\t\t\t\t\tstatusMessage = \"Could not read the INI. See the log for why.\";\n"
        "\t\t\t\t\t}\n"
        "\t\t\t\t});\n"
        "\t\t\t}\n"
        "\t\t\tHelpMarker(\"Throws away any change made here since the last save and re-reads the INI from disk. Also picks up edits made to the file by hand.\");\n"
        "\n"
        "\t\t\tImGuiMCP::SameLine();\n"
        "\n"
        "\t\t\tif (ImGuiMCP::Button(\"Restore defaults\"))\n"
        "\t\t\t{\n"
        "\t\t\t\tOnMainThread([]() {\n"
        "\t\t\t\t\tsettings::RestoreDefaults();\n"
        "\t\t\t\t\tApplyLiveSettings();\n"
        "\t\t\t\t});\n"
        "\n"
        "\t\t\t\tstatusMessage = \"Defaults restored. Press Save to keep them.\";\n"
        "\t\t\t}\n"
        "\t\t\tHelpMarker(\"Puts every setting back to the value it has on a fresh install. Nothing is written until you press Save.\");",
        "\t\t\tif (ImGuiMCP::Button(strings::TR(\"CNO_SaveBtn\", \"Save\")))\n"
        "\t\t\t{\n"
        "\t\t\t\tOnMainThread([]() {\n"
        "\t\t\t\t\tstatusMessage = settings::Save() ? strings::TR(\"CNO_StatusSaved\", \"Settings saved.\")\n"
        "\t\t\t\t\t\t\t\t\t\t\t\t\t   : strings::TR(\"CNO_StatusSaveFail\", \"Could not save the INI. See the log for why.\");\n"
        "\t\t\t\t});\n"
        "\t\t\t}\n"
        "\t\t\tHelpMarker(strings::TR(\"CNO_HelpSave\", \"Writes every setting above back to the INI. Comments and unrelated keys are left alone.\"));\n"
        "\n"
        "\t\t\tImGuiMCP::SameLine();\n"
        "\n"
        "\t\t\tif (ImGuiMCP::Button(strings::TR(\"CNO_ReloadBtn\", \"Reload from INI\")))\n"
        "\t\t\t{\n"
        "\t\t\t\tOnMainThread([]() {\n"
        "\t\t\t\t\tif (settings::Reload())\n"
        "\t\t\t\t\t{\n"
        "\t\t\t\t\t\tApplyLiveSettings();\n"
        "\n"
        "\t\t\t\t\t\tstatusMessage = strings::TR(\"CNO_StatusReloaded\", \"Settings reloaded from the INI.\");\n"
        "\t\t\t\t\t}\n"
        "\t\t\t\t\telse\n"
        "\t\t\t\t\t{\n"
        "\t\t\t\t\t\tstatusMessage = strings::TR(\"CNO_StatusReloadFail\", \"Could not read the INI. See the log for why.\");\n"
        "\t\t\t\t\t}\n"
        "\t\t\t\t});\n"
        "\t\t\t}\n"
        "\t\t\tHelpMarker(strings::TR(\"CNO_HelpReload\", \"Throws away any change made here since the last save and re-reads the INI from disk. Also picks up edits made to the file by hand.\"));\n"
        "\n"
        "\t\t\tImGuiMCP::SameLine();\n"
        "\n"
        "\t\t\tif (ImGuiMCP::Button(strings::TR(\"CNO_RestoreBtn\", \"Restore defaults\")))\n"
        "\t\t\t{\n"
        "\t\t\t\tOnMainThread([]() {\n"
        "\t\t\t\t\tsettings::RestoreDefaults();\n"
        "\t\t\t\t\tApplyLiveSettings();\n"
        "\t\t\t\t});\n"
        "\n"
        "\t\t\t\tstatusMessage = strings::TR(\"CNO_StatusRestored\", \"Defaults restored. Press Save to keep them.\");\n"
        "\t\t\t}\n"
        "\t\t\tHelpMarker(strings::TR(\"CNO_HelpRestore\", \"Puts every setting back to the value it has on a fresh install. Nothing is written until you press Save.\"));",
        "UI.cpp:RenderButtons",
    )

    # --- SettingsPanel::Render: strings::Tick() first, then the intro text ------------------------
    text = apply_one(
        text,
        "\tvoid __stdcall SettingsPanel::Render()\n"
        "\t{\n"
        "\t\tImGuiMCP::TextWrapped(\"Most settings apply as soon as you make them. Press Save to keep them for the next time you play.\");",
        "\tvoid __stdcall SettingsPanel::Render()\n"
        "\t{\n"
        "\t\tstrings::Tick();\n"
        "\n"
        "\t\tImGuiMCP::TextWrapped(\"%s\", strings::TR(\"CNO_Intro\", \"Most settings apply as soon as you make them. Press Save to keep them for the next time you play.\"));",
        "UI.cpp:Render Tick+Intro",
    )

    write(path, text, crlf=False)


def main():
    patch_skse_menu_framework_h()
    patch_message_listeners_cpp()
    patch_diagnostics_cpp()
    patch_ui_cpp()
    print("lang-patch.py: all anchors matched and patched.")


if __name__ == "__main__":
    main()
