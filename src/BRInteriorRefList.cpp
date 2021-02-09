#include "bindings.h"

#include "utils.h"
#include "NativeFunctions.h"
#include "FormIds.h"

const float PI = 3.14159265358979f;
const std::unordered_map<uint32_t, uint32_t> shelf_types { {0x00603f, 1} };
const std::unordered_map<uint32_t, uint32_t> shelf_form_ids { {1, 0x00603f} };
struct Position {
	float position_x;
	float position_y;
	float position_z;
	float angle_x;
	float angle_y;
	float angle_z;
	uint16_t scale;
};
struct Button {
	uint32_t form_id;
	Position position;
};
enum ButtonType { Toggle, Next, Previous };
const std::unordered_map<uint32_t, std::unordered_map<ButtonType, Button>> shelf_buttons {
	{ 1, {
		{ ButtonType::Next, Button {
			0x002fab,
			Position {
				68.,
				-25.,
				83.,
				PI / 2.,
				PI,
				PI,
				50
			}
		} },
		{ ButtonType::Previous, Button {
			0x002fac,
			Position {
				-68.,
				-25.,
				83.,
				PI / 2.,
				PI,
				PI,
				50
			}
		} },
		{ ButtonType::Toggle, Button {
			0x002fad,
			Position {
				0.,
				-27.,
				157.,
				PI / 2.,
				PI,
				PI,
				50
			}
		} },
	} }
};
const std::set<uint32_t> ignored_shelf_related_form_ids {0x002fab, 0x002fac, 0x002fad};

void CreateInteriorRefListImpl(RE::BSFixedString api_url, RE::BSFixedString api_key, int32_t shop_id, RE::TESQuest* quest) {	
	logger::info("Entered CreateInteriorRefListImpl");

	if (!quest) {
		logger::error("CreateInteriorRefListImpl quest is null!");
		return;
	}

	SKSE::RegistrationMap<int> successReg = SKSE::RegistrationMap<int>();
	successReg.Register(quest, RE::BSFixedString("OnCreateInteriorRefListSuccess"));
	SKSE::RegistrationMap<RE::BSFixedString> failReg = SKSE::RegistrationMap<RE::BSFixedString>();
	failReg.Register(quest, RE::BSFixedString("OnCreateInteriorRefListFail"));

	// TODO: may need to dynamically pass shop cell into this function
	RE::TESObjectCELL * cell = RE::TESObjectCELL::LookupByEditorID<RE::TESObjectCELL>("BREmpty");
	logger::info(FMT_STRING("CreateInteriorRefListImpl lookup cell override name: {} id: {:x}"), cell->GetName(), (uint32_t)cell->GetFormID());
	if (!cell) {
		logger::error("CreateInteriorRefListImpl cell is null!");
		failReg.SendEvent("Could not find Cell with the editor ID: BREmpty");
		successReg.Unregister(quest);
		failReg.Unregister(quest);
		return;
	}

	RE::TESDataHandler * data_handler = RE::TESDataHandler::GetSingleton();
	std::vector<RawInteriorRef> raw_interior_refs;
	std::vector<RawShelf> raw_shelves;
	for (auto entry = cell->references.begin(); entry != cell->references.end(); ++entry) {
		RE::TESObjectREFR * ref = (*entry).get();
		const char * name = ref->GetName();
		RE::FormID ref_form_id = ref->GetFormID();
		logger::info(FMT_STRING("CreateInteriorRefList ref: {}, form_id: {:x}"), name, (uint32_t)ref_form_id);
		if (ref->IsDisabled()) {
			logger::info("CreateInteriorRefList skipping ref since it is disabled");
			continue;
		}
		RE::TESBoundObject * base = ref->GetBaseObject();
		if (base) {
			RE::FormID base_form_id = base->GetFormID();
			const RE::FormType form_type = base->GetFormType();
			if (base_form_id == 0x7 || form_type == RE::FormType::NPC || form_type == RE::FormType::LeveledNPC) {
				// skip saving player ref or other companions
				continue;
			}
			const char * form_name = base->GetName();
			logger::info(FMT_STRING("CreateInteriorRefList form: {} ({:x})"), form_name, (uint32_t)form_type);
			float position_x = ref->GetPositionX();
			float position_y = ref->GetPositionY();
			float position_z = ref->GetPositionZ();
			float angle_x = ref->GetAngleX();
			float angle_y = ref->GetAngleY();
			float angle_z = ref->GetAngleZ();
			uint16_t scale = ref->refScale;
			logger::info(FMT_STRING("CreateInteriorRefList position: {:.2f}, {:.2f}, {:.2f} angle: {:.2f}, {:.2f}, {:.2f}  scale: {:d}"), position_x, position_y, position_z, angle_x, angle_y, angle_z, scale);
			logger::info(FMT_STRING("CreateInteriorRefList deleted: {:d}, wants delete: {:d}"), ref->IsMarkedForDeletion(), ref->inGameFormFlags.all(RE::TESObjectREFR::InGameFormFlag::kWantsDelete));
			
			std::pair<uint32_t, const char*> id_parts = get_local_form_id_and_mod_name(base);
			uint32_t base_local_form_id = id_parts.first;
			const char * base_file_name = id_parts.second;
			uint16_t ref_mod_index = ref_form_id >> 24;
			char * ref_file_name = nullptr;
			uint32_t ref_local_form_id = ref_form_id & 0xFFFFFF;
			if (ref_mod_index != 255) { // not a temp ref
				if (ref_mod_index == 254) { // is a light mod
					ref_local_form_id = ref_form_id & 0xfff;
					ref_mod_index = (ref_form_id >> 12) & 0xfff;
					const RE::TESFile * ref_file = data_handler->LookupLoadedLightModByIndex(ref_mod_index);
					ref_file_name = _strdup(ref_file->fileName);
				} else {
					const RE::TESFile * ref_file = data_handler->LookupLoadedModByIndex(ref_mod_index);
					ref_file_name = _strdup(ref_file->fileName);
				}
			}
			logger::info(FMT_STRING("CreateInteriorRefList ref_file_name: {}, base_file_name: {}"), ref_file_name, base_file_name);
			if (strcmp(base_file_name, MOD_NAME) == 0) {
				logger::info(FMT_STRING("CreateInteriorRefList ref base is in Bazaar Ream.esp: {:x}"), base_local_form_id);
				if (ignored_shelf_related_form_ids.find(base_local_form_id) != ignored_shelf_related_form_ids.end()) {
					logger::info("CreateInteriorRefList ref is an ignored shelf related form");
					continue;
				}
				auto maybe_shelf_type = shelf_types.find(base_local_form_id);
				if (maybe_shelf_type != shelf_types.end()) {
					logger::info("CreateInteriorRefList ref is a shelf!");
					uint32_t shelf_type = (*maybe_shelf_type).second;
					// TODO: actually set these values based off the state of the shelf
					uint32_t page = 1;
					uint32_t filter_form_type = 0;
					bool filter_is_food = false;
					const char* search = nullptr;
					const char* sort_on = nullptr;
					bool sort_asc = true;
					raw_shelves.push_back({
						shelf_type,
						position_x,
						position_y,
						position_z,
						angle_x,
						angle_y,
						angle_z,
						scale,
						page,
						filter_form_type,
						filter_is_food,
						search,
						sort_on,
						sort_asc,
					});
					continue;
				}
			}

			raw_interior_refs.push_back({
				base_file_name,
				base_local_form_id,
				ref_file_name,
				ref_local_form_id,
				position_x,
				position_y,
				position_z,
				angle_x,
				angle_y,
				angle_z,
				scale,
			});
		}
	}

	RawInteriorRef* refs_front = raw_interior_refs.empty() ? nullptr : &raw_interior_refs.front();
	RawShelf* shelves_front = raw_shelves.empty() ? nullptr : &raw_shelves.front();
	FFIResult<int32_t> result = update_interior_ref_list(api_url.c_str(), api_key.c_str(), shop_id, refs_front, raw_interior_refs.size(), shelves_front, raw_shelves.size());
	if (result.IsOk()) {
		int32_t interior_ref_list_id = result.AsOk();
		logger::info(FMT_STRING("CreateInteriorRefList success: {}"), interior_ref_list_id);
		successReg.SendEvent(interior_ref_list_id);
	} else {
		const char* error = result.AsErr();
		logger::error(FMT_STRING("CreateInteriorRefList failure: {}"), error);
		failReg.SendEvent(RE::BSFixedString(error));
	}
	successReg.Unregister(quest);
	failReg.Unregister(quest);
}

bool CreateInteriorRefList(RE::StaticFunctionTag*, RE::BSFixedString api_url, RE::BSFixedString api_key, int32_t shop_id, RE::TESQuest* quest) {
	logger::info("Entered CreateInteriorRefList");

	if (!quest) {
		logger::error("CreateInteriorRefList quest is null!");
		return false;
	}

	std::thread thread(CreateInteriorRefListImpl, api_url, api_key, shop_id, quest);
	thread.detach();
	return true;
}

bool ClearCell(RE::StaticFunctionTag*) {
	logger::info("Entered ClearCell");

	using func_t = bool(RE::TESObjectREFR* a_thisObj, void* a_param1, void* a_param2, double& a_result);
	// TODO: the cell will need to be dynamically passed in
	RE::TESObjectCELL * cell = RE::TESObjectCELL::LookupByEditorID<RE::TESObjectCELL>("BREmpty");
	logger::info(FMT_STRING("ClearCell lookup cell override name: {} id: {:x}"), cell->GetName(), (uint32_t)cell->GetFormID());
	if (!cell) {
		logger::error("ClearCell cell is null!");
		return false;
	}

	// Destroy existing references
	for (auto entry = cell->references.begin(); entry != cell->references.end();) {
		RE::TESObjectREFR * ref = (*entry).get();
		RE::FormID form_id = ref->GetFormID();
		logger::info(FMT_STRING("ClearCell ref form_id: {:x}"), (uint32_t)form_id);

		int mod_index = form_id >> 24;
		if (mod_index != 255) {
			// TODO: recognize somehow that this ref was a pre-placed initially disabled furnature upgrade piece and disable it now if so
			++entry;
		} else {
			logger::info(FMT_STRING("ClearCell ref is a temp ref, deleting {:x} {}"), (uint32_t)form_id, ref->GetName());
			ref->Disable(); // disabling first is required to prevent CTD on unloading cell
			ref->SetDelete(true);
			cell->references.erase(*entry++); // prevents slowdowns after many runs of ClearCell
		}
	}

	return true;
}

void LoadRefsTask(FFIResult<RawInteriorRefData> result, RE::TESObjectREFR* target_ref, RE::TESObjectREFR* private_chest, RE::TESObjectREFR* public_chest, RE::TESQuest* quest) {
	logger::info("Entered LoadRefsTask");

	if (!quest) {
		logger::error("LoadRefsTask quest is null!");
		return;
	}

	// Testing to see what ExtraLinkedRefChildren stores
	RE::ExtraLinkedRefChildren* linkedChildren = (RE::ExtraLinkedRefChildren*)public_chest->extraList.GetByType(RE::ExtraDataType::kLinkedRefChildren);
	if (linkedChildren) {
		logger::info(FMT_STRING("CreateMerchandiseList public_chest has linkedChildren: size: {}"), linkedChildren->linkedChildren.size());
	} else {
		logger::info("CreateMerchandiseList public_chest has no linkedChildren");
	}

	// Placing the refs must be done on the main thread otherwise calling MoveTo causes a crash
	auto task = SKSE::GetTaskInterface();
	task->AddTask([result, target_ref, private_chest, public_chest, quest]() {
		RE::TESDataHandler * data_handler = RE::TESDataHandler::GetSingleton();
		RE::BSScript::Internal::VirtualMachine * a_vm = RE::BSScript::Internal::VirtualMachine::GetSingleton();
		using func_t = decltype(&PlaceAtMe);
		REL::Relocation<func_t> PlaceAtMe_Native{ REL::ID(55672) };
		using func_t2 = decltype(&MoveTo);
		REL::Relocation<func_t2> MoveTo_Native(RE::Offset::TESObjectREFR::MoveTo);
		REL::ID extra_linked_ref_vtbl(static_cast<std::uint64_t>(229564));
		std::vector<RE::TESObjectREFR*> shelves;

		RE::BGSKeyword* shelf_keyword = data_handler->LookupForm<RE::BGSKeyword>(KEYWORD_SHELF, MOD_NAME);
		RE::BGSKeyword* chest_keyword = data_handler->LookupForm<RE::BGSKeyword>(KEYWORD_CHEST, MOD_NAME);
		RE::BGSKeyword* public_chest_keyword = data_handler->LookupForm<RE::BGSKeyword>(KEYWORD_PUBLIC_CHEST, MOD_NAME);
		RE::BGSKeyword* toggle_keyword = data_handler->LookupForm<RE::BGSKeyword>(KEYWORD_TOGGLE, MOD_NAME);
		RE::BGSKeyword* next_keyword = data_handler->LookupForm<RE::BGSKeyword>(KEYWORD_NEXT, MOD_NAME);
		RE::BGSKeyword* prev_keyword = data_handler->LookupForm<RE::BGSKeyword>(KEYWORD_PREV, MOD_NAME);
		
		SKSE::RegistrationMap<bool, std::vector<RE::TESObjectREFR*>> successReg = SKSE::RegistrationMap<bool, std::vector<RE::TESObjectREFR*>>();
		successReg.Register(quest, RE::BSFixedString("OnLoadInteriorRefListSuccess"));
		SKSE::RegistrationMap<RE::BSFixedString> failReg = SKSE::RegistrationMap<RE::BSFixedString>();
		failReg.Register(quest, RE::BSFixedString("OnLoadInteriorRefListFail"));

		if (!target_ref) {
			logger::error("LoadRefsTask target_ref is null!");
			failReg.SendEvent("Spawn target reference is null");
			successReg.Unregister(quest);
			failReg.Unregister(quest);
			return;
		}
		if (!private_chest) {
			logger::error("LoadRefsTask private_chest is null!");
			failReg.SendEvent("Private merchant chest reference is null");
			successReg.Unregister(quest);
			failReg.Unregister(quest);
			return;
		}
		if (!public_chest) {
			logger::error("LoadRefsTask public_chest is null!");
			failReg.SendEvent("Public merchant chest reference is null");
			successReg.Unregister(quest);
			failReg.Unregister(quest);
			return;
		}

		RE::TESObjectCELL * cell = RE::TESObjectCELL::LookupByEditorID<RE::TESObjectCELL>("BREmpty");
		logger::info(FMT_STRING("LoadRefsImpl lookup cell override name: {} id: {:x}"), cell->GetName(), (uint32_t)cell->GetFormID());
		if (!cell) {
			logger::error("LoadRefsTask cell is null!");
			failReg.SendEvent("Lookup failed for the shop Cell: BREmpty");
			successReg.Unregister(quest);
			failReg.Unregister(quest);
			return;
		}
	
		if (result.IsOk()) {
			logger::info("LoadInteriorRefList get_interior_ref_list result: OK");
			RawInteriorRefData data = result.AsOk();
			RawInteriorRefVec ref_vec = data.interior_ref_vec;
			logger::info(FMT_STRING("LoadInteriorRefList refs_vec len: {:d}, cap: {:d}"), ref_vec.len, ref_vec.cap);

			for (int i = 0; i < ref_vec.len; i++) {
				RawInteriorRef ref = ref_vec.ptr[i];
				logger::info(FMT_STRING("LoadInteriorRefList ref base_mod_name: {}, base_local_form_id: {:x}"), ref.base_mod_name, ref.base_local_form_id);
				logger::info(FMT_STRING("LoadInteriorRefList ref position {:.2f} {:.2f} {:.2f}, angle: {:.2f} {:.2f} {:.2f}, scale: {:d}"), ref.position_x, ref.position_y, ref.position_z, ref.angle_x, ref.angle_y, ref.angle_z, ref.scale);
				if (strcmp(ref.base_mod_name, "Skyrim.esm") == 0 && ref.base_local_form_id == 7) {
					logger::info("LoadInteriorRefList skipping player ref");
					continue;
				}

				RE::NiPoint3 position = RE::NiPoint3::NiPoint3(ref.position_x, ref.position_y, ref.position_z);
				RE::NiPoint3 angle = RE::NiPoint3::NiPoint3(ref.angle_x, ref.angle_y, ref.angle_z);
				RE::TESObjectREFR * game_ref = nullptr;
				if (ref.ref_mod_name) { // non-temp ref, try to lookup and reposition
					game_ref = data_handler->LookupForm<RE::TESObjectREFR>(ref.ref_local_form_id, ref.ref_mod_name);
					if (game_ref) {
						logger::info(FMT_STRING("LoadInteriorRefList lookup ref name: {}, form_id: {:x}"), game_ref->GetName(), (uint32_t)game_ref->GetFormID());
						if (game_ref->IsDisabled()) {
							logger::info("LoadInteriorRefList lookup ref is disabled, enabling");
							game_ref->formFlags &= ~RE::TESObjectREFR::RecordFlags::kInitiallyDisabled;
						}
					} else {
						logger::info(FMT_STRING("LoadInteriorRefList lookup ref not found, ref_mod_name: {}, ref_local_form_id: {:x}"), ref.ref_mod_name, ref.ref_local_form_id);
					}
				}

				if (!game_ref) { // temporary reference or non-temp ref not found, recreate from base form
					RE::TESForm * form = data_handler->LookupForm(ref.base_local_form_id, ref.base_mod_name);
					if (!form) { // form is not found, might be in an uninstalled mod
						logger::warn(FMT_STRING("LoadInteriorRefList not spawning ref for form that could not be found in installed mods, base_mod_name: {}, base_local_form_id: {:d}"), ref.base_mod_name, ref.base_local_form_id);
						continue;
					}
					logger::info(FMT_STRING("LoadInteriorRefList lookup form name: {}, form_id: {:x}"), form->GetName(), (uint32_t)form->GetFormID());
					game_ref = PlaceAtMe_Native(a_vm, 0, target_ref, form, 1, false, false);
					if (!game_ref) {
						logger::error("LoadInteriorRefList failed to place new ref in cell!");
						failReg.SendEvent("Failed to place a new ref into the cell");
						successReg.Unregister(quest);
						failReg.Unregister(quest);
						return;
					}
				}

				MoveTo_Native(game_ref, game_ref->CreateRefHandle(), cell, cell->worldSpace, position, angle);
				game_ref->data.angle = angle; // set angle directly to fix bug with MoveTo in an unloaded target cell
			}

			RawShelfVec shelf_vec = data.shelf_vec;
			logger::info(FMT_STRING("LoadInteriorRefList shelf_vec len: {:d}, cap: {:d}"), shelf_vec.len, shelf_vec.cap);
			for (int i = 0; i < shelf_vec.len; i++) {
				RawShelf shelf = shelf_vec.ptr[i];
				logger::info(FMT_STRING("LoadInteriorRefList shelf shelf_type: {}"), shelf.shelf_type);
				logger::info(FMT_STRING("LoadInteriorRefList shelf position {:.2f} {:.2f} {:.2f}, angle: {:.2f} {:.2f} {:.2f}, scale: {:d}"), shelf.position_x, shelf.position_y, shelf.position_z, shelf.angle_x, shelf.angle_y, shelf.angle_z, shelf.scale);
				RE::NiPoint3 position = RE::NiPoint3::NiPoint3(shelf.position_x, shelf.position_y, shelf.position_z);
				RE::NiPoint3 angle = RE::NiPoint3::NiPoint3(shelf.angle_x, shelf.angle_y, shelf.angle_z);
				auto maybe_form_id = shelf_form_ids.find(shelf.shelf_type);
				if (maybe_form_id != shelf_form_ids.end()) {
					logger::info(FMT_STRING("LoadInteriorRefList shelf form_id: {}"), (*maybe_form_id).second);
					RE::TESForm* form = data_handler->LookupForm((*maybe_form_id).second, MOD_NAME);
					if (!form) {
						logger::error("LoadInteriorRefList failed to find shelf base form!");
						failReg.SendEvent("Failed to place a shelf into the cell, could not find shelf base form");
						successReg.Unregister(quest);
						failReg.Unregister(quest);
						return;
					}
					RE::TESObjectREFR* shelf_ref = PlaceAtMe_Native(a_vm, 0, target_ref, form, 1, false, false);
					MoveTo_Native(shelf_ref, shelf_ref->CreateRefHandle(), cell, cell->worldSpace, position, angle);
					shelf_ref->data.angle = angle; // set angle directly to fix bug with MoveTo in an unloaded target cell
					shelves.push_back(shelf_ref);
					RE::ExtraLinkedRef* shelf_extra_linked_ref = RE::BSExtraData::Create<RE::ExtraLinkedRef>(extra_linked_ref_vtbl.address());
					shelf_extra_linked_ref->linkedRefs.push_back({public_chest_keyword, public_chest});
					shelf_extra_linked_ref->linkedRefs.push_back({chest_keyword, private_chest});
					shelf_ref->extraList.Add(shelf_extra_linked_ref);
					const std::map<ButtonType, RE::BGSKeyword*> button_type_to_keyword = {
						{ ButtonType::Toggle, toggle_keyword },
						{ ButtonType::Next, next_keyword },
						{ ButtonType::Previous, prev_keyword },
					};
					RE::NiMatrix3 rotation_matrix = get_rotation_matrix(angle);

					auto maybe_buttons = shelf_buttons.find(shelf.shelf_type);
					if (maybe_buttons != shelf_buttons.end()) {
						std::unordered_map<ButtonType, Button> buttons = (*maybe_buttons).second;
						for (auto entry = buttons.begin(); entry != buttons.end(); ++entry) {
							ButtonType button_type = (*entry).first;
							Button button = (*entry).second;
							
							RE::TESForm* button_form = data_handler->LookupForm(button.form_id, MOD_NAME);
							if (!button_form) {
								logger::error("LoadInteriorRefList failed to find shelf button base form!");
								failReg.SendEvent("Failed to place a shelf button into the cell, could not find shelf button base form");
								successReg.Unregister(quest);
								failReg.Unregister(quest);
								return;
							}
							RE::NiPoint3 button_position = RE::NiPoint3::NiPoint3(button.position.position_x, button.position.position_y, button.position.position_z);
							RE::NiPoint3 rotated_position = rotate_point(button_position, rotation_matrix);
							button_position = RE::NiPoint3::NiPoint3(position.x + rotated_position.x, position.y + rotated_position.y, position.z + rotated_position.z);
							RE::NiPoint3 button_angle = RE::NiPoint3::NiPoint3(angle.x + button.position.angle_x, angle.z + button.position.angle_y, angle.y + button.position.angle_z);
							logger::info(FMT_STRING("LoadInteriorRefList (adjusted) button position_x: {}, position_y: {}, position_z: {}, angle_x: {}, angle_y: {}, angle_z: {}"),
								button_position.x, button_position.y, button_position.z, button_angle.x, button_angle.y, button_angle.z);
							RE::TESObjectREFR* button_ref = PlaceAtMe_Native(a_vm, 0, target_ref, button_form, 1, false, false);
							MoveTo_Native(button_ref, button_ref->CreateRefHandle(), cell, cell->worldSpace, button_position, button_angle);
							button_ref->data.angle = button_angle; // set angle directly to fix bug with MoveTo in an unloaded target cell
							button_ref->refScale = button.position.scale;

							RE::ExtraLinkedRef* button_extra_linked_ref = RE::BSExtraData::Create<RE::ExtraLinkedRef>(extra_linked_ref_vtbl.address());
							button_extra_linked_ref->linkedRefs.push_back({shelf_keyword, shelf_ref});
							button_ref->extraList.Add(button_extra_linked_ref);

							auto maybe_keyword = button_type_to_keyword.find(button_type);
							if (maybe_keyword != button_type_to_keyword.end()) {
								RE::BGSKeyword* keyword = (*maybe_keyword).second;
								shelf_extra_linked_ref->linkedRefs.push_back({keyword, button_ref});
							}
						}
					} else {
						logger::warn(FMT_STRING("LoadInteriorRefList found no buttons for shelf_type: {}"), shelf.shelf_type);
					}
				} else {
					logger::error("LoadInteriorRefList unrecognized shelf type!");
					failReg.SendEvent(fmt::format(FMT_STRING("Failed to place a shelf into the cell, unrecognized shelf_type: {}"), shelf.shelf_type));
					successReg.Unregister(quest);
					failReg.Unregister(quest);
					return;
				}
			}
		} else {
			const char * error = result.AsErr();
			logger::error(FMT_STRING("LoadInteriorRefList get_interior_ref_list error: {}"), error);
			failReg.SendEvent(RE::BSFixedString(error));
			successReg.Unregister(quest);
			failReg.Unregister(quest);
			return;
		}

		successReg.SendEvent(true, shelves);
		successReg.Unregister(quest);
		failReg.Unregister(quest);
	});
}

void LoadInteriorRefListImpl(RE::BSFixedString api_url, RE::BSFixedString api_key, int32_t interior_ref_list_id, RE::TESObjectREFR* target_ref, RE::TESObjectREFR* private_chest, RE::TESObjectREFR* public_chest, RE::TESQuest* quest) {
	logger::info("Entered LoadInteriorRefListImpl");

	LoadRefsTask(get_interior_ref_list(api_url.c_str(), api_key.c_str(), interior_ref_list_id), target_ref, private_chest, public_chest, quest);
}

bool LoadInteriorRefList(RE::StaticFunctionTag*, RE::BSFixedString api_url, RE::BSFixedString api_key, int32_t interior_ref_list_id, RE::TESObjectREFR* target_ref, RE::TESObjectREFR* private_chest, RE::TESObjectREFR* public_chest, RE::TESQuest* quest) {
	logger::info("Entered LoadInteriorRefList");

	if (!target_ref) {
		logger::error("LoadInteriorRefList target_ref is null!");
		return false;
	}
	if (!private_chest) {
		logger::error("LoadInteriorRefList private_chest is null!");
		return false;
	}
	if (!public_chest) {
		logger::error("LoadInteriorRefList public_chest is null!");
		return false;
	}
	if (!quest) {
		logger::error("LoadInteriorRefList quest is null!");
		return false;
	}

	std::thread thread(LoadInteriorRefListImpl, api_url, api_key, interior_ref_list_id, target_ref, private_chest, public_chest, quest);
	thread.detach();
	return true;
}

void LoadInteriorRefListByShopIdImpl(RE::BSFixedString api_url, RE::BSFixedString api_key, int32_t shop_id, RE::TESObjectREFR* target_ref, RE::TESObjectREFR* private_chest, RE::TESObjectREFR* public_chest, RE::TESQuest* quest) {
	logger::info("Entered LoadInteriorRefListByShopIdImpl");

	LoadRefsTask(get_interior_ref_list_by_shop_id(api_url.c_str(), api_key.c_str(), shop_id), target_ref, private_chest, public_chest, quest);
}

bool LoadInteriorRefListByShopId(RE::StaticFunctionTag*, RE::BSFixedString api_url, RE::BSFixedString api_key, int32_t shop_id, RE::TESObjectREFR* target_ref, RE::TESObjectREFR* private_chest, RE::TESObjectREFR* public_chest, RE::TESQuest* quest) {
	logger::info("Entered LoadInteriorRefListByShopId");

	if (!target_ref) {
		logger::error("LoadInteriorRefListByShopId target_ref is null!");
		return false;
	}
	if (!private_chest) {
		logger::error("LoadInteriorRefListByShopId private_chest is null!");
		return false;
	}
	if (!public_chest) {
		logger::error("LoadInteriorRefListByShopId public_chest is null!");
		return false;
	}
	if (!quest) {
		logger::error("LoadInteriorRefListByShopId quest is null!");
		return false;
	}

	std::thread thread(LoadInteriorRefListByShopIdImpl, api_url, api_key, shop_id, target_ref, private_chest, public_chest, quest);
	thread.detach();
	return true;
}
