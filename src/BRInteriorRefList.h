#pragma once

bool CreateInteriorRefList(RE::StaticFunctionTag*, RE::BSFixedString api_url, RE::BSFixedString api_key, uint32_t shop_id, RE::TESQuest* quest);
bool ClearCell(RE::StaticFunctionTag*);
bool LoadInteriorRefList(RE::StaticFunctionTag*, RE::BSFixedString api_url, RE::BSFixedString api_key, uint32_t interior_ref_list_id, RE::TESObjectREFR* target_ref, RE::TESQuest* quest);
bool LoadInteriorRefListByShopId(RE::StaticFunctionTag*, RE::BSFixedString api_url, RE::BSFixedString api_key, uint32_t shop_id, RE::TESObjectREFR* target_ref, RE::TESQuest* quest);
