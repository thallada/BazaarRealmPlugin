#pragma once

bool Init(RE::StaticFunctionTag*);
std::string GenerateApiKey(RE::StaticFunctionTag*);
bool StatusCheck(RE::StaticFunctionTag*, RE::BSFixedString api_url, RE::TESQuest* quest);
