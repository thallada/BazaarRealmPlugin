#include "bindings.h"

int CreateOwnerImpl(RE::BSFixedString api_url, RE::BSFixedString api_key, RE::BSFixedString name, uint32_t mod_version, RE::TESQuest* quest)
{
	logger::info("Entered CreateOwnerImpl");
	if (!quest) {
		logger::error("CreateOwner quest is null!");
		return -1;
	}

	SKSE::RegistrationMap<int> regMap = SKSE::RegistrationMap<int>();
	regMap.Register(quest, RE::BSFixedString("OnCreateOwner"));

	logger::info(FMT_STRING("CreateOwner api_url: {}"), api_url);
	logger::info(FMT_STRING("CreateOwner api_key: {}"), api_key);
	logger::info(FMT_STRING("CreateOwner name: {}"), name);
	logger::info(FMT_STRING("CreateOwner mod_version: {}"), mod_version);
	int owner_id = create_owner(api_url.c_str(), api_key.c_str(), name.c_str(), mod_version);
	logger::info(FMT_STRING("CreateOwner result: {}"), owner_id);
	regMap.SendEvent(owner_id);
	regMap.Unregister(quest);
	return owner_id;
}

bool CreateOwner(RE::StaticFunctionTag*, RE::BSFixedString api_url, RE::BSFixedString api_key, RE::BSFixedString name, uint32_t mod_version, RE::TESQuest* quest) {
	logger::info("Entered CreateOwner");
	if (!quest) {
		logger::error("CreateOwner quest is null!");
		return false;
	}

	std::thread thread(CreateOwnerImpl, api_url, api_key, name, mod_version, quest);
	thread.detach();
	return true;
}
