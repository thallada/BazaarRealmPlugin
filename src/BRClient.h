#pragma once

void Init(RE::StaticFunctionTag*);
std::string GenerateApiKey(RE::StaticFunctionTag*);
bool StatusCheck(RE::StaticFunctionTag*, RE::BSFixedString api_url);
