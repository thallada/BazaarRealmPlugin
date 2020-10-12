#pragma once

int CreateInteriorRefList(RE::StaticFunctionTag*, RE::BSFixedString api_url, RE::BSFixedString api_key, uint32_t shop_id, RE::TESObjectCELL * cell);
bool ClearCell(RE::StaticFunctionTag*, RE::TESObjectCELL* cell_ignored);
bool LoadInteriorRefList(RE::StaticFunctionTag*, RE::BSFixedString api_url, RE::BSFixedString api_key, uint32_t interior_ref_list_id, RE::TESObjectCELL* cell_ignored, RE::TESObjectREFR* target_ref);
