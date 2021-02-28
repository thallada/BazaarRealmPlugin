#include "bindings.h"
#include "FormIds.h"

void CreateShopImpl(RE::BSFixedString api_url, RE::BSFixedString api_key, RE::BSFixedString name, RE::BSFixedString description, RE::TESQuest* quest) {
	logger::info("Entered CreateShopImpl");
	if (!quest) {
		logger::error("CreateShopImpl quest is null!");
		return;
	}

	SKSE::RegistrationMap<int, RE::BSFixedString, RE::BSFixedString, int, RE::BSFixedString, std::vector<RE::BGSKeyword*>, bool> successReg =
		SKSE::RegistrationMap<int, RE::BSFixedString, RE::BSFixedString, int, RE::BSFixedString, std::vector<RE::BGSKeyword*>, bool>();
	successReg.Register(quest, RE::BSFixedString("OnCreateShopSuccess"));
	SKSE::RegistrationMap<bool, int, RE::BSFixedString, RE::BSFixedString, RE::BSFixedString> failReg = SKSE::RegistrationMap<bool, int, RE::BSFixedString, RE::BSFixedString, RE::BSFixedString>();
	failReg.Register(quest, RE::BSFixedString("OnCreateShopFail"));

	logger::info(FMT_STRING("CreateShop api_url: {}, api_key: {}, name: {}, description: {}"), api_url, api_key, name, description);
	FFIResult<RawShop> result = create_shop(api_url.c_str(), api_key.c_str(), name.c_str(), description.c_str());
	if (result.IsOk()) {
		RawShop shop = result.AsOk();
		logger::info(FMT_STRING("CreateShop success: {}"), shop.id);
		std::vector<RE::BGSKeyword*> keyword_forms;
		for (int i = 0; i < shop.vendor_keywords_len; i++) {
			RE::BSFixedString keyword = shop.vendor_keywords[i];
			logger::info(FMT_STRING("CreateShop keyword {:d}: {}"), i, keyword);

			RE::TESForm* form = RE::TESForm::LookupByEditorID(keyword);
			if (!form) { // form is not found, might be in an uninstalled mod
				logger::warn(FMT_STRING("CreateShop could not find keyword form for: {}"), keyword);
				continue;
			}
			RE::BGSKeyword* keyword_form = static_cast<RE::BGSKeyword*>(form);
			if (!keyword_form) {
				logger::warn(FMT_STRING("CreateShop could cast form to keyword with id {:x} for: {}"), (uint32_t)form->GetFormID(), keyword);
				continue;
			}
			keyword_forms.push_back(keyword_form);
		}
		successReg.SendEvent(shop.id, RE::BSFixedString(shop.name), RE::BSFixedString(shop.description), shop.gold, RE::BSFixedString(shop.shop_type), keyword_forms, shop.vendor_keywords_exclude);
	} else {
		FFIError error = result.AsErr();
		if (error.IsServer()) {
			FFIServerError server_error = error.AsServer();
			logger::error(FMT_STRING("CreateShop server error: {} {} {}"), server_error.status, server_error.title, server_error.detail);
			failReg.SendEvent(true, server_error.status, RE::BSFixedString(server_error.title), RE::BSFixedString(server_error.detail), RE::BSFixedString());
		} else {
			const char* network_error = error.AsNetwork();
			logger::error(FMT_STRING("CreateShop network error: {}"), network_error);
			failReg.SendEvent(false, 0, RE::BSFixedString(), RE::BSFixedString(), RE::BSFixedString(network_error));
		}
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

void UpdateShopImpl(
	RE::BSFixedString api_url,
	RE::BSFixedString api_key,
	int32_t id,
	RE::BSFixedString name,
	RE::BSFixedString description,
	int gold,
	RE::BSFixedString shop_type,
	std::vector<RE::BGSKeyword*> keywords,
	bool keywords_exclude,
	RE::TESQuest* quest
) {
	logger::info("Entered UpdateShopImpl");
	if (!quest) {
		logger::error("UpdateShopImpl quest is null!");
		return;
	}

	SKSE::RegistrationMap<int, RE::BSFixedString, RE::BSFixedString, int, RE::BSFixedString, std::vector<RE::BGSKeyword*>, bool> successReg =
		SKSE::RegistrationMap<int, RE::BSFixedString, RE::BSFixedString, int, RE::BSFixedString, std::vector<RE::BGSKeyword*>, bool>();
	successReg.Register(quest, RE::BSFixedString("OnUpdateShopSuccess"));
	SKSE::RegistrationMap<bool, int, RE::BSFixedString, RE::BSFixedString, RE::BSFixedString> failReg = SKSE::RegistrationMap<bool, int, RE::BSFixedString, RE::BSFixedString, RE::BSFixedString>();
	failReg.Register(quest, RE::BSFixedString("OnUpdateShopFail"));

	std::vector<const char*> keyword_strings;
	for (auto keyword = keywords.begin(); keyword != keywords.end(); ++keyword) {
		keyword_strings.push_back((*keyword)->GetFormEditorID());
	}

	logger::info(FMT_STRING("UpdateShop api_url: {}, api_key: {}, name: {}, description: {}, gold: {}, shop_type: {}, keywords.size(): {}, keywords_exclude: {}"), api_url, api_key, name, description, gold, shop_type, keywords.size(), keywords_exclude);
	FFIResult<RawShop> result = update_shop(api_url.c_str(), api_key.c_str(), id, name.c_str(), description.c_str(), gold, shop_type.c_str(), &keyword_strings[0], keyword_strings.size(), keywords_exclude);
	if (result.IsOk()) {
		RawShop shop = result.AsOk();
		logger::info(FMT_STRING("UpdateShop success: {}"), shop.id);
		std::vector<RE::BGSKeyword*> keyword_forms;
		for (int i = 0; i < shop.vendor_keywords_len; i++) {
			RE::BSFixedString keyword = shop.vendor_keywords[i];
			logger::info(FMT_STRING("UpdateShop keyword {:d}: {}"), i, keyword);

			RE::TESForm* form = RE::TESForm::LookupByEditorID(keyword);
			if (!form) { // form is not found, might be in an uninstalled mod
				logger::warn(FMT_STRING("UpdateShop could not find keyword form for: {}"), keyword);
				continue;
			}
			RE::BGSKeyword* keyword_form = static_cast<RE::BGSKeyword*>(form);
			if (!keyword_form) {
				logger::warn(FMT_STRING("UpdateShop could cast form to keyword with id {:x} for: {}"), (uint32_t)form->GetFormID(), keyword);
				continue;
			}
			keyword_forms.push_back(keyword_form);
		}
		successReg.SendEvent(shop.id, RE::BSFixedString(shop.name), RE::BSFixedString(shop.description), shop.gold, RE::BSFixedString(shop.shop_type), keyword_forms, shop.vendor_keywords_exclude);
	} else {
		FFIError error = result.AsErr();
		if (error.IsServer()) {
			FFIServerError server_error = error.AsServer();
			logger::error(FMT_STRING("UpdateShop server error: {} {} {}"), server_error.status, server_error.title, server_error.detail);
			failReg.SendEvent(true, server_error.status, RE::BSFixedString(server_error.title), RE::BSFixedString(server_error.detail), RE::BSFixedString());
		} else {
			const char* network_error = error.AsNetwork();
			logger::error(FMT_STRING("UpdateShop network error: {}"), network_error);
			failReg.SendEvent(false, 0, RE::BSFixedString(), RE::BSFixedString(), RE::BSFixedString(network_error));
		}
	}
	successReg.Unregister(quest);
	failReg.Unregister(quest);
}

bool UpdateShop(
	RE::StaticFunctionTag*,
	RE::BSFixedString api_url,
	RE::BSFixedString api_key,
	int32_t id,
	RE::BSFixedString name,
	RE::BSFixedString description,
	int gold,
	RE::BSFixedString shop_type,
	std::vector<RE::BGSKeyword*> keywords,
	bool keywords_exclude,
	RE::TESQuest* quest
) {
	logger::info("Entered UpdateShop");
	if (!quest) {
		logger::error("UpdateShop quest is null!");
		return false;
	}

	std::thread thread(UpdateShopImpl, api_url, api_key, id, name, description, gold, shop_type, keywords, keywords_exclude, quest);
	thread.detach();
	return true;
}

void GetShopImpl(RE::BSFixedString api_url, RE::BSFixedString api_key, int32_t id, RE::TESQuest* quest) {
	logger::info("Entered GetShopImpl");
	if (!quest) {
		logger::error("GetShopImpl quest is null!");
		return;
	}

	SKSE::RegistrationMap<int, RE::BSFixedString, RE::BSFixedString, int, RE::BSFixedString, std::vector<RE::BGSKeyword*>, bool> successReg =
		SKSE::RegistrationMap<int, RE::BSFixedString, RE::BSFixedString, int, RE::BSFixedString, std::vector<RE::BGSKeyword*>, bool>();
	successReg.Register(quest, RE::BSFixedString("OnGetShopSuccess"));
	SKSE::RegistrationMap<bool, int, RE::BSFixedString, RE::BSFixedString, RE::BSFixedString> failReg = SKSE::RegistrationMap<bool, int, RE::BSFixedString, RE::BSFixedString, RE::BSFixedString>();
	failReg.Register(quest, RE::BSFixedString("OnGetShopFail"));

	logger::info(FMT_STRING("GetShop api_url: {}, api_key: {}, id: {}"), api_url, api_key, id);
	FFIResult<RawShop> result = get_shop(api_url.c_str(), api_key.c_str(), id);
	if (result.IsOk()) {
		RawShop shop = result.AsOk();
		logger::info(FMT_STRING("GetShop success: {}"), shop.id);
		std::vector<RE::BGSKeyword*> keyword_forms;
		for (int i = 0; i < shop.vendor_keywords_len; i++) {
			RE::BSFixedString keyword = shop.vendor_keywords[i];
			logger::info(FMT_STRING("GetShop keyword {:d}: {}"), i, keyword);

			RE::TESForm* form = RE::TESForm::LookupByEditorID(keyword);
			if (!form) { // form is not found, might be in an uninstalled mod
				logger::warn(FMT_STRING("GetShop could not find keyword form for: {}"), keyword);
				continue;
			}
			RE::BGSKeyword* keyword_form = static_cast<RE::BGSKeyword*>(form);
			if (!keyword_form) {
				logger::warn(FMT_STRING("GetShop could cast form to keyword with id {:x} for: {}"), (uint32_t)form->GetFormID(), keyword);
				continue;
			}
			keyword_forms.push_back(keyword_form);
		}
		successReg.SendEvent(shop.id, RE::BSFixedString(shop.name), RE::BSFixedString(shop.description), shop.gold, RE::BSFixedString(shop.shop_type), keyword_forms, shop.vendor_keywords_exclude);
	} else {
		FFIError error = result.AsErr();
		if (error.IsServer()) {
			FFIServerError server_error = error.AsServer();
			logger::error(FMT_STRING("GetShop server error: {} {} {}"), server_error.status, server_error.title, server_error.detail);
			failReg.SendEvent(true, server_error.status, RE::BSFixedString(server_error.title), RE::BSFixedString(server_error.detail), RE::BSFixedString());
		} else {
			const char* network_error = error.AsNetwork();
			logger::error(FMT_STRING("GetShop network error: {}"), network_error);
			failReg.SendEvent(false, 0, RE::BSFixedString(), RE::BSFixedString(), RE::BSFixedString(network_error));
		}
	}
	successReg.Unregister(quest);
	failReg.Unregister(quest);
}

bool GetShop(RE::StaticFunctionTag*, RE::BSFixedString api_url, RE::BSFixedString api_key, int32_t id, RE::TESQuest* quest) {
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
	SKSE::RegistrationMap<std::vector<int>, std::vector<RE::BSFixedString>, std::vector<RE::BSFixedString>, std::vector<int>, std::vector<RE::BSFixedString>, std::vector<RE::BGSKeyword*>, std::vector<bool>> successReg =
		SKSE::RegistrationMap<std::vector<int>, std::vector<RE::BSFixedString>, std::vector<RE::BSFixedString>, std::vector<int>, std::vector<RE::BSFixedString>, std::vector<RE::BGSKeyword*>, std::vector<bool>>();
	successReg.Register(quest, RE::BSFixedString("OnListShopsSuccess"));
	SKSE::RegistrationMap<bool, int, RE::BSFixedString, RE::BSFixedString, RE::BSFixedString> failReg = SKSE::RegistrationMap<bool, int, RE::BSFixedString, RE::BSFixedString, RE::BSFixedString>();
	failReg.Register(quest, RE::BSFixedString("OnListShopsFail"));

	logger::info(FMT_STRING("ListShops api_url: {}, api_key: {}"), api_url, api_key);
	FFIResult<RawShopVec> result = list_shops(api_url.c_str(), api_key.c_str());

	if (result.IsOk()) {
		RawShopVec vec = result.AsOk();
		logger::info(FMT_STRING("ListShops success vec len: {:d}, cap: {:d}"), vec.len, vec.cap);
		std::vector<int> id_vec = std::vector<int>();
		std::vector<RE::BSFixedString> name_vec = std::vector<RE::BSFixedString>();
		std::vector<RE::BSFixedString> description_vec = std::vector<RE::BSFixedString>();
		std::vector<int> gold_vec = std::vector<int>();
		std::vector<RE::BSFixedString> shop_type_vec = std::vector<RE::BSFixedString>();
		std::vector<RE::BGSKeyword*> keywords_vec = std::vector<RE::BGSKeyword*>();
		std::vector<bool> keywords_exclude_vec = std::vector<bool>();
		for (int i = 0; i < vec.len; i++) {
			RawShop shop = vec.ptr[i];
			id_vec.push_back(shop.id);
			name_vec.push_back(RE::BSFixedString(shop.name));
			description_vec.push_back(RE::BSFixedString(shop.description));
			gold_vec.push_back(shop.gold);
			shop_type_vec.push_back(RE::BSFixedString(shop.shop_type));
			std::vector<RE::BGSKeyword*> keyword_forms;
			for (int j = 0; j < shop.vendor_keywords_len; j++) {
				RE::BSFixedString keyword = shop.vendor_keywords[j];
				logger::info(FMT_STRING("ListShop keyword {:d}: {}"), j, keyword);

				RE::TESForm* form = RE::TESForm::LookupByEditorID(keyword);
				if (!form) { // form is not found, might be in an uninstalled mod
					logger::warn(FMT_STRING("ListShop could not find keyword form for: {}"), keyword);
					continue;
				}
				RE::BGSKeyword* keyword_form = static_cast<RE::BGSKeyword*>(form);
				if (!keyword_form) {
					logger::warn(FMT_STRING("ListShop could cast form to keyword with id {:x} for: {}"), (uint32_t)form->GetFormID(), keyword);
					continue;
				}
				keyword_forms.push_back(keyword_form);
			}
			// Since papyrus doesn't have multi-dimensional arrays, I have to fake it with one flat array with each keyword list separated by a nullptr
			keywords_vec.insert(keywords_vec.end(), keyword_forms.begin(), keyword_forms.end());
			keywords_vec.push_back(nullptr);
			keywords_exclude_vec.push_back(shop.vendor_keywords_exclude);

		}
		successReg.SendEvent(id_vec, name_vec, description_vec, gold_vec, shop_type_vec, keywords_vec, keywords_exclude_vec);
	} else {
		FFIError error = result.AsErr();
		if (error.IsServer()) {
			FFIServerError server_error = error.AsServer();
			logger::error(FMT_STRING("ListShops server error: {} {} {}"), server_error.status, server_error.title, server_error.detail);
			failReg.SendEvent(true, server_error.status, RE::BSFixedString(server_error.title), RE::BSFixedString(server_error.detail), RE::BSFixedString());
		} else {
			const char* network_error = error.AsNetwork();
			logger::error(FMT_STRING("ListShops network error: {}"), network_error);
			failReg.SendEvent(false, 0, RE::BSFixedString(), RE::BSFixedString(), RE::BSFixedString(network_error));
		}
	}
	successReg.Unregister(quest);
	failReg.Unregister(quest);
}

std::vector<RE::BGSKeyword*> GetKeywordsSubArray(RE::StaticFunctionTag*, std::vector<RE::BGSKeyword*> flat_keywords_array, int sub_array_index) {
	int i = 0;
	int current_sub_array = 0;
	while (current_sub_array < sub_array_index) {
		if (flat_keywords_array[i] == nullptr) {
			current_sub_array += 1;
		}
		i += 1;
	}
	std::vector<RE::BGSKeyword*> sub_array = std::vector<RE::BGSKeyword*>();
	while (true) {
		if (flat_keywords_array[i] == nullptr) {
			break;
		} else {
			sub_array.push_back(flat_keywords_array[i]);
		}
		i += 1;
	}
	return sub_array;
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

bool SetVendorKeywords(RE::StaticFunctionTag*, std::vector<RE::BGSKeyword*> keywords, bool keywords_exclude) {
	logger::info("Entered SetVendorKeywords");
	RE::TESDataHandler * data_handler = RE::TESDataHandler::GetSingleton();
	RE::BGSListForm* vendor_items = data_handler->LookupForm<RE::BGSListForm>(FORM_LIST_VENDOR_ITEMS, MOD_NAME);
	RE::TESFaction* vendor_services_faction = data_handler->LookupForm<RE::TESFaction>(FACTION_SERVICES_VENDOR, MOD_NAME);
	if (!vendor_items) {
		logger::error("SetVendorKeywords vendor_items could not be loaded!");
		return false;
	}
	if (!vendor_services_faction) {
		logger::error("SetVendorKeywords vendor_services_faction could not be loaded!");
		return false;
	}

	vendor_items->ClearData();
	for (auto keyword = keywords.begin(); keyword != keywords.end(); ++keyword) {
		vendor_items->AddForm(*keyword);
	}
	vendor_services_faction->vendorData.vendorValues.notBuySell = keywords_exclude;
	return true;
}

void RefreshShopGoldImpl(RE::BSFixedString api_url, RE::BSFixedString api_key, int32_t id, RE::TESObjectREFR* merchant_chest) {
	logger::info("Entered RefreshShopGoldImpl");
	if (!merchant_chest) {
		logger::error("RefreshShopGoldImpl merch_chest is null!");
		return;
	}

	SKSE::RegistrationMap<int> successReg = SKSE::RegistrationMap<int>();
	successReg.Register(merchant_chest, RE::BSFixedString("OnRefreshShopGoldSuccess"));
	SKSE::RegistrationMap<bool, int, RE::BSFixedString, RE::BSFixedString, RE::BSFixedString> failReg = SKSE::RegistrationMap<bool, int, RE::BSFixedString, RE::BSFixedString, RE::BSFixedString>();
	failReg.Register(merchant_chest, RE::BSFixedString("OnRefreshShopGoldFail"));

	RE::TESForm* gold_form = RE::TESForm::LookupByID(15);
	if (!gold_form) {
		logger::error("RefreshShopGoldImpl failed to lookup gold form");
		failReg.SendEvent(false, 0, "", "", RE::BSFixedString("Failed to lookup gold form"));
		successReg.Unregister(merchant_chest);
		failReg.Unregister(merchant_chest);
		return;
	}
	RE::TESBoundObject* gold = static_cast<RE::TESBoundObject*>(gold_form);
	if (!gold) {
		logger::error("RefreshShopGoldImpl failed to cast gold form to gold bound object");
		failReg.SendEvent(false, 0, "", "", RE::BSFixedString("Failed to cast gold form to gold bound object"));
		successReg.Unregister(merchant_chest);
		failReg.Unregister(merchant_chest);
		return;
	}

	logger::info(FMT_STRING("RefreshShopGold api_url: {}, api_key: {}, id: {}"), api_url, api_key, id);
	FFIResult<RawShop> result = get_shop(api_url.c_str(), api_key.c_str(), id);
	if (result.IsOk()) {
		RawShop shop = result.AsOk();
		logger::info(FMT_STRING("RefreshShopGold success id: {} gold: {}"), shop.id, shop.gold);
		if (shop.gold > 0) {
			merchant_chest->AddObjectToContainer(gold, nullptr, shop.gold, merchant_chest);
		}
		successReg.SendEvent(shop.gold);
	} else {
		FFIError error = result.AsErr();
		if (error.IsServer()) {
			FFIServerError server_error = error.AsServer();
			logger::error(FMT_STRING("RefreshShopGold server error: {} {} {}"), server_error.status, server_error.title, server_error.detail);
			failReg.SendEvent(true, server_error.status, RE::BSFixedString(server_error.title), RE::BSFixedString(server_error.detail), RE::BSFixedString());
		} else {
			const char* network_error = error.AsNetwork();
			logger::error(FMT_STRING("RefreshShopGold network error: {}"), network_error);
			failReg.SendEvent(false, 0, RE::BSFixedString(), RE::BSFixedString(), RE::BSFixedString(network_error));
		}
	}
	successReg.Unregister(merchant_chest);
	failReg.Unregister(merchant_chest);
}

bool RefreshShopGold(RE::StaticFunctionTag*, RE::BSFixedString api_url, RE::BSFixedString api_key, int32_t id, RE::TESObjectREFR* merchant_chest) {
	logger::info("Entered RefreshShopGold");
	if (!merchant_chest) {
		logger::error("RefreshShopGold merch_chest is null!");
		return false;
	}

	std::thread thread(RefreshShopGoldImpl, api_url, api_key, id, merchant_chest);
	thread.detach();
	return true;
}
