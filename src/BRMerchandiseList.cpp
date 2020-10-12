#include "bindings.h"

#include "NativeFunctions.h"

// TODO: replace "placeholder" with "buy_activator" and "ref" with "item_ref"
bool ClearMerchandiseImpl(RE::TESObjectREFR* merchant_chest, RE::TESObjectREFR* merchant_shelf, RE::TESForm* placeholder_static, RE::BGSKeyword * shelf_keyword, RE::BGSKeyword * item_keyword)
{
	logger::info("Entered ClearMerchandise");

	if (merchant_chest && merchant_shelf) {
		RE::TESObjectCELL * cell = merchant_shelf->GetParentCell();
		RE::FormID placeholder_form_id = placeholder_static->GetFormID();
		RE::FormID shelf_form_id = merchant_shelf->GetFormID();
		for (auto entry = cell->references.begin(); entry != cell->references.end();)
		{
			RE::TESObjectREFR * ref = (*entry).get();
			logger::info(FMT_STRING("ClearMerchandise ref form_id: {:x}, disabled: {:d}, marked for deletion: {:d}, deleted: {:d}"), (uint32_t)ref->GetFormID(), ref->IsDisabled(), ref->IsMarkedForDeletion(), ref->IsDeleted());
			RE::TESBoundObject * base = ref->GetBaseObject();
			if (base) {
				RE::FormID form_id = base->GetFormID();
				if (form_id == placeholder_form_id) {
					logger::info("ClearMerchandise found placeholder ref");
					RE::TESObjectREFR * shelf_linked_ref = ref->GetLinkedRef(shelf_keyword);
					if (shelf_linked_ref && shelf_linked_ref->GetFormID() == shelf_form_id) {
						logger::info("ClearMerchandise placeholder ref is linked with cleared shelf");
						RE::TESObjectREFR * linked_ref = ref->GetLinkedRef(item_keyword);
						if (linked_ref) {
							logger::info(FMT_STRING("ClearMerchandise deleting ref linked to placeholder ref: {:x}"), (uint32_t)linked_ref->GetFormID());
							// TODO: should I use the MemoryManager to free these references?
							// linked_ref->Load3D(false);
							// linked_ref->SetPosition(linked_ref->GetPosition() -= RE::NiPoint3(-10000, -10000, -10000));
							// linked_ref->Update3DPosition(false);
							// linked_ref->Set3D(ref->GetCurrent3D());
							// linked_ref->Release3DRelatedData();
							linked_ref->Disable(); // disabling first is required to prevent CTD on unloading cell
							linked_ref->SetDelete(true);
							// linked_ref->DeleteThis(); // does this do anything?
						}
						logger::info(FMT_STRING("ClearMerchandise deleting existing placeholder ref: {:x}"), (uint32_t)ref->GetFormID());
						// ref->Load3D(false);
						// ref->SetPosition(ref->GetPosition() -= RE::NiPoint3(-10000, -10000, -10000));
						// ref->Update3DPosition(false);
						// ref->Release3DRelatedData();
						ref->Disable(); // disabling first is required to prevent CTD on unloading cell
						ref->SetDelete(true);
						// ref->DeleteThis(); // does this do anything?
						cell->references.erase(*entry++); // prevents slowdowns after many runs of ClearMerchandise
					}
					else {
						++entry;
					}
				}
				else {
					++entry;
				}
			}
			else {
				if (ref->IsDisabled() && ref->IsMarkedForDeletion() && ref->IsDeleted()) {
					logger::info("ClearMerchandise ref is probably an item from old LoadMerchandise, clearing from cell now");
					cell->references.erase(*entry++);
				}
				else {
					logger::info("ClearMerchandise ref has no base, skipping");
					++entry;
				}
			}
		}
	} else {
		logger::error("ClearMerchandise merchant_chest or merchant_shelf is null!");
		return false;
	}
	return true;
}

bool LoadMerchandiseImpl(
	RE::BSFixedString api_url,
	RE::BSFixedString api_key,
	uint32_t merchandise_list_id,
	RE::TESObjectREFR* merchant_shelf,
	RE::TESForm* placeholder_static,
	RE::BGSKeyword* shelf_keyword,
	RE::BGSKeyword* chest_keyword,
	RE::BGSKeyword* item_keyword,
	RE::BGSKeyword* toggle_keyword,
	RE::BGSKeyword* next_keyword,
	RE::BGSKeyword* prev_keyword,
	int page)
{
	logger::info("Entered LoadMerchandise");
	logger::info(FMT_STRING("LoadMerchandise page: {:d}"), page);
	
	RE::TESDataHandler * data_handler = RE::TESDataHandler::GetSingleton();
	RE::BSScript::Internal::VirtualMachine * a_vm = RE::BSScript::Internal::VirtualMachine::GetSingleton();
	using func_t = decltype(&PlaceAtMe);
	REL::Relocation<func_t> PlaceAtMe_Native{ REL::ID(55672) };
	using func_t2 = decltype(&MoveTo);
	REL::Relocation<func_t2> MoveTo_Native(RE::Offset::TESObjectREFR::MoveTo);
	REL::ID extra_linked_ref_vtbl(static_cast<std::uint64_t>(229564));
	if (!merchant_shelf) {
		logger::error("LoadMerchandise merchant_shelf is null!");
		return false;
	}
	RE::TESObjectREFR * merchant_chest = merchant_shelf->GetLinkedRef(chest_keyword);
	if (!merchant_chest) {
		logger::error("LoadMerchandise merchant_chest is null!");
		return false;
	}

	FFIResult<MerchRecordVec> result = get_merchandise_list(api_url.c_str(), api_key.c_str(), merchandise_list_id);
	if (result.IsOk()) {
		logger::info("LoadMerchandise get_merchandise_list result OK");
		MerchRecordVec vec = result.AsOk();
		logger::info(FMT_STRING("LoadMerchandise vec len: {:d}, cap: {:d}"), vec.len, vec.cap);
		int max_page = std::ceil((float)(vec.len - 1) / (float)9);

		if (vec.len > 0 && page > max_page) {
			logger::info(FMT_STRING("LoadMerchandise page {:d} is greater than max_page {:d}, doing nothing"), page, max_page);
			return true;
		}

		ClearMerchandiseImpl(merchant_chest, merchant_shelf, placeholder_static, shelf_keyword, item_keyword);

		logger::info(FMT_STRING("LoadMerchandise current shelf page is: {:d}"), merchant_shelf->extraList.GetCount());
		for (int i = 0; i < vec.len; i++) {
			MerchRecord rec = vec.ptr[i];
			logger::info(FMT_STRING("LoadMerchandise item: {:d}"), i);
			if (i < (page - 1) * 9 || i >= (page - 1) * 9 + 9) {
				continue;
			}

			RE::TESForm * form = data_handler->LookupForm(rec.local_form_id, rec.mod_name);
			if (!form) { // form is not found, might be in an uninstalled mod
				logger::warn(FMT_STRING("LoadMerchandise not spawning ref for form that could not be found in installed mods: {} {:d}"), rec.mod_name, rec.local_form_id);
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

			RE::TESObjectREFR * placeholder_ref = PlaceAtMe_Native(a_vm, 0, merchant_shelf, placeholder_static, 1, false, false);

			RE::NiPoint3 bound_min = ref->GetBoundMin();
			RE::NiPoint3 bound_max = ref->GetBoundMax();
			logger::info(FMT_STRING("LoadMerchandise ref bounds min: {:.2f} {:.2f} {:.2f}, max: {:.2f} {:.2f} {:.2f}"), bound_min.x, bound_min.y, bound_min.z, bound_max.x, bound_max.y, bound_max.z);

			RE::ExtraLinkedRef * extra_linked_ref = (RE::ExtraLinkedRef*)RE::BSExtraData::Create(sizeof(RE::ExtraLinkedRef), extra_linked_ref_vtbl.address());
			// RE::BGSKeywordForm * place_keyword1 = data_handler->LookupForm<RE::BGSKeywordForm>(595228, "BazaarRealm.esm");
			extra_linked_ref->linkedRefs.push_back({shelf_keyword, merchant_shelf});
			placeholder_ref->extraList.Add(extra_linked_ref);
			// _MESSAGE("PLACEHOLDER LINKED REF: %s", placeholder_ref->GetLinkedRef(nullptr)->GetName());

			// This extra count stored on the placeholder_ref indicates the quanity of the merchandise item it is linked to
			RE::ExtraCount * extra_page_num = (RE::ExtraCount*)RE::BSExtraData::Create(sizeof(RE::ExtraCount), RE::Offset::ExtraCount::Vtbl.address());
			extra_page_num->count = rec.quantity;
			placeholder_ref->extraList.Add(extra_page_num);

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
				placeholder_ref->refScale = static_cast<uint16_t>(scale);
			}

			RE::NiPoint3 shelf_position = merchant_shelf->data.location;
			RE::NiPoint3 shelf_angle = merchant_shelf->data.angle;
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
				ref_position = RE::NiPoint3(shelf_position.x + 40 + x_imbalance, shelf_position.y + y_imbalance, shelf_position.z + 110 + z_imbalance);
			}
			else if (i % 9 == 1) {
				ref_position = RE::NiPoint3(shelf_position.x + x_imbalance, shelf_position.y + y_imbalance, shelf_position.z + 110 + z_imbalance);
			}
			else if (i % 9 == 2) {
				ref_position = RE::NiPoint3(shelf_position.x - 40 + x_imbalance, shelf_position.y + y_imbalance, shelf_position.z + 110 + z_imbalance);
			}
			else if (i % 9 == 3) {
				ref_position = RE::NiPoint3(shelf_position.x + 40 + x_imbalance, shelf_position.y + y_imbalance, shelf_position.z + 65 + z_imbalance);
			}
			else if (i % 9 == 4) {
				ref_position = RE::NiPoint3(shelf_position.x + x_imbalance, shelf_position.y + y_imbalance, shelf_position.z + 65 + z_imbalance);
			}
			else if (i % 9 == 5) {
				ref_position = RE::NiPoint3(shelf_position.x - 40 + x_imbalance, shelf_position.y + y_imbalance, shelf_position.z + 65 + z_imbalance);
			}
			else if (i % 9 == 6) {
				ref_position = RE::NiPoint3(shelf_position.x + 40 + x_imbalance, shelf_position.y + y_imbalance, shelf_position.z + 20 + z_imbalance);
			}
			else if (i % 9 == 7) {
				ref_position = RE::NiPoint3(shelf_position.x + x_imbalance, shelf_position.y + y_imbalance, shelf_position.z + 20 + z_imbalance);
			}
			else if (i % 9 == 8) {
				ref_position = RE::NiPoint3(shelf_position.x - 40 + x_imbalance, shelf_position.y + y_imbalance, shelf_position.z + 20 + z_imbalance);
			}
			RE::TESObjectCELL * cell = merchant_shelf->GetParentCell();
			MoveTo_Native(ref, ref->CreateRefHandle(), cell, cell->worldSpace, ref_position - RE::NiPoint3(10000, 10000, 10000), ref_angle);
			MoveTo_Native(placeholder_ref, placeholder_ref->CreateRefHandle(), cell, cell->worldSpace, ref_position, ref_angle);
			// ref->Load3D(false);
			// Note: passing false to this method occasionally causes the game to crash due to access violation
			// RE::NiAVObject * obj_3d = ref->Load3D(true);
			// None of this works, havok is still applied, which isn't that bad really
			// _MESSAGE("objectReference: %x, GetBaseObject: %x", *ref->data.objectReference, *ref->GetBaseObject());
			// RE::NiAVObject * cloned_obj_3d = ref->data.objectReference->Clone3D(placeholder_ref, false);
			// _MESSAGE("loaded 3d: %x, cloned 3d: %x", *obj_3d, *cloned_obj_3d);
			// Need to Load3D() before calling this:
			// ref->SetMotionType(RE::TESObjectREFR::MotionType::kKeyframed);
			// obj_3d->SetMotionType(static_cast<uint32_t>(RE::TESObjectREFR::MotionType::kKeyframed));
			// obj_3d->SetMotionType(5);
			// Fails if loadedData is nullptr (if Load3D is not called first):
			// ref->loadedData->flags = ref->loadedData->flags | RE::TESObjectREFR::RecordFlags::kDontHavokSettle | RE::TESObjectREFR::RecordFlags::kCollisionsDisabled | RE::TESObjectREFR::RecordFlags::kCollisionsDisabled;
			// ref->InitHavok();
			/// ref->DetachHavok(obj_3d);
			// ref->SetCollision(false);
			// ref->ClampToGround();
			// placeholder_ref->Load3D(false);
			// RE::NiPointer<RE::NiAVObject> placeholder_3d_data = placeholder_ref->loadedData->data3D;
			// _MESSAGE("PLACEHOLDER 3D (pre-set-3d): %x", placeholder_3d_data.get());
			// placeholder_ref->Set3D(obj_3d); // steal the 3D model from the item ref

			// RE::ExtraLight * x_light = ref->extraList.GetByType<RE::ExtraLight>();
			// if (x_light) {
				// _MESSAGE("ExtraLight exists on ref: %x", x_light);
				// ref->extraList.RemoveByType(RE::ExtraDataType::kLight);
				// x_light = ref->extraList.GetByType<RE::ExtraLight>();
				// if (!x_light) {
					// _MESSAGE("ExtraLight removed");
				// }
				// else {
					// _MESSAGE("After removing ExtraLight: %x", x_light);
				// }
			// }
			
			// x_light = placeholder_ref->extraList.GetByType<RE::ExtraLight>();
			// if (x_light) {
				// _MESSAGE("ExtraLight exists on placeholder_ref: %x", x_light);
				// placeholder_ref->extraList.RemoveByType(RE::ExtraDataType::kLight);
				// x_light = placeholder_ref->extraList.GetByType<RE::ExtraLight>();
				// if (!x_light) {
					// _MESSAGE("ExtraLight removed");
				// }
				// else {
					// _MESSAGE("After removing ExtraLight: %x", x_light);
				// }
			// }
			RE::BSFixedString name = RE::BSFixedString::BSFixedString(ref->GetName());
			placeholder_ref->SetDisplayName(name, true);
			// placeholder_ref->SetObjectReference(base);
			placeholder_ref->extraList.SetOwner(base); // I'm abusing "owner" to link the activator with the Form that should be bought once activated

			// Do I still need to set this flag? I could maybe use the deleted flag instead
			// uint32_t phantom_ref_flag = 1 << 9; // this is my own made up ExtraFlags::Flag that marks the reference we stole the 3D from as needing to be deleted at the start of the next LoadMerchandise
			// RE::ExtraFlags * x_flags = ref->extraList.GetByType<RE::ExtraFlags>();
			// RE::ExtraFlags::Flag new_flags;
			// if (x_flags) {
				// _MESSAGE("REF XFLAGS pre-set: %x", x_flags->flags);
				// new_flags = (RE::ExtraFlags::Flag)((uint32_t)(x_flags->flags) | phantom_ref_flag);
			// }
			// else {
				// new_flags = (RE::ExtraFlags::Flag)phantom_ref_flag;
			// }
			//ref->extraList.SetExtraFlags(new_flags, true);
			// x_flags = ref->extraList.GetByType<RE::ExtraFlags>();
			extra_linked_ref->linkedRefs.push_back({item_keyword, ref});
			// _MESSAGE("REF XFLAGS post-set: %x", x_flags->flags);

			// Test deleting ref that owns 3d
			// ref->Disable(); // disabling first is required to prevent CTD on unloading cell
			// ref->SetDelete(true);
			// ref->Predestroy();
			// ref->formFlags |= RE::TESObjectREFR::RecordFlags::kDeleted;
			// ref->AddChange(RE::TESObjectREFR::ChangeFlags::kItemExtraData);
			// ref->AddChange(RE::TESObjectREFR::ChangeFlags::kGameOnlyExtra);
			// ref->AddChange(RE::TESObjectREFR::ChangeFlags::kCreatedOnlyExtra);

			// placeholder_ref->inGameFormFlags |= RE::TESObjectREFR::InGameFormFlag::kWantsDelete;
			// placeholder_ref->AddChange(RE::TESObjectREFR::ChangeFlags::kGameOnlyExtra);
		}

		// I'm abusing the ExtraCount ExtraData type for storing the current page number state of the shelf
		RE::ExtraCount * extra_page_num = merchant_shelf->extraList.GetByType<RE::ExtraCount>();
		if (!extra_page_num) {
			extra_page_num = (RE::ExtraCount*)RE::BSExtraData::Create(sizeof(RE::ExtraCount), RE::Offset::ExtraCount::Vtbl.address());
			merchant_shelf->extraList.Add(extra_page_num);
		}
		extra_page_num->count = page;
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

		RE::TESObjectREFR * toggle_ref = merchant_shelf->GetLinkedRef(toggle_keyword);
		if (!toggle_ref) {
			logger::error("LoadMerchandise toggle_ref is null!");
			return false;
		}
		RE::TESObjectREFR * next_ref = merchant_shelf->GetLinkedRef(next_keyword);
		if (!next_ref) {
			logger::error("LoadMerchandise next_ref is null!");
			return false;
		}
		RE::TESObjectREFR * prev_ref = merchant_shelf->GetLinkedRef(prev_keyword);
		if (!prev_ref) {
			logger::error("LoadMerchandise prev_ref is null!");
			return false;
		}
		toggle_ref->SetDisplayName("Clear merchandise", true);
		if (page == max_page) {
			next_ref->SetDisplayName("(No next page)", true);
		}
		else {
			next_ref->SetDisplayName(fmt::format("Advance to page %d", page + 1).c_str(), true);
		}
		if (page == 1) {
			prev_ref->SetDisplayName("(No previous page)", true);
		}
		else {
			prev_ref->SetDisplayName(fmt::format("Back to page %d", page - 1).c_str(), true);
		}
	}
	else {
		const char * error = result.AsErr();
		logger::error(FMT_STRING("LoadMerchandise get_merchandise_list error: {}"), error);
		return false;
	}
	
	return true;
}

bool ToggleMerchandise(
	RE::StaticFunctionTag*,
	RE::BSFixedString api_url,
	RE::BSFixedString api_key,
	uint32_t merchandise_list_id,
	RE::TESObjectREFR* merchant_shelf,
	RE::TESForm* placeholder_static,
	RE::BGSKeyword* shelf_keyword,
	RE::BGSKeyword* chest_keyword,
	RE::BGSKeyword* item_keyword,
	RE::BGSKeyword* toggle_keyword,
	RE::BGSKeyword* next_keyword,
	RE::BGSKeyword* prev_keyword)
{
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
		if (!ClearMerchandiseImpl(merchant_chest, merchant_shelf, placeholder_static, shelf_keyword, item_keyword)) {
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
	}
	else {
		// Load merchandise
		int page = merchant_shelf->extraList.GetCount();
		return LoadMerchandiseImpl(api_url, api_key, merchandise_list_id, merchant_shelf, placeholder_static, shelf_keyword, chest_keyword, item_keyword, toggle_keyword, next_keyword, prev_keyword, page);
	}
}

bool LoadNextMerchandise(
	RE::StaticFunctionTag*,
	RE::BSFixedString api_url,
	RE::BSFixedString api_key,
	uint32_t merchandise_list_id,
	RE::TESObjectREFR* merchant_shelf,
	RE::TESForm* placeholder_static,
	RE::BGSKeyword* shelf_keyword,
	RE::BGSKeyword* chest_keyword,
	RE::BGSKeyword* item_keyword,
	RE::BGSKeyword* toggle_keyword,
	RE::BGSKeyword* next_keyword,
	RE::BGSKeyword* prev_keyword)
{
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

	return LoadMerchandiseImpl(api_url, api_key, merchandise_list_id, merchant_shelf, placeholder_static, shelf_keyword, chest_keyword, item_keyword, toggle_keyword, next_keyword, prev_keyword, page);
}

bool LoadPrevMerchandise(
	RE::StaticFunctionTag*,
	RE::BSFixedString api_url,
	RE::BSFixedString api_key,
	uint32_t merchandise_list_id,
	RE::TESObjectREFR* merchant_shelf,
	RE::TESForm* placeholder_static,
	RE::BGSKeyword* shelf_keyword,
	RE::BGSKeyword* chest_keyword,
	RE::BGSKeyword* item_keyword,
	RE::BGSKeyword* toggle_keyword,
	RE::BGSKeyword* next_keyword,
	RE::BGSKeyword* prev_keyword)
{
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
	
	return LoadMerchandiseImpl(api_url, api_key, merchandise_list_id, merchant_shelf, placeholder_static, shelf_keyword, chest_keyword, item_keyword, toggle_keyword, next_keyword, prev_keyword, page);
}

bool ReplaceMerch3D(RE::StaticFunctionTag*, RE::TESObjectREFR* merchant_shelf, RE::TESForm* placeholder_static, RE::BGSKeyword* shelf_keyword, RE::BGSKeyword* item_keyword) {
	logger::info("Entered ReplaceMerch3D");

	if (!merchant_shelf) {
		logger::error("LoadMerchandise merchant_shelf is null!");
		return false;
	}
	RE::TESObjectCELL * cell = merchant_shelf->GetParentCell();
	RE::FormID placeholder_form_id = placeholder_static->GetFormID();
	RE::FormID shelf_form_id = merchant_shelf->GetFormID();
	for (auto entry = cell->references.begin(); entry != cell->references.end(); ++entry)
	{
		RE::TESObjectREFR * ref = (*entry).get();
		RE::TESBoundObject * base = ref->GetBaseObject();
		if (base) {
			if (base->GetFormID() == placeholder_form_id) {
				logger::info(FMT_STRING("ReplaceMerch3D REF is a placeholder ref: {:x}"), (uint32_t)ref->GetFormID());
				RE::TESObjectREFR * shelf_linked_ref = ref->GetLinkedRef(shelf_keyword);
				if (shelf_linked_ref && shelf_linked_ref->GetFormID() == shelf_form_id) {
					logger::info("ReplaceMerch3D placeholder ref is linked with loaded shelf");
					RE::TESObjectREFR * linked_ref = ref->GetLinkedRef(item_keyword);
					if (linked_ref) {
						logger::info(FMT_STRING("ReplaceMerch3D placeholder has linked item ref: {:x}"), (uint32_t)linked_ref->GetFormID());
						if (linked_ref->Is3DLoaded()) {
							logger::info("ReplaceMerch3D replaceing placeholder 3D with linked item 3D");
							ref->Set3D(linked_ref->GetCurrent3D());
						}
						else {
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

RE::TESForm * BuyMerchandise(RE::StaticFunctionTag*, RE::TESObjectREFR * merchandise_placeholder) {
	logger::info("Entered BuyMerchandise");
	logger::info(FMT_STRING("BuyMerchandise activated ref: {}"), merchandise_placeholder->GetName());
	RE::TESForm * owner = merchandise_placeholder->GetOwner();
	logger::info(FMT_STRING("BuyMerchandise owner: {}"), owner->GetName());
	logger::info(FMT_STRING("BuyMerchandise count: {:d}"), merchandise_placeholder->extraList.GetCount());
	// TODO: do add item here
	// player->AddObjectToContainer(owner, *RE::ExtraDataList::ExtraDataList(), 1, merchandise_placeholder)
	return owner;
}

// Return code:
// -2: No changes to save, no create request was made
// -1: Error occured
// >= 0: ID of created MerchandiseList returned by API
int CreateMerchandiseList(RE::StaticFunctionTag*, RE::BSFixedString api_url, RE::BSFixedString api_key, uint32_t shop_id, RE::TESObjectREFR* merchant_chest)
{	
	logger::info("Entered CreateMerchandiseList");
	RE::TESDataHandler * data_handler = RE::TESDataHandler::GetSingleton();
	std::vector<MerchRecord> merch_records;
	if (!merchant_chest) {
		logger::error("CreateMerchandiseList merchant_chest is null!");
		return -1;
	}

	RE::InventoryChanges * inventory_changes = merchant_chest->GetInventoryChanges();
	if (inventory_changes == nullptr) {
		logger::info("CreateMerchandiseList container empty, nothing to save");
		return -2;
	}

	RE::BSSimpleList<RE::InventoryEntryData*>* entries = inventory_changes->entryList;
	int count = 0;
	for (auto entry = entries->begin(); entry != entries->end(); ++entry) {
		logger::info(FMT_STRING("CreateMerchandiseList container iterator count: {:d}"), count);
		RE::InventoryEntryData * entry_data = *entry;
		RE::TESBoundObject * base = entry_data->GetObject();
		if (base) {
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
			//_MESSAGE("FILE: %s isLight: %d formID: 0x%x localFormId: 0x%x", file_name, is_light, form_id, local_form_id);

			logger::info(FMT_STRING("CreateMerchandiseList base form file_name: {}, local_form_id"), mod_name, local_form_id);

			// TODO: implement is_food
			uint8_t is_food = 0;
			// TODO: implement price
			uint32_t price = 0;

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

	int merchandise_list_id = create_merchandise_list(api_url.c_str(), api_key.c_str(), shop_id, &merch_records[0], merch_records.size());
	logger::info(FMT_STRING("CreateMerchandiseList create_merchandise_list result: {:d}"), merchandise_list_id);
	return merchandise_list_id;
}
