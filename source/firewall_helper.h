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

#ifndef RME_FIREWALL_HELPER_H_
#define RME_FIREWALL_HELPER_H_

#include "main.h"

#ifdef __WINDOWS__
#include <windows.h>
#include <netfw.h>
#include <objbase.h>
#include <shellapi.h>

inline bool IsWindowsFirewallPortAllowed(int port) {
	HRESULT hrCom = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
	bool allowed = false;

	INetFwPolicy2* pNetFwPolicy2 = nullptr;
	HRESULT hr = CoCreateInstance(
		__uuidof(NetFwPolicy2),
		nullptr,
		CLSCTX_INPROC_SERVER,
		__uuidof(INetFwPolicy2),
		reinterpret_cast<void**>(&pNetFwPolicy2)
	);

	if (SUCCEEDED(hr) && pNetFwPolicy2) {
		INetFwRules* pFwRules = nullptr;
		hr = pNetFwPolicy2->get_Rules(&pFwRules);
		if (SUCCEEDED(hr) && pFwRules) {
			wchar_t currentExe[MAX_PATH];
			GetModuleFileNameW(nullptr, currentExe, MAX_PATH);

			IUnknown* pEnumerator = nullptr;
			hr = pFwRules->get__NewEnum(&pEnumerator);
			if (SUCCEEDED(hr) && pEnumerator) {
				IEnumVARIANT* pVariant = nullptr;
				hr = pEnumerator->QueryInterface(__uuidof(IEnumVARIANT), reinterpret_cast<void**>(&pVariant));
				if (SUCCEEDED(hr) && pVariant) {
					VARIANT varRule;
					VariantInit(&varRule);
					while (pVariant->Next(1, &varRule, nullptr) == S_OK) {
						if (varRule.vt == VT_DISPATCH && varRule.pdispVal) {
							INetFwRule* pFwRule = nullptr;
							if (SUCCEEDED(varRule.pdispVal->QueryInterface(__uuidof(INetFwRule), reinterpret_cast<void**>(&pFwRule))) && pFwRule) {
								VARIANT_BOOL isEnabled = VARIANT_FALSE;
								NET_FW_RULE_DIRECTION dir = NET_FW_RULE_DIR_IN;
								NET_FW_ACTION action = NET_FW_ACTION_ALLOW;

								pFwRule->get_Enabled(&isEnabled);
								pFwRule->get_Direction(&dir);
								pFwRule->get_Action(&action);

								if (isEnabled == VARIANT_TRUE && dir == NET_FW_RULE_DIR_IN && action == NET_FW_ACTION_ALLOW) {
									BSTR appName = nullptr;
									pFwRule->get_ApplicationName(&appName);
									if (appName) {
										if (_wcsicmp(appName, currentExe) == 0) {
											allowed = true;
										}
										SysFreeString(appName);
									}

									if (!allowed) {
										BSTR localPorts = nullptr;
										pFwRule->get_LocalPorts(&localPorts);
										if (localPorts) {
											std::wstring portStr = std::to_wstring(port);
											if (wcsstr(localPorts, portStr.c_str()) != nullptr) {
												allowed = true;
											}
											SysFreeString(localPorts);
										}
									}
								}
								pFwRule->Release();
							}
						}
						VariantClear(&varRule);
						if (allowed) break;
					}
					pVariant->Release();
				}
				pEnumerator->Release();
			}
			pFwRules->Release();
		}
		pNetFwPolicy2->Release();
	}

	if (SUCCEEDED(hrCom)) {
		CoUninitialize();
	}

	return allowed;
}

inline bool RequestWindowsFirewallPortRule(wxWindow* parent, int port) {
	wchar_t exePath[MAX_PATH];
	GetModuleFileNameW(nullptr, exePath, MAX_PATH);

	const wxString params = wxString::Format(
		"advfirewall firewall add rule name=\"Mios Map Editor (Port %d)\" dir=in action=allow protocol=TCP localport=%d profile=any & "
		"netsh advfirewall firewall add rule name=\"Mios Map Editor\" dir=in action=allow program=\"%s\" enable=yes profile=any",
		port,
		port,
		wxString(exePath)
	);

	HINSTANCE result = ShellExecuteW(
		reinterpret_cast<HWND>(parent ? parent->GetHandle() : nullptr),
		L"runas",
		L"cmd.exe",
		(L"/c netsh " + params).wc_str(),
		nullptr,
		SW_HIDE
	);
	return reinterpret_cast<INT_PTR>(result) > 32;
}

inline bool EnsureWindowsFirewallAllowed(wxWindow* parent, int port) {
	// If already allowed (native COM query), return immediately without doing or showing anything.
	if (IsWindowsFirewallPortAllowed(port)) {
		return true;
	}

	// Request rule via standard Windows UAC prompt directly without intermediate popups.
	if (RequestWindowsFirewallPortRule(parent, port)) {
		wxMilliSleep(200);
		return true;
	}
	return false;
}
#else
inline bool IsWindowsFirewallPortAllowed(int port) { return true; }
inline bool RequestWindowsFirewallPortRule(wxWindow* parent, int port) { return true; }
inline bool EnsureWindowsFirewallAllowed(wxWindow* parent, int port) { return true; }
#endif

#endif // RME_FIREWALL_HELPER_H_
