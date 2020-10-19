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
	SKSE::RegistrationMap<RE::BSFixedString> failReg = SKSE::RegistrationMap<RE::BSFixedString>();
	failReg.Register(quest, RE::BSFixedString("OnStatusCheckFail"));

	logger::info(FMT_STRING("StatusCheck api_url: {}"), api_url);
	FFIResult<bool> result = status_check(api_url.c_str());
	if (result.IsOk()) {
		bool success = result.AsOk();
		logger::info(FMT_STRING("StatusCheck success: {}"), success);
		successReg.SendEvent(success);
	} else {
		const char* error = result.AsErr();
		logger::error(FMT_STRING("StatusCheck failure: {}"), error);
		failReg.SendEvent(RE::BSFixedString(error));
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
