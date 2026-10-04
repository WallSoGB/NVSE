#include "GameData.h"


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

ModInfo* DataHandler::GetModByFormID(UInt32 formID) const {
	UInt32 modIndex = (formID >> 24) & 0xFF;
	if (SupportsSmallPugins() && modIndex == 0xFE) {
		modIndex = formID & 0xFFFFF000;
	}
	return GetMod(modIndex);
}

ModInfo* DataHandler::GetMod(UInt32 auiIndex) const {
#if RUNTIME
	return ThisStdCall<ModInfo*>(0x465010, this, auiIndex);
#else
	return ThisStdCall<ModInfo*>(0x4CDFA0, this, auiIndex);
#endif
}

const ModInfo * DataHandler::LookupModByName(const char * modName) const {
	return ThisStdCall<const ModInfo*>(0x462F40, this, modName);
}

UInt8 DataHandler::GetModIndex(const char* modName) const {
	const ModInfo* mod = LookupModByName(modName);
	if (mod)
		return mod->modIndex;

	return 0xFF;
}

const char* DataHandler::GetNthModName(UInt8 modIndex) const {
	if (SupportsSmallPugins() && modIndex == 0xFE)
		return "Small Mod";

	if (modList.GetNormalModCount() <= modIndex || modIndex == 0xFF)
		return "";

	ModInfo* modInfo = modList.GetMod(modIndex);
	if (modInfo)
		return modInfo->name;
	
	return "";
}

const char* DataHandler::GetNthModName(UInt8 modIndex, UInt16 smallIndex) const {
	UInt32 finalModIndex;
	if (SupportsSmallPugins() && modIndex == 0xFE)
		finalModIndex = smallIndex << 12 | modIndex << 24;
	else
		finalModIndex = modIndex;

	const ModInfo* mod = GetMod(finalModIndex);
	if (mod)
		return mod->name;
	else
		return "";
}

const char* DataHandler::GetModNameForForm(const TESForm* form) const {
	const UInt8 index = form->GetModIndex();
	if (SupportsSmallPugins() && index == 0xFE) {
		const UInt16 smallIndex = (form->refID & 0xFFF000) >> 12;
		return GetNthModName(0xFE, smallIndex);
	}

	return GetNthModName(index);
}

UInt32 DataHandler::GetNormalModCount() const {
	return modList.GetNormalModCount();
}

ModInfo* DataHandler::GetNormalMod(UInt32 auiIndex) const {
	return modList.GetMod(auiIndex);
}

UInt32 DataHandler::GetSmallModCount() const {
	if (SupportsSmallPugins())
		return modList.GetSmallModCount();
	return 0;
}

ModInfo* DataHandler::GetSmallMod(UInt32 auiIndex) const {
	if (SupportsSmallPugins())
		return modList.GetSmallMod(auiIndex);
	return nullptr;
}

UInt32 DataHandler::GetOverlayModCount() const {
	if (SupportsOverlayPugins())
		return modList.GetOverlayModCount();
	return 0;
}

ModInfo* DataHandler::GetOverlayMod(UInt32 auiIndex) const {
	if (SupportsOverlayPugins())
		return modList.GetOverlayMod(auiIndex);
	return nullptr;
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

	if (DataHandler::HasExtendedPlugins())
		return normalFiles.GetAt(modIndex);

	return loadedMods[modIndex];
}

ModInfo* ModList::GetSmallMod(UInt16 modIndex) const {
	if (modIndex >= GetSmallModCount())
		return nullptr;

	return smallFiles.GetAt(modIndex);
}

ModInfo* ModList::GetOverlayMod(UInt32 modIndex) const {
	if (modIndex >= GetOverlayModCount())
		return nullptr;

	return overlayFiles.GetAt(modIndex);
}

UInt32 ModList::GetNormalModCount() const {
	if (DataHandler::HasExtendedPlugins())
		return normalFiles.GetSize();

	return loadedModCount;
}

UInt32 ModList::GetSmallModCount() const {
	return smallFiles.GetSize();
}

UInt32 ModList::GetOverlayModCount() const {
	return overlayFiles.GetSize();
}
