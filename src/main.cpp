#include "version.h"
#include "bindings.h"

#include "BRClient.h"
#include "BROwner.h"
#include "BRShop.h"
#include "BRInteriorRefList.h"
#include "BRMerchandiseList.h"
#include "BRTransaction.h"

bool RegisterFuncs(RE::BSScript::IVirtualMachine* a_vm)
{
	a_vm->RegisterFunction("Init", "BRClient", Init);
	a_vm->RegisterFunction("StatusCheck", "BRClient", StatusCheck);
	a_vm->RegisterFunction("GenerateApiKey", "BRClient", GenerateApiKey);
	a_vm->RegisterFunction("Create", "BROwner", CreateOwner);
	a_vm->RegisterFunction("Update", "BROwner", UpdateOwner);
	a_vm->RegisterFunction("Create", "BRShop", CreateShop);
	a_vm->RegisterFunction("Update", "BRShop", UpdateShop);
	a_vm->RegisterFunction("Get", "BRShop", GetShop);
	a_vm->RegisterFunction("List", "BRShop", ListShops);
	a_vm->RegisterFunction("Create", "BRInteriorRefList", CreateInteriorRefList);
	a_vm->RegisterFunction("ClearCell", "BRInteriorRefList", ClearCell);
	a_vm->RegisterFunction("Load", "BRInteriorRefList", LoadInteriorRefList);
	a_vm->RegisterFunction("LoadByShopId", "BRInteriorRefList", LoadInteriorRefListByShopId);
	a_vm->RegisterFunction("Toggle", "BRMerchandiseList", ToggleMerchandise);
	a_vm->RegisterFunction("NextPage", "BRMerchandiseList", LoadNextMerchandise);
	a_vm->RegisterFunction("PrevPage", "BRMerchandiseList", LoadPrevMerchandise);
	a_vm->RegisterFunction("Load", "BRMerchandiseList", LoadMerchandiseByShopId);
	a_vm->RegisterFunction("Refresh", "BRMerchandiseList", RefreshMerchandise);
	a_vm->RegisterFunction("Replace3D", "BRMerchandiseList", ReplaceMerch3D);
	a_vm->RegisterFunction("Create", "BRMerchandiseList", CreateMerchandiseList);
	a_vm->RegisterFunction("GetQuantity", "BRMerchandiseList", GetMerchandiseQuantity);
	a_vm->RegisterFunction("GetPrice", "BRMerchandiseList", GetMerchandisePrice);
	a_vm->RegisterFunction("Create", "BRTransaction", CreateTransaction);
	return true;
}

extern "C" DLLEXPORT bool SKSEAPI SKSEPlugin_Query(const SKSE::QueryInterface* a_skse, SKSE::PluginInfo* a_info)
{
#ifndef NDEBUG
	auto sink = std::make_shared<spdlog::sinks::msvc_sink_mt>();
#else
	auto path = logger::log_directory();
	if (!path) {
		return false;
	}

	*path /= "BazaarRealmPlugin.log"sv;
	auto sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(path->string(), true);
#endif

	auto log = std::make_shared<spdlog::logger>("global log"s, std::move(sink));

#ifndef NDEBUG
	log->set_level(spdlog::level::trace);
#else
	log->set_level(spdlog::level::info);
	log->flush_on(spdlog::level::warn);
#endif

	spdlog::set_default_logger(std::move(log));
	spdlog::set_pattern("%g(%#): [%^%l%$] %v"s);

	logger::info(FMT_STRING("BazaarRealmPlugin v{}"), MYFP_VERSION_VERSTRING);

	a_info->infoVersion = SKSE::PluginInfo::kVersion;
	a_info->name = "BazaarRealmPlugin";
	a_info->version = MYFP_VERSION_MAJOR;

	if (a_skse->IsEditor()) {
		logger::critical("Loaded in editor, marking as incompatible"sv);
		return false;
	}

	const auto ver = a_skse->RuntimeVersion();
	if (ver < SKSE::RUNTIME_1_5_39) {
		logger::critical(FMT_STRING("Unsupported runtime version {}"), ver.string());
		return false;
	}

	return true;
}


extern "C" DLLEXPORT bool SKSEAPI SKSEPlugin_Load(const SKSE::LoadInterface* a_skse)
{
	logger::info("BazaarRealmPlugin loaded");

	SKSE::Init(a_skse);

	auto papyrus = SKSE::GetPapyrusInterface();
	if (!papyrus->Register(RegisterFuncs)) {
		return false;
	}

	return true;
}
