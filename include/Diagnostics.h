#pragma once

#include <cstddef>
#include <cstdint>
#include <string_view>

// Backs the "compassnavigationoverhaul.status" DevBench tool - see CLAUDE.md rule 31 (every
// mod's first version ships with live-queryable state, not just logs reconstructed after the
// fact).
//
// Every Record* function here is called from the main thread, at the exact point a decision is
// made, and only ever writes a mutex-guarded snapshot. The DevBench tool handler runs on
// devbench's own listener thread and only ever reads that snapshot - it never reaches back into
// game state itself. That matters more here than in most of this project's mods: this one's
// bookkeeping hangs off the compass update vfunc and the HUDMarkerManager hooks, i.e. off the
// render/HUD path, so anything the handler touched directly would be read while the main thread
// is mid-frame writing it.
namespace diagnostics
{
	// Looks up the DevBench interface (present only if the DevBench plugin is installed) and
	// registers "compassnavigationoverhaul.status". Safe to call repeatedly - a real launch
	// showed devbench's own server can still be finishing startup a moment after SKSE sends
	// kPostLoad (its own documented earliest-safe point), so this is a rule-17 retry, not a
	// one-shot lookup: call it again at kPostPostLoad and kDataLoaded too. Every call after the
	// first successful one is a cheap no-op; only the final call (a_lastAttempt = true) logs
	// that DevBench was never found, so the "not installed" conclusion isn't reported before
	// every retry is exhausted.
	void Init(bool a_lastAttempt = false);

	// ---------------------------------------------------------------------------------------
	// Hook installation
	// ---------------------------------------------------------------------------------------

	// One entry per hook SITE GROUP written by hooks::Install(). kAllowedToShowMapMarker covers
	// both of its two sites and is only marked installed once both have been written, since a
	// half-written pair is the failure mode that actually happened here (the trampoline running
	// out of room on the second write).
	enum class HookGroup
	{
		kUpdateQuests,
		kAllowedToShowMapMarker,
		kUpdateLocations,
		kUpdateEnemies,
		kUpdatePlayerSetMarker,
		kCompassUpdateVFunc,

		kTotal
	};

	// hooks::Install() has been entered. Recorded before anything is written, so a status query
	// after a partial install can tell "never ran" from "ran and stopped part way".
	void RecordHookInstallStarted();

	// The shared SKSE trampoline has been allocated. a_valid is Trampoline::IsValid() - false
	// means no usable memory block was reserved and every write through it would be bogus.
	void RecordTrampolineAllocated(std::size_t a_bytes, bool a_valid);

	// The named group's branch/call (or vfunc write) returned. Marks that one group installed.
	void RecordHookGroupInstalled(HookGroup a_group);

	// hooks::Install() ran to completion with every group written.
	void RecordHookInstallCompleted();

	// ---------------------------------------------------------------------------------------
	// CoMAP (MapMarkerFramework) compatibility patch - explicit tri-state
	// ---------------------------------------------------------------------------------------

	// CoMAP was looked for at kPostLoad. a_present/a_version come straight from
	// SKSE::LoadInterface::GetPluginInfo, so a query can see the version the decision was made
	// on rather than having to trust the decision.
	void RecordComapDetection(bool a_present, std::uint32_t a_version);

	// The patch was never tried at all (CoMAP absent, or new enough not to need it). a_reason
	// says which.
	void RecordComapPatchNotAttempted(std::string_view a_reason);

	// hooks::compat::MapMarkerFramework::Install() returned true - the hook is live.
	void RecordComapPatchApplied();

	// Install() bailed out. Called at each of its own `return false` sites so the reason is the
	// real one from where the decision was made, not a guess reconstructed by the caller.
	void RecordComapPatchSkipped(std::string_view a_reason);

	// ---------------------------------------------------------------------------------------
	// Quest Marker Limit Fix compatibility and the built-in quest marker cap
	// ---------------------------------------------------------------------------------------

	// Quest Marker Limit Fix was looked for at kPostLoad (SKSE::LoadInterface::GetPluginInfo). Reporting only - the
	// deferred quest-marker path is passive and works the same whether or not it is installed.
	void RecordQmlfDetection(bool a_present, std::uint32_t a_version);

	// One reconciliation pass inside SetMarkersExtraInfo() finished. Totals are cumulative; the last pair is what the
	// most recent pass matched and dropped. All zero with no timestamp means nothing ever needed reconciling - the
	// expected state without Quest Marker Limit Fix.
	void RecordQuestMarkersReconciled(std::size_t a_matched, std::size_t a_unmatched);

	// hooks::QuestMarkerLimit::Install() decided. a_applied is false when the cap comparison was not found, held an
	// unexpected value or could not be written (the runtime then keeps its own cap); a_offset is the comparison's
	// offset in UpdateQuests (0 when not found); a_note says which case happened.
	void RecordQuestMarkerLimitPatch(bool a_applied, std::size_t a_offset, std::uint32_t a_limit, std::string_view a_note);

	// ---------------------------------------------------------------------------------------
	// Infinity UI / HUD patch lifecycle
	// ---------------------------------------------------------------------------------------

	// Result of RegisterListener("InfinityUI", ...) at kPostLoad. False means nothing below
	// this line can ever happen, which is the single most useful thing to know when the compass
	// looks vanilla in game.
	void RecordInfinityUiListener(bool a_registered);

	// kStartLoadInstances / kFinishLoadInstances for the HUD menu.
	void RecordHudPatchStarted();
	void RecordHudPatchFinished();

	// The replaced compass / quest item list instances were handed to us and their singletons
	// came back non-null (or did not).
	void RecordCompassInstanceReady(bool a_ready);
	void RecordQuestItemListReady(bool a_ready);

	// 1.1.7: the settings page moved the quest list live; a_screenX/Y is where its origin landed on screen
	// (localToGlobal of the clip after the move), so a test can read the move without a picture.
	void RecordQuestListMoved(float a_screenX, float a_screenY);

	// 1.2.0: the compass text size was written to the movie (a_applied) or could not be (no FocusedMarkerInfo clip).
	void RecordTextScaleApplied(float a_scale, bool a_applied);

	// ---------------------------------------------------------------------------------------
	// Per-frame marker bookkeeping
	// ---------------------------------------------------------------------------------------
	//
	// These are the cheap half of the marker story: plain counter increments at the four
	// CNO::HUDMarkerManager::Process*Marker entry points, which the game's own marker loop
	// already calls once per marker per frame. Deliberately NOT recorded: anything about the
	// individual marker (its ref, name, distance). Those live in game memory the handler must
	// never touch, and copying them per marker per frame would put a heap allocation on the HUD
	// path to answer a question the aggregate counts already answer.
	void RecordQuestMarkerProcessed();
	void RecordLocationMarkerProcessed();
	void RecordEnemyMarkerProcessed();
	void RecordPlayerSetMarkerProcessed();

	// 1.1.7 (makihaze, 2026-10-08: location markers missing while many quests are active): every AddMarker offer at the
	// quest and location call sites, accepted or declined, counted for the frame (atomics - no lock on the HUD path);
	// RecordMarkerFrameEnd() snapshots the frame once per compass update. The status JSON's markers.lastFrame then says
	// whether quest markers took the slots and location markers were turned away, measured rather than guessed.
	void RecordQuestMarkerOffer(bool a_added);
	void RecordLocationMarkerOffer(bool a_added);
	void RecordMarkerFrameEnd();

	// End of SetMarkersExtraInfo(), taken just before the per-frame containers are cleared - so
	// these are the counts that frame actually worked with. All four are sizes already
	// maintained by the update itself, so snapshotting them costs a lock and four loads.
	void RecordCompassUpdate(std::size_t a_facedMarkers, std::size_t a_questMarkerRefs,
							 std::size_t a_miscQuestMarkerRefs, bool a_hasFocusedMarker);

	// SetMarkersExtraInfo() bailed out before doing anything because a HUD singleton it needs
	// was not ready yet.
	void RecordCompassUpdateSkipped(std::string_view a_reason);

	// The focused marker changed. Only here does a description string get copied, because focus
	// changes are user-paced (a few a second at worst) rather than per-frame.
	void RecordFocusChanged(std::string_view a_focusedDescription);
}
