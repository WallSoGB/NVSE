#include "GameData.h"

bool DataHandler::bHasExtendedPlugins = false;

#if RUNTIME
DataHandler* DataHandler::Get()
{
	DataHandler** g_dataHandler = (DataHandler**)0x011C3F2C;
	return *g_dataHandler;
}
#else

DataHandler* DataHandler::Get()
{
	DataHandler** g_dataHandler = (DataHandler**)0xED3B0C;
	return *g_dataHandler;
}

#endif

const ModInfo * DataHandler::LookupModByName(const char * modName)
{
	return ThisStdCall<const ModInfo*>(0x462F40, this, modName);
}

UInt8 DataHandler::GetModIndex(const char* modName)
{
	const ModInfo* mod = LookupModByName(modName);
	if (mod)
		return mod->modIndex;

	return 0xFF;
}

const char* DataHandler::GetNthModName(UInt32 modIndex)
{
	if (bHasExtendedPlugins && modIndex == 0xFE)
		return "Small Mod";

	if (modList.GetNormalModCount() <= modIndex || modIndex == 0xFF)
		return "";

	ModInfo* modInfo = modList.GetMod(modIndex);
	if (modInfo)
		return modInfo->name;
	
	return "";
}

void DataHandler::DisableAssignFormIDs(bool shouldAsssign)
{
	ThisStdCall(0x464D30, this, shouldAsssign);
}

struct IsModLoaded
{
	bool Accept(ModInfo* pModInfo) const {
		return pModInfo->IsLoaded();
	}
};

UInt8 DataHandler::GetActiveModCount() const
{
	return modList.GetNormalModCount();
}

ModInfo::ModInfo() {
	//
};

ModInfo::~ModInfo() {
	//
};

ModInfo* ModList::GetMod(UInt8 modIndex) const {
	if (modIndex >= GetNormalModCount())
		return nullptr;

	if (DataHandler::bHasExtendedPlugins)
		return normalFiles.GetAt(modIndex);

	return loadedMods[modIndex];
}

ModInfo* ModList::GetSmallMod(UInt16 modIndex) const {
	if (modIndex >= GetSmallModCount())
		return nullptr;

	if (DataHandler::bHasExtendedPlugins)
		return smallFiles.GetAt(modIndex);

	return nullptr;
}

ModInfo* ModList::GetOverlayMod(UInt32 modIndex) const {
	if (modIndex >= GetOverlayModCount())
		return nullptr;

	if (DataHandler::bHasExtendedPlugins)
		return overlayFiles.GetAt(modIndex);

	return nullptr;
}

UInt32 ModList::GetNormalModCount() const {
	if (DataHandler::bHasExtendedPlugins)
		return normalFiles.GetSize();

	return loadedModCount;
}

UInt32 ModList::GetSmallModCount() const {
	if (DataHandler::bHasExtendedPlugins)
		return smallFiles.GetSize();

	return 0;
}

UInt32 ModList::GetOverlayModCount() const {
	if (DataHandler::bHasExtendedPlugins)
		return overlayFiles.GetSize();

	return 0;
}
