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

#include <wx/dir.h>
#include <wx/stdpaths.h>
#include <wx/filename.h>

#include "editor.h"
#include "items.h"
#include "creatures.h"

#include "gui.h"
#include "materials.h"
#include "brush.h"
#include "creature_brush.h"
#include "raw_brush.h"
#include "client_version.h"

Materials g_materials;

static wxFileName GetFavoritesFilePath() {
	if (g_gui.IsEditorOpen()) {
		Editor* ed = g_gui.GetCurrentEditor();
		if (ed && !ed->map.getFilename().empty()) {
			wxFileName mapFn(ed->map.getFilename());
			wxString dir = mapFn.GetPath();
			if (!dir.empty() && wxDirExists(dir)) {
				return wxFileName(dir, "favorites.xml");
			}
		}
	}
	ClientVersionID verId = g_gui.GetCurrentVersionID();
	if (verId != CLIENT_VERSION_NONE) {
		if (ClientVersion* cv = ClientVersion::get(verId)) {
			FileName dataPath = cv->getDataPath();
			return wxFileName(dataPath.GetPath(), "favorites.xml");
		}
	}
	if (ClientVersion* latest = ClientVersion::getLatestVersion()) {
		FileName dataPath = latest->getDataPath();
		return wxFileName(dataPath.GetPath(), "favorites.xml");
	}
	return wxFileName(g_gui.GetDataDirectory(), "favorites.xml");
}

Materials::Materials() {
	////
}

Materials::~Materials() {
	clear();
}

void Materials::clear() {
	for (TilesetContainer::iterator iter = tilesets.begin(); iter != tilesets.end(); ++iter) {
		delete iter->second;
	}

	for (MaterialsExtensionList::iterator iter = extensions.begin(); iter != extensions.end(); ++iter) {
		delete *iter;
	}

	tilesets.clear();
	extensions.clear();
}

const MaterialsExtensionList& Materials::getExtensions() {
	return extensions;
}

MaterialsExtensionList Materials::getExtensionsByVersion(uint16_t version_id) {
	MaterialsExtensionList ret_list;
	for (MaterialsExtensionList::iterator iter = extensions.begin(); iter != extensions.end(); ++iter) {
		if ((*iter)->isForVersion(version_id)) {
			ret_list.push_back(*iter);
		}
	}
	return ret_list;
}

bool Materials::loadMaterials(const FileName& identifier, wxString& error, wxArrayString& warnings) {
	pugi::xml_document doc;
	pugi::xml_parse_result result = doc.load_file(identifier.GetFullPath().mb_str());
	if (!result) {
		warnings.push_back("Could not open " + identifier.GetFullName() + " (file not found or syntax error)");
		return false;
	}

	pugi::xml_node node = doc.child("materials");
	if (!node) {
		warnings.push_back(identifier.GetFullName() + ": Invalid rootheader.");
		return false;
	}

	unserializeMaterials(identifier, node, error, warnings);
	return true;
}

bool Materials::loadExtensions(FileName directoryName, wxString& error, wxArrayString& warnings) {
	directoryName.Mkdir(0755, wxPATH_MKDIR_FULL); // Create if it doesn't exist

	wxDir ext_dir(directoryName.GetPath());
	if (!ext_dir.IsOpened()) {
		error = "Could not open extensions directory.";
		return false;
	}

	wxString filename;
	if (!ext_dir.GetFirst(&filename)) {
		// No extensions found
		return true;
	}

	StringVector clientVersions;
	do {
		FileName fn;
		fn.SetPath(directoryName.GetPath());
		fn.SetFullName(filename);
		if (fn.GetExt() != "xml") {
			continue;
		}

		pugi::xml_document doc;
		pugi::xml_parse_result result = doc.load_file(fn.GetFullPath().mb_str());
		if (!result) {
			warnings.push_back("Could not open " + filename + " (file not found or syntax error)");
			continue;
		}

		pugi::xml_node extensionNode = doc.child("materialsextension");
		if (!extensionNode) {
			warnings.push_back(filename + ": Invalid rootheader.");
			continue;
		}

		pugi::xml_attribute attribute;
		if (!(attribute = extensionNode.attribute("name"))) {
			warnings.push_back(filename + ": Couldn't read extension name.");
			continue;
		}

		const std::string& extensionName = attribute.as_string();
		if (!(attribute = extensionNode.attribute("author"))) {
			warnings.push_back(filename + ": Couldn't read extension name.");
			continue;
		}

		const std::string& extensionAuthor = attribute.as_string();
		if (!(attribute = extensionNode.attribute("description"))) {
			warnings.push_back(filename + ": Couldn't read extension name.");
			continue;
		}

		const std::string& extensionDescription = attribute.as_string();
		if (extensionName.empty() || extensionAuthor.empty() || extensionDescription.empty()) {
			warnings.push_back(filename + ": Couldn't read extension attributes (name, author, description).");
			continue;
		}

		std::string extensionUrl = extensionNode.attribute("url").as_string();
		extensionUrl.erase(std::remove(extensionUrl.begin(), extensionUrl.end(), '\''));

		std::string extensionAuthorLink = extensionNode.attribute("authorurl").as_string();
		extensionAuthorLink.erase(std::remove(extensionAuthorLink.begin(), extensionAuthorLink.end(), '\''));

		MaterialsExtension* materialExtension = newd MaterialsExtension(extensionName, extensionAuthor, extensionDescription);
		materialExtension->url = extensionUrl;
		materialExtension->author_url = extensionAuthorLink;

		if ((attribute = extensionNode.attribute("client"))) {
			clientVersions.clear();
			const std::string& extensionClientString = attribute.as_string();

			size_t lastPosition = 0;
			size_t position = extensionClientString.find(';');
			while (position != std::string::npos) {
				clientVersions.push_back(extensionClientString.substr(lastPosition, position - lastPosition));
				lastPosition = position + 1;
				position = extensionClientString.find(';', lastPosition);
			}

			clientVersions.push_back(extensionClientString.substr(lastPosition));
			for (const std::string& version : clientVersions) {
				materialExtension->addVersion(version);
			}

			std::sort(materialExtension->version_list.begin(), materialExtension->version_list.end(), VersionComparisonPredicate);

			auto duplicate = std::unique(materialExtension->version_list.begin(), materialExtension->version_list.end());
			while (duplicate != materialExtension->version_list.end()) {
				materialExtension->version_list.erase(duplicate);
				duplicate = std::unique(materialExtension->version_list.begin(), materialExtension->version_list.end());
			}
		} else {
			warnings.push_back(filename + ": Extension is not available for any version.");
		}

		extensions.push_back(materialExtension);
		if (materialExtension->isForVersion(g_gui.GetCurrentVersionID())) {
			unserializeMaterials(filename, extensionNode, error, warnings);
		}
	} while (ext_dir.GetNext(&filename));

	return true;
}

bool Materials::unserializeMaterials(const FileName& filename, pugi::xml_node node, wxString& error, wxArrayString& warnings) {
	wxString warning;
	pugi::xml_attribute attribute;
	for (pugi::xml_node childNode = node.first_child(); childNode; childNode = childNode.next_sibling()) {
		const std::string& childName = as_lower_str(childNode.name());
		if (childName == "include") {
			if (!(attribute = childNode.attribute("file"))) {
				continue;
			}

			FileName includeName;
			includeName.SetPath(filename.GetPath());
			includeName.SetFullName(wxString(attribute.as_string(), wxConvUTF8));

			wxString subError;
			if (!loadMaterials(includeName, subError, warnings)) {
				warnings.push_back("Error while loading file \"" + includeName.GetFullName() + "\": " + subError);
			}
		} else if (childName == "metaitem") {
			g_items.loadMetaItem(childNode);
		} else if (childName == "border") {
			g_brushes.unserializeBorder(childNode, warnings);
			if (warning.size()) {
				warnings.push_back("materials.xml: " + warning);
			}
		} else if (childName == "brush") {
			g_brushes.unserializeBrush(childNode, warnings);
			if (warning.size()) {
				warnings.push_back("materials.xml: " + warning);
			}
		} else if (childName == "tileset") {
			unserializeTileset(childNode, warnings);
		}
	}
	return true;
}

void Materials::createOtherTileset() {
	Tileset* others;
	Tileset* npc_tileset;

	if (tilesets["Creatures"] != nullptr) {
		others = tilesets["Creatures"];
		others->clear();
	} else {
		others = newd Tileset(g_brushes, "Creatures");
		tilesets["Creatures"] = others;
	}

	if (tilesets["Favorites"] == nullptr) {
		Tileset* favs = newd Tileset(g_brushes, "Favorites");
		favs->getCategory(TILESET_FAVORITE);
		favs->getCategory(TILESET_TERRAIN);
		favs->getCategory(TILESET_DOODAD);
		favs->getCategory(TILESET_ITEM);
		favs->getCategory(TILESET_CREATURE);
		favs->getCategory(TILESET_NPC);
		tilesets["Favorites"] = favs;
	}

	if (tilesets["NPCs"] != nullptr) {
		npc_tileset = tilesets["NPCs"];
		npc_tileset->clear();
	} else {
		npc_tileset = newd Tileset(g_brushes, "NPCs");
		tilesets["NPCs"] = npc_tileset;
	}

	// There should really be an iterator to do this
	for (int32_t id = 0; id <= g_items.getMaxID(); ++id) {
		ItemType& it = g_items[id];
		if (it.id == 0) {
			continue;
		}

		if (!it.isMetaItem()) {
			Brush* brush;
			if (it.in_other_tileset) {
				others->getCategory(TILESET_RAW)->brushlist.push_back(it.raw_brush);
				continue;
			} else if (it.raw_brush == nullptr) {
				brush = it.raw_brush = newd RAWBrush(it.id);
				it.has_raw = true;
				g_brushes.addBrush(it.raw_brush);
			} else if (!it.has_raw) {
				brush = it.raw_brush;
			} else {
				continue;
			}

			brush->flagAsVisible();
			others->getCategory(TILESET_RAW)->brushlist.push_back(it.raw_brush);
			it.in_other_tileset = true;
		}
	}

	for (CreatureMap::iterator iter = g_creatures.begin(); iter != g_creatures.end(); ++iter) {
		CreatureType* type = iter->second;
		if (type->in_other_tileset) {
			if (type->isNpc) {
				npc_tileset->getCategory(TILESET_CREATURE)->brushlist.push_back(type->brush);
			} else {
				others->getCategory(TILESET_CREATURE)->brushlist.push_back(type->brush);
			}
		} else if (type->brush == nullptr) {
			type->brush = newd CreatureBrush(type);
			g_brushes.addBrush(type->brush);
			type->brush->flagAsVisible();
			type->in_other_tileset = true;
			if (type->isNpc) {
				npc_tileset->getCategory(TILESET_CREATURE)->brushlist.push_back(type->brush);
			} else {
				others->getCategory(TILESET_CREATURE)->brushlist.push_back(type->brush);
			}
		}
	}

	loadFavorites();
}

void Materials::rebuildFavorites() {
	Tileset* favs = tilesets["Favorites"];
	if (!favs) return;

	TilesetCategory* catFav = favs->getCategory(TILESET_FAVORITE);
	if (!catFav) return;

	// Collect unique real brushes (ignoring any existing separators)
	std::vector<Brush*> terrain_favs;
	std::vector<Brush*> wall_favs;
	std::vector<Brush*> doodad_favs;
	std::vector<Brush*> item_favs;
	std::vector<Brush*> monster_favs;
	std::vector<Brush*> npc_favs;
	std::vector<Brush*> other_favs;

	std::set<Brush*> seen;

	for (TilesetCategory* cat : favs->categories) {
		for (Brush* b : cat->brushlist) {
			if (!b || b->isSeparator()) continue;
			if (seen.find(b) != seen.end()) continue;
			seen.insert(b);

			if (b->isCreature()) {
				CreatureBrush* cb = static_cast<CreatureBrush*>(b);
				if (cb && cb->getType() && cb->getType()->isNpc) {
					npc_favs.push_back(b);
				} else {
					monster_favs.push_back(b);
				}
			} else if (b->isWall()) {
				wall_favs.push_back(b);
			} else if (b->isDoodad() || b->isTable() || b->isCarpet() || wxstr(b->getName()).Lower().Contains("ladder")) {
				doodad_favs.push_back(b);
			} else if (b->isRaw()) {
				item_favs.push_back(b);
			} else if (b->isGround() || b->isTerrain() || b->isOptionalBorder()) {
				terrain_favs.push_back(b);
			} else {
				other_favs.push_back(b);
			}
		}
	}

	// Update subcategories
	if (TilesetCategory* c = favs->getCategory(TILESET_TERRAIN)) {
		c->brushlist = terrain_favs;
	}
	if (TilesetCategory* c = favs->getCategory(TILESET_DOODAD)) {
		c->brushlist = doodad_favs;
	}
	if (TilesetCategory* c = favs->getCategory(TILESET_ITEM)) {
		c->brushlist = item_favs;
	}
	if (TilesetCategory* c = favs->getCategory(TILESET_CREATURE)) {
		c->brushlist = monster_favs;
	}
	if (TilesetCategory* c = favs->getCategory(TILESET_NPC)) {
		c->brushlist = npc_favs;
	}

	// Build "All Favorites" with collapsible separators
	catFav->brushlist.clear();

	static SeparatorBrush sep_terrain("Terrain");
	static SeparatorBrush sep_walls("Walls & Railings");
	static SeparatorBrush sep_doodads("Doodads");
	static SeparatorBrush sep_items("Items");
	static SeparatorBrush sep_monsters("Monsters");
	static SeparatorBrush sep_npcs("NPCs");
	static SeparatorBrush sep_other("Other");

	if (!terrain_favs.empty()) {
		catFav->brushlist.push_back(&sep_terrain);
		catFav->brushlist.insert(catFav->brushlist.end(), terrain_favs.begin(), terrain_favs.end());
	}
	if (!wall_favs.empty()) {
		catFav->brushlist.push_back(&sep_walls);
		catFav->brushlist.insert(catFav->brushlist.end(), wall_favs.begin(), wall_favs.end());
	}
	if (!doodad_favs.empty()) {
		catFav->brushlist.push_back(&sep_doodads);
		catFav->brushlist.insert(catFav->brushlist.end(), doodad_favs.begin(), doodad_favs.end());
	}
	if (!item_favs.empty()) {
		catFav->brushlist.push_back(&sep_items);
		catFav->brushlist.insert(catFav->brushlist.end(), item_favs.begin(), item_favs.end());
	}
	if (!monster_favs.empty()) {
		catFav->brushlist.push_back(&sep_monsters);
		catFav->brushlist.insert(catFav->brushlist.end(), monster_favs.begin(), monster_favs.end());
	}
	if (!npc_favs.empty()) {
		catFav->brushlist.push_back(&sep_npcs);
		catFav->brushlist.insert(catFav->brushlist.end(), npc_favs.begin(), npc_favs.end());
	}
	if (!other_favs.empty()) {
		catFav->brushlist.push_back(&sep_other);
		catFav->brushlist.insert(catFav->brushlist.end(), other_favs.begin(), other_favs.end());
	}
}

void Materials::saveFavorites() {
	Tileset* favs = tilesets["Favorites"];
	if (!favs) return;

	pugi::xml_document doc;
	pugi::xml_node root = doc.append_child("favorites");

	const TilesetCategory* catFav = favs->getCategory(TILESET_FAVORITE);
	if (catFav) {
		std::set<std::string> saved_names;
		for (Brush* b : catFav->brushlist) {
			if (b && !b->isSeparator() && !b->getName().empty()) {
				if (saved_names.find(b->getName()) == saved_names.end()) {
					saved_names.insert(b->getName());
					pugi::xml_node item = root.append_child("brush");
					item.append_attribute("name") = b->getName().c_str();
				}
			}
		}
	}

	wxFileName fn = GetFavoritesFilePath();
	if (!wxDirExists(fn.GetPath())) {
		wxFileName::Mkdir(fn.GetPath(), 511, wxPATH_MKDIR_FULL);
	}
	doc.save_file(fn.GetFullPath().mb_str());

	// Also sync to client version folder as default/backup
	ClientVersionID verId = g_gui.GetCurrentVersionID();
	if (verId != CLIENT_VERSION_NONE) {
		if (ClientVersion* cv = ClientVersion::get(verId)) {
			wxFileName verFn(cv->getDataPath().GetPath(), "favorites.xml");
			if (verFn.GetFullPath() != fn.GetFullPath()) {
				if (!wxDirExists(verFn.GetPath())) {
					wxFileName::Mkdir(verFn.GetPath(), 511, wxPATH_MKDIR_FULL);
				}
				doc.save_file(verFn.GetFullPath().mb_str());
			}
		}
	}
}

void Materials::loadFavorites() {
	wxFileName fn = GetFavoritesFilePath();
	if (!fn.FileExists()) {
		// Fallback to client version folder
		ClientVersionID verId = g_gui.GetCurrentVersionID();
		if (verId != CLIENT_VERSION_NONE) {
			if (ClientVersion* cv = ClientVersion::get(verId)) {
				wxFileName verFn(cv->getDataPath().GetPath(), "favorites.xml");
				if (verFn.FileExists()) {
					fn = verFn;
				}
			}
		}
	}
	if (!fn.FileExists()) {
		wxFileName fallbackFn(wxStandardPaths::Get().GetUserDataDir(), "favorites.xml");
		if (fallbackFn.FileExists()) {
			fn = fallbackFn;
		} else {
			return;
		}
	}

	pugi::xml_document doc;
	pugi::xml_parse_result result = doc.load_file(fn.GetFullPath().mb_str());
	if (!result) return;

	pugi::xml_node root = doc.child("favorites");
	if (!root) return;

	Tileset* favs = tilesets["Favorites"];
	if (!favs) return;
	TilesetCategory* catFav = favs->getCategory(TILESET_FAVORITE);
	if (!catFav) return;

	for (pugi::xml_node item = root.child("brush"); item; item = item.next_sibling("brush")) {
		const char* brush_name = item.attribute("name").as_string();
		if (brush_name && strlen(brush_name) > 0) {
			Brush* b = g_brushes.getBrush(brush_name);
			if (b && !b->isSeparator() && !catFav->containsBrush(b)) {
				catFav->brushlist.push_back(b);
			}
		}
	}

	rebuildFavorites();
}

bool Materials::unserializeTileset(pugi::xml_node node, wxArrayString& warnings) {
	pugi::xml_attribute attribute;
	if (!(attribute = node.attribute("name"))) {
		warnings.push_back("Couldn't read tileset name");
		return false;
	}

	const std::string& name = attribute.as_string();

	Tileset* tileset;
	auto it = tilesets.find(name);
	if (it != tilesets.end()) {
		tileset = it->second;
	} else {
		tileset = newd Tileset(g_brushes, name);
		tilesets.insert(std::make_pair(name, tileset));
	}

	for (pugi::xml_node childNode = node.first_child(); childNode; childNode = childNode.next_sibling()) {
		tileset->loadCategory(childNode, warnings);
	}
	return true;
}

void Materials::addToTileset(std::string tilesetName, int itemId, TilesetCategoryType categoryType) {
	ItemType& it = g_items[itemId];

	if (it.id == 0) {
		return;
	}

	Tileset* tileset;
	auto _it = tilesets.find(tilesetName);
	if (_it != tilesets.end()) {
		tileset = _it->second;
	} else {
		tileset = newd Tileset(g_brushes, tilesetName);
		tilesets.insert(std::make_pair(tilesetName, tileset));
	}

	TilesetCategory* category = tileset->getCategory(categoryType);

	if (!it.isMetaItem()) {
		Brush* brush;
		if (it.in_other_tileset) {
			category->brushlist.push_back(it.raw_brush);
			return;
		} else if (it.raw_brush == nullptr) {
			brush = it.raw_brush = newd RAWBrush(it.id);
			it.has_raw = true;
			g_brushes.addBrush(it.raw_brush);
		} else {
			brush = it.raw_brush;
		}

		brush->flagAsVisible();
		category->brushlist.push_back(it.raw_brush);
		it.in_other_tileset = true;
	}
}

bool Materials::isInTileset(Item* item, std::string tilesetName) const {
	const ItemType& it = g_items[item->getID()];

	return it.id != 0 && (isInTileset(it.brush, tilesetName) || isInTileset(it.doodad_brush, tilesetName) || isInTileset(it.raw_brush, tilesetName)) || isInTileset(it.collection_brush, tilesetName);
}

bool Materials::isInTileset(Brush* brush, std::string tilesetName) const {
	if (!brush) {
		return false;
	}

	TilesetContainer::const_iterator tilesetiter = tilesets.find(tilesetName);
	if (tilesetiter == tilesets.end()) {
		return false;
	}
	Tileset* tileset = tilesetiter->second;

	return tileset->containsBrush(brush);
}
