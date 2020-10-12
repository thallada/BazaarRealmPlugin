#include "bindings.h"

int CreateShop(RE::StaticFunctionTag*, RE::BSFixedString api_url, RE::BSFixedString api_key, RE::BSFixedString name, RE::BSFixedString description)
{
	logger::info("Entered CreateShop");
	logger::info(FMT_STRING("CreateShop api_url: {}"), api_url);
	logger::info(FMT_STRING("CreateShop api_key: {}"), api_key);
	logger::info(FMT_STRING("CreateShop name: {}"), name);
	logger::info(FMT_STRING("CreateShop description: {}"), description);
	int shop_id = create_shop(api_url.c_str(), api_key.c_str(), name.c_str(), description.c_str());
	logger::info(FMT_STRING("CreateShop result: {}"), shop_id);
	return shop_id;
}
