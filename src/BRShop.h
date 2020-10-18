#pragma once

bool CreateShop(RE::StaticFunctionTag*, RE::BSFixedString api_url, RE::BSFixedString api_key, RE::BSFixedString name, RE::BSFixedString description, RE::TESQuest* quest);
bool UpdateShop(RE::StaticFunctionTag*, RE::BSFixedString api_url, RE::BSFixedString api_key, uint32_t id, RE::BSFixedString name, RE::BSFixedString description, RE::TESQuest* quest);
bool GetShop(RE::StaticFunctionTag*, RE::BSFixedString api_url, RE::BSFixedString api_key, uint32_t id, RE::TESQuest* quest);
