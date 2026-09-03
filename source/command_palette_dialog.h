#ifndef RME_COMMAND_PALETTE_DIALOG_H
#define RME_COMMAND_PALETTE_DIALOG_H

#include <wx/dialog.h>
#include <wx/textctrl.h>
#include <wx/listbox.h>
#include <wx/sizer.h>
#include <vector>
#include <string>
#include "position.h"

class Brush;

enum PaletteItemType {
	PALETTE_ITEM_ACTION,
	PALETTE_ITEM_BRUSH,
	PALETTE_ITEM_TELEPORT_POS
};

struct PaletteCommand {
	std::string name;
	std::string category;
	PaletteItemType type = PALETTE_ITEM_ACTION;
	int action_id = -1;
	Brush* brush = nullptr;
	Position target_pos;
};

class CommandPaletteDialog : public wxDialog {
public:
	CommandPaletteDialog(wxWindow* parent);
	virtual ~CommandPaletteDialog();

	const PaletteCommand* GetSelectedResult() const { return (selected_result.action_id != -1 || selected_result.brush != nullptr || selected_result.target_pos.isValid()) ? &selected_result : nullptr; }
	int GetSelectedActionID() const { return selected_result.action_id; }

private:
	void OnSearchText(wxCommandEvent& evt);
	void OnKeyDown(wxKeyEvent& evt);
	void OnListDClick(wxCommandEvent& evt);
	void OnClickOK(wxCommandEvent& evt);
	void PopulateCommands();
	void FilterCommands();

	wxTextCtrl* search_field;
	wxListBox* results_list;
	std::vector<PaletteCommand> all_commands;
	std::vector<PaletteCommand> filtered_commands;
	PaletteCommand selected_result;

	DECLARE_EVENT_TABLE()
};

#endif // RME_COMMAND_PALETTE_DIALOG_H
