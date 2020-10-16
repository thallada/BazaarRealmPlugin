#include "bindings.h"

int CreateShopImpl(RE::BSFixedString api_url, RE::BSFixedString api_key, RE::BSFixedString name, RE::BSFixedString description, RE::TESQuest* quest)
{
	logger::info("Entered CreateShopImpl");
	if (!quest) {
		logger::error("CreateShop quest is null!");
		return -1;
	}

	SKSE::RegistrationMap<int> regMap = SKSE::RegistrationMap<int>();
	regMap.Register(quest, RE::BSFixedString("OnCreateShop"));

	logger::info(FMT_STRING("CreateShop api_url: {}"), api_url);
	logger::info(FMT_STRING("CreateShop api_key: {}"), api_key);
	logger::info(FMT_STRING("CreateShop name: {}"), name);
	logger::info(FMT_STRING("CreateShop description: {}"), description);
	int shop_id = create_shop(api_url.c_str(), api_key.c_str(), name.c_str(), description.c_str());
	logger::info(FMT_STRING("CreateShop result: {}"), shop_id);
	regMap.SendEvent(shop_id);
	regMap.Unregister(quest);
	return shop_id;
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
