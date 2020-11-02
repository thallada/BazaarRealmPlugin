#include "bindings.h"

void CreateTransactionImpl(
	RE::BSFixedString api_url,
	RE::BSFixedString api_key,
	uint32_t shop_id,
	bool is_sell,
	uint32_t quantity,
	uint32_t amount,
	RE::BGSKeyword* item_keyword,
	RE::TESObjectREFR* activator
) {	
	logger::info("Entered CreateTransactionImpl");
	RE::TESDataHandler * data_handler = RE::TESDataHandler::GetSingleton();

	if (!activator) {
		logger::error("CreateTransactionImpl activator is null!");
		return;
	}

	SKSE::RegistrationMap<int, int, int> successReg = SKSE::RegistrationMap<int, int, int>();
	successReg.Register(activator, RE::BSFixedString("OnCreateTransactionSuccess"));
	SKSE::RegistrationMap<RE::BSFixedString> failReg = SKSE::RegistrationMap<RE::BSFixedString>();
	failReg.Register(activator, RE::BSFixedString("OnCreateTransactionFail"));

	
	RE::TESObjectREFR * merch_ref = activator->GetLinkedRef(item_keyword);
	RE::TESBoundObject * base = merch_ref->GetBaseObject();
	if (!base) {
		logger::error("CreateTransactionImpl merchandise item base is null!");
		failReg.SendEvent(RE::BSFixedString("Could not find the merchandise for the activator"));
		successReg.Unregister(activator);
		failReg.Unregister(activator);
	}
	const char * name = base->GetName();
	uint32_t form_type = (uint32_t)base->GetFormType();
	RE::FormID form_id = base->GetFormID();
	logger::info(FMT_STRING("CreateTransactionImpl base form_id: {:x}, name: {}, type: {:x}"), (uint32_t)form_id, name, (uint32_t)form_type);

	RE::TESFile * file = base->GetFile(0);
	const char * mod_name = file->fileName;
	bool is_light = file->recordFlags.all(RE::TESFile::RecordFlag::kSmallFile);
	uint32_t local_form_id = is_light ? form_id & 0xfff : form_id & 0xFFFFFF;
	logger::info(FMT_STRING("CreateTransactionImpl base form file_name: {}, local_form_id: {:x}"), mod_name, local_form_id);

	// TODO: implement is_food
	bool is_food = false;
	uint32_t price = 0;
	RE::ExtraCount * extra_quantity_price = activator->extraList.GetByType<RE::ExtraCount>();
	if (extra_quantity_price) {
		price = extra_quantity_price->pad14;
	}

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
	successReg.Unregister(activator);
	failReg.Unregister(activator);
}
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
) {
	logger::info("Entered CreateTransaction");

	if (!activator) {
		logger::error("CreateTransaction activator is null!");
		return false;
	}

	std::thread thread(CreateTransactionImpl, api_url, api_key, shop_id, is_sell, quantity, amount, item_keyword, activator);
	thread.detach();
	return true;
}
