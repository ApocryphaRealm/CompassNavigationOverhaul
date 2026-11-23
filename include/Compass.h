#pragma once

#include "IUI/GFxArray.h"
#include "IUI/GFxDisplayObject.h"

#include "utils/Geometry.h"
#include "utils/Logger.h"

namespace CNO
{
	class Compass : public IUI::GFxDisplayObject
	{
	public:
		struct Marker
		{
			Marker(RE::TESObjectREFR* a_markerRef, float a_angleToPlayerCamera,
				   std::uint32_t a_index, std::uint32_t a_icon, const std::string_view& a_description) :
				ref{ a_markerRef }, angleToPlayerCamera{ a_angleToPlayerCamera },
				index{ a_index }, icon{ a_icon }, description{ a_description }
			{}

			RE::TESObjectREFR* ref;
			float angleToPlayerCamera;
			float distanceToPlayer = util::GetDistanceBetween(RE::PlayerCharacter::GetSingleton(), ref);
			float heightDifference = util::GetHeightDifferenceBetween(RE::PlayerCharacter::GetSingleton(), ref);
			std::uint32_t index;
			std::uint32_t icon;
			std::string description;
		};

		static constexpr inline std::string_view path = "_level0.HUDMovieBaseInstance.CompassShoutMeterHolder.Compass";

		static void InitSingleton(const GFxDisplayObject& a_originalCompass)
		{
			if (!singleton)
			{
				static Compass singletonInstance{ a_originalCompass };
				singleton = &singletonInstance;
			}
		}

		static Compass* GetSingleton() { return singleton; }

		void SetupMod(const GFxDisplayObject& a_replaceCompass)
		{
			if (a_replaceCompass.HasMember("Compass"))
			{
				*static_cast<GFxDisplayObject*>(this) = a_replaceCompass;
				textBaseY.reset();

				Invoke("Compass");

				WireSetCompassMarkers();
			}
		}

		// True when HUDMenu.SetCompassMarkers is this movie's own SetMarkers, so a marker whose icon is 0 is drawn as
		// the movie's "Compass Marker Unknown" ("?"). With the vanilla SetCompassMarkers an icon of 0 lands on frame 1,
		// the quest arrow - the small "v" seen in CNO 1.2.2.
		bool IsUnknownMarkerWired() const { return unknownMarkerWired; }

		// 1.2.3. Upstream's newer swf\Compass.as assigns HUDMenu.SetCompassMarkers = SetMarkers in its constructor
		// (commit 6f6cb8e, which also removed the DLL's call-site hook), but the shipped Compass.swf - ours, and Norden
		// UI's, read with FFDec 2026-11-22 - defines SetMarkers and never assigns it. The assignment is made here, the
		// same one upstream's AS makes: no game address, so SE, AE and 1.7 take the same path. A third-party movie
		// with no SetMarkers keeps the vanilla function, and the unknown-marker setting then leaves the game's own
		// undiscovered icon rather than drawing the quest arrow.
		void WireSetCompassMarkers()
		{
			unknownMarkerWired = false;

			RE::GFxValue setMarkers = GetMember("SetMarkers");
			RE::GFxMovieView* view = GetMovieView();

			if (!setMarkers.IsObject() || !view)
			{
				logger::warn("SetCompassMarkers override NOT wired: the compass movie {} has no SetMarkers function; "
							 "undiscovered locations keep the game's own icon when \"Undiscovered means unknown marker\" is on",
							 ToString().c_str());
				return;
			}

			RE::GFxValue hudMenu;
			if (!view->GetVariable(&hudMenu, "_level0.HUDMovieBaseInstance") || !hudMenu.IsDisplayObject())
			{
				logger::warn("SetCompassMarkers override NOT wired: _level0.HUDMovieBaseInstance was not found");
				return;
			}

			unknownMarkerWired = hudMenu.SetMember("SetCompassMarkers", setMarkers);

			if (unknownMarkerWired)
			{
				logger::info("SetCompassMarkers override wired: HUDMenu.SetCompassMarkers is now {}.SetMarkers "
							 "(unknown marker drawn by the compass movie)", ToString().c_str());
			}
			else
			{
				logger::warn("SetCompassMarkers override NOT wired: setting HUDMenu.SetCompassMarkers failed");
			}
		}

		void SetUnits(bool a_useMetric)
		{
			Invoke("SetUnits", a_useMetric);
		}

	// The text the compass draws lives in one clip: the marker's name or objective (Target) and its distance with the
		// height arrow (Distance). The shipped movies (ours and Norden UI's, read from the SWFs 2026-10-23) name it
		// FocusedMarkerInstance; swf\Compass.as names it FocusedMarkerInfo - both are tried, the shipped name first.
		// Returns the name of the clip it scaled, or nullptr when the compass movie in the load order has neither.
		//
		// Placement (combined run 2026-10-23: at 2.00 the name went off the bottom of the screen). The clip's origin is
		// on the compass's centre line, the distance sits above it and the name below it (the fade-in slides Target to
		// rest at y 6.5, its text field 12.5 lower), so scaling about the origin pushes the name down by its offset as
		// well as its own height. Only the clip's own transform is ours - the fade tween rewrites Target and Distance
		// every frame - so the clip is moved up by (scale - 1) x the name's top edge, which keeps that edge where it was
		// at 1.00, and then up again by whatever of the name would still fall below the visible frame. Horizontally the
		// text is centred on the origin, which the movie keeps on the focused marker, so it stays centred.
		const char* SetTextScale(float a_scale)
		{
			constexpr const char* kClips[] = { "FocusedMarkerInstance", "FocusedMarkerInfo" };
			for (const char* name : kClips)
			{
				RE::GFxValue clip = GetMember(name);
				if (!clip.IsDisplayObject())
				{
					continue;
				}

				RE::GFxValue y;
				if (!textBaseY)
				{
					clip.GetMember("_y", &y);
					textBaseY = y.IsNumber() ? y.GetNumber() : 0.0;   // the authored place, before any shift of ours
				}

				// the name's text field inside Target (12.5 / 21.7 in our movie when it cannot be read)
				double fieldY = 12.5;
				double fieldH = 21.7;
				RE::GFxValue target, field, v;
				if (clip.GetMember("Target", &target) && target.IsDisplayObject() &&
					target.GetMember("TextFieldInstance", &field) && field.IsDisplayObject())
				{
					if (field.GetMember("_y", &v) && v.IsNumber()) { fieldY = v.GetNumber(); }
					if (field.GetMember("_height", &v) && v.IsNumber() && v.GetNumber() > 0.0) { fieldH = v.GetNumber(); }
				}
				constexpr double kTargetRestY = 6.5;
				const double nameTop = kTargetRestY + fieldY;
				const double nameBottom = nameTop + fieldH;
				const double scale = static_cast<double>(a_scale);

				double shift = -(scale - 1.0) * nameTop;
				const double lift = OverflowBelowFrame(*textBaseY + shift + scale * nameBottom);
				shift -= lift;

				const RE::GFxValue percent{ scale * 100.0 };
				const RE::GFxValue newY{ *textBaseY + shift };
				if (clip.SetMember("_xscale", percent) && clip.SetMember("_yscale", percent) && clip.SetMember("_y", newY))
				{
					logger::debug("Compass text size {:.2f} on {}: y {:.1f} -> {:.1f} (name {:.1f}..{:.1f}, lifted {:.1f} to stay on screen)",
						a_scale, name, *textBaseY, *textBaseY + shift, nameTop, nameBottom, lift);
					return name;
				}
			}
			return nullptr;
		}

		void SetMarkers()
		{
			Invoke("SetMarkers");
		}

		void SetFocusedMarkerInfo(const std::string_view& a_targetText, float a_distance,
								  float a_heightDifference, std::uint32_t a_markerIndex)
		{
			Invoke("SetFocusedMarkerInfo", a_targetText.data(), a_distance, a_heightDifference,
										   a_markerIndex);
		}

		// The shipped Compass.swf (upstream 2.2.0) reads the marker index from FocusMarker's and
		// UpdateFocusedMarker's argument; the newer source ignores it (AS2 drops extra arguments), so
		// passing it is right for either movie. Without it the focused marker's name and distance stay
		// at the compass centre while the icon sits aside.
		void FocusMarker(std::uint32_t a_focusedMarkerIndex)
		{
			Invoke("FocusMarker", a_focusedMarkerIndex);
		}

		void UnfocusMarker()
		{
			Invoke("UnfocusMarker");
		}

		// 1.2.4 (bug 1179030). With SetMarkers wired (1.2.3), the shipped movies remove and re-attach EVERY marker clip on
		// each SetCompassMarkers call, while their FocusMarker stores the focused clip only when the focus changes and
		// their UpdateFocusedMarker keeps placing the name on that stored clip - a clip that has since been removed, or
		// whose name now belongs to another slot's marker once markers enter or leave range. The HUD's own function,
		// used until 1.2.2, kept the clips, so the stored one stayed valid. The stored clip and index are pointed at
		// this frame's clip for the focused slot before UpdateFocusedMarker runs (the same thing the Vibrant Map
		// Markers movie's UpdateFocusedMarker does itself). Returns false when the clip or the slot is not there.
		bool RepointFocusedMarker(std::uint32_t a_focusedMarkerIndex)
		{
			RE::GFxMovieView* view = GetMovieView();
			if (!view)
			{
				return false;
			}

			RE::GFxValue markerList;
			if (!view->GetVariable(&markerList, "_level0.HUDMovieBaseInstance.CompassMarkerList") || !markerList.IsArray() ||
				a_focusedMarkerIndex >= markerList.GetArraySize())
			{
				return false;
			}

			RE::GFxValue entry;
			RE::GFxValue movie;
			if (!markerList.GetElement(a_focusedMarkerIndex, &entry) || !entry.IsObject() ||
				!entry.GetMember("movie", &movie) || !movie.IsDisplayObject())
			{
				return false;
			}

			constexpr const char* kClips[] = { "FocusedMarkerInstance", "FocusedMarkerInfo" };
			for (const char* name : kClips)
			{
				RE::GFxValue clip = GetMember(name);
				if (clip.IsDisplayObject())
				{
					const RE::GFxValue index{ static_cast<double>(a_focusedMarkerIndex) };
					return clip.SetMember("Movie", movie) && clip.SetMember("Index", index);
				}
			}

			return false;
		}

		// 1.2.4 (bug 1179030 follow-up: icon and name size flicker while moving with Show undiscovered on). Norden UI's
		// SetMarkers enlarges a freshly attached clip only when it is the SAME object as FocusedMarkerInstance.Movie, a
		// reference to a clip the previous call removed. Whenever the focused marker's slot moves (a marker before it
		// enters or leaves the list - frequent with undiscovered markers in range) the enlargement lands on another
		// slot's marker or on none, so the focused icon flips between its normal and enlarged size, and tonight's run
		// showed it staying small after the set changed. The size is set here from this frame's data instead: every
		// marker at the movie's own size (CompassTargetDataA scale x SkyHUD's scl_fCompassMarker, capped at 135), the
		// focused slot at Norden's enlarged size (x 1.325, capped at 150). Our own movie's UpdateFocusedMarker, run
		// after this, re-applies its 145 cap to the focused one. Returns false when the arrays are not there.
		bool ApplyMarkerScales(std::uint32_t a_focusedMarkerIndex)
		{
			RE::GFxMovieView* view = GetMovieView();
			if (!view)
			{
				return false;
			}

			RE::GFxValue markerList;
			RE::GFxValue markersData;
			if (!view->GetVariable(&markerList, "_level0.HUDMovieBaseInstance.CompassMarkerList") || !markerList.IsArray() ||
				!view->GetVariable(&markersData, "_level0.HUDMovieBaseInstance.CompassTargetDataA") || !markersData.IsArray())
			{
				return false;
			}

			double skyHUDScaleMult = 1.0;
			RE::GFxValue skyHUDScale;
			if (view->GetVariable(&skyHUDScale, "_level0.HUDMovieBaseInstance.scl_fCompassMarker") && skyHUDScale.IsNumber() &&
				!std::isnan(skyHUDScale.GetNumber()))
			{
				skyHUDScaleMult = skyHUDScale.GetNumber();
			}

			constexpr std::uint32_t kStride = 4;
			constexpr std::uint32_t kScale = 3;

			const std::uint32_t markersCount = markerList.GetArraySize();
			const std::uint32_t dataSize = markersData.GetArraySize();

			for (std::uint32_t i = 0; i < markersCount && i * kStride + kScale < dataSize; i++)
			{
				RE::GFxValue entry;
				RE::GFxValue movie;
				RE::GFxValue scaleValue;
				if (!markerList.GetElement(i, &entry) || !entry.IsObject() || !entry.GetMember("movie", &movie) ||
					!movie.IsDisplayObject() || !markersData.GetElement(i * kStride + kScale, &scaleValue) ||
					!scaleValue.IsNumber())
				{
					continue;
				}

				const double scale = scaleValue.GetNumber() * skyHUDScaleMult;
				const RE::GFxValue percent{ i == a_focusedMarkerIndex ? (std::min)(150.0, scale * 1.325) :
																	   (std::min)(135.0, scale) };

				movie.SetMember("_xscale", percent);
				movie.SetMember("_yscale", percent);
			}

			return true;
		}

		void UpdateFocusedMarker(std::uint32_t a_focusedMarkerIndex)
		{
			Invoke("UpdateFocusedMarker", a_focusedMarkerIndex);
		}

		void PostProcessMarkers(const std::unordered_map<std::uint32_t, bool>& a_unknownLocations, std::uint32_t a_markersCount)
		{
			GFxArray gfxIsUnknownLocations{ GetMovieView() };

			for (std::uint32_t i = 0; i < a_markersCount; i++)
			{
				gfxIsUnknownLocations.PushBack(a_unknownLocations.contains(i));
			}

			Invoke("PostProcessMarkers", gfxIsUnknownLocations);
		}

	private:
		Compass(const GFxDisplayObject& a_originalCompass) : GFxDisplayObject{ a_originalCompass }
		{}

		// How far below the visible frame a point at a_localY of this compass clip falls, in this clip's units (0 when
		// it is on screen). Measured through localToGlobal, so the compass's own place and size on screen count.
		double OverflowBelowFrame(double a_localY)
		{
			auto* view = GetMovieView();
			if (!view)
			{
				return 0.0;
			}
			const auto toGlobal = [&](double a_y) {
				GFxObject pt(view);
				pt.SetMember("x", RE::GFxValue{ 0.0 });
				pt.SetMember("y", RE::GFxValue{ a_y });
				Invoke("localToGlobal", pt);
				const RE::GFxValue gy = pt.GetMember("y");
				return gy.IsNumber() ? gy.GetNumber() : a_y;
			};
			const double g0 = toGlobal(0.0);
			const double unit = toGlobal(100.0) - g0;   // global units per 100 of ours
			if (unit <= 0.0)
			{
				return 0.0;
			}
			constexpr double kMargin = 4.0;   // global units kept clear above the bottom edge
			const double bottom = static_cast<double>(view->GetVisibleFrameRect().bottom) - kMargin;
			const double over = toGlobal(a_localY) - bottom;
			return over > 0.0 ? over * 100.0 / unit : 0.0;
		}

		std::optional<double> textBaseY;   // FocusedMarkerInstance's authored _y in the current movie
		bool unknownMarkerWired = false;   // see WireSetCompassMarkers

		static inline Compass* singleton = nullptr;
	};
}
