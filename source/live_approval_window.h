//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////

#ifndef LIVE_APPROVAL_WINDOW_H
#define LIVE_APPROVAL_WINDOW_H

#include "main.h"
#include "position.h"
#include <vector>
#include <string>

enum LiveApprovalType {
	APPROVAL_TOWN = 1,
	APPROVAL_UNIQUE_ID = 2,
	APPROVAL_ACTION_ID = 3,
	APPROVAL_DOOR_KEY = 4,
	APPROVAL_CONTAINER = 5
};

struct LiveApprovalRequest {
	uint32_t reqId = 0;
	uint32_t clientId = 0;
	wxString requesterName;
	LiveApprovalType type = APPROVAL_TOWN;
	Position pos;
	uint32_t requestedValue = 0;
	wxString details;
	uint64_t timestamp = 0;
	bool resolved = false;
	bool approved = false;
};

class LiveServer;

class LiveApprovalWindow : public wxDialog {
public:
	static void ShowWindow(wxWindow* parent, LiveServer* server);
	static void AddPendingRequest(const LiveApprovalRequest& req);
	static LiveApprovalWindow* GetInstance() { return s_instance; }
	static std::vector<LiveApprovalRequest>& GetRequests() { return s_requests; }

	LiveApprovalWindow(wxWindow* parent, LiveServer* server);
	virtual ~LiveApprovalWindow();

	void RefreshList();

private:
	void OnSelect(wxCommandEvent& event);
	void OnJumpToPos(wxCommandEvent& event);
	void OnApprove(wxCommandEvent& event);
	void OnReject(wxCommandEvent& event);
	void OnClose(wxCloseEvent& event);

	LiveServer* server = nullptr;
	wxListBox* listbox = nullptr;
	wxButton* btn_jump = nullptr;
	wxButton* btn_approve = nullptr;
	wxButton* btn_reject = nullptr;
	wxStaticText* status_label = nullptr;

	static LiveApprovalWindow* s_instance;
	static std::vector<LiveApprovalRequest> s_requests;
	static uint32_t s_nextReqId;
};

#endif
