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

const ModInfo * DataHandler::LookupModByName(const char * modName) 
{
	return ThisStdCall<ModInfo*>(0x462F40, this, modName);
}

ModInfo* DataHandler::GetCompiledFile(UInt32 index) const 
{
	return ThisStdCall<ModInfo*>(0x465010, this, index);
}

UInt8 DataHandler::GetModIndex(const char* modName) 
{
	const ModInfo* pFile = LookupModByName(modName);
	return pFile ? pFile->modIndex : 0xFF;
}

const char* DataHandler::GetNthModName(UInt32 modIndex)
{
	const ModInfo* pFile = GetCompiledFile(modIndex);
	return pFile ? pFile->name : "";
}

void DataHandler::DisableAssignFormIDs(bool shouldAsssign)
{
	ThisStdCall(0x464D30, this, shouldAsssign);
}

UInt8 DataHandler::GetActiveModCount() const
{
	return ThisStdCall<UInt32>(0x51F550, this);
}

ModInfo::ModInfo() {
	//
};

ModInfo::~ModInfo() {
	//
};
