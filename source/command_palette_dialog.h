#ifndef RME_COMMAND_PALETTE_DIALOG_H
#define RME_COMMAND_PALETTE_DIALOG_H

#include <wx/dialog.h>
#include <wx/textctrl.h>
#include <wx/listbox.h>
#include <wx/sizer.h>
#include <vector>
#include <string>

struct PaletteCommand {
	std::string name;
	std::string category;
	int action_id;
};

class CommandPaletteDialog : public wxDialog {
public:
	CommandPaletteDialog(wxWindow* parent);
	virtual ~CommandPaletteDialog();

	int GetSelectedActionID() const { return selected_action_id; }

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
	int selected_action_id;

	DECLARE_EVENT_TABLE()
};

#endif // RME_COMMAND_PALETTE_DIALOG_H
