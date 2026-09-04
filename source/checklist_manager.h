#ifndef RME_CHECKLIST_MANAGER_H_
#define RME_CHECKLIST_MANAGER_H_

#include <cstdint>
#include <string>
#include <vector>
#include <mutex>
#include <functional>

struct ChecklistItem {
	uint32_t id = 0;
	std::string text;
	std::string author;
	bool completed = false;
	uint64_t timestamp = 0;
};

class ChecklistManager {
public:
	static ChecklistManager& getInstance();

	ChecklistManager();
	~ChecklistManager() = default;

	// Core operations
	uint32_t addItem(const std::string& text, const std::string& author, bool completed = false, uint32_t forcedId = 0);
	bool toggleItem(uint32_t id, bool completed);
	bool deleteItem(uint32_t id);
	void clearCompleted();
	void clearAll();

	// Query
	std::vector<ChecklistItem> getActiveItems() const;
	std::vector<ChecklistItem> getCompletedItems() const;
	std::vector<ChecklistItem> getAllItems() const;
	size_t getActiveCount() const;
	size_t getCompletedCount() const;
	size_t getTotalCount() const;

	// Bulk replace (e.g. from network full sync or load)
	void setAllItems(const std::vector<ChecklistItem>& items);

	// Persistence to file (.notes or config)
	void saveToFile(const std::string& filepath);
	void loadFromFile(const std::string& filepath);

	// Callback on change
	void setChangeCallback(std::function<void()> cb) { onChangeCallback = cb; }

private:
	void notifyChanged();

	mutable std::mutex itemsMutex;
	std::vector<ChecklistItem> items;
	uint32_t nextId = 1;
	std::function<void()> onChangeCallback;
};

#endif // RME_CHECKLIST_MANAGER_H_
