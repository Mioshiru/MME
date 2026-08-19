#ifndef RME_MME_UPDATER_H_
#define RME_MME_UPDATER_H_

#include "main.h"
#include <string>
#include <wx/window.h>

class MMEUpdater {
public:
	static MMEUpdater& Instance();

	void CheckForUpdates(wxWindow* parent, bool user_initiated = true);
	void CheckForUpdatesAsync(wxWindow* parent);

	bool IsUpdateAvailable() const { return update_available; }
	std::string GetLatestVersion() const { return latest_tag; }
	std::string GetCurrentVersion() const;

private:
	MMEUpdater();
	~MMEUpdater() = default;

	bool PerformCheck(std::string& out_tag, std::string& out_url, std::string& out_zip_url, std::string& out_notes);
	bool DownloadAndInstall(wxWindow* parent, const std::string& zip_url, const std::string& tag);

	bool update_available;
	std::string latest_tag;
	std::string latest_url;
	std::string latest_zip_url;
};

#endif // RME_MME_UPDATER_H_
