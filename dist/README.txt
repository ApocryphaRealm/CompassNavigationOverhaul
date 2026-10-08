# CompassNavigationOverhaul

Version 1.1.7

https://www.nexusmods.com/skyrimspecialedition/mods/74484

WHAT CHANGED
------------

Version 1.1.7
Fixed location markers disappearing from the compass while many quests are active (1.1.6 let quest markers take all 48 compass slots). The quest marker limit now defaults to 32, which always leaves room for location markers; uQuestMarkerLimit in the INI still goes up to 48.

Version 1.1.6
Fixed quest markers losing their hover details and quest list when Quest Marker Limit Fix is installed.
Added a built-in quest marker limit of up to 48 (uQuestMarkerLimit in the INI, default 48, 16 is the game's own), so Quest Marker Limit Fix is no longer needed and should be removed.
Added a log warning when Quest Marker Limit Fix is still installed; on Skyrim 1.7 that mod now declines to load beside this one and SKSE shows a box naming it.
Added the Skyrim 1.7 build line; the installer asks which game version you play.
Updated the log to ship at level info.
Version 1.1.5
Added a check, before any address is resolved, that names in the log the game version detected, the Address Library file it needs, the folder it looks in and whether the file is there; when it is missing a message says exactly that and the plugin loads inert, instead of the bare "failed to open the address library file".

Version 1.1.2
The settings page is shown in the game's language: Japanese, Korean, Chinese, Russian, German, French, Spanish, Italian, Polish and Czech translation files ship beside the DLL, and the page follows the Apocrypha Menu Framework's Language setting; English is the fallback.
Version 1.1.1
Fixed the settings menu never registering under Apocrypha Menu Framework, so the page was silently absent.
Fixed settings reverting to their values from game start on reload, and being overwritten between sessions, when PrivateProfileRedirector is installed.
Debug symbols now ship inside the main download so Crash Logger can resolve this mod's crash frames.
Version 1.1.0
On/off settings now render as a toggle switch, green when on and red when off, instead of a checkbox.
Sliders can now be nudged with the arrow keys as well as dragged - click a slider to select it first.
Version 1.0.6
Logging now defaults to its most detailed level, so a first bug report is useful without reproducing the problem again.
Version 1.0.5
The NPC Name Distributor integration is now retried later in startup, so marker names no longer silently fall back to plain names for the whole session if NND had not finished loading yet.
Fixed the log claiming the CoMAP compatibility patch had loaded when it had actually just failed to apply.
Version 1.0.4
Fixed settings never actually being saved - pressing Save reported success but wrote nothing to disk. Set your preferences again after updating if you were on an earlier version.
Fixed four settings whose plugin defaults did not match the shipped INI, so Restore defaults now produces the state a fresh install actually has.
The DLL now carries a proper version resource.
Version 1.0.3
Fixed the real cause of a crash in the CoMAP compatibility patch.
Version 1.0.2
Fixed a crash caused by an undersized trampoline where a hooked function was patched in two places.
Version 1.0.1
Safety audit across the plugin. No known crash was found; this was preventative.
Version 1.0.0
First release. Adds an in-game settings page through SKSE Menu Framework, so the compass and quest list can be configured while playing instead of only by editing the INI before launching.
