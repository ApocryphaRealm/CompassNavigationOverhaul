#include "Hooks.h"

#include "Settings.h"

#include "HUDMarkerManager.h"

namespace hooks
{
	namespace
	{
		// The quest and tracked objective a quest target belongs to, found by comparing it with every target of every
		// objective the player tracks. It has to be resolved while the AddMarker call is on the stack - the
		// TESQuestTarget is only valid for that call - which is why the deferred path stores the result.
		struct QuestTargetInfo
		{
			RE::TESQuest* quest = nullptr;
			RE::BGSInstancedQuestObjective* questObjective = nullptr;
			int questAgeIndex = -1;
		};

		QuestTargetInfo ResolveQuestTarget(const RE::TESQuestTarget* a_questTarget)
		{
			if (!a_questTarget)
			{
				return {};
			}

			auto questObjectiveTarget = reinterpret_cast<RE::TESQuestTarget*>(a_questTarget->unk00);

			auto player = RE::PlayerCharacter::GetSingleton();

#if RUNTIME_LINE == 17
			// CommonLibSSE-NG 7.2: no VR on this line, and GetPlayerRuntimeData() already applies 1.7.99's +8 shift
			RE::BSTArray<RE::BGSInstancedQuestObjective>& playerObjectives = player->GetPlayerRuntimeData().objectives;
#else
			RE::BSTArray<RE::BGSInstancedQuestObjective>& playerObjectives = REL::Module::IsVR() ?
																				player->GetVRPlayerRuntimeData().objectives :
																				player->GetPlayerRuntimeData().objectives;
#endif

			// The objectives are in oldest-to-newest order, so we iterate from newest-to-oldest
			// to have it in the same order as in the journal
			for (int ageIndex = playerObjectives.size() - 1; ageIndex >= 0; ageIndex--)
			{
				RE::BGSInstancedQuestObjective* playerObjective = &playerObjectives[ageIndex];
				RE::BGSQuestObjective* questObjective = playerObjective->objective;

				for (int j = 0; j < questObjective->numTargets; j++)
				{
					if (questObjectiveTarget == questObjective->targets[j])
					{
						return { questObjective->ownerQuest, playerObjective, ageIndex };
					}
				}
			}

			return {};
		}

		// One byte of executable memory: lift the page protection, write, flush the instruction cache, restore. The
		// read-back is what makes the result honest rather than trusting VirtualProtect's word for it.
		bool WriteProtectedByte(std::uintptr_t a_address, std::uint8_t a_value)
		{
			auto address = reinterpret_cast<std::uint8_t*>(a_address);

			DWORD oldProtect = 0;

			if (!VirtualProtect(address, 1, PAGE_EXECUTE_READWRITE, &oldProtect))
			{
				return false;
			}

			*address = a_value;

			FlushInstructionCache(GetCurrentProcess(), address, 1);

			DWORD restoreProtect = 0;
			VirtualProtect(address, 1, oldProtect, &restoreProtect);

			return *address == a_value;
		}
	}

	bool QuestMarkerLimit::Install()
	{
		// The cap instruction: `cmp dword ptr [reg+0x14BC], imm8; jae ...` - HUDMarkerManager's currentMarkerIndex
		// (offset 0x14BC, RE/H/HUDMarkerManager.h) against vanilla's 16, jumping past this target's AddMarker call when
		// the quest share is full. Found by scanning the opening bytes of UpdateQuests rather than by a hardcoded
		// offset, so a runtime it was never checked on either finds it or keeps its shipped cap. Known hit on AE
		// 1.6.1170: UpdateQuests+0xAB (the offset Quest Marker Limit Fix patches). Encodings accepted:
		//   83 B8+r BC 14 00 00 ii 73 xx               cmp [reg+0x14BC], ii ; jae rel8
		//   83 B8+r BC 14 00 00 ii 0F 83 xx xx xx xx   cmp [reg+0x14BC], ii ; jae rel32
		// The window covers the call site the UpdateQuests hook patches (SE +0x114, AE +0x180): the cap has to sit
		// ahead of the AddMarker call it guards. Ported from hidden-kc's fork (their 1.1.4), 2026-09-26.
		constexpr std::size_t kScanLength = 0x200;
		constexpr std::size_t kImmediateOffset = 6;
		constexpr std::uint8_t kVanillaCap = 0x10;

		const std::uintptr_t functionStart = HUDMarkerManager::UpdateQuests.address();
		const auto code = reinterpret_cast<const std::uint8_t*>(functionStart);
		const auto target = static_cast<std::uint8_t>(settings::display::questMarkerLimit);

		logger::debug("Quest marker limit: scanning UpdateQuests at {:#x} for the cap comparison (target {})",
					  functionStart, settings::display::questMarkerLimit);

		std::size_t cmpOffset = kScanLength;

		for (std::size_t offset = 0; offset + 9 <= kScanLength; offset++)
		{
			const bool isCmp = code[offset] == 0x83 &&
							   (code[offset + 1] & 0xF8) == 0xB8 &&  // mod=10 (disp32), reg=/7 (cmp), rm=any register
							   code[offset + 2] == 0xBC && code[offset + 3] == 0x14 &&
							   code[offset + 4] == 0x00 && code[offset + 5] == 0x00;
			const bool isJae = code[offset + 7] == 0x73 || (code[offset + 7] == 0x0F && code[offset + 8] == 0x83);

			if (isCmp && isJae)
			{
				cmpOffset = offset;

				break;
			}
		}

		if (cmpOffset == kScanLength)
		{
			logger::warn("Quest marker limit: the cap comparison was not found in the first {:#x} bytes of "
						 "UpdateQuests; the quest marker cap stays at this runtime's own (vanilla: 16)", kScanLength);

			diagnostics::RecordQuestMarkerLimitPatch(false, 0, settings::display::questMarkerLimit,
				"the cap comparison was not found in UpdateQuests' first 0x200 bytes (a runtime this scan was never "
				"checked against); the cap is unchanged");

			return false;
		}

		const std::size_t immediateOffset = cmpOffset + kImmediateOffset;
		const std::uint8_t current = code[immediateOffset];

		// A byte already at the target is success, not work: written earlier this session or by Quest Marker Limit
		// Fix's own identical patch.
		if (current == target)
		{
			logger::info("Quest marker limit: the cap is already {} (UpdateQuests+{:#x}); nothing to patch",
						 settings::display::questMarkerLimit, cmpOffset);

			diagnostics::RecordQuestMarkerLimitPatch(true, cmpOffset, settings::display::questMarkerLimit,
				"the cap already held the target value (patched earlier, or by Quest Marker Limit Fix); no byte "
				"was written");

			installed = true;

			return true;
		}

		// Neither vanilla's 16 nor the target: a comparison this scan does not understand. Leave it alone - skipping
		// costs the raise, writing blind could cost a crash.
		if (current != kVanillaCap)
		{
			logger::warn("Quest marker limit: the cap comparison at UpdateQuests+{:#x} holds an unexpected value "
						 "({:#x}); skipping so no unknown byte is overwritten", cmpOffset, current);

			diagnostics::RecordQuestMarkerLimitPatch(false, cmpOffset, settings::display::questMarkerLimit,
				std::format("the cap held {:#x}, neither vanilla 16 nor the target; nothing was written", current));

			return false;
		}

		if (!WriteProtectedByte(functionStart + immediateOffset, target))
		{
			logger::error("Quest marker limit: could not write the cap at UpdateQuests+{:#x}; it stays at 16",
						  immediateOffset);

			diagnostics::RecordQuestMarkerLimitPatch(false, cmpOffset, settings::display::questMarkerLimit,
				"writing the cap failed (VirtualProtect refused, or the read-back disagreed); the cap is unchanged");

			return false;
		}

		logger::info("Quest marker limit: raised the game's quest marker cap from 16 to {} (UpdateQuests+{:#x})",
					 settings::display::questMarkerLimit, cmpOffset);

		diagnostics::RecordQuestMarkerLimitPatch(true, cmpOffset, settings::display::questMarkerLimit,
			"the vanilla 16-marker cap was rewritten to the configured limit");

		installed = true;

		return true;
	}

	bool UpdateQuests(const RE::HUDMarkerManager* a_hudMarkerManager, RE::HUDMarker::ScaleformData* a_markerData,
					  RE::NiPoint3* a_pos, const RE::RefHandle& a_refHandle, std::uint32_t a_markerGotoFrame,
					  RE::TESQuestTarget* a_questTarget)
	{
		// The game loops through active quest targets, and calls `AddMarker` for each one.
		// If multiple targets correspond to the same marker, `AddMarker` returns the
		// previously-created marker (via `a_refHandle`), so we can iteratively
		// build the structure containing all the targets, objectives, etc. corresponding
		// to the marker.
		const bool added = HUDMarkerManager::AddMarker(a_hudMarkerManager, a_markerData, a_pos, a_refHandle, a_markerGotoFrame);

		RE::TESObjectREFR* marker = RE::TESObjectREFR::LookupByHandle(a_refHandle).get();

		// The handle can go stale between when the vanilla marker list was built and this hook running (the
		// reference unloading, a cell transition, etc). When it was added, AddMarker has already committed it, so the
		// marker still shows - we only skip the quest bookkeeping. When it was declined there is nothing to defer, and
		// warning would repeat once per target per frame under a marker-limit mod.
		if (!marker)
		{
			if (added)
			{
				logger::warn("Quest marker's reference handle did not resolve to a live reference; skipping its quest bookkeeping this frame");
			}

			return added;
		}

		const QuestTargetInfo target = ResolveQuestTarget(a_questTarget);

		if (added)
		{
			if (target.quest)
			{
				CNO::HUDMarkerManager::GetSingleton()->ProcessQuestMarker(target.quest, target.questObjective,
																		  target.questAgeIndex, marker, a_markerGotoFrame,
																		  a_hudMarkerManager->currentMarkerIndex - 1);
			}

			return true;
		}

		// AddMarker declined this target. Quest Marker Limit Fix makes that happen for every quest target: it swallows
		// the call and later replays its chosen markers straight into the original AddMarker, which this call-site hook
		// never sees. Keep the target with a copy of the position (the pointer is only valid for this call); if
		// something commits it this frame, ReconcilePendingQuestMarkers() finds it in the marker array.
		if (target.quest && a_pos)
		{
			CNO::HUDMarkerManager::GetSingleton()->DeferQuestMarker(marker, *a_pos, target.quest, target.questObjective,
																	target.questAgeIndex, a_markerGotoFrame);
		}

		return false;
	}

	RE::TESWorldSpace* AllowedToShowMapMarker(const RE::TESObjectREFR* a_marker)
	{
		RE::TESWorldSpace* markerWorldspace = a_marker->GetWorldspace();

		if (settings::display::showInteriorMarkers)
		{
			auto player = RE::PlayerCharacter::GetSingleton();

			RE::TESWorldSpace* playerWorldspace = player->GetWorldspace();

			if (playerWorldspace && markerWorldspace && playerWorldspace != markerWorldspace)
			{
				if (!playerWorldspace->parentWorld && markerWorldspace->parentWorld == playerWorldspace) 
				{
					return playerWorldspace;
				}
			}
		}
			
		return markerWorldspace;
	}

	bool UpdateLocations(const RE::HUDMarkerManager* a_hudMarkerManager, RE::HUDMarker::ScaleformData* a_markerData,
							   RE::NiPoint3* a_pos, const RE::RefHandle& a_refHandle, std::uint32_t a_markerGotoFrame)
	{
		RE::TESObjectREFR* marker = RE::TESObjectREFR::LookupByHandle(a_refHandle).get();

		// The handle can go stale between when the vanilla marker list was built and this
		// hook running (the reference unloading, a cell transition, etc). Nothing has been
		// committed for this marker yet, so skip it entirely rather than dereference it.
		if (!marker)
		{
			logger::warn("Location marker's reference handle did not resolve to a live reference; skipping this marker");

			return false;
		}

		RE::PlayerCharacter* player = RE::PlayerCharacter::GetSingleton();

		RE::NiPoint3 markerPos = util::GetRealPosition(marker);
		RE::NiPoint3 playerPos = util::GetRealPosition(player);

		float sqDistanceToMarker = playerPos.GetSquaredDistance(markerPos);

		if (sqDistanceToMarker < RE::HUDMarkerManager::GetSingleton()->sqRadiusToAddLocation)
		{
			auto mapMarker = marker->extraList.GetByType<RE::ExtraMapMarker>();

			// A location marker's reference is expected to carry ExtraMapMarker data, but a
			// mod that adds a marker ref without it (or removes it) would otherwise crash here.
			if (!mapMarker)
			{
				logger::warn("Location marker reference has no ExtraMapMarker data; skipping this marker");

				return false;
			}

			// Unvisited markers keep being shown in any case
			if (settings::display::showUndiscoveredLocationMarkers || mapMarker->mapData->flags.all(RE::MapMarkerData::Flag::kVisible))
			{
				if (HUDMarkerManager::AddMarker(a_hudMarkerManager, a_markerData, a_pos, a_refHandle, a_markerGotoFrame)) 
				{
					CNO::HUDMarkerManager::GetSingleton()->ProcessLocationMarker(mapMarker, marker, a_markerGotoFrame);

					return true;
				}
			}
		}

		return false;
	}

	bool UpdateEnemies(const RE::HUDMarkerManager* a_hudMarkerManager, RE::HUDMarker::ScaleformData* a_markerData,
							RE::NiPoint3* a_pos, const RE::RefHandle& a_refHandle, std::uint32_t a_markerGotoFrame)
	{
		if (settings::display::showEnemyMarkers)
		{
			if (HUDMarkerManager::AddMarker(a_hudMarkerManager, a_markerData, a_pos, a_refHandle, a_markerGotoFrame))
			{
				RE::TESObjectREFR* marker = RE::TESObjectREFR::LookupByHandle(a_refHandle).get();

				// The handle can go stale between when the vanilla marker list was built and
				// this hook running. AddMarker already committed this marker to the scaleform
				// data above, so it still shows on the compass - we just skip the enemy-name
				// bookkeeping without a live reference.
				if (!marker)
				{
					logger::warn("Enemy marker's reference handle did not resolve to a live reference; skipping its bookkeeping this frame");

					return true;
				}

				CNO::HUDMarkerManager::GetSingleton()->ProcessEnemyMarker(marker->As<RE::Character>(), a_markerGotoFrame);

				return true;
			}
		}

		return false;
	}

	bool UpdatePlayerSetMarker(const RE::HUDMarkerManager* a_hudMarkerManager, RE::HUDMarker::ScaleformData* a_markerData,
								RE::NiPoint3* a_pos, const RE::RefHandle& a_refHandle, std::uint32_t a_markerGotoFrame)
	{
		if (HUDMarkerManager::AddMarker(a_hudMarkerManager, a_markerData, a_pos, a_refHandle, a_markerGotoFrame))
		{
			RE::TESObjectREFR* marker = RE::TESObjectREFR::LookupByHandle(a_refHandle).get();

			// The handle can go stale between when the vanilla marker list was built and this
			// hook running. AddMarker already committed this marker to the scaleform data
			// above, so it still shows on the compass - we just skip the bookkeeping below
			// without a live reference.
			if (!marker)
			{
				logger::warn("Player-set marker's reference handle did not resolve to a live reference; skipping its bookkeeping this frame");

				return true;
			}

			CNO::HUDMarkerManager::GetSingleton()->ProcessPlayerSetMarker(marker, a_markerGotoFrame);

			return true;
		}

		return false;
	}

	void UpdateCompass(RE::Compass* a_compass)
	{
		hooks::Compass::Update(a_compass);

		CNO::HUDMarkerManager::GetSingleton()->SetMarkersExtraInfo();
	}

	namespace compat
	{
		RE::GFxMovieDef* MapMarkerFramework::GetCompassMovieDef()
		{
			return compassMovieDef;
		}
	}
}
