#include "bindings.h"
#include "utils.h"

void CreateTransactionImpl(
	RE::BSFixedString api_url,
	RE::BSFixedString api_key,
	int32_t shop_id,
	bool is_sell,
	int32_t quantity,
	int32_t amount,
	RE::TESForm* merch_base,
	RE::TESObjectREFR* result_handler
) {	
	logger::info("Entered CreateTransactionImpl");
	RE::TESDataHandler * data_handler = RE::TESDataHandler::GetSingleton();

	if (!result_handler) {
		logger::error("CreateTransactionImpl result_handler is null!");
		return;
	}

	if (!merch_base) {
		logger::error("CreateTransactionImpl merch_base is null!");
		return;
	}

	SKSE::RegistrationMap<int, int, int> successReg = SKSE::RegistrationMap<int, int, int>();
	successReg.Register(result_handler, RE::BSFixedString("OnCreateTransactionSuccess"));
	SKSE::RegistrationMap<RE::BSFixedString> failReg = SKSE::RegistrationMap<RE::BSFixedString>();
	failReg.Register(result_handler, RE::BSFixedString("OnCreateTransactionFail"));

	const char * name = merch_base->GetName();
	uint32_t form_type = (uint32_t)merch_base->GetFormType();
	RE::FormID form_id = merch_base->GetFormID();
	logger::info(FMT_STRING("CreateTransactionImpl merch_base form_id: {:x}, name: {}, type: {:x}"), (uint32_t)form_id, name, (uint32_t)form_type);

	std::pair<uint32_t, const char*> id_parts = get_local_form_id_and_mod_name(merch_base);
	uint32_t local_form_id = id_parts.first;
	const char* mod_name = id_parts.second;
	logger::info(FMT_STRING("CreateTransactionImpl merch_base form file_name: {}, local_form_id: {:x}"), mod_name, local_form_id);

	// TODO: implement is_food
	bool is_food = false;
	uint32_t price = amount / quantity;
	// RE::ExtraCount * extra_quantity_price = activator->extraList.GetByType<RE::ExtraCount>();
	// if (extra_quantity_price) {
		// price = extra_quantity_price->pad14;
	// }

	RawTransaction input_transaction = { 0, shop_id, mod_name, local_form_id, name, form_type, is_food, price, is_sell, quantity, amount };
	FFIResult<RawTransaction> result = create_transaction(api_url.c_str(), api_key.c_str(), input_transaction);
	if (result.IsOk()) {
		RawTransaction saved_transaction = result.AsOk();
		logger::info(FMT_STRING("CreateTransaction success: {}"), saved_transaction.id);
		successReg.SendEvent(saved_transaction.id, saved_transaction.quantity, saved_transaction.amount);
	} else {
		const char* error = result.AsErr();
		logger::error(FMT_STRING("CreateTransaction failure: {}"), error);
		failReg.SendEvent(RE::BSFixedString(error));
	}
	successReg.Unregister(result_handler);
	failReg.Unregister(result_handler);
}

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
) {
	logger::info("Entered CreateTransaction");

	if (!activator) {
		logger::error("CreateTransaction activator is null!");
		return false;
	}

	RE::TESObjectREFR * merch_ref = activator->GetLinkedRef(item_keyword);
	if (!merch_ref) {
		logger::error("CreateTransaction merch_ref is null!");
		return false;
	}

	RE::TESBoundObject * merch_base = merch_ref->GetBaseObject();
	if (!merch_base) {
		logger::error("CreateTransaction merch_base is null!");
		return false;
	}

	std::thread thread(CreateTransactionImpl, api_url, api_key, shop_id, is_sell, quantity, amount, merch_base, activator);
	thread.detach();
	return true;
}

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
) {
	logger::info("Entered CreateTransactionFromVendorSale");

	if (!merch_chest) {
		logger::error("CreateTransactionFromVendorSale activator is null!");
		return false;
	}

	if (!merch_base) {
		logger::error("CreateTransactionFromVendorSale merch_base is null!");
		return false;
	}

	std::thread thread(CreateTransactionImpl, api_url, api_key, shop_id, is_sell, quantity, amount, merch_base, merch_chest);
	thread.detach();
	return true;
}
