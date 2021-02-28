#include "bindings.h"

bool Init(RE::StaticFunctionTag*) {
	logger::info("Entered Init");
	bool result = init();
	if (result) {
		logger::info("Init successful");
		return true;
	} else {
		logger::error("Init failed");
		return false;
	}
}

std::string GenerateApiKey(RE::StaticFunctionTag*) {
	logger::info("Entered GenerateApiKey");
	char *api_key = generate_api_key();
	logger::info(FMT_STRING("GenerateApiKey api_key: {}"), api_key);
	return api_key;
}

void StatusCheckImpl(RE::BSFixedString api_url, RE::TESQuest* quest) {
	logger::info("Entered StatusCheckImpl");
	if (!quest) {
		logger::error("StatusCheck quest is null!");
		return;
	}

	SKSE::RegistrationMap<bool> successReg = SKSE::RegistrationMap<bool>();
	successReg.Register(quest, RE::BSFixedString("OnStatusCheckSuccess"));
	SKSE::RegistrationMap<bool, int, RE::BSFixedString, RE::BSFixedString, RE::BSFixedString> failReg = SKSE::RegistrationMap<bool, int, RE::BSFixedString, RE::BSFixedString, RE::BSFixedString>();
	failReg.Register(quest, RE::BSFixedString("OnStatusCheckFail"));

	logger::info(FMT_STRING("StatusCheck api_url: {}"), api_url);
	FFIResult<bool> result = status_check(api_url.c_str());
	if (result.IsOk()) {
		bool success = result.AsOk();
		logger::info(FMT_STRING("StatusCheck success: {}"), success);
		successReg.SendEvent(success);
	} else {
		FFIError error = result.AsErr();
		if (error.IsServer()) {
			FFIServerError server_error = error.AsServer();
			logger::error(FMT_STRING("StatusCheck server error: {} {} {}"), server_error.status, server_error.title, server_error.detail);
			failReg.SendEvent(true, server_error.status, RE::BSFixedString(server_error.title), RE::BSFixedString(server_error.detail), RE::BSFixedString());
		} else {
			const char* network_error = error.AsNetwork();
			logger::error(FMT_STRING("StatusCheck network error: {}"), network_error);
			failReg.SendEvent(false, 0, RE::BSFixedString(), RE::BSFixedString(), RE::BSFixedString(network_error));
		}
	}
	successReg.Unregister(quest);
	failReg.Unregister(quest);
	return;
}

bool StatusCheck(RE::StaticFunctionTag*, RE::BSFixedString api_url, RE::TESQuest* quest) {
	logger::info("Entered StatusCheck");
	if (!quest) {
		logger::error("StatusCheck quest is null!");
		return false;
	}

	std::thread thread(StatusCheckImpl, api_url, quest);
	thread.detach();
	return true;
}
