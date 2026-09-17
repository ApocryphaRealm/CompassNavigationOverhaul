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
> * Numbers are never typed. ONE tool, `.MD\scripts\version-gate.ps1`, holds every version rule
>   and is a GATE that fails: `bump` issues the next number and writes every location, `record`
>   proves it working (evidence + the binary's hash), `gate` refuses packaging or finalizing
>   anything it did not issue. Documents go through `docs-pipeline.ps1 -Action bump` and the rules
>   through `rules-version.ps1 -Action bump`, both of which take their arithmetic from that same
>   tool. A number typed by hand is wrong until the tool agrees.

## 1.1.4 - 2026-09-17 - untested

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

