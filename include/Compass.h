#pragma once

#include "IUI/GFxArray.h"
#include "IUI/GFxDisplayObject.h"

#include "utils/Geometry.h"

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

				Invoke("Compass");
			}
		}

		void SetUnits(bool a_useMetric)
		{
			Invoke("SetUnits", a_useMetric);
		}

		// The text the compass draws lives in one clip, FocusedMarkerInfo (Compass.as): the marker's name or objective
		// (Target) and its distance with the height arrow (Distance). Scaling that clip scales all of it about its own
		// origin, which the movie keeps on the focused marker, so the text stays centred under it. Returns false when
		// the clip is not there (another compass movie won the load order).
		bool SetTextScale(float a_scale)
		{
			RE::GFxValue info = GetMember("FocusedMarkerInfo");
			if (!info.IsDisplayObject())
			{
				return false;
			}
			const RE::GFxValue percent{ static_cast<double>(a_scale) * 100.0 };
			return info.SetMember("_xscale", percent) && info.SetMember("_yscale", percent);
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

		static inline Compass* singleton = nullptr;
	};
}
