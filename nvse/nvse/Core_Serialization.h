#pragma once

enum ModListType : UInt32 {
	kNormal = 0,
	kSmall = 1,
	kMedium = 2,

	kCount
};

extern std::vector<const class ModInfo*> g_modList[ModListType::kCount];
extern std::vector<std::string> g_modsLoaded[ModListType::kCount];

void Init_CoreSerialization_Callbacks();

