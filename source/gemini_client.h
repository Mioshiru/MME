#ifndef RME_GEMINI_CLIENT_H_
#define RME_GEMINI_CLIENT_H_

#include <string>
#include <functional>
#include <nlohmann/json.hpp>
#include <cpr/cpr.h>
#include "settings.h"

class GeminiClient {
public:
	static GeminiClient& Get() {
		static GeminiClient instance;
		return instance;
	}

	std::string GetApiKey() const {
		return g_settings.getString(Config::GEMINI_API_KEY);
	}

	void SetApiKey(const std::string& key) {
		g_settings.setString(Config::GEMINI_API_KEY, key);
	}

	bool HasApiKey() const {
		std::string k = GetApiKey();
		return !k.empty();
	}

	// Sends a prompt to Gemini 1.5 Flash (or gemini-2.0-flash / gemini-1.5-flash free endpoint)
	// Returns true if successful and populates responseText.
	bool GenerateContent(const std::string& prompt, const std::string& systemInstruction, std::string& responseText, std::string& errorMsg) {
		std::string apiKey = GetApiKey();
		if (apiKey.empty()) {
			errorMsg = "No Gemini API Key provided. Please enter a valid Gemini Flash API Key in Settings or Wizard.";
			return false;
		}

		try {
			// Construct JSON body
			nlohmann::json root;
			
			if (!systemInstruction.empty()) {
				root["systemInstruction"] = {
					{ "parts", { { { "text", systemInstruction } } } }
				};
			}

			root["contents"] = nlohmann::json::array({
				{
					{ "parts", { { { "text", prompt } } } }
				}
			});

			root["generationConfig"] = {
				{ "temperature", 0.7 },
				{ "maxOutputTokens", 2048 }
			};

			std::string url = "https://generativelanguage.googleapis.com/v1beta/models/gemini-1.5-flash:generateContent?key=" + apiKey;

			cpr::Response r = cpr::Post(
				cpr::Url{ url },
				cpr::Header{ { "Content-Type", "application/json" } },
				cpr::Body{ root.dump() },
				cpr::Timeout{ std::chrono::seconds(15) }
			);

			if (r.status_code != 200) {
				try {
					auto errJson = nlohmann::json::parse(r.text);
					if (errJson.contains("error") && errJson["error"].contains("message")) {
						errorMsg = "API Error (" + std::to_string(r.status_code) + "): " + errJson["error"]["message"].get<std::string>();
					} else {
						errorMsg = "HTTP Error " + std::to_string(r.status_code) + ": " + r.text;
					}
				} catch (...) {
					errorMsg = "HTTP Error " + std::to_string(r.status_code) + ": " + r.error.message;
				}
				return false;
			}

			auto respJson = nlohmann::json::parse(r.text);
			if (respJson.contains("candidates") && !respJson["candidates"].empty()) {
				auto& firstCand = respJson["candidates"][0];
				if (firstCand.contains("content") && firstCand["content"].contains("parts") && !firstCand["content"]["parts"].empty()) {
					responseText = firstCand["content"]["parts"][0]["text"].get<std::string>();
					return true;
				}
			}

			errorMsg = "Empty or unparseable response from Gemini Flash.";
			return false;
		} catch (const std::exception& ex) {
			errorMsg = std::string("Exception: ") + ex.what();
			return false;
		}
	}

private:
	GeminiClient() = default;
	~GeminiClient() = default;
};

#endif // RME_GEMINI_CLIENT_H_
