#pragma once

bool CreateOwner(RE::StaticFunctionTag*, RE::BSFixedString api_url, RE::BSFixedString api_key, RE::BSFixedString name, int32_t mod_version, RE::TESQuest* quest);
bool UpdateOwner(RE::StaticFunctionTag*, RE::BSFixedString api_url, RE::BSFixedString api_key, int32_t id, RE::BSFixedString name, int32_t mod_version, RE::TESQuest* quest);
