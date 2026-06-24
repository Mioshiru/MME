#include "main.h"
#include "gui.h"
#include "materials.h"
#include "sprites.h"
#include <wx/stdpaths.h>

wxString GUI::GetDataDirectory() {
	std::string cfg_str = g_settings.getString(Config::DATA_DIRECTORY);
	if (!cfg_str.empty()) {
		FileName dir;
		dir.Assign(wxstr(cfg_str));
		wxString path;
		if (dir.DirExists()) {
			path = dir.GetPath(wxPATH_GET_VOLUME | wxPATH_GET_SEPARATOR);
			return path;
		}
	}
	FileName exec_directory;
	try {
		exec_directory = dynamic_cast<wxStandardPaths&>(wxStandardPaths::Get()).GetExecutablePath();
	} catch (const std::bad_cast) { throw; }
	exec_directory.AppendDir("data");
	return exec_directory.GetPath(wxPATH_GET_VOLUME | wxPATH_GET_SEPARATOR);
}

wxString GUI::GetExecDirectory() {
	FileName exec_directory;
	try {
		exec_directory = dynamic_cast<wxStandardPaths&>(wxStandardPaths::Get()).GetExecutablePath();
	} catch (const std::bad_cast) { wxLogError("Could not fetch executable directory."); }
	return exec_directory.GetPath(wxPATH_GET_VOLUME | wxPATH_GET_SEPARATOR);
}

wxString GUI::GetLocalDataDirectory() {
	if (g_settings.getInteger(Config::INDIRECTORY_INSTALLATION)) {
		FileName dir = GetDataDirectory();
		dir.AppendDir("user"); dir.AppendDir("data");
		dir.Mkdir(0755, wxPATH_MKDIR_FULL);
		return dir.GetPath(wxPATH_GET_VOLUME | wxPATH_GET_SEPARATOR);
	} else {
		FileName dir = dynamic_cast<wxStandardPaths&>(wxStandardPaths::Get()).GetUserDataDir();
#ifdef __WINDOWS__
		dir.AppendDir("Remere's Map Editor");
#else
		dir.AppendDir(".rme");
#endif
		dir.AppendDir("data");
		dir.Mkdir(0755, wxPATH_MKDIR_FULL);
		return dir.GetPath(wxPATH_GET_VOLUME | wxPATH_GET_SEPARATOR);
	}
}

wxString GUI::GetLocalDirectory() {
	if (g_settings.getInteger(Config::INDIRECTORY_INSTALLATION)) {
		FileName dir = GetDataDirectory(); dir.AppendDir("user");
		dir.Mkdir(0755, wxPATH_MKDIR_FULL);
		return dir.GetPath(wxPATH_GET_VOLUME | wxPATH_GET_SEPARATOR);
	} else {
		FileName dir = dynamic_cast<wxStandardPaths&>(wxStandardPaths::Get()).GetUserDataDir();
#ifdef __WINDOWS__
		dir.AppendDir("Remere's Map Editor");
#else
		dir.AppendDir(".rme");
#endif
		dir.Mkdir(0755, wxPATH_MKDIR_FULL);
		return dir.GetPath(wxPATH_GET_VOLUME | wxPATH_GET_SEPARATOR);
	}
}

wxString GUI::GetExtensionsDirectory() {
	std::string cfg_str = g_settings.getString(Config::EXTENSIONS_DIRECTORY);
	if (!cfg_str.empty()) {
		FileName dir; dir.Assign(wxstr(cfg_str));
		if (dir.DirExists()) return dir.GetPath(wxPATH_GET_VOLUME | wxPATH_GET_SEPARATOR);
	}
	FileName local_directory = GetLocalDirectory();
	local_directory.AppendDir("extensions");
	local_directory.Mkdir(0755, wxPATH_MKDIR_FULL);
	return local_directory.GetPath(wxPATH_GET_VOLUME | wxPATH_GET_SEPARATOR);
}

void GUI::discoverDataDirectory(const wxString& existentFile) {
	wxString currentDir = wxGetCwd();
	wxString execDir = GetExecDirectory();
	wxString possiblePaths[] = { execDir, currentDir + "/", execDir + "/../", execDir + "/../../", currentDir + "/../" };
	bool found = false;
	for (const wxString& path : possiblePaths) {
		if (wxFileName(path + "data/" + existentFile).FileExists()) {
			m_dataDirectory = path + "data/";
			found = true; break;
		}
	}
	if (!found) wxLogError(wxString() + "Could not find data directory.\n");
}

bool GUI::LoadVersion(ClientVersionID version, wxString& error, wxArrayString& warnings, bool force) {
	if (ClientVersion::get(version) == nullptr) {
		error = "Unsupported client version!"; return false;
	}
	if (version != loaded_version || force) {
		if (getLoadedVersion() != nullptr) SavePerspective();
		UnnamedRenderingLock();
		DestroyPalettes();
		UnloadVersion();
		loaded_version = version;
		if (!getLoadedVersion()->hasValidPaths()) {
			if (!getLoadedVersion()->loadValidPaths()) {
				error = "Couldn't load asset files"; loaded_version = CLIENT_VERSION_NONE; return false;
			}
		}
		bool ret = LoadDataFiles(error, warnings);
		if (ret) LoadPerspective(); else loaded_version = CLIENT_VERSION_NONE;
		return ret;
	}
	return true;
}

bool GUI::LoadDataFiles(wxString& error, wxArrayString& warnings) {
	FileName data_path = getLoadedVersion()->getDataPath();
	FileName client_path = getLoadedVersion()->getClientPath();
	FileName extension_path = GetExtensionsDirectory();

	gfx.client_version = getLoadedVersion();
	gfx.loadOTFI(client_path, error, warnings);

	CreateLoadBar("Loading asset files");
	SetLoadDone(0, "Loading .dat file...");
	if (!gfx.loadSpriteMetadata(gfx.getMetadataFileName(), error, warnings)) {
		DestroyLoadBar(); UnloadVersion(); return false;
	}

	SetLoadDone(10, "Loading sprites file...");
	if (!gfx.loadSpriteData(gfx.getSpritesFileName(), error, warnings)) {
		DestroyLoadBar(); UnloadVersion(); return false;
	}

	SetLoadDone(20, "Loading items.otb file...");
	if (!g_items.loadFromOtb(wxString(data_path.GetPath(wxPATH_GET_VOLUME | wxPATH_GET_SEPARATOR) + "items.otb"), error, warnings)) {
		DestroyLoadBar(); UnloadVersion(); return false;
	}

	SetLoadDone(30, "Loading items.xml ...");
	if (!g_items.loadFromGameXml(wxString(data_path.GetPath(wxPATH_GET_VOLUME | wxPATH_GET_SEPARATOR) + "items.xml"), error, warnings)) {
		warnings.push_back("Couldn't load items.xml: " + error);
	}

	SetLoadDone(45, "Loading creatures.xml ...");
	if (!g_creatures.loadFromXML(wxString(data_path.GetPath(wxPATH_GET_VOLUME | wxPATH_GET_SEPARATOR) + "creatures.xml"), true, error, warnings)) {
		warnings.push_back("Couldn't load creatures.xml: " + error);
	}

	SetLoadDone(50, "Loading materials.xml ...");
	if (!g_materials.loadMaterials(wxString(data_path.GetPath(wxPATH_GET_VOLUME | wxPATH_GET_SEPARATOR) + "materials.xml"), error, warnings)) {
		warnings.push_back("Couldn't load materials.xml: " + error);
	}

	SetLoadDone(70, "Loading extensions...");
	g_materials.loadExtensions(extension_path, error, warnings);

	SetLoadDone(70, "Finishing...");
	g_brushes.init();
	g_materials.createOtherTileset();
	DestroyLoadBar();
	return true;
}

void GUI::UnloadVersion() {
	UnnamedRenderingLock();
	DestroyPalettes();
	gfx.clear();
	current_brush = nullptr;
	previous_brush = nullptr;

	house_brush = nullptr; house_exit_brush = nullptr;
	waypoint_brush = nullptr; optional_brush = nullptr;
	eraser = nullptr; normal_door_brush = nullptr;
	locked_door_brush = nullptr; magic_door_brush = nullptr;
	quest_door_brush = nullptr; hatch_door_brush = nullptr;
	window_door_brush = nullptr;

	if (loaded_version != CLIENT_VERSION_NONE) {
		g_materials.clear();
		g_brushes.clear();
		g_items.clear();
		gfx.clear();
		FileName cdb = getLoadedVersion()->getLocalDataPath();
		cdb.SetFullName("creatures.xml");
		g_creatures.saveToXML(cdb);
		g_creatures.clear();
		loaded_version = CLIENT_VERSION_NONE;
	}
}