# CompassNavigationOverhaul - changelog

Rule 61: this mod's own history, kept beside the code it describes.

> **The entries below this line were RECONSTRUCTED from `version-ledger.json` on
> 2026-08-27, not written at the time of the change.** They carry only what the ledger
> recorded - the status and the evidence - so they are thinner than a real entry and may
> be missing changes the ledger never captured. Treat them as a starting point rather
> than a record. Everything from the next version onward is written as it happens.

Each version carries its **version-ledger status**: **working** (observed in game),
**untested** (built, not confirmed), **failed** (built but broken; the number was
reclaimed), **scratch** (a hypothesis-test build that never held a real number).

<!-- VERSIONING-RULES -->
> **Versioning rules (CLAUDE.md rules 6 and 48 - identical for mods and documents):**
> * `X.Y.Z`. A change increments the THIRD number. At `.9` the MINOR rolls: `1.0.9 -> 1.1.0`;
>   `1.0.10` never exists.
> * The next number is **LAST WORKING + 1**. A failed, scratch or untested test build does NOT
>   consume its number - the next attempt at the same step REUSES it.
> * Numbers are never typed. ONE tool, role `version-gate` (`resolve.py run version-gate`), holds every version rule
>   and is a GATE that fails: `bump` issues the next number and writes every location, `record`
>   proves it working (evidence + the binary's hash), `gate` refuses packaging or finalizing
>   anything it did not issue. Documents go through `docs-pipeline.ps1 -Action bump` and the rules
>   through `rules-version.ps1 -Action bump`, both of which take their arithmetic from that same
>   tool. A number typed by hand is wrong until the tool agrees.

## 1.1.7 - 2026-10-08 - untested

### Fixed
- Vibrant Map Markers' coloured compass icons are kept (ladydias on Nexus, 2026-10-15: CoMAP, CoMAP Addons, Vibrant Map Markers and its Dear Diary Dark patch; with only this mod some compass icons had colour and others did not, with the original CNO kept and ours overriding it every icon had colour). Vibrant Map Markers ships its own coloured copy of CNO's compass movie at Interface\InfinityUI\HUDMenu\HUDMovieBaseInstance\CompassShoutMeterHolder\Compass.swf (its main file and its Dear Diary Dark Mode / Untarnished UI / Dragonbreaker UI compass files - Nexus 83529 content previews). This package ships the original CNO 2.2.0 Compass.swf at the same path, byte-identical (sha256 8a2f661c...). Installed in place of the original CNO, this mod lands at the bottom of Mod Organizer 2's list, so its uncoloured copy beat VMM's and the location icons drawn by that movie lost their colour, while CoMAP's own icons (VMM's comapresources.swf) kept theirs; merged into the original's slot, above VMM, VMM's copy still won. The installer now asks which compass art to install: this mod's (default) or none, for anyone whose compass mod supplies CNO's Compass.swf. Nothing else changed: the DLLs are the same build. (Added 2026-10-16; not yet tested in game.)
- Location markers no longer vanish while many quests are active (makihaze on Nexus, 2026-10-08: 'limit in 16, The map markers appear again'). The compass's 48 slots are shared and quests are added first; 1.1.6's default quest cap of 48 let quest markers take every slot. uQuestMarkerLimit now defaults to 32, which always leaves 16 for location, enemy and player-set markers (the INI still allows 16-48).

- **The game starts 6 seconds sooner.** `SKSEPluginLoad` slept for 6 seconds, unconditionally, before doing anything (`std::this_thread::sleep_for(6s)`, in from upstream's 2.2.0 update - most likely a wait for a debugger to attach, left in a release build). It is removed. Nothing after it depended on time: SKSE loads plugins one at a time on that thread, the game code patched at load is already unpacked, and the HUD work starts from SKSE and Infinity UI messages sent later. (Added 2026-10-13.)
- **The quest list's Position X, Position Y and Max height now move the list while you change them** on the settings page, instead of only after the next start (they reached the HUD movie once, through its constructor, and never again). The move runs the loaded QuestItemList movie's own constructor again - with the clip put back where it started and its list of on-screen quests kept - so it works the same whichever QuestItemList.swf wins: ours, or Norden UI's restyled one with the same script, and lands exactly where a fresh start with those values would. Press Save to keep the position. (Added 2026-10-13.)
- **Which INI copy is used, with Norden UI:** Norden UI ships its own tuned `CompassNavigationOverhaul.ini` (fPositionX 0.775 against our 0.008), which wins the load order in Mod Organizer 2. The plugin reads and saves the INI through the same game-folder path, which the virtual file system resolves to that winning copy both ways, so a saved position is the one read at the next start. The log now names the quest list position read at start, so a log shows which copy it came from. (Added 2026-10-13.)

### Changed
- Every slider on the settings page is a precise slider (rule 68): a keyboard or controller nudge moves one unit of the digit it shows (Position X/Y 0.001, delays 0.01, angles 1) - the arrow-key nudge had moved 0.01 on the three-decimal position sliders. A frame where the slider already moved the value adds no second nudge. (Added 2026-10-13.)
- The settings page has a (?) note under the quest list position sliders, in all eleven languages: the list moves as you change them, and Save keeps the position. (Added 2026-10-13.)

### Added
- DevBench status: markers.lastFrame counts the last frame's quest and location AddMarker offers, accepted and declined, and framesWithLocationsDeclined - so a test shows directly whether quest markers are still crowding out locations. The second half of makihaze's report (the tracked quest's far destination not shown) is NOT addressed yet: it needs the caller of UpdateQuests mapped first; these counters are for that.
- DevBench status: infinityUi.questListMoves counts live quest list moves, with questListMoveScreen (where the list's origin landed on screen after the last one) and lastQuestListMoveSecondsAgo. (Added 2026-10-13.)

### Credit
- Illustar0 (Nexus, 2026-10-10) found the 6-second start-up sleep and asked why it was there.
- blckknight119 (Nexus, 2026-10-11) suggested moving the quest list in game with live updates.

## 1.1.6 - 2026-09-26 - working

### Fixed
- **Quest markers keep their hover details and quest list with Quest Marker Limit Fix installed** (Nexus 189762, AE 1.6.1170 only). That mod swallows every quest AddMarker call and replays the markers it picks straight into the original function, which our call-site hook never sees, so no quest marker ever got its objective, distance or quest list. A declined quest target is now kept with a copy of its position, and at the end of the frame it is matched to the compass slot something else committed it to, then processed as usual. Without such a mod a declined marker matches nothing and is dropped, so nothing changes. Slots this mod committed itself are never matched; a skipped compass frame discards its pending markers.
- **The quest marker cap is raised inside this mod, so Quest Marker Limit Fix is no longer needed.** Its replay passes a pointer into a stack frame that has already returned, so in an interior entered from another interior and after a quest completes the replayed positions stop matching and hover details go missing again. The game's own cap (`cmp [reg+0x14BC], 16` in UpdateQuests) is found by scanning the function rather than by a fixed address and set to `uQuestMarkerLimit` (`[Display]`, 16 to 48, default 48, read at start). A cap already at the target is left alone, an unexpected value is never overwritten, and every outcome is logged and shown by the status tool. With both installed the log warns to remove Quest Marker Limit Fix.

### Changed
- Ships at log level info (`uLogLevel=2`, compiled default to match), the standing default since 2026-09-26. The shipped INI now lives in the repo (`dist\SKSE\Plugins`).
- The DevBench status tool reports `questMarkerLimit` (applied, offset, limit, why) and `questMarkerLimitFix` (detected, reconciled and unmatched counts).

### Known
- On Skyrim 1.7.x, Quest Marker Limit Fix's own check requires the cap to still read 16, so with this mod raising it first that plugin refuses to load and SKSE shows its "incompatible plugin" box naming QuestMarkerLimitFix.dll. That is the intended end state - remove it. On AE 1.6.1170 it still loads and this mod's log warns instead. Setting `uQuestMarkerLimit=16` leaves the game's byte untouched for anyone who wants to keep it.
- This release is the first to ship the Skyrim 1.7 build line (Line-17 in the installer), tested on 1.7.104. The DevBench status tool does not register on that line (the same for every mod of ours on 1.7; DevBench's messaging there is not answered).

### Verified
- AE 1.6.1170 with Quest Marker Limit Fix: 6,420 replayed quest markers matched to their slots, none unmatched; hover text and quest list on screen. Without it: 27 quest markers per frame (1.1.5 on the same save: 16), hover in Dragonsreach and in the Jarl's quarters. SE 1.5.97: 25 per frame, hover on several markers. 1.7.104: cap raised, hover and quest list on screen.

### Credit
- Both fixes are ported from hidden-kc's fork of this mod (github.com/hidden-kc/CompassNavigationOverhaul, their 1.1.2 and 1.1.4, verified in game there on AE 1.6.1170), which is GPL-3.0-or-later like this one. Their distance filter (their 1.1.3) is a separate feature and was not taken.

## 1.1.5 - 2026-09-18 - untested

### Fixed
- **The Address Library guard now runs before SKSE::Init.** CommonLibSSE-NG's Init opens the Address Library itself, so the guard added for a missing file sat after the very call that fails on it and never ran; oproso's log (Perfected Wheeler 1.3.2, 2026-09-18) showed the banner, then CommonLib's bare 'failed to open address library file', and no [AddressLibrary] line. The check is now the first thing after the logger, so a missing file is named - game version, file, folder - and the plugin loads inert.

## 1.1.4 - 2026-09-17 - working

### Added
- **The Address Library check speaks before CommonLib can fail** (oproso on the Nexus page, 16-17 Sep 2026, Fluorine on SteamOS: *"failed to open the address library file"* from this patch and from Wheeler). That line is CommonLibSSE-NG's, raised the first time an address is resolved, and it names neither the file it wanted, the folder it looked in nor the game version it decided on. Before this plugin resolves anything it now writes all of that to its log - runtime and edition, the executable it read them from, the exact file (`versionlib-<v>.bin` for AE, `version-<v>.bin` for SE), the working directory the path is relative to, whether the file is there and whether it is beside the executable instead - and when the file is missing shows a message with the same facts and loads inert so the game continues. `include/AddressLibraryGuard.h` is the shared guard from Wheeler - Refined 1.2.9; `APOCRYPHA_SIMULATE_MISSING_ADDRESS_LIBRARY=1` in the environment drives the failure path for a proof.

## 1.1.3 - 2026-09-16 - untested

### Changed
- Relicensed to GPL-3.0-or-later. The mod links CommonLibSSE-NG and builds against the SKSE Menu Framework header, both GPL-3.0, so the MIT licence it shipped with was never available to it. alexsylex's original MIT notice is preserved in THIRD_PARTY_NOTICES.md, as his licence requires. No code changed.

## 1.1.2 - 2026-09-07 - working

### Added
- The settings page is shown in the game's language: Japanese, Korean, Chinese, Russian, German, French, Spanish, Italian, Polish and Czech translation files ship beside the DLL (Interface/Translations/CompassNavigationOverhaul_<language>.txt) and the page follows the Apocrypha Menu Framework's Language setting; English is the fallback. The framework is looked up by its sort-first name first; compassnavigationoverhaul.status gained op=strings.

## 1.1.1 - 2026-09-01 - working

### Fixed
- The settings menu registers with Apocrypha Menu Framework (AMF) by its real module name,
  with stock SKSE Menu Framework as the fallback - on an AMF stack the page previously never
  registered and the menu was silently absent. Same fix as Dragon's Eye Minimap 1.5.8.
- The .pdb debug symbols now ship inside the main download so Crash Logger can resolve this
  mod's stack frames. There is no separate Debug Symbols download.
- Settings saved in game were lost on reload: Save() wrote the INI with plain file I/O, but Init() and Reload() read it back through INISettingCollection::ReadFromFile, which uses the Win32 profile API that PrivateProfileRedirector hooks and caches - so a reload was served the values from game start, and the Redirector could later flush its stale cache back over the file. Settings are now parsed straight from the INI with plain file I/O and preferred over the collection, which is left holding only the compiled-in defaults; the plugin never hands its INI to the profile API in either direction, so it behaves identically with or without the Redirector installed. Same fix as Dragon's Eye Minimap 1.5.7.

## 1.1.0 - 2026-08-27 - working

### Changed
- published on Nexus 189628 (file_id 795461, MAIN, 2026-08-26); git tag v1.1.0 - LOCKED. Note the tag sits on the commit whose CMakeLists still reads 1.0.10

### Known
- Ships a dedicated CoMAP compatibility patch - hooks::compat::MapMarkerFramework in source/MessageListeners.cpp and include/Hooks.h - gated on CoMAP's own SKSE plugin version. This is why the 2026-08-27 compatibility pass recorded CoMAP as compatible: the integration is deliberate, not incidental. Also carries the rule 6 retroactive renumbering (a v1.0.10 was published before the pre-push hook existed to reject a patch component of 10; sorting 1.0.10 against 1.0.9 as strings puts them in the wrong order).
- LATENT BUG, verified by source inspection 2026-08-27, not yet fixed here: this mod saves its INI with plain file I/O but reads it back through INISettingCollection::ReadFromFile, which uses the Win32 profile API that PrivateProfileRedirector hooks and caches. Under the Redirector a reload is served the values from game start rather than the ones just written - settings appear to save then revert. Worse, once the plugin's INI has been read through that API the Redirector caches it and can write its stale copy back over the file on game-save or exit, losing settings between sessions. Dragon's Eye Minimap 1.5.7 fixed exactly this: prefer values parsed directly from the file in the shared Read<T> helper, and stop calling ReadFromFile so the Redirector never caches our INI at all. CustomDifficultyUI-SMF was fixed earlier and is the precedent.

## 1.0.9 - 2026-08-27 - working

### Changed
- PROGRESS.md in-game test 2026-08-26 - swallow confirmed firing on all four directions, toggle colours confirmed by the author

## 1.0.8 - 2026-08-27 - untested

### Changed
- local package only - no tag; superseded the same development cycle

## 1.0.7 - 2026-08-27 - untested

### Changed
- local package only - no tag; the arrow-key swallow, reverted before 1.1.0

## 1.0.6 - 2026-08-27 - working

### Changed
- published on Nexus 189628 (file_id 795107/795110, OLD_VERSION, 2026-08-25); git tag v1.0.6 - LOCKED, do not renumber

## 1.0.5 - 2026-08-27 - working

### Changed
- git tag v1.0.5 pushed

## 1.0.4 - 2026-08-27 - working

### Changed
- git tag v1.0.4 pushed

## 1.0.3 - 2026-08-27 - working

### Changed
- git tag v1.0.3 pushed; real cause fixed (CoMAP compat patch unchecked failure paths)

## 1.0.2 - 2026-08-27 - failed

### Known
- Ships a dedicated CoMAP compatibility patch - hooks::compat::MapMarkerFramework in source/MessageListeners.cpp and include/Hooks.h - gated on CoMAP's own SKSE plugin version. This is why the 2026-08-27 compatibility pass recorded CoMAP as compatible: the integration is deliberate, not incidental. Also carries the rule 6 retroactive renumbering (a v1.0.10 was published before the pre-push hook existed to reject a patch component of 10; sorting 1.0.10 against 1.0.9 as strings puts them in the wrong order).
- same trampoline crash recurred unchanged - the 1.0.2 fix was a misdiagnosis

## 1.0.1 - 2026-08-27 - failed

### Known
- Ships a dedicated CoMAP compatibility patch - hooks::compat::MapMarkerFramework in source/MessageListeners.cpp and include/Hooks.h - gated on CoMAP's own SKSE plugin version. This is why the 2026-08-27 compatibility pass recorded CoMAP as compatible: the integration is deliberate, not incidental. Also carries the rule 6 retroactive renumbering (a v1.0.10 was published before the pre-push hook existed to reject a patch component of 10; sorting 1.0.10 against 1.0.9 as strings puts them in the wrong order).
- failed to load in game - SKSE/Trampoline.h(287) displacement is out of range, confirmed by a real error dialog

## 1.0.0 - 2026-08-27 - working

### Changed
- git tag v1.0.0 pushed 2026-08-24

