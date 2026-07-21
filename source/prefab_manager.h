#ifndef RME_PREFAB_MANAGER_H_
#define RME_PREFAB_MANAGER_H_

#include "main.h"
#include "copybuffer.h"
#include <wx/dialog.h>
#include <wx/listbox.h>
#include <map>

#include "editor.h"

class PrefabManager {
public:
	static PrefabManager& getInstance();

	void addPrefab(const wxString& name, CopyBuffer* buffer);
	CopyBuffer* getPrefab(const wxString& name);
	std::vector<wxString> getPrefabNames() const;

private:
	PrefabManager() {}
	~PrefabManager() {}

	std::map<wxString, CopyBuffer*> prefabs;
};

class PrefabLibraryDialog : public wxDialog {
public:
	PrefabLibraryDialog(wxWindow* parent, Editor& editor);
	virtual ~PrefabLibraryDialog();

	void OnClickSaveCurrentSelection(wxCommandEvent& event);
	void OnClickPastePrefab(wxCommandEvent& event);
	void OnClickClose(wxCommandEvent& event);

private:
	Editor& editor;
	wxListBox* prefabListBox;

	DECLARE_EVENT_TABLE()
};

#endif
