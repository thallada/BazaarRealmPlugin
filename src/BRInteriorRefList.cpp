#include "bindings.h"

#include "NativeFunctions.h"

int CreateInteriorRefListImpl(RE::BSFixedString api_url, RE::BSFixedString api_key, uint32_t shop_id, RE::TESQuest* quest)
{	
	logger::info("Entered CreateInteriorRefListImpl");

	if (!quest) {
		logger::error("LoadInteriorRefList quest is null!");
		return -1;
	}

	SKSE::RegistrationMap<int> regMap = SKSE::RegistrationMap<int>();
	regMap.Register(quest, RE::BSFixedString("OnCreateInteriorRefList"));

	RE::TESObjectCELL * cell = RE::TESObjectCELL::LookupByEditorID<RE::TESObjectCELL>("BREmpty");
	logger::info(FMT_STRING("ClearCell lookup cell override name: {} id: {:x}"), cell->GetName(), (uint32_t)cell->GetFormID());
	if (!cell) {
		logger::error("ClearCell cell is null!");
		regMap.SendEvent(-1);
		regMap.Unregister(quest);
		return -1;
	}

	RE::TESDataHandler * data_handler = RE::TESDataHandler::GetSingleton();
	std::vector<RefRecord> ref_records;
	for (auto entry = cell->references.begin(); entry != cell->references.end(); ++entry)
	{
		RE::TESObjectREFR * ref = (*entry).get();
		const char * name = ref->GetName();
		logger::info(FMT_STRING("CreateInteriorRefList ref: {}"), name);
		const RE::TESBoundObject * base = ref->GetBaseObject();
		if (base) {
			RE::FormID base_form_id = base->GetFormID();
			const RE::FormType form_type = base->GetFormType();
			if (base_form_id == 0x7 || form_type == RE::FormType::ActorCharacter) {
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
			RE::NiNPShortPoint3 boundMin = base->boundData.boundMin;
			RE::NiNPShortPoint3 boundMax = base->boundData.boundMax;
			uint16_t bound_x = boundMax.x > boundMin.x ? boundMax.x - boundMin.x : boundMin.x - boundMax.x;
			uint16_t bound_y = boundMax.y > boundMin.y ? boundMax.y - boundMin.y : boundMin.y - boundMax.y;
			uint16_t bound_z = boundMax.z > boundMin.z ? boundMax.z - boundMin.z : boundMin.z - boundMax.z;
			logger::info(FMT_STRING("CreateInteriorRefList bounds: width: {:d}, length: {:d}, height: {:d}"), bound_x, bound_y, bound_z);
			uint16_t scale = ref->refScale;
			logger::info(FMT_STRING("CreateInteriorRefList position: {:.2f}, {:.2f}, {:.2f} angle: {:.2f}, {:.2f}, {:.2f}  scale: {:d}"), position_x, position_y, position_z, angle_x, angle_y, angle_z, scale);
			logger::info(FMT_STRING("CreateInteriorRefList deleted: {:d}, wants delete: {:d}"), ref->IsMarkedForDeletion(), ref->inGameFormFlags.all(RE::TESObjectREFR::InGameFormFlag::kWantsDelete));
			
			RE::TESFile * base_file = base->GetFile(0);
			char * base_file_name = base_file->fileName;
			bool is_light = base_file->recordFlags.all(RE::TESFile::RecordFlag::kSmallFile);
			uint32_t base_local_form_id = is_light ? base_form_id & 0xfff : base_form_id & 0xFFFFFF;
			RE::FormID ref_form_id = ref->GetFormID();
			uint16_t ref_mod_index = ref_form_id >> 24;
			char * ref_file_name = nullptr;
			uint32_t ref_local_form_id = ref_form_id & 0xFFFFFF;
			if (ref_mod_index != 255) { // not a temp ref
				if (ref_mod_index == 254) { // is a light mod
					ref_local_form_id = ref_form_id & 0xfff;
					ref_mod_index = (ref_form_id >> 12) & 0xfff;
					const RE::TESFile * ref_file = data_handler->LookupLoadedLightModByIndex(ref_mod_index);
					ref_file_name = _strdup(ref_file->fileName);
				}
				else {
					const RE::TESFile * ref_file = data_handler->LookupLoadedModByIndex(ref_mod_index);
					ref_file_name = _strdup(ref_file->fileName);
				}
			}

			ref_records.push_back({
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

	int interior_ref_list_id = create_interior_ref_list(api_url.c_str(), api_key.c_str(), shop_id, &ref_records[0], ref_records.size());
	logger::info(FMT_STRING("CreateInteriorRefList result: {}"), interior_ref_list_id);
	regMap.SendEvent(interior_ref_list_id);
	regMap.Unregister(quest);
	return interior_ref_list_id;
}

bool CreateInteriorRefList(RE::StaticFunctionTag*, RE::BSFixedString api_url, RE::BSFixedString api_key, uint32_t shop_id, RE::TESQuest* quest) {
	logger::info("Entered CreateInteriorRefList");

	if (!quest) {
		logger::error("LoadInteriorRefList quest is null!");
		return false;
	}

	std::thread thread(CreateInteriorRefListImpl, api_url, api_key, shop_id, quest);
	thread.detach();
	return true;
}

bool ClearCell(RE::StaticFunctionTag*)
{
	logger::info("Entered ClearCell");

	RE::TESDataHandler * data_handler = RE::TESDataHandler::GetSingleton();
	using func_t = bool(RE::TESObjectREFR* a_thisObj, void* a_param1, void* a_param2, double& a_result);
	RE::TESObjectCELL * cell = RE::TESObjectCELL::LookupByEditorID<RE::TESObjectCELL>("BREmpty");
	logger::info(FMT_STRING("ClearCell lookup cell override name: {} id: {:x}"), cell->GetName(), (uint32_t)cell->GetFormID());
	if (!cell) {
		logger::error("ClearCell cell is null!");
		return false;
	}

	// Destroy existing references
	for (auto entry = cell->references.begin(); entry != cell->references.end();)
	{
		RE::TESObjectREFR * ref = (*entry).get();
		RE::FormID form_id = ref->GetFormID();
		logger::info(FMT_STRING("ClearCell ref form_id: {:x}"), (uint32_t)form_id);

		int mod_index = form_id >> 24;
		if (mod_index != 255) {
			RE::TESFile * file = ref->GetDescriptionOwnerFile();
			if (file) {
				bool is_light = file->recordFlags.all(RE::TESFile::RecordFlag::kSmallFile);
				uint32_t local_form_id = is_light ? ref->GetFormID() & 0xfff : form_id & 0xFFFFFF;
				if (!data_handler->LookupForm<RE::TESObjectREFR>(local_form_id, file->fileName)) {
					logger::info(FMT_STRING("ClearCell ref was not in mod file! {:x} {}"), local_form_id, ref->GetName());
				}
				else {
					logger::info(FMT_STRING("ClearCell ref in mod file {:x} {}"), local_form_id, ref->GetName());
				}
			} else {
				logger::info(FMT_STRING("ClearCell ref not in ANY file! {:x} {}"), (uint32_t)form_id, ref->GetName());
			}
			++entry;
		}
		else {
			logger::info(FMT_STRING("ClearCell ref is a temp ref, deleting {:x} {}"), (uint32_t)form_id, ref->GetName());
			ref->Disable(); // disabling first is required to prevent CTD on unloading cell
			ref->SetDelete(true);
			cell->references.erase(*entry++); // prevents slowdowns after many runs of ClearCell
		}
	}

	return true;
}

bool LoadInteriorRefListImpl(RE::BSFixedString api_url, RE::BSFixedString api_key, uint32_t interior_ref_list_id, RE::TESObjectREFR* target_ref, RE::TESQuest* quest)
{
	logger::info("Entered LoadInteriorRefListImpl");

	if (!quest) {
		logger::error("LoadInteriorRefList quest is null!");
		return false;
	}

	RE::TESDataHandler * data_handler = RE::TESDataHandler::GetSingleton();
	RE::BSScript::Internal::VirtualMachine * a_vm = RE::BSScript::Internal::VirtualMachine::GetSingleton();
	using func_t = decltype(&PlaceAtMe);
	REL::Relocation<func_t> PlaceAtMe_Native{ REL::ID(55672) };
	using func_t2 = decltype(&MoveTo);
	REL::Relocation<func_t2> MoveTo_Native(RE::Offset::TESObjectREFR::MoveTo);
	
	RE::TESObjectCELL * cell = RE::TESObjectCELL::LookupByEditorID<RE::TESObjectCELL>("BREmpty");
	logger::info(FMT_STRING("LoadInteriorRefList lookup cell override name: {} id: {:x}"), cell->GetName(), (uint32_t)cell->GetFormID());
	if (!cell) {
		logger::error("LoadInteriorRefList cell is null!");
		return false;
	}

	if (target_ref) {
		FFIResult<RefRecordVec> result = get_interior_ref_list(api_url.c_str(), api_key.c_str(), interior_ref_list_id);

		// Placing the refs must be done on the main thread otherwise calling MoveTo causes a crash
		auto task = SKSE::GetTaskInterface();
		task->AddTask([result, target_ref, quest, cell, data_handler, a_vm, MoveTo_Native, PlaceAtMe_Native]() {
			SKSE::RegistrationMap<bool> regMap = SKSE::RegistrationMap<bool>();
			regMap.Register(quest, RE::BSFixedString("OnLoadInteriorRefList"));
		
			if (result.IsOk()) {
				logger::info("LoadInteriorRefList get_interior_ref_list result: OK");
				RefRecordVec vec = result.AsOk();
				logger::info(FMT_STRING("LoadInteriorRefList vec len: {:d}, cap: {:d}"), vec.len, vec.cap);

				for (int i = 0; i < vec.len; i++) {
					RefRecord ref = vec.ptr[i];
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
						}
						else {
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
							regMap.SendEvent(false);
							regMap.Unregister(quest);
							return false;
						}
					}

					MoveTo_Native(game_ref, game_ref->CreateRefHandle(), cell, cell->worldSpace, position, angle);
					game_ref->data.angle = angle; // set angle directly to fix bug with MoveTo in an unloaded target cell
				}
			}
			else {
				const char * error = result.AsErr();
				logger::error(FMT_STRING("LoadInteriorRefList get_interior_ref_list error: {}"), error);
				regMap.SendEvent(false);
				regMap.Unregister(quest);
				return false;
			}

			regMap.SendEvent(true);
			regMap.Unregister(quest);
		});
	}
	else {
		logger::error("LoadInteriorRefList target_ref is null!");
		return false;
	}
	
	return true;
}

bool LoadInteriorRefList(RE::StaticFunctionTag*, RE::BSFixedString api_url, RE::BSFixedString api_key, uint32_t interior_ref_list_id, RE::TESObjectREFR* target_ref, RE::TESQuest* quest) {
	logger::info("Entered LoadInteriorRefList");

	if (!target_ref) {
		logger::error("LoadInteriorRefList target_ref is null!");
		return false;
	}
	if (!quest) {
		logger::error("LoadInteriorRefList quest is null!");
		return false;
	}

	std::thread thread(LoadInteriorRefListImpl, api_url, api_key, interior_ref_list_id, target_ref, quest);
	thread.detach();
	return true;
}
