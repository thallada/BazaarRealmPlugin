#pragma once

bool CreateTransaction(
	RE::StaticFunctionTag*,
	RE::BSFixedString api_url,
	RE::BSFixedString api_key,
	uint32_t shop_id,
	bool is_sell,
	uint32_t quantity,
	uint32_t amount,
	RE::BGSKeyword* item_keyword,
	RE::TESObjectREFR* activator
);
