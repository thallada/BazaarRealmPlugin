#include "bindings.h"

void Init(RE::StaticFunctionTag*)
{
	logger::info("Entered Init");
	init();
	logger::info("Init done");
}

std::string GenerateApiKey(RE::StaticFunctionTag*)
{
	logger::info("Entered GenerateApiKey");
	char *api_key = generate_api_key();
	logger::info(FMT_STRING("GenerateApiKey api_key: {}"), api_key);
	return api_key;
}

bool StatusCheckImpl(RE::BSFixedString api_url, RE::TESQuest* quest)
{
	logger::info("Entered StatusCheckImpl");
	if (!quest) {
		logger::error("StatusCheck quest is null!");
		return false;
	}


	SKSE::RegistrationMap<bool> regMap = SKSE::RegistrationMap<bool>();
	regMap.Register(quest, RE::BSFixedString("OnStatusCheck"));

	logger::info(FMT_STRING("StatusCheck api_url: {}"), api_url);
	bool result = status_check(api_url.c_str());
	logger::info(FMT_STRING("StatusCheck result: {}"), result ? "true" : "false");
	regMap.SendEvent(result);
	regMap.Unregister(quest);
	return result;
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
