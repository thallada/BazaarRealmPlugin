#pragma once

//bool ToggleMerchandise(
//	RE::StaticFunctionTag*,
//	RE::BSFixedString api_url,
//	RE::BSFixedString api_key,
//	int32_t shop_id,
//	RE::TESObjectREFR* merchant_shelf
//);
bool LoadNextMerchandise(
	RE::StaticFunctionTag*,
	RE::TESObjectREFR* merchant_shelf
);
bool LoadPrevMerchandise(
	RE::StaticFunctionTag*,
	RE::TESObjectREFR* merchant_shelf
);
bool LoadMerchandiseByShopId(
	RE::StaticFunctionTag*,
	RE::BSFixedString api_url,
	RE::BSFixedString api_key,
	int32_t shop_id,
	RE::TESObjectCELL* cell,
	std::vector<RE::TESObjectREFR*> merchant_shelves,
	RE::TESObjectREFR* merchant_chest
);
//bool RefreshMerchandise(
//	RE::StaticFunctionTag*,
//	RE::BSFixedString api_url,
//	RE::BSFixedString api_key,
//	int32_t shop_id,
//	RE::TESObjectREFR* merchant_shelf
//);
bool ReplaceMerch3D(RE::StaticFunctionTag*, RE::TESObjectREFR* merchant_shelf);
bool ReplaceAllMerch3D(RE::StaticFunctionTag*, RE::TESObjectCELL* cell);
bool CreateMerchandiseList(
	RE::StaticFunctionTag*,
	RE::BSFixedString api_url,
	RE::BSFixedString api_key,
	int32_t shop_id,
	RE::TESObjectCELL* cell,
	std::vector<RE::TESObjectREFR*> merchant_shelves,
	RE::TESObjectREFR* merchant_chest
);
int GetMerchandiseQuantity(RE::StaticFunctionTag*, RE::TESObjectREFR* activator);
int GetMerchandisePrice(RE::StaticFunctionTag*, RE::TESObjectREFR* activator);
