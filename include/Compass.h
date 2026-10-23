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

		static inline Compass* singleton = nullptr;
	};
}
