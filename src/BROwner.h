#pragma once

bool CreateOwner(RE::StaticFunctionTag*, RE::BSFixedString api_url, RE::BSFixedString api_key, RE::BSFixedString name, uint32_t mod_version, RE::TESQuest* quest);
bool UpdateOwner(RE::StaticFunctionTag*, RE::BSFixedString api_url, RE::BSFixedString api_key, uint32_t id, RE::BSFixedString name, uint32_t mod_version, RE::TESQuest* quest);
