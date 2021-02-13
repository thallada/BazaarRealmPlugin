#pragma once

bool CreateShop(RE::StaticFunctionTag*, RE::BSFixedString api_url, RE::BSFixedString api_key, RE::BSFixedString name, RE::BSFixedString description, RE::TESQuest* quest);
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
);
bool GetShop(RE::StaticFunctionTag*, RE::BSFixedString api_url, RE::BSFixedString api_key, int32_t id, RE::TESQuest* quest);
bool ListShops(RE::StaticFunctionTag*, RE::BSFixedString api_url, RE::BSFixedString api_key, RE::TESQuest* quest);
std::vector<RE::BGSKeyword*> GetKeywordsSubArray(RE::StaticFunctionTag*, std::vector<RE::BGSKeyword*> flat_keywords_array, int sub_array_index);
bool SetVendorKeywords(RE::StaticFunctionTag*, std::vector<RE::BGSKeyword*> keywords, bool keywords_exclude);
bool RefreshShopGold(RE::StaticFunctionTag*, RE::BSFixedString api_url, RE::BSFixedString api_key, int32_t id, RE::TESObjectREFR* merchant_chest);
