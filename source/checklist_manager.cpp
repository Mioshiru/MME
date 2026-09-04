#include "checklist_manager.h"
#include <algorithm>
#include <fstream>
#include <sstream>
#include <chrono>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

ChecklistManager& ChecklistManager::getInstance() {
	static ChecklistManager s_instance;
	return s_instance;
}

ChecklistManager::ChecklistManager() {
	// Sample initial task if empty
}

uint32_t ChecklistManager::addItem(const std::string& text, const std::string& author, bool completed, uint32_t forcedId) {
	std::lock_guard<std::mutex> lock(itemsMutex);
	if (text.empty()) return 0;

	ChecklistItem item;
	if (forcedId > 0) {
		item.id = forcedId;
		if (forcedId >= nextId) {
			nextId = forcedId + 1;
		}
	} else {
		item.id = nextId++;
	}

	item.text = text;
	item.author = author.empty() ? "Mapper" : author;
	item.completed = completed;
	item.timestamp = (uint64_t)std::chrono::duration_cast<std::chrono::seconds>(
		std::chrono::system_clock::now().time_since_epoch()).count();

	items.push_back(item);
	notifyChanged();
	return item.id;
}

bool ChecklistManager::toggleItem(uint32_t id, bool completed) {
	std::lock_guard<std::mutex> lock(itemsMutex);
	for (auto& item : items) {
		if (item.id == id) {
			item.completed = completed;
			notifyChanged();
			return true;
		}
	}
	return false;
}

bool ChecklistManager::deleteItem(uint32_t id) {
	std::lock_guard<std::mutex> lock(itemsMutex);
	auto it = std::remove_if(items.begin(), items.end(), [id](const ChecklistItem& item) {
		return item.id == id;
	});
	if (it != items.end()) {
		items.erase(it, items.end());
		notifyChanged();
		return true;
	}
	return false;
}

void ChecklistManager::clearCompleted() {
	std::lock_guard<std::mutex> lock(itemsMutex);
	auto it = std::remove_if(items.begin(), items.end(), [](const ChecklistItem& item) {
		return item.completed;
	});
	if (it != items.end()) {
		items.erase(it, items.end());
		notifyChanged();
	}
}

void ChecklistManager::clearAll() {
	std::lock_guard<std::mutex> lock(itemsMutex);
	items.clear();
	notifyChanged();
}

std::vector<ChecklistItem> ChecklistManager::getActiveItems() const {
	std::lock_guard<std::mutex> lock(itemsMutex);
	std::vector<ChecklistItem> active;
	for (const auto& item : items) {
		if (!item.completed) {
			active.push_back(item);
		}
	}
	return active;
}

std::vector<ChecklistItem> ChecklistManager::getCompletedItems() const {
	std::lock_guard<std::mutex> lock(itemsMutex);
	std::vector<ChecklistItem> completed;
	for (const auto& item : items) {
		if (item.completed) {
			completed.push_back(item);
		}
	}
	return completed;
}

std::vector<ChecklistItem> ChecklistManager::getAllItems() const {
	std::lock_guard<std::mutex> lock(itemsMutex);
	return items;
}

size_t ChecklistManager::getActiveCount() const {
	std::lock_guard<std::mutex> lock(itemsMutex);
	size_t count = 0;
	for (const auto& item : items) {
		if (!item.completed) count++;
	}
	return count;
}

size_t ChecklistManager::getCompletedCount() const {
	std::lock_guard<std::mutex> lock(itemsMutex);
	size_t count = 0;
	for (const auto& item : items) {
		if (item.completed) count++;
	}
	return count;
}

size_t ChecklistManager::getTotalCount() const {
	std::lock_guard<std::mutex> lock(itemsMutex);
	return items.size();
}

void ChecklistManager::setAllItems(const std::vector<ChecklistItem>& newItems) {
	{
		std::lock_guard<std::mutex> lock(itemsMutex);
		items = newItems;
		uint32_t maxId = 0;
		for (const auto& item : items) {
			if (item.id > maxId) maxId = item.id;
		}
		nextId = maxId + 1;
	}
	notifyChanged();
}

void ChecklistManager::saveToFile(const std::string& filepath) {
	std::lock_guard<std::mutex> lock(itemsMutex);
	json j = json::array();
	for (const auto& item : items) {
		json itemJson;
		itemJson["id"] = item.id;
		itemJson["text"] = item.text;
		itemJson["author"] = item.author;
		itemJson["completed"] = item.completed;
		itemJson["timestamp"] = item.timestamp;
		j.push_back(itemJson);
	}

	std::ofstream file(filepath);
	if (file.is_open()) {
		file << j.dump(2);
		file.close();
	}
}

void ChecklistManager::loadFromFile(const std::string& filepath) {
	std::ifstream file(filepath);
	if (!file.is_open()) return;

	try {
		json j;
		file >> j;
		file.close();

		std::vector<ChecklistItem> loadedItems;
		uint32_t maxId = 0;
		for (const auto& itemJson : j) {
			ChecklistItem item;
			item.id = itemJson.value("id", 0u);
			item.text = itemJson.value("text", "");
			item.author = itemJson.value("author", "");
			item.completed = itemJson.value("completed", false);
			item.timestamp = itemJson.value("timestamp", 0ull);
			if (item.id > maxId) maxId = item.id;
			if (!item.text.empty()) {
				loadedItems.push_back(item);
			}
		}

		{
			std::lock_guard<std::mutex> lock(itemsMutex);
			items = loadedItems;
			nextId = maxId + 1;
		}
		notifyChanged();
	} catch (...) {
		// Ignore parse errors on corrupt file
	}
}

void ChecklistManager::notifyChanged() {
	if (onChangeCallback) {
		onChangeCallback();
	}
}
