#include "bindings.h"

void CreateShopImpl(RE::BSFixedString api_url, RE::BSFixedString api_key, RE::BSFixedString name, RE::BSFixedString description, RE::TESQuest* quest) {
	logger::info("Entered CreateShopImpl");
	if (!quest) {
		logger::error("CreateShopImpl quest is null!");
		return;
	}

	SKSE::RegistrationMap<int, RE::BSFixedString, RE::BSFixedString> successReg = SKSE::RegistrationMap<int, RE::BSFixedString, RE::BSFixedString>();
	successReg.Register(quest, RE::BSFixedString("OnCreateShopSuccess"));
	SKSE::RegistrationMap<RE::BSFixedString> failReg = SKSE::RegistrationMap<RE::BSFixedString>();
	failReg.Register(quest, RE::BSFixedString("OnCreateShopFail"));

	logger::info(FMT_STRING("CreateShop api_url: {}, api_key: {}, name: {}, description: {}"), api_url, api_key, name, description);
	FFIResult<RawShop> result = create_shop(api_url.c_str(), api_key.c_str(), name.c_str(), description.c_str());
	if (result.IsOk()) {
		RawShop shop = result.AsOk();
		logger::info(FMT_STRING("CreateShop success: {}"), shop.id);
		successReg.SendEvent(shop.id, RE::BSFixedString(shop.name), RE::BSFixedString(shop.description));
	} else {
		const char* error = result.AsErr();
		logger::error(FMT_STRING("CreateShop failure: {}"), error);
		failReg.SendEvent(RE::BSFixedString(error));
	}
	successReg.Unregister(quest);
	failReg.Unregister(quest);
}

bool CreateShop(RE::StaticFunctionTag*, RE::BSFixedString api_url, RE::BSFixedString api_key, RE::BSFixedString name, RE::BSFixedString description, RE::TESQuest* quest) {
	logger::info("Entered CreateShop");
	if (!quest) {
		logger::error("CreateShop quest is null!");
		return false;
	}

	std::thread thread(CreateShopImpl, api_url, api_key, name, description, quest);
	thread.detach();
	return true;
}

void UpdateShopImpl(RE::BSFixedString api_url, RE::BSFixedString api_key, uint32_t id, RE::BSFixedString name, RE::BSFixedString description, RE::TESQuest* quest) {
	logger::info("Entered UpdateShopImpl");
	if (!quest) {
		logger::error("UpdateShopImpl quest is null!");
		return;
	}

	SKSE::RegistrationMap<int, RE::BSFixedString, RE::BSFixedString> successReg = SKSE::RegistrationMap<int, RE::BSFixedString, RE::BSFixedString>();
	successReg.Register(quest, RE::BSFixedString("OnUpdateShopSuccess"));
	SKSE::RegistrationMap<RE::BSFixedString> failReg = SKSE::RegistrationMap<RE::BSFixedString>();
	failReg.Register(quest, RE::BSFixedString("OnUpdateShopFail"));

	logger::info(FMT_STRING("UpdateShop api_url: {}, api_key: {}, name: {}, description: {}"), api_url, api_key, name, description);
	FFIResult<RawShop> result = update_shop(api_url.c_str(), api_key.c_str(), id, name.c_str(), description.c_str());
	if (result.IsOk()) {
		RawShop shop = result.AsOk();
		logger::info(FMT_STRING("UpdateShop success: {}"), shop.id);
		successReg.SendEvent(shop.id, RE::BSFixedString(shop.name), RE::BSFixedString(shop.description));
	} else {
		const char* error = result.AsErr();
		logger::error(FMT_STRING("UpdateShop failure: {}"), error);
		failReg.SendEvent(RE::BSFixedString(error));
	}
	successReg.Unregister(quest);
	failReg.Unregister(quest);
}

bool UpdateShop(RE::StaticFunctionTag*, RE::BSFixedString api_url, RE::BSFixedString api_key, uint32_t id, RE::BSFixedString name, RE::BSFixedString description, RE::TESQuest* quest) {
	logger::info("Entered UpdateShop");
	if (!quest) {
		logger::error("UpdateShop quest is null!");
		return false;
	}

	std::thread thread(UpdateShopImpl, api_url, api_key, id, name, description, quest);
	thread.detach();
	return true;
}

void GetShopImpl(RE::BSFixedString api_url, RE::BSFixedString api_key, uint32_t id, RE::TESQuest* quest) {
	logger::info("Entered GetShopImpl");
	if (!quest) {
		logger::error("GetShopImpl quest is null!");
		return;
	}

	SKSE::RegistrationMap<int, RE::BSFixedString, RE::BSFixedString> successReg = SKSE::RegistrationMap<int, RE::BSFixedString, RE::BSFixedString>();
	successReg.Register(quest, RE::BSFixedString("OnGetShopSuccess"));
	SKSE::RegistrationMap<RE::BSFixedString> failReg = SKSE::RegistrationMap<RE::BSFixedString>();
	failReg.Register(quest, RE::BSFixedString("OnGetShopFail"));

	logger::info(FMT_STRING("GetShop api_url: {}"), api_url);
	logger::info(FMT_STRING("GetShop api_key: {}"), api_key);
	FFIResult<RawShop> result = get_shop(api_url.c_str(), api_key.c_str(), id);
	if (result.IsOk()) {
		RawShop shop = result.AsOk();
		logger::info(FMT_STRING("GetShop success: {}"), shop.id);
		successReg.SendEvent(shop.id, RE::BSFixedString(shop.name), RE::BSFixedString(shop.description));
	} else {
		const char* error = result.AsErr();
		logger::error(FMT_STRING("GetShop failure: {}"), error);
		failReg.SendEvent(RE::BSFixedString(error));
	}
	successReg.Unregister(quest);
	failReg.Unregister(quest);
}

bool GetShop(RE::StaticFunctionTag*, RE::BSFixedString api_url, RE::BSFixedString api_key, uint32_t id, RE::TESQuest* quest) {
	logger::info("Entered GetShop");
	if (!quest) {
		logger::error("GetShop quest is null!");
		return false;
	}

	std::thread thread(GetShopImpl, api_url, api_key, id, quest);
	thread.detach();
	return true;
}
