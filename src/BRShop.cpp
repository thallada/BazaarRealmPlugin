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

	logger::info(FMT_STRING("GetShop api_url: {}, api_key: {}, id: {}"), api_url, api_key, id);
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

void ListShopsImpl(RE::BSFixedString api_url, RE::BSFixedString api_key, RE::TESQuest* quest) {
	logger::info("Entered ListShopsImpl");
	if (!quest) {
		logger::error("ListShopsImpl quest is null!");
		return;
	}
	
	// Since Papyrus arrays are single-type, the array of shops from the API needs to be decomposed into multiple arrays for each shop field
	SKSE::RegistrationMap<std::vector<int>, std::vector<RE::BSFixedString>, std::vector<RE::BSFixedString>> successReg = SKSE::RegistrationMap<std::vector<int>, std::vector<RE::BSFixedString>, std::vector<RE::BSFixedString>>();
	successReg.Register(quest, RE::BSFixedString("OnListShopsSuccess"));
	SKSE::RegistrationMap<RE::BSFixedString> failReg = SKSE::RegistrationMap<RE::BSFixedString>();
	failReg.Register(quest, RE::BSFixedString("OnListShopsFail"));

	logger::info(FMT_STRING("ListShops api_url: {}, api_key: {}"), api_url, api_key);
	FFIResult<RawShopVec> result = list_shops(api_url.c_str(), api_key.c_str());

	if (result.IsOk()) {
		RawShopVec vec = result.AsOk();
		logger::info(FMT_STRING("ListShops success vec len: {:d}, cap: {:d}"), vec.len, vec.cap);
		std::vector<int> id_vec = std::vector<int>();
		std::vector<RE::BSFixedString> name_vec = std::vector<RE::BSFixedString>();
		std::vector<RE::BSFixedString> description_vec = std::vector<RE::BSFixedString>();
		for (int i = 0; i < vec.len; i++) {
			RawShop shop = vec.ptr[i];
			id_vec.push_back(shop.id);
			name_vec.push_back(RE::BSFixedString(shop.name));
			description_vec.push_back(RE::BSFixedString(shop.description));
		}
		successReg.SendEvent(id_vec, name_vec, description_vec);
	} else {
		const char* error = result.AsErr();
		logger::error(FMT_STRING("ListShops failure: {}"), error);
		failReg.SendEvent(RE::BSFixedString(error));
	}
	successReg.Unregister(quest);
	failReg.Unregister(quest);
}

bool ListShops(RE::StaticFunctionTag*, RE::BSFixedString api_url, RE::BSFixedString api_key, RE::TESQuest* quest) {
	logger::info("Entered ListShops");
	if (!quest) {
		logger::error("ListShops quest is null!");
		return false;
	}

	std::thread thread(ListShopsImpl, api_url, api_key, quest);
	thread.detach();
	return true;
}
