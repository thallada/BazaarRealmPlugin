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
		// TODO: skip saving the ref if it is part of mod file
		//   OR: continue to save it, but with a special flag (useful in order to allow repositioning default statics)
		//     Then, during Load, skip PlaceAtMe, lookup the ref by id and then reposition it
		RE::TESObjectREFR * ref = (*entry).get();
		const char * name = ref->GetName();
		logger::info(FMT_STRING("CreateInteriorRefList ref: {}"), name);
		const RE::TESBoundObject * base = ref->GetBaseObject();
		if (base) {
			RE::FormID base_form_id = base->GetFormID();
			if (base_form_id == 7) {
				// skip saving player ref
				continue;
			}
			const char * form_name = base->GetName();
			const RE::FormType form_type = base->GetFormType();
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
			//_MESSAGE("FILE: %s isLight: %d formID: 0x%x localFormId: 0x%x", file_name, is_light, form_id, local_form_id);
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
	for (auto entry = cell->references.begin(); entry != cell->references.end(); ++entry)
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
		}
		else {
			logger::info(FMT_STRING("ClearCell ref is a temp ref, deleting {:x} {}"), (uint32_t)form_id, ref->GetName());
			ref->Disable(); // disabling first is required to prevent CTD on unloading cell
			ref->SetDelete(true);
		}
	}

	return true;
}

// TODO: store the FFIResult in a HashMap cache (with ids assigned from incrementing a global int) and return its key in this function
//   * bool BRResult.IsOk(int resultId) ; if false, then only GetErr() shall be accessed
//   * bool BRResult.IsErr(int resultId) ; maybe not needed
//   * string BRResult.GetErr(int resultId) ; always a string
//	 * string BRResult.GetType(int resultId) ; returns type of Ok value encoded as string
//   * BRResult.Drop(int resultId) ; once we are done using the data from the result or displaying an error, delete it from the cache and free the memory

// These will panic if a) the result is an error or b) the type of the result value different than what was requested
// That will cause CTDs but there's not much else to do, silently causing undefined behavior would be worse
// Every step of this API has an explicit check you must make before continuing. If you break that contract, that's on you, the scripter.
//   * string BRResult.GetString(int resultId)
//   * int BRResult.GetInt(int resultId)
//   * bool BRResult.GetBool(int resultId)
//   * int BRResult.GetResponseId(int resultId)

// For BRResponse (maybe just completely get rid of this):
//   * int BRResponse.GetStatus(int responseId) ; if >= 300 then none of the other functions should be accessed
//   * bool BRResponse.IsFromCache(int responseId)
//   * string BRResponse.GetType(int responseId) ; may return things like "string" or "shop"
//   * string BRResponse.GetString(int responseId) ; is this going to be needed?
//	 * int BRResponse.GetShopId(int responseId) ; for a get_shop request
//   * int BRResponse.GetInteriorRefListId(int responseId) ; for a get_interior_ref_list request
//   * BRResponse.Drop()

// The following will return values from a HashMap cache on the rust-side where entries are not evicted unless explicitly told to by papyrus.
// These methods will panic if there isn't an entry in the cache for the id. This can happen if a request was not made beforehand and verified to be non-err result with a non-error response.
// For BRShop
//   * string BRShop.GetName(int shopId)
//   * int BRShop.GetOwnerId(int shopId)
//   * BRShop.Drop()

// For BROwner
//   * string BROwner.GetName(int ownerId)
//   * BROwner.Drop()

// Should split this up into GetInteriorRefList and LoadInteriorRefList.
// Get: makes the request to the api and stores the Result in a rust cache, returns id of Result
// Load: given id of interior ref list, gets the data from the rust cache, does the PlaceAtMe loop and returns an id of another Result
bool LoadInteriorRefListImpl(RE::BSFixedString api_url, RE::BSFixedString api_key, uint32_t interior_ref_list_id, RE::TESObjectREFR* target_ref, RE::TESQuest* quest)
{
	logger::info("Entered LoadInteriorRefListImpl");

	if (!quest) {
		logger::error("LoadInteriorRefList quest is null!");
		return false;
	}

	SKSE::RegistrationMap<bool> regMap = SKSE::RegistrationMap<bool>();
	regMap.Register(quest, RE::BSFixedString("OnLoadInteriorRefList"));
	
	RE::TESDataHandler * data_handler = RE::TESDataHandler::GetSingleton();
	RE::BSScript::Internal::VirtualMachine * a_vm = RE::BSScript::Internal::VirtualMachine::GetSingleton();
	using func_t = decltype(&PlaceAtMe);
	REL::Relocation<func_t> PlaceAtMe_Native{ REL::ID(55672) };
	using func_t2 = decltype(&MoveTo);
	REL::Relocation<func_t2> MoveTo_Native(RE::Offset::TESObjectREFR::MoveTo);
	
	// testing returning a script object
	// RE::BSTSmartPointer<RE::BSScript::Object> ret = RE::BSTSmartPointer<RE::BSScript::Object>::BSTSmartPointer();
	// a_vm->CreateObject(RE::BSFixedString("BRResult"), ret);
	// RE::BSScript::Variable var = RE::BSScript::Variable::Variable(RE::BSScript::TypeInfo::TypeInfo(RE::BSScript::TypeInfo::RawType::kBool));
	// var.SetBool(false);
	// a_vm->SetPropertyValue(ret, "value", var);
	// RE::BSScript::Variable ret_var = RE::BSScript::Variable::Variable(RE::BSScript::TypeInfo::TypeInfo(RE::BSScript::TypeInfo::RawType::kObject));
	// ret_var.SetObject(ret);
	// return ret_var;

	RE::TESObjectCELL * cell = RE::TESObjectCELL::LookupByEditorID<RE::TESObjectCELL>("BREmpty");
	logger::info(FMT_STRING("LoadInteriorRefList lookup cell override name: {} id: {:x}"), cell->GetName(), (uint32_t)cell->GetFormID());
	if (!cell) {
		logger::error("LoadInteriorRefList cell is null!");
		regMap.SendEvent(false);
		regMap.Unregister(quest);
		return false;
	}

	//RE::TESObjectREFR * x_marker = data_handler->LookupForm<RE::TESObjectREFR>(6628, "BazaarRealm.esp");
	if (target_ref) {
		FFIResult<RefRecordVec> result = get_interior_ref_list(api_url.c_str(), api_key.c_str(), interior_ref_list_id);
		if (result.IsOk()) {
			logger::info("LoadInteriorRefList get_interior_ref_list result: OK");
			RefRecordVec vec = result.AsOk();
			logger::info(FMT_STRING("LoadInteriorRefList vec len: {:d}, cap: {:d}"), vec.len, vec.cap);

			for (int i = 0; i < vec.len; i++) {
				RefRecord ref = vec.ptr[i];
				logger::info(FMT_STRING("LoadInteriorRefList ref base_mod_name: {}, base_local_form_id: {:x}"), ref.base_mod_name, ref.base_local_form_id);
				logger::info(FMT_STRING("LoadInteriorRefList ref position {:.2f} {:.2f} {:.2f}, angle: {:.2f} {:.2f} {:.2f}, scale: {:d}"), ref.position_x, ref.position_y, ref.position_z, ref.angle_x, ref.angle_y, ref.angle_z, ref.scale);
				if (ref.base_local_form_id == 7) {
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

						// Failed experiment at trying to call the ObjectReference.Enable() papyrus native function
						//RE::BSTSmartPointer<RE::BSScript::Object> object;
						//RE::BSTSmartPointer<RE::BSScript::IStackCallbackFunctor> callback = RE::BSTSmartPointer<RE::BSScript::IStackCallbackFunctor>::BSTSmartPointer(RE::BSScript::IStackCallbackFunctor());
						//a_vm->CreateObject(RE::BSFixedString("ObjectReference"), game_ref, object);
						//_MESSAGE("Created Object isConstructed: %d, isInitialized: %d, isValid: %d, type: %s", object->IsConstructed(), object->IsInitialized(), object->IsValid(), object->GetTypeInfo()->GetName());
						//RE::BSTHashMap<RE::VMStackID, RE::BSTSmartPointer<RE::BSScript::Stack>> runningStacks = a_vm->allRunningStacks;
						//_MESSAGE("allRunningStacks size: %d", runningStacks.size());
						//for (auto entry = runningStacks.begin(); entry != runningStacks.end(); ++entry) {
							//_MESSAGE("allRunningStacks %d", entry->first);
						//}
						//a_vm->DispatchMethodCall(object, RE::BSFixedString("Enable"), RE::MakeFunctionArguments<>(), RE::BSScript::IStackCallbackFunctor::IStackCallbackFunctor())
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

				RE::ObjectRefHandle handle = game_ref->CreateRefHandle();
				logger::info(FMT_STRING("LoadInteriorRefList ref handle: {:x}, game_ref: {:x}"), (uint32_t)handle.get().get(), (uint32_t)game_ref);
				MoveTo_Native(game_ref, handle, cell, cell->worldSpace, position, angle);
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
		
	}
	else {
		logger::error("LoadInteriorRefList target_ref is null!");
		regMap.SendEvent(false);
		regMap.Unregister(quest);
		return false;
	}
	
	//RE::TESForm * gold = RE::TESForm::LookupByID(15);
	//_MESSAGE("Gold form name: %s", gold->GetName());
	//_MESSAGE("Gold form id: %d", gold->GetFormID());
	//using func_t = decltype(&PlaceAtMe);
	//REL::Offset<func_t> func{ REL::ID(55672) };
	//RE::TESObjectREFR * new_ref = func(RE::BSScript::Internal::VirtualMachine::GetSingleton(), 1, player, gold, 50, false, false);
	//_MESSAGE("New ref initially disabled: %d", new_ref->IsDisabled());
	//_MESSAGE("New ref persistent: %d", new_ref->loadedData->flags & RE::TESObjectREFR::RecordFlags::kPersistent);
	regMap.SendEvent(true);
	regMap.Unregister(quest);
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

	LoadInteriorRefListImpl(api_url, api_key, interior_ref_list_id, target_ref, quest);
	// TODO: making this async causes a crash
	// std::thread thread(LoadInteriorRefListImpl, api_url, api_key, interior_ref_list_id, target_ref, quest);
	// thread.detach();
	return true;
}
