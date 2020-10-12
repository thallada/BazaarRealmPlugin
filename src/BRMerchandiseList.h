#pragma once

bool ClearMerchandiseImpl(RE::TESObjectREFR* merchant_chest, RE::TESObjectREFR* merchant_shelf, RE::TESForm* placeholder_static, RE::BGSKeyword * shelf_keyword, RE::BGSKeyword * item_keyword);
bool ToggleMerchandise(
	RE::StaticFunctionTag*,
	RE::BSFixedString api_url,
	RE::BSFixedString api_key,
	uint32_t merchandise_list_id,
	RE::TESObjectREFR* merchant_shelf,
	RE::TESForm* placeholder_static,
	RE::BGSKeyword* shelf_keyword,
	RE::BGSKeyword* chest_keyword,
	RE::BGSKeyword* item_keyword,
	RE::BGSKeyword* toggle_keyword,
	RE::BGSKeyword* next_keyword,
	RE::BGSKeyword* prev_keyword);
bool LoadNextMerchandise(
	RE::StaticFunctionTag*,
	RE::BSFixedString api_url,
	RE::BSFixedString api_key,
	uint32_t merchandise_list_id,
	RE::TESObjectREFR* merchant_shelf,
	RE::TESForm* placeholder_static,
	RE::BGSKeyword* shelf_keyword,
	RE::BGSKeyword* chest_keyword,
	RE::BGSKeyword* item_keyword,
	RE::BGSKeyword* toggle_keyword,
	RE::BGSKeyword* next_keyword,
	RE::BGSKeyword* prev_keyword);
bool LoadPrevMerchandise(
	RE::StaticFunctionTag*,
	RE::BSFixedString api_url,
	RE::BSFixedString api_key,
	uint32_t merchandise_list_id,
	RE::TESObjectREFR* merchant_shelf,
	RE::TESForm* placeholder_static,
	RE::BGSKeyword* shelf_keyword,
	RE::BGSKeyword* chest_keyword,
	RE::BGSKeyword* item_keyword,
	RE::BGSKeyword* toggle_keyword,
	RE::BGSKeyword* next_keyword,
	RE::BGSKeyword* prev_keyword);
bool ReplaceMerch3D(RE::StaticFunctionTag*, RE::TESObjectREFR* merchant_shelf, RE::TESForm* placeholder_static, RE::BGSKeyword* shelf_keyword, RE::BGSKeyword* item_keyword);
RE::TESForm * BuyMerchandise(RE::StaticFunctionTag*, RE::TESObjectREFR * merchandise_placeholder);
int CreateMerchandiseList(RE::StaticFunctionTag*, RE::BSFixedString api_url, RE::BSFixedString api_key, uint32_t shop_id, RE::TESObjectREFR* merchant_chest);
