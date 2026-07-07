//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////
// Remere's Map Editor is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// Remere's Map Editor is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program. If not, see <http://www.gnu.org/licenses/>.
//////////////////////////////////////////////////////////////////////

#include "main.h"
#include "artprovider.h"

#include <wx/mstream.h>

#include "pngfiles.h"

namespace {
wxBitmap LoadBitmapFromMemory(const unsigned char* data, int length) {
	wxMemoryInputStream input(data, length);
	wxImage image(input, "image/png");
	if (!image.IsOk()) {
		return wxNullBitmap;
	}
	return wxBitmap(image, -1);
}

wxBitmap GetCachedPngBitmap(const wxArtID& id, const unsigned char* data, int length) {
	static std::map<wxString, wxBitmap> cache;
	auto it = cache.find(id);
	if (it != cache.end()) {
		return it->second;
	}

	wxBitmap bitmap = LoadBitmapFromMemory(data, length);
	if (bitmap.IsOk()) {
		cache.emplace(id, bitmap);
	}
	return bitmap;
}

wxBitmap GetCachedFileBitmap(const wxArtID& id, const std::initializer_list<const char*>& paths) {
	static std::map<wxString, wxBitmap> cache;
	auto it = cache.find(id);
	if (it != cache.end()) {
		return it->second;
	}

	for (const char* path : paths) {
		const wxString file(path);
		if (!wxFileExists(file)) {
			continue;
		}

		wxImage image;
		if (!image.LoadFile(file, wxBITMAP_TYPE_PNG)) {
			continue;
		}

		wxBitmap bitmap(image, -1);
		if (bitmap.IsOk()) {
			cache.emplace(id, bitmap);
			return bitmap;
		}
	}

	return wxNullBitmap;
}
}

wxBitmap ArtProvider::CreateBitmap(const wxArtID& id, const wxArtClient& client, const wxSize& WXUNUSED(size)) {
	if (client != wxART_TOOLBAR) {
		return wxNullBitmap;
	}

	if (id == ART_CIRCULAR_1) {
		return GetCachedPngBitmap(id, circular_1_png, sizeof(circular_1_png));
	} else if (id == ART_CIRCULAR_2) {
		return GetCachedPngBitmap(id, circular_2_png, sizeof(circular_2_png));
	} else if (id == ART_CIRCULAR_3) {
		return GetCachedPngBitmap(id, circular_3_png, sizeof(circular_3_png));
	} else if (id == ART_CIRCULAR_4) {
		return GetCachedPngBitmap(id, circular_4_png, sizeof(circular_4_png));
	} else if (id == ART_CIRCULAR_5) {
		return GetCachedPngBitmap(id, circular_5_png, sizeof(circular_5_png));
	} else if (id == ART_CIRCULAR_6) {
		return GetCachedPngBitmap(id, circular_6_png, sizeof(circular_6_png));
	} else if (id == ART_CIRCULAR_7) {
		return GetCachedPngBitmap(id, circular_7_png, sizeof(circular_7_png));
	} else if (id == ART_NOLOOUT_BRUSH) {
		return GetCachedPngBitmap(id, no_logout_png, sizeof(no_logout_png));
	} else if (id == ART_NOPVP_BRUSH) {
		return GetCachedPngBitmap(id, no_pvp_png, sizeof(no_pvp_png));
	} else if (id == ART_POSITION_GO) {
		wxBitmap bitmap = GetCachedFileBitmap(id, {
			"icons/position_go.png",
			"../icons/position_go.png",
			"Map Editor/icons/position_go.png",
			"../Map Editor/icons/position_go.png"
		});
		if (bitmap.IsOk()) {
			return bitmap;
		}
		return wxNullBitmap;
	} else if (id == ART_PVP_BRUSH) {
		return GetCachedPngBitmap(id, pvp_zone_png, sizeof(pvp_zone_png));
	} else if (id == ART_PZ_BRUSH) {
		return GetCachedPngBitmap(id, protection_zone_png, sizeof(protection_zone_png));
	} else if (id == ART_RECTANGULAR) {
		return GetCachedPngBitmap(id, rectangular_4_png, sizeof(rectangular_4_png));
	} else if (id == ART_RECTANGULAR_1) {
		return GetCachedPngBitmap(id, rectangular_1_png, sizeof(rectangular_1_png));
	} else if (id == ART_RECTANGULAR_2) {
		return GetCachedPngBitmap(id, rectangular_2_png, sizeof(rectangular_2_png));
	} else if (id == ART_RECTANGULAR_3) {
		return GetCachedPngBitmap(id, rectangular_3_png, sizeof(rectangular_3_png));
	} else if (id == ART_RECTANGULAR_4) {
		return GetCachedPngBitmap(id, rectangular_4_png, sizeof(rectangular_4_png));
	} else if (id == ART_RECTANGULAR_5) {
		return GetCachedPngBitmap(id, rectangular_5_png, sizeof(rectangular_5_png));
	} else if (id == ART_RECTANGULAR_6) {
		return GetCachedPngBitmap(id, rectangular_6_png, sizeof(rectangular_6_png));
	} else if (id == ART_RECTANGULAR_7) {
		return GetCachedPngBitmap(id, rectangular_7_png, sizeof(rectangular_7_png));
	} else if (id == ART_DOOR_NORMAL_SMALL) {
		return GetCachedPngBitmap(id, door_normal_small_png, sizeof(door_normal_small_png));
	} else if (id == ART_DOOR_LOCKED_SMALL) {
		return GetCachedPngBitmap(id, door_locked_small_png, sizeof(door_locked_small_png));
	} else if (id == ART_DOOR_MAGIC_SMALL) {
		return GetCachedPngBitmap(id, door_magic_small_png, sizeof(door_magic_small_png));
	} else if (id == ART_DOOR_QUEST_SMALL) {
		return GetCachedPngBitmap(id, door_quest_small_png, sizeof(door_quest_small_png));
	} else if (id == ART_DOOR_NORMAL_ALT_SMALL) {
		wxBitmap bitmap = GetCachedFileBitmap(id, {
			"brushes/door_normal_alt_small.png",
			"../brushes/door_normal_alt_small.png",
			"Map Editor/brushes/door_normal_alt_small.png",
			"../Map Editor/brushes/door_normal_alt_small.png"
		});
		if (bitmap.IsOk()) {
			return bitmap;
		}
		return GetCachedPngBitmap(id, door_normal_small_png, sizeof(door_normal_small_png));
	} else if (id == ART_DOOR_ARCHWAY_SMALL) {
		wxBitmap bitmap = GetCachedFileBitmap(id, {
			"brushes/door_archway_small.png",
			"../brushes/door_archway_small.png",
			"Map Editor/brushes/door_archway_small.png",
			"../Map Editor/brushes/door_archway_small.png"
		});
		if (bitmap.IsOk()) {
			return bitmap;
		}
		return GetCachedPngBitmap(id, door_normal_small_png, sizeof(door_normal_small_png));
	}

	return wxNullBitmap;
}
