#include "bindings.h"

int CreateOwner(RE::StaticFunctionTag*, RE::BSFixedString api_url, RE::BSFixedString api_key, RE::BSFixedString name, uint32_t mod_version)
{
	logger::info("Entered CreateOwner");
	logger::info(FMT_STRING("CreateOwner api_url: {}"), api_url);
	logger::info(FMT_STRING("CreateOwner api_key: {}"), api_key);
	logger::info(FMT_STRING("CreateOwner name: {}"), name);
	logger::info(FMT_STRING("CreateOwner mod_version: {}"), mod_version);
	int owner_id = create_owner(api_url.c_str(), api_key.c_str(), name.c_str(), mod_version);
	logger::info(FMT_STRING("CreateOwner result: {}"), owner_id);
	return owner_id;
}
