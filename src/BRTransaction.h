#pragma once

bool CreateTransaction(
	RE::StaticFunctionTag*,
	RE::BSFixedString api_url,
	RE::BSFixedString api_key,
	int32_t shop_id,
	bool is_sell,
	int32_t quantity,
	int32_t amount,
	RE::BGSKeyword* item_keyword,
	RE::TESObjectREFR* activator
);

bool CreateTransactionFromVendorSale(
	RE::StaticFunctionTag*,
	RE::BSFixedString api_url,
	RE::BSFixedString api_key,
	int32_t shop_id,
	bool is_sell,
	int32_t quantity,
	int32_t amount,
	RE::TESForm* merch_base,
	RE::TESObjectREFR* merch_chest
);
