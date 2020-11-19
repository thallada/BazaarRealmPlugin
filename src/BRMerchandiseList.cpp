#include "bindings.h"

#include "NativeFunctions.h"

class HaggleVisitor : public RE::PerkEntryVisitor {
public:
	HaggleVisitor(RE::Actor *a_actor) {
		actor = a_actor;
		result = 1;
	}

	virtual RE::PerkEntryVisitor::ReturnType Visit(RE::BGSPerkEntry *perkEntry) override {
		RE::BGSEntryPointPerkEntry *entryPoint = (RE::BGSEntryPointPerkEntry *)perkEntry;
		RE::BGSPerk *perk = entryPoint->perk;
		logger::info(FMT_STRING("HaggleVisitor perk: {} ({:x})"), perk->GetFullName(), (uint32_t)perk->GetFormID());
		RE::BGSPerk *next_perk = perk->nextPerk;
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
			RE::BGSEntryPointFunctionDataOneValue *oneValue = (RE::BGSEntryPointFunctionDataOneValue*)entryPoint->functionData;
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

bool ClearMerchandiseImpl(RE::TESObjectREFR* merchant_chest, RE::TESObjectREFR* merchant_shelf, RE::TESForm* activator_static, RE::BGSKeyword* shelf_keyword, RE::BGSKeyword* item_keyword, RE::BGSKeyword* activator_keyword)
{
	logger::info("Entered ClearMerchandiseImpl");

	if (merchant_chest && merchant_shelf) {
		RE::TESObjectCELL * cell = merchant_shelf->GetParentCell();
		RE::FormID activator_form_id = activator_static->GetFormID();
		RE::FormID shelf_form_id = merchant_shelf->GetFormID();
		for (auto entry = cell->references.begin(); entry != cell->references.end();)
		{
			RE::TESObjectREFR * ref = (*entry).get();
			logger::info(FMT_STRING("ClearMerchandise ref form_id: {:x}, disabled: {:d}, marked for deletion: {:d}, deleted: {:d}"), (uint32_t)ref->GetFormID(), ref->IsDisabled(), ref->IsMarkedForDeletion(), ref->IsDeleted());
			RE::TESBoundObject * base = ref->GetBaseObject();
			if (base) {
				RE::FormID form_id = base->GetFormID();
				if (form_id == activator_form_id) {
					logger::info("ClearMerchandise found activator ref");
					RE::TESObjectREFR * shelf_linked_ref = ref->GetLinkedRef(shelf_keyword);
					if (shelf_linked_ref && shelf_linked_ref->GetFormID() == shelf_form_id) {
						logger::info("ClearMerchandise activator ref is linked with cleared shelf");
						RE::TESObjectREFR * linked_ref = ref->GetLinkedRef(item_keyword);
						if (linked_ref) {
							logger::info(FMT_STRING("ClearMerchandise deleting ref linked to activator ref: {:x}"), (uint32_t)linked_ref->GetFormID());
							// TODO: should I use the MemoryManager to free these references?
							linked_ref->Disable(); // disabling first is required to prevent CTD on unloading cell
							linked_ref->SetDelete(true);
						}
						logger::info(FMT_STRING("ClearMerchandise deleting existing activator ref: {:x}"), (uint32_t)ref->GetFormID());
						ref->Disable(); // disabling first is required to prevent CTD on unloading cell
						ref->SetDelete(true);
						cell->references.erase(*entry++); // prevents slowdowns after many runs of ClearMerchandise
					} else {
						++entry;
					}
				} else {
					++entry;
				}
			}
			else {
				if (ref->IsDisabled() && ref->IsMarkedForDeletion() && ref->IsDeleted()) {
					logger::info("ClearMerchandise ref is probably an item from old LoadMerchandise, clearing from cell now");
					cell->references.erase(*entry++);
				} else {
					logger::info("ClearMerchandise ref has no base, skipping");
					++entry;
				}
			}
		}
	} else {
		logger::error("ClearMerchandiseImpl merchant_chest or merchant_shelf is null!");
		return false;
	}

	return true;
}

RE::NiMatrix3 get_rotation_matrix(RE::NiPoint3 rotation) {
	RE::NiMatrix3 rotation_matrix = RE::NiMatrix3();
	rotation_matrix.entry[0][0] = cos(rotation.z) * cos(rotation.y);
	rotation_matrix.entry[0][1] = (cos(rotation.z) * sin(rotation.y) * sin(rotation.x)) - (sin(rotation.z) * cos(rotation.x));
	rotation_matrix.entry[0][2] = (cos(rotation.z) * sin(rotation.y) * cos(rotation.x)) + (sin(rotation.z) * sin(rotation.x));
	rotation_matrix.entry[1][0] = sin(rotation.z) * cos(rotation.y);
	rotation_matrix.entry[1][1] = (sin(rotation.z) * sin(rotation.y) * sin(rotation.x)) + (cos(rotation.z) * cos(rotation.x));
	rotation_matrix.entry[1][2] = (sin(rotation.z) * sin(rotation.y) * cos(rotation.x)) - (cos(rotation.z) * sin(rotation.x));
	rotation_matrix.entry[2][0] = sin(rotation.y) * -1;
	rotation_matrix.entry[2][1] = cos(rotation.y) * sin(rotation.x);
	rotation_matrix.entry[2][2] = cos(rotation.y) * cos(rotation.x);
	return rotation_matrix;
}

RE::NiPoint3 rotate_point(RE::NiPoint3 point, RE::NiMatrix3 rotation_matrix) {
	return RE::NiPoint3(
		(point.x * rotation_matrix.entry[0][0]) + (point.y * rotation_matrix.entry[1][0]) + (point.z * rotation_matrix.entry[2][0]),
		(point.x * rotation_matrix.entry[0][1]) + (point.y * rotation_matrix.entry[1][1]) + (point.z * rotation_matrix.entry[2][1]),
		(point.x * rotation_matrix.entry[0][2]) + (point.y * rotation_matrix.entry[1][2]) + (point.z * rotation_matrix.entry[2][2])
	);
}

void LoadMerchTask(
	FFIResult<RawMerchandiseVec> result,
	RE::TESObjectREFR* merchant_shelf,
	RE::TESForm* activator_static,
	RE::BGSKeyword* shelf_keyword,
	RE::BGSKeyword* chest_keyword,
	RE::BGSKeyword* item_keyword,
	RE::BGSKeyword* activator_keyword,
	RE::BGSKeyword* toggle_keyword,
	RE::BGSKeyword* next_keyword,
	RE::BGSKeyword* prev_keyword,
	int page,
	bool load_max_page_if_over
) {
	if (!merchant_shelf) {
		logger::error("LoadMerchTask merchant_shelf is null!");
		return;
	}

	RE::TESObjectREFR * toggle_ref = merchant_shelf->GetLinkedRef(toggle_keyword);
	if (!toggle_ref) {
		logger::error("LoadMerchTask toggle_ref is null!");
		return;
	}

	// Placing the refs must be done on the main thread otherwise disabling & deleting refs in ClearMerchandiseImpl causes a crash
	auto task = SKSE::GetTaskInterface();
	task->AddTask([result, merchant_shelf, activator_static, shelf_keyword, chest_keyword, item_keyword, activator_keyword, prev_keyword, next_keyword, page, load_max_page_if_over, toggle_ref]() {
		RE::TESDataHandler * data_handler = RE::TESDataHandler::GetSingleton();
		RE::BSScript::Internal::VirtualMachine * a_vm = RE::BSScript::Internal::VirtualMachine::GetSingleton();
		using func_t = decltype(&PlaceAtMe);
		REL::Relocation<func_t> PlaceAtMe_Native{ REL::ID(55672) };
		using func_t2 = decltype(&MoveTo);
		REL::Relocation<func_t2> MoveTo_Native(RE::Offset::TESObjectREFR::MoveTo);
		REL::ID extra_linked_ref_vtbl(static_cast<std::uint64_t>(229564));

		// Since this method is running asyncronously in a thread, set up a callback on the trigger ref that will receive an event with the result
		SKSE::RegistrationMap<bool> successReg = SKSE::RegistrationMap<bool>();
		successReg.Register(toggle_ref, RE::BSFixedString("OnLoadMerchandiseSuccess"));
		SKSE::RegistrationMap<RE::BSFixedString> failReg = SKSE::RegistrationMap<RE::BSFixedString>();
		failReg.Register(toggle_ref, RE::BSFixedString("OnLoadMerchandiseFail"));

		RE::TESObjectREFR * merchant_chest = merchant_shelf->GetLinkedRef(chest_keyword);
		if (!merchant_chest) {
			logger::error("LoadMerchTask merchant_chest is null!");
			failReg.SendEvent("Merchant chest reference is null");
			successReg.Unregister(toggle_ref);
			failReg.Unregister(toggle_ref);
			return;
		}
		RE::TESObjectCELL * cell = merchant_shelf->GetParentCell();

		if (result.IsOk()) {
			logger::info("LoadMerchandise get_merchandise_list result OK");
			RawMerchandiseVec vec = result.AsOk();
			logger::info(FMT_STRING("LoadMerchandise vec len: {:d}, cap: {:d}"), vec.len, vec.cap);
			int max_page = std::ceil((float)(vec.len) / (float)9);
			int load_page = page;

			// Calculate the actual barter price using the same formula Skyrim uses in the barter menu
			// Formula from: http://en.uesp.net/wiki/Skyrim:Speech#Prices
			// Allure perk is not counted because merchandise has no gender and is asexual
			RE::GameSettingCollection* game_settings = RE::GameSettingCollection::GetSingleton();
			float f_barter_min = game_settings->GetSetting("fBarterMin")->GetFloat();
			float f_barter_max = game_settings->GetSetting("fBarterMax")->GetFloat();
			logger::info(FMT_STRING("LoadMerchandise fBarterMin: {:.2f}, fBarterMax: {:.2f}"), f_barter_min, f_barter_max);

			RE::PlayerCharacter* player = RE::PlayerCharacter::GetSingleton();
			float speech_skill = player->GetActorValue(RE::ActorValue::kSpeech);
			float speech_skill_advance = player->GetActorValue(RE::ActorValue::kSpeechcraftSkillAdvance);
			float speech_skill_modifier = player->GetActorValue(RE::ActorValue::kSpeechcraftModifier);
			float speech_skill_power_modifier = player->GetActorValue(RE::ActorValue::kSpeechcraftPowerModifier);
			logger::info(FMT_STRING("LoadMerchandise speech_skill: {:.2f}, speech_skill_advance: {:.2f}, speech_skill_modifier: {:.2f}, speech_skill_power_modifier: {:.2f}"), speech_skill, speech_skill_advance, speech_skill_modifier, speech_skill_power_modifier);

			float price_factor = f_barter_max - (f_barter_max - f_barter_min) * std::min(speech_skill, 100.f) / 100.f;
			logger::info(FMT_STRING("LoadMerchandise price_factor: {:.2f}"), price_factor);

			float buy_haggle = 1;
			float sell_haggle = 1;
			if (player->HasPerkEntries(RE::BGSEntryPoint::ENTRY_POINTS::kModBuyPrices)) {
				HaggleVisitor buy_visitor = HaggleVisitor(reinterpret_cast<RE::Actor*>(player));
				player->ForEachPerkEntry(RE::BGSEntryPoint::ENTRY_POINTS::kModBuyPrices, buy_visitor);
				buy_haggle = buy_visitor.GetResult();
				logger::info(FMT_STRING("LoadMerchandise buy_haggle: {:.2f}"), buy_haggle);
			}
			if (player->HasPerkEntries(RE::BGSEntryPoint::ENTRY_POINTS::kModSellPrices)) {
				HaggleVisitor sell_visitor = HaggleVisitor(reinterpret_cast<RE::Actor*>(player));
				player->ForEachPerkEntry(RE::BGSEntryPoint::ENTRY_POINTS::kModSellPrices, sell_visitor);
				sell_haggle = sell_visitor.GetResult();
				logger::info(FMT_STRING("LoadMerchandise sell_haggle: {:.2f}"), sell_haggle);
			}
			logger::info(FMT_STRING("LoadMerchandise 1 - speech_power_mod: {:.2f}"), (1.f - speech_skill_power_modifier / 100.f));
			logger::info(FMT_STRING("LoadMerchandise 1 - speech_mod: {:.2f}"), (1.f - speech_skill_modifier / 100.f));
			float buy_price_modifier = buy_haggle * (1.f - speech_skill_power_modifier / 100.f) * (1.f - speech_skill_modifier / 100.f);
			logger::info(FMT_STRING("LoadMerchandise buy_price_modifier: {:.2f}"), buy_price_modifier);
			float sell_price_modifier = sell_haggle * (1.f + speech_skill_power_modifier / 100.f) * (1.f + speech_skill_modifier / 100.f);
			logger::info(FMT_STRING("LoadMerchandise buy_price_modifier: {:.2f}"), sell_price_modifier);

			if (vec.len > 0 && page > max_page) {
				if (load_max_page_if_over) {
					load_page = max_page;
				} else {
					logger::info(FMT_STRING("LoadMerchandise page {:d} is greater than max_page {:d}, doing nothing"), page, max_page);
					successReg.SendEvent(true);
					successReg.Unregister(toggle_ref);
					failReg.Unregister(toggle_ref);
					return;
				}
			}

			if (!ClearMerchandiseImpl(merchant_chest, merchant_shelf, activator_static, shelf_keyword, item_keyword, activator_keyword)) {
				logger::error("LoadMerchandise ClearMerchandiseImpl returned a fail code");
				failReg.SendEvent(RE::BSFixedString("Failed to clear existing merchandise from shelf"));
				successReg.Unregister(toggle_ref);
				failReg.Unregister(toggle_ref);
				return;
			}

			RE::NiPoint3 shelf_position = merchant_shelf->data.location;
			RE::NiPoint3 shelf_angle = merchant_shelf->data.angle;
			RE::NiMatrix3 rotation_matrix = get_rotation_matrix(shelf_angle);

			merchant_chest->ResetInventory(false);

			logger::info(FMT_STRING("LoadMerchandise current shelf page is: {:d}"), merchant_shelf->extraList.GetCount());
			for (int i = 0; i < vec.len; i++) {
				RawMerchandise merch = vec.ptr[i];
				logger::info(FMT_STRING("LoadMerchandise item: {:d}"), i);
				if (i < (load_page - 1) * 9 || i >= (load_page - 1) * 9 + 9) {
					continue;
				}

				RE::TESForm * form = data_handler->LookupForm(merch.local_form_id, merch.mod_name);
				if (!form) { // form is not found, might be in an uninstalled mod
					logger::warn(FMT_STRING("LoadMerchandise not spawning ref for form that could not be found in installed mods: {} {:d}"), merch.mod_name, merch.local_form_id);
					continue;
				}
				logger::info(FMT_STRING("LoadMerchandise lookup form name: {}, form_id: {:x}, form_type: {:x}"), form->GetName(), (uint32_t)form->GetFormID(), (uint32_t)form->GetFormType());
				RE::TESObjectREFR * ref = PlaceAtMe_Native(a_vm, 0, merchant_shelf, form, 1, false, false);

				RE::TESBoundObject * base = ref->GetBaseObject();
				RE::NiNPShortPoint3 boundMin = base->boundData.boundMin;
				RE::NiNPShortPoint3 boundMax = base->boundData.boundMax;
				uint16_t bound_x = boundMax.x > boundMin.x ? boundMax.x - boundMin.x : boundMin.x - boundMax.x;
				uint16_t bound_y = boundMax.y > boundMin.y ? boundMax.y - boundMin.y : boundMin.y - boundMax.y;
				uint16_t bound_z = boundMax.z > boundMin.z ? boundMax.z - boundMin.z : boundMin.z - boundMax.z;
				logger::info(FMT_STRING("LoadMerchandise ref bounds width: {:d}, length: {:d}, height: {:d}"), bound_x, bound_y, bound_z);

				RE::TESObjectREFR * activator_ref = PlaceAtMe_Native(a_vm, 0, merchant_shelf, activator_static, 1, false, false);

				RE::NiPoint3 bound_min = ref->GetBoundMin();
				RE::NiPoint3 bound_max = ref->GetBoundMax();
				logger::info(FMT_STRING("LoadMerchandise ref bounds min: {:.2f} {:.2f} {:.2f}, max: {:.2f} {:.2f} {:.2f}"), bound_min.x, bound_min.y, bound_min.z, bound_max.x, bound_max.y, bound_max.z);

				RE::ExtraLinkedRef * activator_extra_linked_ref = (RE::ExtraLinkedRef*)RE::BSExtraData::Create(sizeof(RE::ExtraLinkedRef), extra_linked_ref_vtbl.address());
				activator_extra_linked_ref->linkedRefs.push_back({shelf_keyword, merchant_shelf});
				activator_ref->extraList.Add(activator_extra_linked_ref);

				RE::ExtraLinkedRef * item_extra_linked_ref = (RE::ExtraLinkedRef*)RE::BSExtraData::Create(sizeof(RE::ExtraLinkedRef), extra_linked_ref_vtbl.address());
				ref->extraList.Add(item_extra_linked_ref);

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
					logger::info(FMT_STRING("LoadMerchandise new scale: {:.2f} {:d} (max_over_bound: {:d}"), scale, static_cast<uint16_t>(scale), max_over_bound);
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
				if (i % 9 == 0) {
					ref_position = RE::NiPoint3(-40 + x_imbalance, y_imbalance, 110 + z_imbalance);
				} else if (i % 9 == 1) {
					ref_position = RE::NiPoint3(x_imbalance, y_imbalance, 110 + z_imbalance);
				} else if (i % 9 == 2) {
					ref_position = RE::NiPoint3(40 + x_imbalance, y_imbalance, 110 + z_imbalance);
				} else if (i % 9 == 3) {
					ref_position = RE::NiPoint3(-40 + x_imbalance, y_imbalance, 65 + z_imbalance);
				} else if (i % 9 == 4) {
					ref_position = RE::NiPoint3(x_imbalance, y_imbalance, 65 + z_imbalance);
				} else if (i % 9 == 5) {
					ref_position = RE::NiPoint3(40 + x_imbalance, y_imbalance, 65 + z_imbalance);
				} else if (i % 9 == 6) {
					ref_position = RE::NiPoint3(-40 + x_imbalance, y_imbalance, 20 + z_imbalance);
				} else if (i % 9 == 7) {
					ref_position = RE::NiPoint3(x_imbalance, y_imbalance, 20 + z_imbalance);
				} else if (i % 9 == 8) {
					ref_position = RE::NiPoint3(40 + x_imbalance, y_imbalance, 20 + z_imbalance);
				}
				logger::info(FMT_STRING("LoadMerchandise relative position x: {:.2f}, y: {:.2f}, z: {:.2f}"), ref_position.x, ref_position.y, ref_position.z);
				ref_position = rotate_point(ref_position, rotation_matrix);

				logger::info(FMT_STRING("LoadMerchandise relative rotated position x: {:.2f}, y: {:.2f}, z: {:.2f}"), ref_position.x, ref_position.y, ref_position.z);
				ref_position = RE::NiPoint3(shelf_position.x + ref_position.x, shelf_position.y + ref_position.y, shelf_position.z + ref_position.z);
				logger::info(FMT_STRING("LoadMerchandise absolute rotated position x: {:.2f}, y: {:.2f}, z: {:.2f}"), ref_position.x, ref_position.y, ref_position.z);
				MoveTo_Native(ref, ref->CreateRefHandle(), cell, cell->worldSpace, ref_position - RE::NiPoint3(10000, 10000, 10000), ref_angle);
				MoveTo_Native(activator_ref, activator_ref->CreateRefHandle(), cell, cell->worldSpace, ref_position, ref_angle);
				activator_extra_linked_ref->linkedRefs.push_back({item_keyword, ref});
				item_extra_linked_ref->linkedRefs.push_back({activator_keyword, activator_ref});

				int32_t buy_price = std::round(merch.price * buy_price_modifier * price_factor);
				logger::info(FMT_STRING("LoadMerchandise buy_price: {:d}"), buy_price);

				// I'm abusing the ExtraCount ExtraData type for storing the quantity and price of the merchandise the activator_ref is linked to
				RE::ExtraCount * extra_quantity_price = activator_ref->extraList.GetByType<RE::ExtraCount>();
				if (!extra_quantity_price) {
					extra_quantity_price = (RE::ExtraCount*)RE::BSExtraData::Create(sizeof(RE::ExtraCount), RE::Offset::ExtraCount::Vtbl.address());
					activator_ref->extraList.Add(extra_quantity_price);
				}
				extra_quantity_price->count = merch.quantity;
				extra_quantity_price->pad14 = buy_price;

				RE::BSFixedString name = RE::BSFixedString::BSFixedString(fmt::format(FMT_STRING("{} for {:d}g ({:d})"), ref->GetName(), buy_price, merch.quantity));
				activator_ref->SetDisplayName(name, true);
				
				merchant_chest->AddObjectToContainer(base, nullptr, merch.quantity, merchant_chest);
			}

			// I'm abusing the ExtraCount ExtraData type for storing the current page number state of the shelf
			RE::ExtraCount * extra_page_num = merchant_shelf->extraList.GetByType<RE::ExtraCount>();
			if (!extra_page_num) {
				extra_page_num = (RE::ExtraCount*)RE::BSExtraData::Create(sizeof(RE::ExtraCount), RE::Offset::ExtraCount::Vtbl.address());
				merchant_shelf->extraList.Add(extra_page_num);
			}
			extra_page_num->count = load_page;
			logger::info(FMT_STRING("LoadMerchandise set shelf page to: {:d}"), merchant_shelf->extraList.GetCount());
			
			// I'm abusing the ExtraCannotWear ExtraData type as a boolean marker which stores whether the shelf is in a loaded or cleared state
			// The presense of ExtraCannotWear == loaded, its absence == cleared
			// Please don't try to wear the shelves :)
			RE::ExtraCannotWear * extra_is_loaded = merchant_shelf->extraList.GetByType<RE::ExtraCannotWear>();
			if (!extra_is_loaded) {
				extra_is_loaded = (RE::ExtraCannotWear*)RE::BSExtraData::Create(sizeof(RE::ExtraCannotWear), RE::Offset::ExtraCannotWear::Vtbl.address());
				merchant_shelf->extraList.Add(extra_is_loaded);
			}
			logger::info(FMT_STRING("LoadMerchandise set loaded: {:d}"), merchant_shelf->extraList.GetByType<RE::ExtraCannotWear>() != nullptr);

			RE::TESObjectREFR * next_ref = merchant_shelf->GetLinkedRef(next_keyword);
			if (!next_ref) {
				logger::error("LoadMerchandise next_ref is null!");
				failReg.SendEvent("Could not find the shelf's next button");
				successReg.Unregister(toggle_ref);
				failReg.Unregister(toggle_ref);
				return;
			}
			RE::TESObjectREFR * prev_ref = merchant_shelf->GetLinkedRef(prev_keyword);
			if (!prev_ref) {
				logger::error("LoadMerchandise prev_ref is null!");
				failReg.SendEvent("Could not find the shelf's previous button");
				successReg.Unregister(toggle_ref);
				failReg.Unregister(toggle_ref);
				return;
			}
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
		} else {
			const char * error = result.AsErr();
			logger::error(FMT_STRING("LoadMerchandise get_merchandise_list error: {}"), error);
			failReg.SendEvent(RE::BSFixedString(error));
			successReg.Unregister(toggle_ref);
			failReg.Unregister(toggle_ref);
			return;
		}
		successReg.SendEvent(true);
		successReg.Unregister(toggle_ref);
		failReg.Unregister(toggle_ref);
	});
}

void LoadMerchandiseImpl(
	RE::BSFixedString api_url,
	RE::BSFixedString api_key,
	int32_t merchandise_list_id,
	RE::TESObjectREFR* merchant_shelf,
	RE::TESForm* activator_static,
	RE::BGSKeyword* shelf_keyword,
	RE::BGSKeyword* chest_keyword,
	RE::BGSKeyword* item_keyword,
	RE::BGSKeyword* activator_keyword,
	RE::BGSKeyword* toggle_keyword,
	RE::BGSKeyword* next_keyword,
	RE::BGSKeyword* prev_keyword,
	int page,
	bool load_max_page_if_over
) {
	logger::info("Entered LoadMerchandiseImpl");
	logger::info(FMT_STRING("LoadMerchandise page: {:d}"), page);

	LoadMerchTask(get_merchandise_list(api_url.c_str(), api_key.c_str(), merchandise_list_id), merchant_shelf, activator_static, shelf_keyword, chest_keyword, item_keyword, activator_keyword, toggle_keyword, next_keyword, prev_keyword, page, load_max_page_if_over);
}

void LoadMerchandiseByShopIdImpl(
	RE::BSFixedString api_url,
	RE::BSFixedString api_key,
	int32_t shop_id,
	RE::TESObjectREFR* merchant_shelf,
	RE::TESForm* activator_static,
	RE::BGSKeyword* shelf_keyword,
	RE::BGSKeyword* chest_keyword,
	RE::BGSKeyword* item_keyword,
	RE::BGSKeyword* activator_keyword,
	RE::BGSKeyword* toggle_keyword,
	RE::BGSKeyword* next_keyword,
	RE::BGSKeyword* prev_keyword,
	int page,
	bool load_max_page_if_over
) {
	logger::info("Entered LoadMerchandiseByShopIdImpl");
	logger::info(FMT_STRING("LoadMerchandiseByShopIdImpl page: {:d}"), page);

	LoadMerchTask(get_merchandise_list_by_shop_id(api_url.c_str(), api_key.c_str(), shop_id), merchant_shelf, activator_static, shelf_keyword, chest_keyword, item_keyword, activator_keyword, toggle_keyword, next_keyword, prev_keyword, page, load_max_page_if_over);
}

bool ToggleMerchandise(
	RE::StaticFunctionTag*,
	RE::BSFixedString api_url,
	RE::BSFixedString api_key,
	int32_t shop_id,
	RE::TESObjectREFR* merchant_shelf,
	RE::TESForm* activator_static,
	RE::BGSKeyword* shelf_keyword,
	RE::BGSKeyword* chest_keyword,
	RE::BGSKeyword* item_keyword,
	RE::BGSKeyword* activator_keyword,
	RE::BGSKeyword* toggle_keyword,
	RE::BGSKeyword* next_keyword,
	RE::BGSKeyword* prev_keyword
) {
	if (!merchant_shelf) {
		logger::error("ToggleMerchandise merchant_shelf is null!");
		return false;
	}
	// I'm abusing the ExtraCannotWear ExtraData type as a boolean marker which stores whether the shelf is in a loaded or cleared state
	// The presense of ExtraCannotWear == loaded, its absence == cleared
	// Please don't try to wear the shelves :)
	RE::ExtraCannotWear * extra_is_loaded = merchant_shelf->extraList.GetByType<RE::ExtraCannotWear>();
	if (extra_is_loaded) {
		// Clear merchandise
		RE::TESObjectREFR * merchant_chest = merchant_shelf->GetLinkedRef(chest_keyword);
		if (!ClearMerchandiseImpl(merchant_chest, merchant_shelf, activator_static, shelf_keyword, item_keyword, activator_keyword)) {
			return false;
		}

		// Reset shelf page to 1 and set state to cleared
		merchant_shelf->extraList.RemoveByType(RE::ExtraDataType::kCount);
		merchant_shelf->extraList.RemoveByType(RE::ExtraDataType::kCannotWear);

		RE::TESObjectREFR * toggle_ref = merchant_shelf->GetLinkedRef(toggle_keyword);
		if (!toggle_ref) {
			logger::error("ToggleMerchandise toggle_ref is null!");
			return false;
		}
		RE::TESObjectREFR * next_ref = merchant_shelf->GetLinkedRef(next_keyword);
		if (!next_ref) {
			logger::error("ToggleMerchandise next_ref is null!");
			return false;
		}
		RE::TESObjectREFR * prev_ref = merchant_shelf->GetLinkedRef(prev_keyword);
		if (!prev_ref) {
			logger::error("ToggleMerchandise prev_ref is null!");
			return false;
		}
		toggle_ref->SetDisplayName("Load merchandise", true);
		next_ref->SetDisplayName("Load merchandise", true);
		prev_ref->SetDisplayName("Load merchandise", true);
		return true;
	} else {
		// Load merchandise
		int page = merchant_shelf->extraList.GetCount();
		std::thread thread(LoadMerchandiseByShopIdImpl, api_url, api_key, shop_id, merchant_shelf, activator_static, shelf_keyword, chest_keyword, item_keyword, activator_keyword, toggle_keyword, next_keyword, prev_keyword, page, false);
		thread.detach();
		return true;
	}
}

bool LoadNextMerchandise(
	RE::StaticFunctionTag*,
	RE::BSFixedString api_url,
	RE::BSFixedString api_key,
	int32_t shop_id,
	RE::TESObjectREFR* merchant_shelf,
	RE::TESForm* activator_static,
	RE::BGSKeyword* shelf_keyword,
	RE::BGSKeyword* chest_keyword,
	RE::BGSKeyword* item_keyword,
	RE::BGSKeyword* activator_keyword,
	RE::BGSKeyword* toggle_keyword,
	RE::BGSKeyword* next_keyword,
	RE::BGSKeyword* prev_keyword
) {
	if (!merchant_shelf) {
		logger::error("LoadNextMerchandise merchant_shelf is null!");
		return false;
	}
	int page = merchant_shelf->extraList.GetCount();

	RE::ExtraCannotWear * extra_is_loaded = merchant_shelf->extraList.GetByType<RE::ExtraCannotWear>();
	if (extra_is_loaded) {
		// Only advance the page if shelf is in loaded state, else just load the (first) page
		page = page + 1;
	}

	std::thread thread(LoadMerchandiseByShopIdImpl, api_url, api_key, shop_id, merchant_shelf, activator_static, shelf_keyword, chest_keyword, item_keyword, activator_keyword, toggle_keyword, next_keyword, prev_keyword, page, false);
	thread.detach();
	return true;
}

bool LoadPrevMerchandise(
	RE::StaticFunctionTag*,
	RE::BSFixedString api_url,
	RE::BSFixedString api_key,
	int32_t shop_id,
	RE::TESObjectREFR* merchant_shelf,
	RE::TESForm* activator_static,
	RE::BGSKeyword* shelf_keyword,
	RE::BGSKeyword* chest_keyword,
	RE::BGSKeyword* item_keyword,
	RE::BGSKeyword* activator_keyword,
	RE::BGSKeyword* toggle_keyword,
	RE::BGSKeyword* next_keyword,
	RE::BGSKeyword* prev_keyword
) {
	if (!merchant_shelf) {
		logger::error("LoadPrevMerchandise merchant_shelf is null!");
		return false;
	}
	int page = merchant_shelf->extraList.GetCount();

	if (page == 1) { // no-op on first page
		return true;
	}

	RE::ExtraCannotWear * extra_is_loaded = merchant_shelf->extraList.GetByType<RE::ExtraCannotWear>();
	if (extra_is_loaded) {
		// Only advance the page if shelf is in loaded state, else just load the (first) page
		page = page - 1;
	}
	
	std::thread thread(LoadMerchandiseByShopIdImpl, api_url, api_key, shop_id, merchant_shelf, activator_static, shelf_keyword, chest_keyword, item_keyword, activator_keyword, toggle_keyword, next_keyword, prev_keyword, page, false);
	thread.detach();
	return true;
}

bool LoadMerchandiseByShopId(
	RE::StaticFunctionTag*,
	RE::BSFixedString api_url,
	RE::BSFixedString api_key,
	int32_t shop_id,
	RE::TESObjectREFR* merchant_shelf,
	RE::TESForm* activator_static,
	RE::BGSKeyword* shelf_keyword,
	RE::BGSKeyword* chest_keyword,
	RE::BGSKeyword* item_keyword,
	RE::BGSKeyword* activator_keyword,
	RE::BGSKeyword* toggle_keyword,
	RE::BGSKeyword* next_keyword,
	RE::BGSKeyword* prev_keyword
) {
	if (!merchant_shelf) {
		logger::error("LoadMerchandiseByShopId merchant_shelf is null!");
		return false;
	}
	// I'm abusing the ExtraCannotWear ExtraData type as a boolean marker which stores whether the shelf is in a loaded or cleared state
	// The presense of ExtraCannotWear == loaded, its absence == cleared
	// Please don't try to wear the shelves :)
	RE::ExtraCannotWear * extra_is_loaded = merchant_shelf->extraList.GetByType<RE::ExtraCannotWear>();
	if (!extra_is_loaded) {
		// Load merchandise
		int page = merchant_shelf->extraList.GetCount();
		std::thread thread(LoadMerchandiseByShopIdImpl, api_url, api_key, shop_id, merchant_shelf, activator_static, shelf_keyword, chest_keyword, item_keyword, activator_keyword, toggle_keyword, next_keyword, prev_keyword, page, true);
		thread.detach();
		return true;
	} else {
		logger::warn("LoadMerchandiseByShopId merchant shelf is marked as already loaded, not doing anything.");
	}
}

bool RefreshMerchandise(
	RE::StaticFunctionTag*,
	RE::BSFixedString api_url,
	RE::BSFixedString api_key,
	int32_t shop_id,
	RE::TESObjectREFR* merchant_shelf,
	RE::TESForm* activator_static,
	RE::BGSKeyword* shelf_keyword,
	RE::BGSKeyword* chest_keyword,
	RE::BGSKeyword* item_keyword,
	RE::BGSKeyword* activator_keyword,
	RE::BGSKeyword* toggle_keyword,
	RE::BGSKeyword* next_keyword,
	RE::BGSKeyword* prev_keyword
) {
	logger::info("Entered RefreshMerchandise");

	RE::TESObjectREFR * merchant_chest = merchant_shelf->GetLinkedRef(chest_keyword);
	if (!merchant_chest) {
		logger::error("RefreshMerchandise could not find merchant_chest linked from merchant_shelf!");
		return false;
	}
	if (!merchant_shelf) {
		logger::error("RefreshMerchandise merchant_shelf is null!");
		return false;
	}
	int page = merchant_shelf->extraList.GetCount();

	ClearMerchandiseImpl(merchant_chest, merchant_shelf, activator_static, shelf_keyword, item_keyword, activator_keyword);

	std::thread thread(LoadMerchandiseByShopIdImpl, api_url, api_key, shop_id, merchant_shelf, activator_static, shelf_keyword, chest_keyword, item_keyword, activator_keyword, toggle_keyword, next_keyword, prev_keyword, page, true);
	thread.detach();
	return true;
}

bool ReplaceMerch3D(RE::StaticFunctionTag*, RE::TESObjectREFR* merchant_shelf, RE::TESForm* activator_static, RE::BGSKeyword* shelf_keyword, RE::BGSKeyword* item_keyword) {
	logger::info("Entered ReplaceMerch3D");

	if (!merchant_shelf) {
		logger::error("LoadMerchandise merchant_shelf is null!");
		return false;
	}
	RE::TESObjectCELL * cell = merchant_shelf->GetParentCell();
	RE::FormID activator_form_id = activator_static->GetFormID();
	RE::FormID shelf_form_id = merchant_shelf->GetFormID();
	for (auto entry = cell->references.begin(); entry != cell->references.end(); ++entry) {
		RE::TESObjectREFR * ref = (*entry).get();
		RE::TESBoundObject * base = ref->GetBaseObject();
		if (base) {
			if (base->GetFormID() == activator_form_id) {
				logger::info(FMT_STRING("ReplaceMerch3D REF is a activator ref: {:x}"), (uint32_t)ref->GetFormID());
				RE::TESObjectREFR * shelf_linked_ref = ref->GetLinkedRef(shelf_keyword);
				if (shelf_linked_ref && shelf_linked_ref->GetFormID() == shelf_form_id) {
					logger::info("ReplaceMerch3D activator ref is linked with loaded shelf");
					RE::TESObjectREFR * linked_ref = ref->GetLinkedRef(item_keyword);
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

void CreateMerchandiseListImpl(RE::BSFixedString api_url, RE::BSFixedString api_key, int32_t shop_id, RE::TESObjectREFR* merchant_chest) {	
	logger::info("Entered CreateMerchandiseListImpl");
	RE::TESDataHandler * data_handler = RE::TESDataHandler::GetSingleton();
	std::vector<RawMerchandise> merch_records;

	if (!merchant_chest) {
		logger::error("CreateMerchandiseListImpl merchant_chest is null!");
		return;
	}

	SKSE::RegistrationMap<bool, int> successReg = SKSE::RegistrationMap<bool, int>();
	successReg.Register(merchant_chest, RE::BSFixedString("OnCreateMerchandiseSuccess"));
	SKSE::RegistrationMap<RE::BSFixedString> failReg = SKSE::RegistrationMap<RE::BSFixedString>();
	failReg.Register(merchant_chest, RE::BSFixedString("OnCreateMerchandiseFail"));

	RE::InventoryChanges * inventory_changes = merchant_chest->GetInventoryChanges();
	if (inventory_changes == nullptr) {
		logger::info("CreateMerchandiseList container empty, nothing to save");
		successReg.SendEvent(false, -1);
		successReg.Unregister(merchant_chest);
		failReg.Unregister(merchant_chest);
		return;
	}

	RE::BSSimpleList<RE::InventoryEntryData*>* entries = inventory_changes->entryList;
	int count = 0;
	for (auto entry = entries->begin(); entry != entries->end(); ++entry) {
		logger::info(FMT_STRING("CreateMerchandiseList container iterator count: {:d}"), count);
		RE::InventoryEntryData * entry_data = *entry;
		RE::TESBoundObject * base = entry_data->GetObject();
		if (base) {
			// Iterating through the entries extraList for debug logging info
			RE::BSSimpleList<RE::ExtraDataList*> * x_lists = entry_data->extraLists;
			if (x_lists) {
				const char * entry_name = entry_data->extraLists->front()->GetDisplayName(base);
				logger::info(FMT_STRING("CreateMerchandiseList container item display_name: {}"), entry_name);
				int x_list_count = 0;
				for (auto x_list = entry_data->extraLists->begin(); x_list != entry_data->extraLists->end(); ++x_list) {
					logger::info(FMT_STRING("CreateMerchandiseList container item x_list: {:d}"), x_list_count);
					int x_data_count = 0;
					for (auto x_data = (*x_list)->begin(); x_data != (*x_list)->end(); ++x_data) {
						logger::info(FMT_STRING("CreateMerchandiseList container item x_data: {:d}, type: {:x}"), x_data_count, (uint32_t)x_data->GetType());
						x_data_count++;
					}

					RE::ExtraEnchantment * enchantment = (RE::ExtraEnchantment*)(*x_list)->GetByType(RE::ExtraDataType::kEnchantment);
					if (enchantment) {
						logger::info(FMT_STRING("CreateMerchandiseList container item enchantment charge: {:d}"), enchantment->charge);
					}
					RE::ExtraScale * scale = (RE::ExtraScale*)(*x_list)->GetByType(RE::ExtraDataType::kScale);
					if (scale) {
						logger::info(FMT_STRING("CreateMerchandiseList container item scale: {:.2f}"), scale->scale);
					}
					x_list_count++;
				}
			}
			const char * name = base->GetName();
			uint32_t form_type = (uint32_t)base->GetFormType();
			uint32_t quantity = entry_data->countDelta;
			RE::FormID form_id = base->GetFormID();
			logger::info(FMT_STRING("CreateMerchandiseList quantity: {:d}"), quantity);
			logger::info(FMT_STRING("CreateMerchandiseList base form_id: {:x}, name: {}, type: {:x}"), (uint32_t)form_id, name, (uint32_t)form_type);

			RE::TESFile * file = base->GetFile(0);
			const char * mod_name = file->fileName;
			bool is_light = file->recordFlags.all(RE::TESFile::RecordFlag::kSmallFile);
			uint32_t local_form_id = is_light ? form_id & 0xfff : form_id & 0xFFFFFF;

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
		}

		count++;
	}

	FFIResult<int32_t> result = update_merchandise_list(api_url.c_str(), api_key.c_str(), shop_id, &merch_records[0], merch_records.size());
	if (result.IsOk()) {
		int32_t merchandise_list_id = result.AsOk();
		logger::info(FMT_STRING("CreateMerchandiseList success: {}"), merchandise_list_id);
		successReg.SendEvent(true, merchandise_list_id);
	} else {
		const char* error = result.AsErr();
		logger::error(FMT_STRING("CreateMerchandiseList failure: {}"), error);
		failReg.SendEvent(RE::BSFixedString(error));
	}
	successReg.Unregister(merchant_chest);
	failReg.Unregister(merchant_chest);
}

bool CreateMerchandiseList(RE::StaticFunctionTag*, RE::BSFixedString api_url, RE::BSFixedString api_key, int32_t shop_id, RE::TESObjectREFR* merchant_chest) {
	logger::info("Entered CreateMerchandiseList");

	if (!merchant_chest) {
		logger::error("CreateMerchandiseList merchant_chest is null!");
		return false;
	}

	std::thread thread(CreateMerchandiseListImpl, api_url, api_key, shop_id, merchant_chest);
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

	RE::ExtraCount * extra_quantity_price = activator->extraList.GetByType<RE::ExtraCount>();
	int price = 0;
	if (extra_quantity_price) {
		price = extra_quantity_price->pad14;
	} else {
		logger::warn("GetMerchandisePrice ExtraCount not attached, returning price as 0");
	}
	return price;
}
