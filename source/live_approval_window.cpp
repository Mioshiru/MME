//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////

#include "main.h"
#include "live_approval_window.h"
#include "live_server.h"
#include "live_peer.h"
#include "live_packets.h"
#include "gui.h"
#include "editor.h"
#include "map.h"
#include "town.h"
#include "tile.h"
#include "item.h"

LiveApprovalWindow* LiveApprovalWindow::s_instance = nullptr;
std::vector<LiveApprovalRequest> LiveApprovalWindow::s_requests;
uint32_t LiveApprovalWindow::s_nextReqId = 1;

enum {
	ID_APPROVAL_LIST = 11100,
	ID_APPROVAL_JUMP,
	ID_APPROVAL_APPROVE,
	ID_APPROVAL_REJECT
};

void LiveApprovalWindow::ShowWindow(wxWindow* parent, LiveServer* server) {
	if (s_instance) {
		s_instance->Raise();
		s_instance->Show(true);
		s_instance->RefreshList();
		return;
	}
	s_instance = new LiveApprovalWindow(parent, server);
	s_instance->Show(true);
}

void LiveApprovalWindow::AddPendingRequest(const LiveApprovalRequest& req) {
	s_requests.push_back(req);
	if (s_instance) {
		s_instance->RefreshList();
	}
	g_gui.SetStatusText(wxString::Format("🛡️ New Approval Request from %s (%s)", req.requesterName, req.details));
}

LiveApprovalWindow::LiveApprovalWindow(wxWindow* parent, LiveServer* server)
	: wxDialog(parent, wxID_ANY, "Multiplayer Approvals & Reviews (Host)", wxDefaultPosition, wxSize(580, 420),
	           wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER), server(server) {
	SetBackgroundColour(wxColour(15, 28, 48));
	SetForegroundColour(wxColour(240, 245, 255));

	wxBoxSizer* main_sizer = new wxBoxSizer(wxVERTICAL);

	// Header
	wxStaticText* header = new wxStaticText(this, wxID_ANY, "Pending Client ID & Creation Requests");
	wxFont font = header->GetFont();
	font.SetPointSize(10);
	font.SetWeight(wxFONTWEIGHT_BOLD);
	header->SetFont(font);
	header->SetForegroundColour(wxColour(255, 205, 50));
	main_sizer->Add(header, 0, wxALL, 10);

	// List box of requests
	listbox = new wxListBox(this, ID_APPROVAL_LIST, wxDefaultPosition, wxDefaultSize, 0, nullptr, wxLB_SINGLE);
	listbox->SetBackgroundColour(wxColour(10, 20, 35));
	listbox->SetForegroundColour(wxColour(220, 235, 255));
	main_sizer->Add(listbox, 1, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 10);

	// Action Buttons
	wxBoxSizer* btn_sizer = new wxBoxSizer(wxHORIZONTAL);
	btn_jump = new wxButton(this, ID_APPROVAL_JUMP, "📍 Jump to Location");
	btn_jump->SetBackgroundColour(wxColour(30, 58, 95));
	btn_jump->SetForegroundColour(wxColour(240, 245, 255));

	btn_approve = new wxButton(this, ID_APPROVAL_APPROVE, "✅ Approve (Freigeben)");
	btn_approve->SetBackgroundColour(wxColour(25, 90, 45));
	btn_approve->SetForegroundColour(wxColour(240, 255, 240));

	btn_reject = new wxButton(this, ID_APPROVAL_REJECT, "❌ Reject (Ablehnen)");
	btn_reject->SetBackgroundColour(wxColour(95, 30, 35));
	btn_reject->SetForegroundColour(wxColour(255, 240, 240));

	btn_sizer->Add(btn_jump, 1, wxRIGHT, 5);
	btn_sizer->Add(btn_approve, 1, wxRIGHT, 5);
	btn_sizer->Add(btn_reject, 1, 0);
	main_sizer->Add(btn_sizer, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 10);

	status_label = new wxStaticText(this, wxID_ANY, "Select a request to review, jump to location, and approve or reject.");
	status_label->SetForegroundColour(wxColour(160, 185, 215));
	main_sizer->Add(status_label, 0, wxLEFT | wxRIGHT | wxBOTTOM, 10);

	SetSizer(main_sizer);

	Bind(wxEVT_LISTBOX, &LiveApprovalWindow::OnSelect, this, ID_APPROVAL_LIST);
	Bind(wxEVT_BUTTON, &LiveApprovalWindow::OnJumpToPos, this, ID_APPROVAL_JUMP);
	Bind(wxEVT_BUTTON, &LiveApprovalWindow::OnApprove, this, ID_APPROVAL_APPROVE);
	Bind(wxEVT_BUTTON, &LiveApprovalWindow::OnReject, this, ID_APPROVAL_REJECT);
	Bind(wxEVT_CLOSE_WINDOW, &LiveApprovalWindow::OnClose, this);

	RefreshList();
}

LiveApprovalWindow::~LiveApprovalWindow() {
	s_instance = nullptr;
}

void LiveApprovalWindow::RefreshList() {
	if (!listbox) return;
	listbox->Clear();

	for (size_t i = 0; i < s_requests.size(); ++i) {
		const auto& r = s_requests[i];
		wxString typeStr;
		switch (r.type) {
			case APPROVAL_TOWN: typeStr = "🏙️ Town"; break;
			case APPROVAL_UNIQUE_ID: typeStr = "🔑 Unique ID"; break;
			case APPROVAL_ACTION_ID: typeStr = "⚡ Action ID"; break;
			case APPROVAL_DOOR_KEY: typeStr = "🚪 Door/Key"; break;
			case APPROVAL_CONTAINER: typeStr = "📦 Container"; break;
			default: typeStr = "Entity"; break;
		}

		wxString statusStr = r.resolved ? (r.approved ? "[APPROVED]" : "[REJECTED]") : "[PENDING]";
		wxString entry = wxString::Format("%s %s by %s @ (%d, %d, %d): %s (ID %u)",
			statusStr, typeStr, r.requesterName, r.pos.x, r.pos.y, r.pos.z, r.details, r.requestedValue);
		listbox->Append(entry);
	}

	if (listbox->GetCount() > 0 && listbox->GetSelection() == wxNOT_FOUND) {
		listbox->SetSelection(0);
	}
}

void LiveApprovalWindow::OnSelect(wxCommandEvent& WXUNUSED(event)) {
	int sel = listbox->GetSelection();
	if (sel >= 0 && sel < (int)s_requests.size()) {
		const auto& r = s_requests[sel];
		btn_approve->Enable(!r.resolved);
		btn_reject->Enable(!r.resolved);
	}
}

void LiveApprovalWindow::OnJumpToPos(wxCommandEvent& WXUNUSED(event)) {
	int sel = listbox->GetSelection();
	if (sel >= 0 && sel < (int)s_requests.size()) {
		const auto& r = s_requests[sel];
		g_gui.SetScreenCenterPosition(r.pos);
		g_gui.RefreshView();
	}
}

void LiveApprovalWindow::OnApprove(wxCommandEvent& WXUNUSED(event)) {
	int sel = listbox->GetSelection();
	if (sel < 0 || sel >= (int)s_requests.size()) return;

	auto& r = s_requests[sel];
	if (r.resolved) return;

	Editor* editor = g_gui.GetCurrentEditor();
	if (!editor) return;

	uint32_t finalId = r.requestedValue;

	if (r.type == APPROVAL_TOWN) {
		Map& map = editor->map;
		if (finalId == 0 || map.towns.find(finalId) != map.towns.end()) {
			uint32_t max_id = 0;
			for (const auto& pair : map.towns) {
				if (pair.second->getID() > max_id) max_id = pair.second->getID();
			}
			finalId = max_id + 1;
		}

		Town* t = new Town(finalId);
		t->setName(nstr(r.details.empty() ? wxString("Town " + std::to_string(finalId)) : r.details));
		t->setTemplePosition(r.pos);
		map.towns.addTown(t);

		Tile* tile = map.getOrCreateTile(r.pos);
		if (tile) {
			tile->getLocation()->increaseTownCount();
		}
		map.doChange();

		if (server) {
			NetworkMessage townListMsg;
			townListMsg.write<uint8_t>(PACKET_TOWN_LIST);
			townListMsg.write<uint32_t>((uint32_t)map.towns.count());
			for (const auto& pair : map.towns) {
				Town* town = pair.second;
				if (town) {
					townListMsg.write<uint32_t>(town->getID());
					townListMsg.write<std::string>(town->getName());
					townListMsg.write<Position>(town->getTemplePosition());
				}
			}
			for (auto& clientEntry : server->getClients()) {
				if (clientEntry.second) clientEntry.second->send(townListMsg);
			}
		}
		g_gui.RefreshMinimapPanel();
	}

	r.resolved = true;
	r.approved = true;

	if (server) {
		NetworkMessage resp;
		resp.write<uint8_t>(PACKET_APPROVAL_RESPONSE);
		resp.write<uint32_t>(r.reqId);
		resp.write<uint8_t>(1); // Approved
		resp.write<uint32_t>(finalId);
		resp.write<std::string>("Approved by Host");
		for (auto& clientEntry : server->getClients()) {
			if (clientEntry.second) clientEntry.second->send(resp);
		}
	}

	RefreshList();
	g_gui.SetStatusText(wxString::Format("Approved %s (ID %u)", r.details, finalId));
}

void LiveApprovalWindow::OnReject(wxCommandEvent& WXUNUSED(event)) {
	int sel = listbox->GetSelection();
	if (sel < 0 || sel >= (int)s_requests.size()) return;

	auto& r = s_requests[sel];
	if (r.resolved) return;

	r.resolved = true;
	r.approved = false;

	if (server) {
		NetworkMessage resp;
		resp.write<uint8_t>(PACKET_APPROVAL_RESPONSE);
		resp.write<uint32_t>(r.reqId);
		resp.write<uint8_t>(0); // Rejected
		resp.write<uint32_t>(0);
		resp.write<std::string>("Rejected by Host");
		for (auto& clientEntry : server->getClients()) {
			if (clientEntry.second) clientEntry.second->send(resp);
		}
	}

	RefreshList();
	g_gui.SetStatusText("Request rejected.");
}

void LiveApprovalWindow::OnClose(wxCloseEvent& WXUNUSED(event)) {
	Destroy();
}
