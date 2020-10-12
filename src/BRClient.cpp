#include "bindings.h"

void Init(RE::StaticFunctionTag*)
{
	logger::info("Entered Init");
	init();
	logger::info("Init done");
}

std::string GenerateApiKey(RE::StaticFunctionTag*)
{
	logger::info("Entered GenerateApiKey");
	char *api_key = generate_api_key();
	logger::info(FMT_STRING("GenerateApiKey api_key: {}"), api_key);
	return api_key;
}

bool StatusCheck(RE::StaticFunctionTag*, RE::BSFixedString api_url)
{
	logger::info("Entered StatusCheck");
	logger::info(FMT_STRING("StatusCheck api_url: {}"), api_url);
	bool result = status_check(api_url.c_str());
	logger::info(FMT_STRING("StatusCheck result: {}"), result ? "true" : "false");
	return result;
}
