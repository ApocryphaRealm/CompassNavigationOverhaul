#pragma once

#include "IUI/GFxArray.h"
#include "IUI/GFxDisplayObject.h"

#include "Settings.h"

struct QuestItem
{
	QuestItem() = default;

	QuestItem(RE::TESObjectREFR* a_markerRef, RE::QUEST_DATA::Type a_questType, const std::string& a_questName,
			  bool a_isInSameLocation, int a_questAgeIndex)
	: markerRef{ a_markerRef }, type{ a_questType }, name{ a_questName }, isInSameLocation{ a_isInSameLocation },
	  ageIndex{ a_questAgeIndex }
	{}

	RE::TESObjectREFR* markerRef;
	RE::QUEST_DATA::Type type;
	std::string name;
	bool isInSameLocation;
	std::vector<RE::BGSInstancedQuestObjective*> objectives;
	int ageIndex;
};

class QuestItemList : public IUI::GFxDisplayObject
{
public:
	static constexpr inline std::string_view path = "_level0.HUDMovieBaseInstance.QuestItemList";

	static void InitSingleton(const GFxDisplayObject& a_questItemList)
	{
		if (!singleton)
		{
			static QuestItemList singletonInstance{ a_questItemList };
			singleton = &singletonInstance;
		}
	}

	static QuestItemList* GetSingleton() { return singleton; }

	bool CanBeDisplayed(RE::TESObjectCELL* a_cell, bool a_isPlayerWeaponDrawn) const
	{
		if (!a_isPlayerWeaponDrawn || !settings::questlist::hideInCombat)
		{
			if (a_cell)
			{
				if ((a_cell->IsInteriorCell() && settings::questlist::showInInteriors) ||
					(a_cell->IsExteriorCell() && settings::questlist::showInExteriors))
				{
					return true;
				}
			}
		}

		return false;
	}

	void SetHiddenByForce(bool a_hiddenByForce) { hiddenByForce = a_hiddenByForce; }

	bool IsHiddenByForce() const { return hiddenByForce; }

	void AddToHudElements()
	{
		Invoke("AddToHudElements");
	}

	void AddQuest(const QuestItem& a_questItem)
	{
		GFxArray gfxQuestObjectives{ GetMovieView() };

		for (const RE::BGSInstancedQuestObjective* questObjective : a_questItem.objectives)
		{
			gfxQuestObjectives.PushBack(questObjective->GetDisplayTextWithReplacedTags().c_str());
		}

		Invoke("AddQuest", a_questItem.type, a_questItem.name.c_str(), a_questItem.isInSameLocation,
			   gfxQuestObjectives, a_questItem.ageIndex);
	}

	void SetQuestSide(const std::string& a_sideName)
	{
		Invoke("SetQuestSide", a_sideName.c_str());
	}

	void Update()
	{
		Invoke("Update");
	}

	void ShowQuest()
	{
		Invoke("ShowQuest");
	}

	void RemoveQuest()
	{
		Invoke("RemoveQuest");
	}

	// 1.1.7: moves the list to the current Position X/Y and Max height while the game runs (blckknight119, Nexus,
	// 2026-10-11: "the quest menu can be moved in-game with live updates"). Before this the three values reached the
	// movie only once, through the AS constructor below, so changing them on the settings page did nothing until the
	// next start.
	//
	// It runs the movie's OWN constructor again rather than a function of ours, because the QuestItemList.swf the game
	// loads is not always ours: Norden UI ships its own (restyled art, the same script), and with it installed ours is
	// overridden. Both constructors do the same thing - place the list at Stage size * position through
	// globalToLocal, and store positionY0 and maxHeight, which Update() clips the list against - so whichever SWF won,
	// the list lands exactly where a fresh start with these values would put it. Three things make a second call safe:
	//  - _x/_y are put back to where the clip was before the first call, because the constructor converts the target
	//    with the clip's own globalToLocal, which counts its current position;
	//  - the constructor starts a new, empty `entries` array, so the array holding the quests on screen is saved and
	//    put back - otherwise those quests would be orphaned on the HUD;
	//  - the HUD-mode flags it sets (All, Favor, StealthMode ...) are constants, the same values every time.
	// Main thread only: Scaleform is not thread-safe, and the settings page draws from the renderer's present hook.
	void ApplyPosition()
	{
		RE::GFxValue savedEntries = GetMember("entries");

		SetMember("_x", initialX);
		SetMember("_y", initialY);

		Invoke("QuestItemList", settings::questlist::positionX, settings::questlist::positionY, settings::questlist::maxHeight);

		if (savedEntries.IsArray())
		{
			SetMember("entries", savedEntries);
		}

		Update();
	}

	void ShowAllQuests()
	{
		Invoke("ShowAllQuests");
	}

	void RemoveAllQuests()
	{
		Invoke("RemoveAllQuests");
	}

private:

	QuestItemList(const GFxDisplayObject& a_questItemList) :
		GFxDisplayObject{ a_questItemList }
	{
		// Where the clip sits before the constructor places it - ApplyPosition() starts from here every time.
		const RE::GFxValue x = GetMember("_x");
		const RE::GFxValue y = GetMember("_y");

		initialX = x.IsNumber() ? x.GetNumber() : 0.0;
		initialY = y.IsNumber() ? y.GetNumber() : 0.0;

		Invoke("QuestItemList", settings::questlist::positionX, settings::questlist::positionY, settings::questlist::maxHeight);
	}

	double initialX = 0.0;
	double initialY = 0.0;

	static inline QuestItemList* singleton = nullptr;

	bool hiddenByForce = false;
};