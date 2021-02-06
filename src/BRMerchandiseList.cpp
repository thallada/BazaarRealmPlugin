#include "bindings.h"

#include "utils.h"
#include "NativeFunctions.h"
#include "FormIds.h"

class HaggleVisitor : public RE::PerkEntryVisitor {
public:
	HaggleVisitor(RE::Actor* a_actor) {
		actor = a_actor;
		result = 1;
	}

	virtual RE::PerkEntryVisitor::ReturnType Visit(RE::BGSPerkEntry *perkEntry) override {
		RE::BGSEntryPointPerkEntry* entryPoint = (RE::BGSEntryPointPerkEntry*)perkEntry;
		RE::BGSPerk* perk = entryPoint->perk;
		logger::info(FMT_STRING("HaggleVisitor perk: {} ({:x})"), perk->GetFullName(), (uint32_t)perk->GetFormID());
		RE::BGSPerk* next_perk = perk->nextPerk;
		if (next_perk) {
			logger::info(FMT_STRING("HaggleVisitor next_perk: {} ({:x})"), next_perk->GetFullName(), (uint32_t)next_perk->GetFormID());
			if (actor->HasPerk(next_perk)) {
				logger::info("HaggleVisitor actor has next_perk, skipping this perk");
				return RE::PerkEntryVisitor::ReturnType::kContinue;
			}
		} else {
			logger::info("HaggleVisitor next_perk is null");
		}
		if (entryPoint->functionData && entryPoint->entryData.function == RE::BGSEntryPointPerkEntry::EntryData::Function::kMultiplyValue) {
			RE::BGSEntryPointFunctionDataOneValue* oneValue = (RE::BGSEntryPointFunctionDataOneValue*)entryPoint->functionData;
			logger::info(FMT_STRING("HaggleVisitor setting oneValue data to result: {:.2f}"), oneValue->data);
			result = oneValue->data;
		}

		return RE::PerkEntryVisitor::ReturnType::kContinue;
	}

	float GetResult() const {
		return result;
	}

protected:
	RE::Actor* actor;
	float result;
};

bool ClearMerchandise(RE::TESObjectREFR* merchant_shelf) {
	logger::info("Entered ClearMerchandise");
	RE::TESDataHandler* data_handler = RE::TESDataHandler::GetSingleton();
	RE::BGSKeyword* shelf_keyword = data_handler->LookupForm<RE::BGSKeyword>(KEYWORD_SHELF, MOD_NAME);
	RE::BGSKeyword* activator_keyword = data_handler->LookupForm<RE::BGSKeyword>(KEYWORD_ACTIVATOR, MOD_NAME);

	if (merchant_shelf) {
		RE::TESObjectCELL* cell = merchant_shelf->GetParentCell();
		RE::FormID shelf_form_id = merchant_shelf->GetFormID();
		logger::info(FMT_STRING("ClearMerchandise shelf_form_id: {:x}"), (uint32_t)shelf_form_id);
		for (auto entry = cell->references.begin(); entry != cell->references.end();)
		{
			RE::TESObjectREFR* ref = (*entry).get();
			logger::info(FMT_STRING("ClearMerchandise ref form_id: {:x}, disabled: {:d}, marked for deletion: {:d}, deleted: {:d}"), (uint32_t)ref->GetFormID(), ref->IsDisabled(), ref->IsMarkedForDeletion(), ref->IsDeleted());
			RE::TESBoundObject* base = ref->GetBaseObject();
			if (base) {
				RE::TESObjectREFR* linked_activator_ref = ref->GetLinkedRef(activator_keyword);
				std::pair<uint32_t, const char*> id_parts = get_local_form_id_and_mod_name(base);
				logger::info(FMT_STRING("ClearMerchandise base local_form_id: {:x}, mod_name: {}"), (uint32_t)id_parts.first, id_parts.second);
				if (id_parts.first == ACTIVATOR_STATIC && strcmp(id_parts.second, MOD_NAME) == 0) {
					logger::info("ClearMerchandise found activator ref");
					RE::TESObjectREFR* shelf_linked_ref = ref->GetLinkedRef(shelf_keyword);
					if (shelf_linked_ref) {
						logger::info(FMT_STRING("ClearMerchandise activator ref shelf_linked_ref: {:x}"), (uint32_t)shelf_linked_ref->GetFormID());
					} else {
						logger::info("ClearMerchandise activator ref no shelf_linked_ref!");
					}
					if (shelf_linked_ref && shelf_linked_ref->GetFormID() == shelf_form_id) {
						logger::info("ClearMerchandise activator ref is linked with cleared shelf");
						logger::info(FMT_STRING("ClearMerchandise deleting existing activator ref: {:x}"), (uint32_t)ref->GetFormID());
						// TODO: should I use the MemoryManager to free these references?
						ref->Disable(); // disabling first is required to prevent CTD on unloading cell
						ref->SetDelete(true);
						cell->references.erase(*entry++); // prevents slowdowns after many runs of ClearMerchandise
					} else {
						++entry;
					}
				} else if (linked_activator_ref) {
					logger::info("ClearMerchandise found item ref linked to activator");
					RE::TESObjectREFR* shelf_linked_ref = linked_activator_ref->GetLinkedRef(shelf_keyword);
					if (shelf_linked_ref && shelf_linked_ref->GetFormID() == shelf_form_id) {
						logger::info("ClearMerchandise activator ref is linked with cleared shelf");
						logger::info(FMT_STRING("ClearMerchandise deleting existing item ref: {:x}"), (uint32_t)ref->GetFormID());
						ref->Disable(); // disabling first is required to prevent CTD on unloading cell
						ref->SetDelete(true);
						cell->references.erase(*entry++); // prevents slowdowns after many runs of ClearMerchandise
					} else {
						++entry;
					}
				} else {
					++entry;
				}
			} else {
				logger::info("ClearMerchandise ref has no base, skipping");
				++entry;
			}
		}
	} else {
		logger::error("ClearMerchandise merchant_shelf is null!");
		return false;
	}

	return true;
}

std::tuple<float, float, float> CalculatePriceModifiers() {
	// Calculate the actual barter price using the same formula Skyrim uses in the barter menu
	// Formula from: http://en.uesp.net/wiki/Skyrim:Speech#Prices
	// Allure perk is not counted because merchandise has no gender and is asexual
	RE::GameSettingCollection* game_settings = RE::GameSettingCollection::GetSingleton();
	float f_barter_min = game_settings->GetSetting("fBarterMin")->GetFloat();
	float f_barter_max = game_settings->GetSetting("fBarterMax")->GetFloat();
	logger::info(FMT_STRING("CalculatePriceModifiers fBarterMin: {:.2f}, fBarterMax: {:.2f}"), f_barter_min, f_barter_max);

	RE::PlayerCharacter* player = RE::PlayerCharacter::GetSingleton();
	float speech_skill = player->GetActorValue(RE::ActorValue::kSpeech);
	float speech_skill_advance = player->GetActorValue(RE::ActorValue::kSpeechcraftSkillAdvance);
	float speech_skill_modifier = player->GetActorValue(RE::ActorValue::kSpeechcraftModifier);
	float speech_skill_power_modifier = player->GetActorValue(RE::ActorValue::kSpeechcraftPowerModifier);
	logger::info(FMT_STRING("CalculatePriceModifiers speech_skill: {:.2f}, speech_skill_advance: {:.2f}, speech_skill_modifier: {:.2f}, speech_skill_power_modifier: {:.2f}"), speech_skill, speech_skill_advance, speech_skill_modifier, speech_skill_power_modifier);

	float price_factor = f_barter_max - (f_barter_max - f_barter_min) * std::min(speech_skill, 100.f) / 100.f;
	logger::info(FMT_STRING("CalculatePriceModifiers price_factor: {:.2f}"), price_factor);

	float buy_haggle = 1;
	float sell_haggle = 1;
	if (player->HasPerkEntries(RE::BGSEntryPoint::ENTRY_POINTS::kModBuyPrices)) {
		HaggleVisitor buy_visitor = HaggleVisitor(reinterpret_cast<RE::Actor*>(player));
		player->ForEachPerkEntry(RE::BGSEntryPoint::ENTRY_POINTS::kModBuyPrices, buy_visitor);
		buy_haggle = buy_visitor.GetResult();
		logger::info(FMT_STRING("CalculatePriceModifiers buy_haggle: {:.2f}"), buy_haggle);
	}
	if (player->HasPerkEntries(RE::BGSEntryPoint::ENTRY_POINTS::kModSellPrices)) {
		HaggleVisitor sell_visitor = HaggleVisitor(reinterpret_cast<RE::Actor*>(player));
		player->ForEachPerkEntry(RE::BGSEntryPoint::ENTRY_POINTS::kModSellPrices, sell_visitor);
		sell_haggle = sell_visitor.GetResult();
		logger::info(FMT_STRING("CalculatePriceModifiers sell_haggle: {:.2f}"), sell_haggle);
	}
	logger::info(FMT_STRING("CalculatePriceModifiers 1 - speech_power_mod: {:.2f}"), (1.f - speech_skill_power_modifier / 100.f));
	logger::info(FMT_STRING("CalculatePriceModifiers 1 - speech_mod: {:.2f}"), (1.f - speech_skill_modifier / 100.f));
	float buy_price_modifier = buy_haggle * (1.f - speech_skill_power_modifier / 100.f) * (1.f - speech_skill_modifier / 100.f);
	logger::info(FMT_STRING("CalculatePriceModifiers buy_price_modifier: {:.2f}"), buy_price_modifier);
	float sell_price_modifier = sell_haggle * (1.f + speech_skill_power_modifier / 100.f) * (1.f + speech_skill_modifier / 100.f);
	logger::info(FMT_STRING("CalculatePriceModifiers buy_price_modifier: {:.2f}"), sell_price_modifier);

	std::tuple modifiers (price_factor, buy_price_modifier, sell_price_modifier);
	return modifiers;
}

int CalculateBuyPrice(RE::TESForm* form, float price_factor, float buy_price_modifier) {
	int32_t buy_price = std::round(form->GetGoldValue() * buy_price_modifier * price_factor);
	logger::info(FMT_STRING("CalculateBuyPrice buy_price: {:d}"), buy_price);
	return buy_price;
}

int CalculateSellPrice(RE::TESForm* form, float price_factor, float sell_price_modifier) {
	int32_t sell_price = std::round(form->GetGoldValue() * sell_price_modifier / price_factor);
	logger::info(FMT_STRING("CalculateSellPrice sell_price: {:d}"), sell_price);
	return sell_price;
}

int GetMerchandiseSellPrice(RE::StaticFunctionTag*, RE::TESForm* form) {
	if (!form) {
		logger::error("GetMerchandiseSellPrice form is null!");
		return false;
	}
	std::tuple modifiers = CalculatePriceModifiers();
	float price_factor = std::get<0>(modifiers);
	float sell_price_modifier = std::get<2>(modifiers);
	CalculateSellPrice(form, price_factor, sell_price_modifier);
}

bool ClearAllMerchandise(RE::TESObjectCELL* cell) {
	logger::info("Entered ClearAllMerchandise");
	RE::TESDataHandler* data_handler = RE::TESDataHandler::GetSingleton();
	RE::BGSKeyword* shelf_keyword = data_handler->LookupForm<RE::BGSKeyword>(KEYWORD_SHELF, MOD_NAME);
	RE::BGSKeyword* activator_keyword = data_handler->LookupForm<RE::BGSKeyword>(KEYWORD_ACTIVATOR, MOD_NAME);

	if (cell) {
		for (auto entry = cell->references.begin(); entry != cell->references.end();)
		{
			RE::TESObjectREFR* ref = (*entry).get();
			logger::info(FMT_STRING("ClearAllMerchandise ref form_id: {:x}, disabled: {:d}, marked for deletion: {:d}, deleted: {:d}"), (uint32_t)ref->GetFormID(), ref->IsDisabled(), ref->IsMarkedForDeletion(), ref->IsDeleted());
			RE::TESBoundObject* base = ref->GetBaseObject();
			if (base) {
				RE::TESObjectREFR* linked_activator_ref = ref->GetLinkedRef(activator_keyword);
				std::pair<uint32_t, const char*> id_parts = get_local_form_id_and_mod_name(base);
				if (id_parts.first == ACTIVATOR_STATIC && strcmp(id_parts.second, MOD_NAME) == 0) {
					logger::info("ClearAllMerchandise found activator ref");
					RE::TESObjectREFR * shelf_linked_ref = ref->GetLinkedRef(shelf_keyword);
					if (shelf_linked_ref) {
						logger::info("ClearAllMerchandise activator ref is linked with a shelf");
						logger::info(FMT_STRING("ClearAllMerchandise deleting existing activator ref: {:x}"), (uint32_t)ref->GetFormID());
						// TODO: should I use the MemoryManager to free these references?
						ref->Disable(); // disabling first is required to prevent CTD on unloading cell
						ref->SetDelete(true);
						cell->references.erase(*entry++); // prevents slowdowns after many runs of ClearMerchandise
					} else {
						++entry;
					}
				} else if (linked_activator_ref) {
					logger::info("ClearAllMerchandise found item ref linked to activator");
					RE::TESObjectREFR* shelf_linked_ref = linked_activator_ref->GetLinkedRef(shelf_keyword);
					if (shelf_linked_ref) {
						logger::info("ClearAllMerchandise activator ref is linked with a shelf");
						logger::info(FMT_STRING("ClearAllMerchandise deleting existing item ref: {:x}"), (uint32_t)ref->GetFormID());
						ref->Disable(); // disabling first is required to prevent CTD on unloading cell
						ref->SetDelete(true);
						cell->references.erase(*entry++); // prevents slowdowns after many runs of ClearMerchandise
					} else {
						++entry;
					}
				} else {
					++entry;
				}
			} else {
				logger::info("ClearMerchandise ref has no base, skipping");
				++entry;
			}
		}
	} else {
		logger::error("ClearAllMerchandise cell is null!");
		return false;
	}

	return true;
}

void FillShelf(
	RE::TESObjectCELL* cell,
	RE::TESObjectREFR* merchant_shelf,
	RE::TESObjectREFR* merchant_chest,
	int page,
	SKSE::RegistrationMap<bool> successReg,
	SKSE::RegistrationMap<RE::BSFixedString> failReg
) {
	RE::TESDataHandler* data_handler = RE::TESDataHandler::GetSingleton();
	RE::BSScript::Internal::VirtualMachine * a_vm = RE::BSScript::Internal::VirtualMachine::GetSingleton();
	using func_t = decltype(&PlaceAtMe);
	REL::Relocation<func_t> PlaceAtMe_Native{ REL::ID(55672) };
	using func_t2 = decltype(&MoveTo);
	REL::Relocation<func_t2> MoveTo_Native(RE::Offset::TESObjectREFR::MoveTo);
	REL::ID extra_linked_ref_vtbl(static_cast<std::uint64_t>(229564));

	RE::BGSKeyword* shelf_keyword = data_handler->LookupForm<RE::BGSKeyword>(KEYWORD_SHELF, MOD_NAME);
	RE::BGSKeyword* item_keyword = data_handler->LookupForm<RE::BGSKeyword>(KEYWORD_ITEM, MOD_NAME);
	RE::BGSKeyword* activator_keyword = data_handler->LookupForm<RE::BGSKeyword>(KEYWORD_ACTIVATOR, MOD_NAME);
	RE::BGSKeyword* toggle_keyword = data_handler->LookupForm<RE::BGSKeyword>(KEYWORD_TOGGLE, MOD_NAME);
	RE::BGSKeyword* next_keyword = data_handler->LookupForm<RE::BGSKeyword>(KEYWORD_NEXT, MOD_NAME);
	RE::BGSKeyword* prev_keyword = data_handler->LookupForm<RE::BGSKeyword>(KEYWORD_PREV, MOD_NAME);
	RE::TESForm* activator_static = data_handler->LookupForm(ACTIVATOR_STATIC, MOD_NAME);

	RE::InventoryChanges* inventory_changes = merchant_chest->GetInventoryChanges();
	if (inventory_changes == nullptr) {
		logger::info("FillShelf container empty, nothing to save");
		successReg.SendEvent(false);
		successReg.Unregister(merchant_chest);
		failReg.Unregister(merchant_chest);
		return;
	}

	RE::BSSimpleList<RE::InventoryEntryData*>* entries = inventory_changes->entryList;
	int total_merchandise = 0;
	for (auto entry = entries->begin(); entry != entries->end(); ++entry) {
		total_merchandise += 1;
	}

	// I'm abusing the ExtraCount ExtraData type for storing the current page number state of the shelf
	RE::ExtraCount* extra_page_num = merchant_shelf->extraList.GetByType<RE::ExtraCount>();
	if (!extra_page_num) {
		extra_page_num = (RE::ExtraCount*)RE::BSExtraData::Create(sizeof(RE::ExtraCount), RE::Offset::ExtraCount::Vtbl.address());
		merchant_shelf->extraList.Add(extra_page_num);
		extra_page_num->count = 1;
	}
	
	// I'm abusing the ExtraCannotWear ExtraData type as a boolean marker which stores whether the shelf is in a loaded or cleared state
	// The presense of ExtraCannotWear == loaded, its absence == cleared
	// Please don't try to wear the shelves :)
	RE::ExtraCannotWear* extra_is_loaded = merchant_shelf->extraList.GetByType<RE::ExtraCannotWear>();
	if (!extra_is_loaded) {
		extra_is_loaded = (RE::ExtraCannotWear*)RE::BSExtraData::Create(sizeof(RE::ExtraCannotWear), RE::Offset::ExtraCannotWear::Vtbl.address());
		merchant_shelf->extraList.Add(extra_is_loaded);
	}
	logger::info(FMT_STRING("FillShelf set loaded: {:d}"), merchant_shelf->extraList.GetByType<RE::ExtraCannotWear>() != nullptr);

	if (merchant_shelf->extraList.GetByType<RE::ExtraCannotWear>() == nullptr) {
		logger::warn(FMT_STRING("FillShelf skipping merchant shelf {:x} that is unloaded"), (uint32_t)merchant_shelf->GetFormID());
		return;
	}

	int max_page = std::ceil((float)(total_merchandise) / (float)9);
	int load_page = page;
	if (page == 0) {
		load_page = merchant_shelf->extraList.GetCount();
	}
	if (load_page > max_page) {
		load_page = max_page;
	}
	extra_page_num->count = load_page;
	logger::info(FMT_STRING("FillShelf set shelf page to: {:d}"), merchant_shelf->extraList.GetCount());

	std::tuple modifiers = CalculatePriceModifiers();
	float price_factor = std::get<0>(modifiers);
	float buy_price_modifier = std::get<1>(modifiers);
	float sell_price_modifier = std::get<2>(modifiers);

	RE::NiPoint3 shelf_position = merchant_shelf->data.location;
	RE::NiPoint3 shelf_angle = merchant_shelf->data.angle;
	RE::NiMatrix3 rotation_matrix = get_rotation_matrix(shelf_angle);

	int count = 0;
	for (auto entry = entries->begin(); entry != entries->end(); ++entry) {
		logger::info(FMT_STRING("FillShelf container iterator count: {:d}"), count);
		if (count < (load_page - 1) * 9 || count >= (load_page - 1) * 9 + 9) {
			count++;
			continue;
		}
		RE::InventoryEntryData* entry_data = *entry;
		int quantity = entry_data->countDelta;
		RE::TESBoundObject* base = entry_data->GetObject();
		if (base) {
			RE::TESObjectREFR* ref = PlaceAtMe_Native(a_vm, 0, merchant_shelf, base, 1, false, false);

			RE::NiNPShortPoint3 boundMin = base->boundData.boundMin;
			RE::NiNPShortPoint3 boundMax = base->boundData.boundMax;
			uint16_t bound_x = boundMax.x > boundMin.x ? boundMax.x - boundMin.x : boundMin.x - boundMax.x;
			uint16_t bound_y = boundMax.y > boundMin.y ? boundMax.y - boundMin.y : boundMin.y - boundMax.y;
			uint16_t bound_z = boundMax.z > boundMin.z ? boundMax.z - boundMin.z : boundMin.z - boundMax.z;
			logger::info(FMT_STRING("FillShelf ref bounds width: {:d}, length: {:d}, height: {:d}"), bound_x, bound_y, bound_z);

			RE::TESObjectREFR* activator_ref = PlaceAtMe_Native(a_vm, 0, merchant_shelf, activator_static, 1, false, false);

			RE::NiPoint3 bound_min = ref->GetBoundMin();
			RE::NiPoint3 bound_max = ref->GetBoundMax();
			logger::info(FMT_STRING("FillShelf ref bounds min: {:.2f} {:.2f} {:.2f}, max: {:.2f} {:.2f} {:.2f}"), bound_min.x, bound_min.y, bound_min.z, bound_max.x, bound_max.y, bound_max.z);

			RE::ExtraLinkedRef* activator_extra_linked_ref = RE::BSExtraData::Create<RE::ExtraLinkedRef>(extra_linked_ref_vtbl.address());
			activator_extra_linked_ref->linkedRefs.push_back({shelf_keyword, merchant_shelf});
			activator_ref->extraList.Add(activator_extra_linked_ref);
			RE::ExtraLinkedRef* item_extra_linked_ref = RE::BSExtraData::Create<RE::ExtraLinkedRef>(extra_linked_ref_vtbl.address());
			item_extra_linked_ref->linkedRefs.push_back({activator_keyword, activator_ref});

			float scale = 1;
			int max_over_bound = 0;
			if (max_over_bound < bound_x - 34) {
				max_over_bound = bound_x - 34;
			}
			if (max_over_bound < bound_y - 34) {
				max_over_bound = bound_y - 34;
			}
			if (max_over_bound < bound_z - 34) {
				max_over_bound = bound_z - 34;
			}
			if (max_over_bound > 0) {
				scale = ((float)34 / (float)(max_over_bound + 34)) * (float)100;
				logger::info(FMT_STRING("FillShelf new scale: {:.2f} {:d} (max_over_bound: {:d}"), scale, static_cast<uint16_t>(scale), max_over_bound);
				ref->refScale = static_cast<uint16_t>(scale);
				activator_ref->refScale = static_cast<uint16_t>(scale);
			}

			RE::NiPoint3 ref_angle = RE::NiPoint3(shelf_angle.x, shelf_angle.y, shelf_angle.z - 3.14);
			RE::NiPoint3 ref_position;
			int x_imbalance = (((bound_min.x  * -1) - bound_max.x) * (scale / 100)) / 2;
			int y_imbalance = (((bound_min.y  * -1) - bound_max.y) * (scale / 100)) / 2;
			// adjusts z-height so item doesn't spawn underneath it's shelf
			int z_imbalance = (bound_min.z  * -1) - bound_max.z;
			if (z_imbalance < 0) {
				z_imbalance = 0;
			}
			// TODO: make page size and buy_activator positions configurable per "shelf" type (where is config stored?)
			if (count % 9 == 0) {
				ref_position = RE::NiPoint3(-40 + x_imbalance, y_imbalance, 110 + z_imbalance);
			} else if (count % 9 == 1) {
				ref_position = RE::NiPoint3(x_imbalance, y_imbalance, 110 + z_imbalance);
			} else if (count % 9 == 2) {
				ref_position = RE::NiPoint3(40 + x_imbalance, y_imbalance, 110 + z_imbalance);
			} else if (count % 9 == 3) {
				ref_position = RE::NiPoint3(-40 + x_imbalance, y_imbalance, 65 + z_imbalance);
			} else if (count % 9 == 4) {
				ref_position = RE::NiPoint3(x_imbalance, y_imbalance, 65 + z_imbalance);
			} else if (count % 9 == 5) {
				ref_position = RE::NiPoint3(40 + x_imbalance, y_imbalance, 65 + z_imbalance);
			} else if (count % 9 == 6) {
				ref_position = RE::NiPoint3(-40 + x_imbalance, y_imbalance, 20 + z_imbalance);
			} else if (count % 9 == 7) {
				ref_position = RE::NiPoint3(x_imbalance, y_imbalance, 20 + z_imbalance);
			} else if (count % 9 == 8) {
				ref_position = RE::NiPoint3(40 + x_imbalance, y_imbalance, 20 + z_imbalance);
			}
			logger::info(FMT_STRING("FillShelf relative position x: {:.2f}, y: {:.2f}, z: {:.2f}"), ref_position.x, ref_position.y, ref_position.z);
			ref_position = rotate_point(ref_position, rotation_matrix);

			logger::info(FMT_STRING("FillShelf relative rotated position x: {:.2f}, y: {:.2f}, z: {:.2f}"), ref_position.x, ref_position.y, ref_position.z);
			ref_position = RE::NiPoint3(shelf_position.x + ref_position.x, shelf_position.y + ref_position.y, shelf_position.z + ref_position.z);
			logger::info(FMT_STRING("FillShelf absolute rotated position x: {:.2f}, y: {:.2f}, z: {:.2f}"), ref_position.x, ref_position.y, ref_position.z);
			MoveTo_Native(ref, ref->CreateRefHandle(), cell, cell->worldSpace, ref_position - RE::NiPoint3(10000, 10000, 10000), ref_angle);
			MoveTo_Native(activator_ref, activator_ref->CreateRefHandle(), cell, cell->worldSpace, ref_position, ref_angle);
			activator_extra_linked_ref->linkedRefs.push_back({item_keyword, ref});
			ref->extraList.Add(item_extra_linked_ref);

			int32_t buy_price = CalculateBuyPrice(base, price_factor, buy_price_modifier);
			logger::info(FMT_STRING("FillShelf buy_price: {:d}"), buy_price);

			// I'm abusing the ExtraCount ExtraData type for storing the quantity and price of the merchandise the activator_ref is linked to
			RE::ExtraCount* extra_quantity_price = activator_ref->extraList.GetByType<RE::ExtraCount>();
			if (!extra_quantity_price) {
				extra_quantity_price = RE::BSExtraData::Create<RE::ExtraCount>(RE::Offset::ExtraCount::Vtbl.address());
				activator_ref->extraList.Add(extra_quantity_price);
			}
			extra_quantity_price->count = quantity;
			extra_quantity_price->pad14 = buy_price;

			RE::BSFixedString name = RE::BSFixedString::BSFixedString(fmt::format(FMT_STRING("{} for {:d}g ({:d})"), ref->GetName(), buy_price, quantity));
			activator_ref->SetDisplayName(name, true);
			
		} else {
			logger::warn("FillShelf skipping container inventory entry which has no base form!");
		}
		count++;
	}

	RE::TESObjectREFR* next_ref = merchant_shelf->GetLinkedRef(next_keyword);
	if (!next_ref) {
		logger::error("FillShelf next_ref is null!");
		failReg.SendEvent("Could not find the shelf's next button");
		successReg.Unregister(merchant_chest);
		failReg.Unregister(merchant_chest);
		return;
	}
	RE::TESObjectREFR* prev_ref = merchant_shelf->GetLinkedRef(prev_keyword);
	if (!prev_ref) {
		logger::error("FillShelf prev_ref is null!");
		failReg.SendEvent("Could not find the shelf's previous button");
		successReg.Unregister(merchant_chest);
		failReg.Unregister(merchant_chest);
		return;
	}
	RE::TESObjectREFR* toggle_ref = merchant_shelf->GetLinkedRef(toggle_keyword);
	toggle_ref->SetDisplayName("Clear merchandise", true);
	if (load_page == max_page) {
		next_ref->SetDisplayName("(No next page)", true);
	} else {
		next_ref->SetDisplayName(fmt::format("Advance to page {:d}", load_page + 1).c_str(), true);
	}
	if (load_page == 1) {
		prev_ref->SetDisplayName("(No previous page)", true);
	} else {
		prev_ref->SetDisplayName(fmt::format("Back to page {:d}", load_page - 1).c_str(), true);
	}
}

void LoadShelfPageTask(
	RE::TESObjectCELL* cell,
	RE::TESObjectREFR* merchant_shelf,
	RE::TESObjectREFR* merchant_chest,
	int page
) {
	if (!merchant_chest) {
		logger::error("LoadShelfPageTask merchant_chest is null!");
		return;
	}

	if (!merchant_shelf) {
		logger::error("LoadShelfPageTask merchant_shelf is null!");
		return;
	}

	auto task = SKSE::GetTaskInterface();
	task->AddTask([cell, merchant_shelf, merchant_chest, page]() {
		// Since this method is running asyncronously in a thread, set up a callback on the trigger ref that will receive an event with the result
		SKSE::RegistrationMap<bool> successReg = SKSE::RegistrationMap<bool>();
		successReg.Register(merchant_chest, RE::BSFixedString("OnLoadShelfPageSuccess"));
		SKSE::RegistrationMap<RE::BSFixedString> failReg = SKSE::RegistrationMap<RE::BSFixedString>();
		failReg.Register(merchant_chest, RE::BSFixedString("OnLoadShelfPageFail"));

		if (!ClearMerchandise(merchant_shelf)) {
			logger::error("LoadShelfPageTask ClearMerchandise returned a fail code");
			failReg.SendEvent(RE::BSFixedString("Failed to clear existing merchandise from shelf"));
			successReg.Unregister(merchant_chest);
			failReg.Unregister(merchant_chest);
			return;
		}

		FillShelf(cell, merchant_shelf, merchant_chest, page, successReg, failReg);

		successReg.SendEvent(true);
		successReg.Unregister(merchant_chest);
		failReg.Unregister(merchant_chest);
	});
}

void FillShelves(
	RE::TESObjectCELL* cell,
	std::vector<RE::TESObjectREFR*> merchant_shelves,
	RE::TESObjectREFR* merchant_chest,
	SKSE::RegistrationMap<bool> successReg,
	SKSE::RegistrationMap<RE::BSFixedString> failReg
) {
	if (!ClearAllMerchandise(cell)) {
		logger::error("FillShelves ClearAllMerchandise returned a fail code");
		failReg.SendEvent(RE::BSFixedString("Failed to clear existing merchandise from shelves"));
		successReg.Unregister(merchant_chest);
		failReg.Unregister(merchant_chest);
		return;
	}

	for (auto shelf = merchant_shelves.begin(); shelf != merchant_shelves.end(); ++shelf) {
		RE::TESObjectREFR* merchant_shelf = (*shelf);
		FillShelf(cell, merchant_shelf, merchant_chest, 0, successReg, failReg);
	}
}

void LoadMerchTask(
	FFIResult<RawMerchandiseVec> result,
	RE::TESObjectCELL* cell,
	std::vector<RE::TESObjectREFR*> merchant_shelves,
	RE::TESObjectREFR* merchant_chest
) {
	if (!merchant_chest) {
		logger::error("LoadMerchTask merchant_chest is null!");
		return;
	}

	// Placing the refs must be done on the main thread otherwise disabling & deleting refs in ClearMerchandiseImpl causes a crash
	auto task = SKSE::GetTaskInterface();
	task->AddTask([result, cell, merchant_shelves, merchant_chest]() {
		RE::TESDataHandler* data_handler = RE::TESDataHandler::GetSingleton();

		// Since this method is running asyncronously in a thread, set up a callback on the trigger ref that will receive an event with the result
		SKSE::RegistrationMap<bool> successReg = SKSE::RegistrationMap<bool>();
		successReg.Register(merchant_chest, RE::BSFixedString("OnLoadMerchandiseSuccess"));
		SKSE::RegistrationMap<RE::BSFixedString> failReg = SKSE::RegistrationMap<RE::BSFixedString>();
		failReg.Register(merchant_chest, RE::BSFixedString("OnLoadMerchandiseFail"));

		if (result.IsOk()) {
			logger::info("LoadMerchTask get_merchandise_list result OK");
			RawMerchandiseVec vec = result.AsOk();
			logger::info(FMT_STRING("LoadMerchTask vec len: {:d}, cap: {:d}"), vec.len, vec.cap);

			merchant_chest->ResetInventory(false);

			int count = 0;
			for (int i = 0; i < vec.len; i++) {
				RawMerchandise merch = vec.ptr[i];
				logger::info(FMT_STRING("LoadMerchTask item: {:d}"), i);

				RE::TESForm* form = data_handler->LookupForm(merch.local_form_id, merch.mod_name);
				if (!form) { // base is not found, might be in an uninstalled mod
					logger::warn(FMT_STRING("LoadMerchTask not spawning ref for form that could not be found in installed mods: {} {:d}"), merch.mod_name, merch.local_form_id);
					continue;
				}
				RE::TESBoundObject* base = static_cast<RE::TESBoundObject*>(form);
				logger::info(FMT_STRING("LoadMerchTask lookup form name: {}, form_id: {:x}, form_type: {:x}"), base->GetName(), (uint32_t)base->GetFormID(), (uint32_t)base->GetFormType());

				merchant_chest->AddObjectToContainer(base, nullptr, merch.quantity, merchant_chest);
				count++;
			}
			logger::info(FMT_STRING("LoadMerchTask added: {:d} items to merchant_chest"), count);

			FillShelves(cell, merchant_shelves, merchant_chest, successReg, failReg);
		} else {
			const char* error = result.AsErr();
			logger::error(FMT_STRING("LoadMerchTask get_merchandise_list error: {}"), error);
			failReg.SendEvent(RE::BSFixedString(error));
			successReg.Unregister(merchant_chest);
			failReg.Unregister(merchant_chest);
			return;
		}
		successReg.SendEvent(true);
		successReg.Unregister(merchant_chest);
		failReg.Unregister(merchant_chest);
	});
}

void LoadMerchandiseImpl(
	RE::BSFixedString api_url,
	RE::BSFixedString api_key,
	int32_t merchandise_list_id,
	RE::TESObjectCELL* cell,
	std::vector<RE::TESObjectREFR*> merchant_shelves,
	RE::TESObjectREFR* merchant_chest
) {
	logger::info("Entered LoadMerchandiseImpl");
	LoadMerchTask(get_merchandise_list(api_url.c_str(), api_key.c_str(), merchandise_list_id), cell, merchant_shelves, merchant_chest);
}

void LoadMerchandiseByShopIdImpl(
	RE::BSFixedString api_url,
	RE::BSFixedString api_key,
	int32_t shop_id,
	RE::TESObjectCELL* cell,
	std::vector<RE::TESObjectREFR*> merchant_shelves,
	RE::TESObjectREFR* merchant_chest
) {
	logger::info("Entered LoadMerchandiseByShopIdImpl");
	LoadMerchTask(get_merchandise_list_by_shop_id(api_url.c_str(), api_key.c_str(), shop_id), cell, merchant_shelves, merchant_chest);
}

//bool ToggleMerchandise(
//	RE::StaticFunctionTag*,
//	RE::BSFixedString api_url,
//	RE::BSFixedString api_key,
//	int32_t shop_id,
//	RE::TESObjectREFR* merchant_shelf
//) {
//	if (!merchant_shelf) {
//		logger::error("ToggleMerchandise merchant_shelf is null!");
//		return false;
//	}
//	RE::TESDataHandler * data_handler = RE::TESDataHandler::GetSingleton();
//	RE::BGSKeyword* toggle_keyword = data_handler->LookupForm<RE::BGSKeyword>(KEYWORD_TOGGLE, MOD_NAME);
//	RE::BGSKeyword* next_keyword = data_handler->LookupForm<RE::BGSKeyword>(KEYWORD_NEXT, MOD_NAME);
//	RE::BGSKeyword* prev_keyword = data_handler->LookupForm<RE::BGSKeyword>(KEYWORD_PREV, MOD_NAME);
//
//	// I'm abusing the ExtraCannotWear ExtraData type as a boolean marker which stores whether the shelf is in a loaded or cleared state
//	// The presense of ExtraCannotWear == loaded, its absence == cleared
//	// Please don't try to wear the shelves :)
//	RE::ExtraCannotWear * extra_is_loaded = merchant_shelf->extraList.GetByType<RE::ExtraCannotWear>();
//	if (extra_is_loaded) {
//		// Clear merchandise
//		if (!ClearMerchandiseImpl(merchant_shelf)) {
//			return false;
//		}
//
//		// Reset shelf page to 1 and set state to cleared
//		merchant_shelf->extraList.RemoveByType(RE::ExtraDataType::kCount);
//		merchant_shelf->extraList.RemoveByType(RE::ExtraDataType::kCannotWear);
//
//		RE::TESObjectREFR * toggle_ref = merchant_shelf->GetLinkedRef(toggle_keyword);
//		if (!toggle_ref) {
//			logger::error("ToggleMerchandise toggle_ref is null!");
//			return false;
//		}
//		RE::TESObjectREFR * next_ref = merchant_shelf->GetLinkedRef(next_keyword);
//		if (!next_ref) {
//			logger::error("ToggleMerchandise next_ref is null!");
//			return false;
//		}
//		RE::TESObjectREFR * prev_ref = merchant_shelf->GetLinkedRef(prev_keyword);
//		if (!prev_ref) {
//			logger::error("ToggleMerchandise prev_ref is null!");
//			return false;
//		}
//		toggle_ref->SetDisplayName("Load merchandise", true);
//		next_ref->SetDisplayName("Load merchandise", true);
//		prev_ref->SetDisplayName("Load merchandise", true);
//		return true;
//	} else {
//		// Load merchandise
//		int page = merchant_shelf->extraList.GetCount();
//		std::thread thread(LoadMerchandiseByShopIdImpl, api_url, api_key, shop_id, merchant_shelf, page, false);
//		thread.detach();
//		return true;
//	}
//}
//
bool LoadNextMerchandise(
	RE::StaticFunctionTag*,
	RE::TESObjectREFR* merchant_shelf
) {
	if (!merchant_shelf) {
		logger::error("LoadNextMerchandise merchant_shelf is null!");
		return false;
	}
	int page = merchant_shelf->extraList.GetCount();

	RE::TESDataHandler* data_handler = RE::TESDataHandler::GetSingleton();
	RE::BGSKeyword* chest_keyword = data_handler->LookupForm<RE::BGSKeyword>(KEYWORD_CHEST, MOD_NAME);
	RE::TESObjectREFR* merchant_chest = merchant_shelf->GetLinkedRef(chest_keyword);
	if (!merchant_chest) {
		logger::error("LoadNextMerchandise merchant_chest is null!");
		return false;
	}

	RE::TESObjectCELL* cell = merchant_shelf->GetParentCell();

	RE::ExtraCannotWear * extra_is_loaded = merchant_shelf->extraList.GetByType<RE::ExtraCannotWear>();
	if (extra_is_loaded) {
		// Only advance the page if shelf is in loaded state, else just load the (first) page
		page = page + 1;
	}

	std::thread thread(LoadShelfPageTask, cell, merchant_shelf, merchant_chest, page);
	thread.detach();
	return true;
}

bool LoadPrevMerchandise(
	RE::StaticFunctionTag*,
	RE::TESObjectREFR* merchant_shelf
) {
	if (!merchant_shelf) {
		logger::error("LoadPrevMerchandise merchant_shelf is null!");
		return false;
	}
	int page = merchant_shelf->extraList.GetCount();

	RE::TESDataHandler* data_handler = RE::TESDataHandler::GetSingleton();
	RE::BGSKeyword* chest_keyword = data_handler->LookupForm<RE::BGSKeyword>(KEYWORD_CHEST, MOD_NAME);
	RE::TESObjectREFR* merchant_chest = merchant_shelf->GetLinkedRef(chest_keyword);
	if (!merchant_chest) {
		logger::error("LoadPrevMerchandise merchant_chest is null!");
		return false;
	}

	RE::TESObjectCELL* cell = merchant_shelf->GetParentCell();

	RE::ExtraCannotWear * extra_is_loaded = merchant_shelf->extraList.GetByType<RE::ExtraCannotWear>();
	if (page == 1 && extra_is_loaded) { // no-op on first page and already loaded
		return true;
	}

	if (extra_is_loaded) {
		// Only advance the page if shelf is in loaded state, else just load the (first) page
		page = page - 1;
	}
	
	std::thread thread(LoadShelfPageTask, cell, merchant_shelf, merchant_chest, page);
	thread.detach();
	return true;
}

bool LoadMerchandiseByShopId(
	RE::StaticFunctionTag*,
	RE::BSFixedString api_url,
	RE::BSFixedString api_key,
	int32_t shop_id,
	RE::TESObjectCELL* cell,
	std::vector<RE::TESObjectREFR*> merchant_shelves,
	RE::TESObjectREFR* merchant_chest
) {
	if (!cell) {
		logger::error("LoadMerchandiseByShopId cell is null!");
		return false;
	}
	if (!merchant_chest) {
		logger::error("LoadMerchandiseByShopId merchant_chest is null!");
		return false;
	}

	std::thread thread(LoadMerchandiseByShopIdImpl, api_url, api_key, shop_id, cell, merchant_shelves, merchant_chest);
	thread.detach();
	return true;
}
//
//bool RefreshMerchandise(
//	RE::StaticFunctionTag*,
//	RE::BSFixedString api_url,
//	RE::BSFixedString api_key,
//	int32_t shop_id,
//	std::vector<RE::TESObjectREFR*> merchant_shelves
//) {
//	logger::info("Entered RefreshMerchandise");
//
//	if (merchant_shelves.size() == 0) {
//		logger::error("RefreshMerchandise merchant_shelves is empty!");
//		return false;
//	}
//	int page = merchant_shelves->extraList.GetCount();
//
//	ClearMerchandiseImpl(merchant_shelf);
//
//	std::thread thread(LoadMerchandiseByShopIdImpl, api_url, api_key, shop_id, merchant_shelf, page, true);
//	thread.detach();
//	return true;
//}

// TODO: Am I going to actually use this method?
bool ReplaceMerch3D(RE::StaticFunctionTag*, RE::TESObjectREFR* merchant_shelf) {
	logger::info("Entered ReplaceMerch3D");

	if (!merchant_shelf) {
		logger::error("ReplaceMerch3D merchant_shelf is null!");
		return false;
	}
	RE::TESDataHandler* data_handler = RE::TESDataHandler::GetSingleton();
	RE::BGSKeyword* shelf_keyword = data_handler->LookupForm<RE::BGSKeyword>(KEYWORD_SHELF, MOD_NAME);
	RE::BGSKeyword* item_keyword = data_handler->LookupForm<RE::BGSKeyword>(KEYWORD_ITEM, MOD_NAME);

	RE::TESObjectCELL* cell = merchant_shelf->GetParentCell();
	RE::FormID shelf_form_id = merchant_shelf->GetFormID();
	for (auto entry = cell->references.begin(); entry != cell->references.end(); ++entry) {
		RE::TESObjectREFR* ref = (*entry).get();
		RE::TESBoundObject* base = ref->GetBaseObject();
		if (base) {
			std::pair<uint32_t, const char*> id_parts = get_local_form_id_and_mod_name(base);
			if (id_parts.first == ACTIVATOR_STATIC && strcmp(id_parts.second, MOD_NAME) == 0) {
				logger::info(FMT_STRING("ReplaceMerch3D REF is a activator ref: {:x}"), (uint32_t)ref->GetFormID());
				RE::TESObjectREFR* shelf_linked_ref = ref->GetLinkedRef(shelf_keyword);
				if (shelf_linked_ref && shelf_linked_ref->GetFormID() == shelf_form_id) {
					logger::info("ReplaceMerch3D activator ref is linked with loaded shelf");
					RE::TESObjectREFR* linked_ref = ref->GetLinkedRef(item_keyword);
					if (linked_ref) {
						logger::info(FMT_STRING("ReplaceMerch3D activator has linked item ref: {:x}"), (uint32_t)linked_ref->GetFormID());
						if (linked_ref->Is3DLoaded()) {
							logger::info("ReplaceMerch3D replacing activator 3D with linked item 3D");
							ref->Set3D(linked_ref->GetCurrent3D());
						} else {
							logger::info("ReplaceMerch3D linked item ref 3D is not loaded yet, returning false");
							return false;
						}
					}
				}
			}
		} else {
			logger::info("ReplaceMerch3D ref has no base, skipping");
		}
	}
	return true;
}

bool ReplaceAllMerch3D(RE::StaticFunctionTag*, RE::TESObjectCELL* cell) {
	logger::info("Entered ReplaceAllMerch3D");

	if (!cell) {
		logger::error("ReplaceAllMerch3D cell is null!");
		return false;
	}
	RE::TESDataHandler* data_handler = RE::TESDataHandler::GetSingleton();
	RE::BGSKeyword* shelf_keyword = data_handler->LookupForm<RE::BGSKeyword>(KEYWORD_SHELF, MOD_NAME);
	RE::BGSKeyword* item_keyword = data_handler->LookupForm<RE::BGSKeyword>(KEYWORD_ITEM, MOD_NAME);

	for (auto entry = cell->references.begin(); entry != cell->references.end(); ++entry) {
		RE::TESObjectREFR* ref = (*entry).get();
		RE::TESBoundObject* base = ref->GetBaseObject();
		if (base) {
			std::pair<uint32_t, const char*> id_parts = get_local_form_id_and_mod_name(base);
			if (id_parts.first == ACTIVATOR_STATIC && strcmp(id_parts.second, MOD_NAME) == 0) {
				logger::info(FMT_STRING("ReplaceAllMerch3D REF is a activator ref: {:x}"), (uint32_t)ref->GetFormID());
				RE::TESObjectREFR* shelf_linked_ref = ref->GetLinkedRef(shelf_keyword);
				if (shelf_linked_ref) {
					logger::info("ReplaceAllMerch3D activator ref is linked with loaded shelf");
					RE::TESObjectREFR* linked_ref = ref->GetLinkedRef(item_keyword);
					if (linked_ref) {
						logger::info(FMT_STRING("ReplaceAllMerch3D activator has linked item ref: {:x}"), (uint32_t)linked_ref->GetFormID());
						if (linked_ref->Is3DLoaded()) {
							logger::info("ReplaceAllMerch3D replacing activator 3D with linked item 3D");
							ref->Set3D(linked_ref->GetCurrent3D());
						} else {
							logger::info("ReplaceAllMerch3D linked item ref 3D is not loaded yet, returning false");
							return false;
						}
					}
				}
			}
		} else {
			logger::info("ReplaceAllMerch3D ref has no base, skipping");
		}
	}
	return true;
}

void CreateMerchandiseListImpl(
	RE::BSFixedString api_url,
	RE::BSFixedString api_key,
	int32_t shop_id,
	RE::TESObjectCELL* cell,
	std::vector<RE::TESObjectREFR*> merchant_shelves,
	RE::TESObjectREFR* merchant_chest
) {	
	logger::info("Entered CreateMerchandiseListImpl");
	RE::TESDataHandler* data_handler = RE::TESDataHandler::GetSingleton();
	std::vector<RawMerchandise> merch_records;

	if (!merchant_chest) {
		logger::error("CreateMerchandiseListImpl merchant_chest is null!");
		return;
	}

	SKSE::RegistrationMap<bool> successReg = SKSE::RegistrationMap<bool>();
	successReg.Register(merchant_chest, RE::BSFixedString("OnCreateMerchandiseSuccess"));
	SKSE::RegistrationMap<RE::BSFixedString> failReg = SKSE::RegistrationMap<RE::BSFixedString>();
	failReg.Register(merchant_chest, RE::BSFixedString("OnCreateMerchandiseFail"));

	RE::InventoryChanges* inventory_changes = merchant_chest->GetInventoryChanges();
	if (inventory_changes == nullptr) {
		logger::info("CreateMerchandiseList container empty, nothing to save");
		successReg.SendEvent(false);
		successReg.Unregister(merchant_chest);
		failReg.Unregister(merchant_chest);
		return;
	}

	RE::BSSimpleList<RE::InventoryEntryData*>* entries = inventory_changes->entryList;
	int count = 0;
	for (auto entry = entries->begin(); entry != entries->end(); ++entry) {
		logger::info(FMT_STRING("CreateMerchandiseList container iterator count: {:d}"), count);
		RE::InventoryEntryData* entry_data = *entry;
		RE::TESBoundObject* base = entry_data->GetObject();
		if (base) {
			const char * name = base->GetName();
			uint32_t form_type = (uint32_t)base->GetFormType();
			uint32_t quantity = entry_data->countDelta;
			RE::FormID form_id = base->GetFormID();
			logger::info(FMT_STRING("CreateMerchandiseList quantity: {:d}"), quantity);
			logger::info(FMT_STRING("CreateMerchandiseList base form_id: {:x}, name: {}, type: {:x}"), (uint32_t)form_id, name, (uint32_t)form_type);

			std::pair<uint32_t, const char*> id_parts = get_local_form_id_and_mod_name(base);
			uint32_t local_form_id = id_parts.first;
			const char* mod_name = id_parts.second;
			logger::info(FMT_STRING("CreateMerchandiseList base form file_name: {}, local_form_id"), mod_name, local_form_id);

			// TODO: implement is_food
			bool is_food = false;
			// TODO: allow user to set price
			uint32_t price = base->GetGoldValue();

			merch_records.push_back({
				mod_name,
				local_form_id,
				name,
				quantity,
				form_type,
				is_food,
				price,
			});
		} else {
			logger::warn("CreateMerchandiseList skipping inventory entry which has no base form!");
		}

		count++;
	}

	if (count == 0) {
		logger::info("CreateMerchandiseList container inventory changes empty, nothing to save");
		successReg.SendEvent(false);
		successReg.Unregister(merchant_chest);
		failReg.Unregister(merchant_chest);
		return;
	}

	FFIResult<RawMerchandiseVec> result = update_merchandise_list(api_url.c_str(), api_key.c_str(), shop_id, &merch_records[0], merch_records.size());
	LoadMerchTask(result, cell, merchant_shelves, merchant_chest);

	if (result.IsOk()) {
		logger::info("CreateMerchandiseList success");
		successReg.SendEvent(true);
	} else {
		const char* error = result.AsErr();
		logger::error(FMT_STRING("CreateMerchandiseList failure: {}"), error);
		failReg.SendEvent(RE::BSFixedString(error));
	}
	successReg.Unregister(merchant_chest);
	failReg.Unregister(merchant_chest);
}

bool CreateMerchandiseList(
	RE::StaticFunctionTag*,
	RE::BSFixedString api_url,
	RE::BSFixedString api_key,
	int32_t shop_id,
	RE::TESObjectCELL* cell,
	std::vector<RE::TESObjectREFR*> merchant_shelves,
	RE::TESObjectREFR* merchant_chest
) {
	logger::info("Entered CreateMerchandiseList");

	if (!merchant_chest) {
		logger::error("CreateMerchandiseList merchant_chest is null!");
		return false;
	}

	std::thread thread(CreateMerchandiseListImpl, api_url, api_key, shop_id, cell, merchant_shelves, merchant_chest);
	thread.detach();
	return true;
}

int GetMerchandiseQuantity(RE::StaticFunctionTag*, RE::TESObjectREFR* activator) {
	logger::info("Entered GetMerchandiseQuantity");

	if (!activator) {
		logger::error("GetMerchandiseQuantity activator is null!");
		return false;
	}

	return activator->extraList.GetCount();
}

int GetMerchandisePrice(RE::StaticFunctionTag*, RE::TESObjectREFR* activator) {
	logger::info("Entered GetMerchandisePrice");

	if (!activator) {
		logger::error("GetMerchandisePrice activator is null!");
		return false;
	}

	RE::ExtraCount* extra_quantity_price = activator->extraList.GetByType<RE::ExtraCount>();
	int price = 0;
	if (extra_quantity_price) {
		price = extra_quantity_price->pad14;
	} else {
		logger::warn("GetMerchandisePrice ExtraCount not attached, returning price as 0");
	}
	return price;
}

