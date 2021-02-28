#include "bindings.h"

void CreateOwnerImpl(RE::BSFixedString api_url, RE::BSFixedString api_key, RE::BSFixedString name, int32_t mod_version, RE::TESQuest* quest) {
	logger::info("Entered CreateOwnerImpl");
	if (!quest) {
		logger::error("CreateOwnerImpl quest is null!");
		return;
	}

	SKSE::RegistrationMap<int> successReg = SKSE::RegistrationMap<int>();
	successReg.Register(quest, RE::BSFixedString("OnCreateOwnerSuccess"));
	SKSE::RegistrationMap<bool, int, RE::BSFixedString, RE::BSFixedString, RE::BSFixedString> failReg = SKSE::RegistrationMap<bool, int, RE::BSFixedString, RE::BSFixedString, RE::BSFixedString>();
	failReg.Register(quest, RE::BSFixedString("OnCreateOwnerFail"));

	logger::info(FMT_STRING("CreateOwner api_url: {}, api_key: {}, name: {}, mod_version: {}"), api_url, api_key, name, mod_version);
	FFIResult<RawOwner> result = create_owner(api_url.c_str(), api_key.c_str(), name.c_str(), mod_version);
	if (result.IsOk()) {
		RawOwner owner = result.AsOk();
		logger::info(FMT_STRING("CreateOwner success: {}"), owner.id);
		successReg.SendEvent(owner.id);
	} else {
		FFIError error = result.AsErr();
		if (error.IsServer()) {
			FFIServerError server_error = error.AsServer();
			logger::error(FMT_STRING("CreateOwner server error: {} {} {}"), server_error.status, server_error.title, server_error.detail);
			failReg.SendEvent(true, server_error.status, RE::BSFixedString(server_error.title), RE::BSFixedString(server_error.detail), RE::BSFixedString());
		} else {
			const char* network_error = error.AsNetwork();
			logger::error(FMT_STRING("CreateOwner network error: {}"), network_error);
			failReg.SendEvent(false, 0, RE::BSFixedString(), RE::BSFixedString(), RE::BSFixedString(network_error));
		}
	}
	successReg.Unregister(quest);
	failReg.Unregister(quest);
}

bool CreateOwner(RE::StaticFunctionTag*, RE::BSFixedString api_url, RE::BSFixedString api_key, RE::BSFixedString name, int32_t mod_version, RE::TESQuest* quest) {
	logger::info("Entered CreateOwner");
	if (!quest) {
		logger::error("CreateOwner quest is null!");
		return false;
	}

	std::thread thread(CreateOwnerImpl, api_url, api_key, name, mod_version, quest);
	thread.detach();
	return true;
}

void UpdateOwnerImpl(RE::BSFixedString api_url, RE::BSFixedString api_key, int32_t id,  RE::BSFixedString name, int32_t mod_version, RE::TESQuest* quest) {
	logger::info("Entered UpdateOwnerImpl");
	if (!quest) {
		logger::error("UpdateOwnerImpl quest is null!");
		return;
	}

	SKSE::RegistrationMap<int> successReg = SKSE::RegistrationMap<int>();
	successReg.Register(quest, RE::BSFixedString("OnUpdateOwnerSuccess"));
	SKSE::RegistrationMap<bool, int, RE::BSFixedString, RE::BSFixedString, RE::BSFixedString> failReg = SKSE::RegistrationMap<bool, int, RE::BSFixedString, RE::BSFixedString, RE::BSFixedString>();
	failReg.Register(quest, RE::BSFixedString("OnUpdateOwnerFail"));

	logger::info(FMT_STRING("UpdateOwner api_url: {}, api_key: {}, name: {}, mod_version: {}"), api_url, api_key, name, mod_version);
	FFIResult<RawOwner> result = update_owner(api_url.c_str(), api_key.c_str(), id, name.c_str(), mod_version);
	if (result.IsOk()) {
		RawOwner owner = result.AsOk();
		logger::info(FMT_STRING("UpdateOwner success: {}"), owner.id);
		successReg.SendEvent(owner.id);
	} else {
		FFIError error = result.AsErr();
		if (error.IsServer()) {
			FFIServerError server_error = error.AsServer();
			logger::error(FMT_STRING("UpdateOwner server error: {} {} {}"), server_error.status, server_error.title, server_error.detail);
			failReg.SendEvent(true, server_error.status, RE::BSFixedString(server_error.title), RE::BSFixedString(server_error.detail), RE::BSFixedString());
		} else {
			const char* network_error = error.AsNetwork();
			logger::error(FMT_STRING("UpdateOwner network error: {}"), network_error);
			failReg.SendEvent(false, 0, RE::BSFixedString(), RE::BSFixedString(), RE::BSFixedString(network_error));
		}
	}
	successReg.Unregister(quest);
	failReg.Unregister(quest);
}

bool UpdateOwner(RE::StaticFunctionTag*, RE::BSFixedString api_url, RE::BSFixedString api_key, int32_t id, RE::BSFixedString name, int32_t mod_version, RE::TESQuest* quest) {
	logger::info("Entered UpdateOwner");
	if (!quest) {
		logger::error("UpdateOwner quest is null!");
		return false;
	}

	std::thread thread(UpdateOwnerImpl, api_url, api_key, id, name, mod_version, quest);
	thread.detach();
	return true;
}
