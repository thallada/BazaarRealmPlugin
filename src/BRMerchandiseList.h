#pragma once

bool ToggleMerchandise(
	RE::StaticFunctionTag*,
	RE::BSFixedString api_url,
	RE::BSFixedString api_key,
	int32_t shop_id,
	RE::TESObjectREFR* merchant_shelf,
	RE::TESForm* activator_static,
	RE::BGSKeyword* shelf_keyword,
	RE::BGSKeyword* chest_keyword,
	RE::BGSKeyword* item_keyword,
	RE::BGSKeyword* activator_keyword,
	RE::BGSKeyword* toggle_keyword,
	RE::BGSKeyword* next_keyword,
	RE::BGSKeyword* prev_keyword
);
bool LoadNextMerchandise(
	RE::StaticFunctionTag*,
	RE::BSFixedString api_url,
	RE::BSFixedString api_key,
	int32_t shop_id,
	RE::TESObjectREFR* merchant_shelf,
	RE::TESForm* activator_static,
	RE::BGSKeyword* shelf_keyword,
	RE::BGSKeyword* chest_keyword,
	RE::BGSKeyword* item_keyword,
	RE::BGSKeyword* activator_keyword,
	RE::BGSKeyword* toggle_keyword,
	RE::BGSKeyword* next_keyword,
	RE::BGSKeyword* prev_keyword
);
bool LoadPrevMerchandise(
	RE::StaticFunctionTag*,
	RE::BSFixedString api_url,
	RE::BSFixedString api_key,
	int32_t shop_id,
	RE::TESObjectREFR* merchant_shelf,
	RE::TESForm* activator_static,
	RE::BGSKeyword* shelf_keyword,
	RE::BGSKeyword* chest_keyword,
	RE::BGSKeyword* item_keyword,
	RE::BGSKeyword* activator_keyword,
	RE::BGSKeyword* toggle_keyword,
	RE::BGSKeyword* next_keyword,
	RE::BGSKeyword* prev_keyword
);
bool LoadMerchandiseByShopId(
	RE::StaticFunctionTag*,
	RE::BSFixedString api_url,
	RE::BSFixedString api_key,
	int32_t shop_id,
	RE::TESObjectREFR* merchant_shelf,
	RE::TESForm* activator_static,
	RE::BGSKeyword* shelf_keyword,
	RE::BGSKeyword* chest_keyword,
	RE::BGSKeyword* item_keyword,
	RE::BGSKeyword* activator_keyword,
	RE::BGSKeyword* toggle_keyword,
	RE::BGSKeyword* next_keyword,
	RE::BGSKeyword* prev_keyword
);
bool RefreshMerchandise(
	RE::StaticFunctionTag*,
	RE::BSFixedString api_url,
	RE::BSFixedString api_key,
	int32_t shop_id,
	RE::TESObjectREFR* merchant_shelf,
	RE::TESForm* activator_static,
	RE::BGSKeyword* shelf_keyword,
	RE::BGSKeyword* chest_keyword,
	RE::BGSKeyword* item_keyword,
	RE::BGSKeyword* activator_keyword,
	RE::BGSKeyword* toggle_keyword,
	RE::BGSKeyword* next_keyword,
	RE::BGSKeyword* prev_keyword
);
bool ReplaceMerch3D(RE::StaticFunctionTag*, RE::TESObjectREFR* merchant_shelf, RE::TESForm* activator_static, RE::BGSKeyword* shelf_keyword, RE::BGSKeyword* item_keyword);
bool CreateMerchandiseList(RE::StaticFunctionTag*, RE::BSFixedString api_url, RE::BSFixedString api_key, int32_t shop_id, RE::TESObjectREFR* merchant_chest);
int GetMerchandiseQuantity(RE::StaticFunctionTag*, RE::TESObjectREFR* activator);
int GetMerchandisePrice(RE::StaticFunctionTag*, RE::TESObjectREFR* activator);
