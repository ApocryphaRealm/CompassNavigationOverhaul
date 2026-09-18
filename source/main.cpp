#include "Hooks.h"
#include "Settings.h"

#include "utils/Logger.h"
#include "AddressLibraryGuard.h"

extern const SKSE::LoadInterface* skse;
void SKSEMessageListener(SKSE::MessagingInterface::Message* a_msg);

SKSEPluginLoad(const SKSE::LoadInterface* a_skse)
{
	REL::Module::reset();
	
	skse = a_skse;

	std::this_thread::sleep_for(6s);

	const SKSE::PluginDeclaration* plugin = SKSE::PluginDeclaration::GetSingleton();

	if (!logger::init(plugin->GetName()))
	{
		return false;
	}

	logger::info("Loading {} {}...", plugin->GetName(), plugin->GetVersion());

	// The guard runs BEFORE SKSE::Init: CommonLibSSE-NG's Init opens the Address Library itself, so a guard placed after it never ran when the file was missing (oproso's wheeler.log, 2026-09-18 - banner, then the bare failure, no [AddressLibrary] line).
	if (!AddressLibraryGuard::Guard(std::string(plugin->GetName()).c_str()))
	{
		logger::critical("[AddressLibrary] loading inert: no hooks, no listeners, nothing resolved");
		return true;
	}
	SKSE::Init(a_skse);

	// 1.1.4: name the Address Library file this game version needs, and whether it is there, BEFORE
	// any address is resolved; a missing file leaves the plugin inert with a message that names it
	// instead of CommonLib's bare "failed to open the address library file" (oproso, Fluorine on
	// SteamOS, 2026-09-17, who saw it from this patch and from Wheeler).

	settings::Init(std::string(plugin->GetName()) + ".ini");

	logger::set_level(settings::debug::logLevel, settings::debug::logLevel);
	logger::describe_level(std::string(plugin->GetName()) + ".ini");

	if (!SKSE::GetMessagingInterface()->RegisterListener("SKSE", SKSEMessageListener))
	{
		return false;
	}

	hooks::Install();

	logger::set_level(logger::level::info, logger::level::info);
	logger::info("Succesfully loaded!");

	logger::set_level(settings::debug::logLevel, settings::debug::logLevel);

	return true;
}
