#include "FormExtraData.h"
#include <shared_mutex>
#include <ranges>
#include "SafeWrite.h"

namespace 
{
	using LegacyExtraDataArray	= std::vector<NiPointer<LegacyFormExtraData>>;
	using ExtraDataArray		= std::vector<NiPointer<FormExtraData>>;

	template<class T>
	using FormMap				= std::unordered_map<TESForm*, T>;

	using ExtraDataFormMap			= FormMap<ExtraDataArray>;
	using LegacyExtraDataFormMap	= FormMap<LegacyExtraDataArray>;

	ExtraDataFormMap g_formExtraDataMap;
	LegacyExtraDataFormMap g_legacyFormExtraDataMap;
	std::shared_mutex g_formExtraDataCS[2];

	inline std::shared_mutex& GetExtraDataLock(bool legacy) noexcept { return g_formExtraDataCS[legacy]; }

#if RUNTIME
	UInt32 g_removeFromAllFormMapsAddr = 0x483C70;
#else
	UInt32 g_removeFromAllFormMapsAddr = 0x4FB910;
#endif
}

bool FormExtraDataManager::Add(TESForm* form, FormExtraData* formExtraData, bool legacyMode) noexcept
{
	if (!form || !formExtraData) [[unlikely]]
		return false;

	const NiFixedString name = legacyMode ? reinterpret_cast<LegacyFormExtraData*>(formExtraData)->name : formExtraData->GetName();
	if (!name) [[unlikely]]
		return false;

#ifdef _DEBUG
	if (!legacyMode && formExtraData->GetVersion() > FormExtraData::kVersion) {
		DebugBreak();
	}
#endif

	std::unique_lock lock(GetExtraDataLock(legacyMode));
	if (legacyMode) [[unlikely]] {
		auto& rMap = g_legacyFormExtraDataMap;
		auto iter = rMap.find(form);
		if (iter != rMap.end()) [[unlikely]] {
			auto& dataList = iter->second;
			if (std::ranges::any_of(dataList, [&](const NiPointer<LegacyFormExtraData>& data)
					{ return data && data->name == name; }
				)) 
			{
				return false; // Already exists
			}
		}

		rMap[form].emplace_back(reinterpret_cast<LegacyFormExtraData*>(formExtraData));
	}
	else [[likely]] {
		auto& rMap = g_formExtraDataMap;
		auto iter = rMap.find(form);
		if (iter != rMap.end()) [[unlikely]] {
			auto& dataList = iter->second;
			if (std::ranges::any_of(dataList, [&](const NiPointer<FormExtraData>& data) 
					{ return data && data->GetName() == name; }
				)) 
			{
				return false; // Already exists
			}
		}

		rMap[form].emplace_back(formExtraData);
	}

	return true;
}

void FormExtraDataManager::RemoveByName(TESForm* form, const char* name, bool legacyMode) noexcept
{
	if (!form || !name) [[unlikely]]
		return;

	std::unique_lock lock(GetExtraDataLock(legacyMode));
	if (legacyMode) [[unlikely]] {
		auto& rMap = g_legacyFormExtraDataMap;
		auto iter = rMap.find(form);
		if (iter != rMap.end()) [[likely]] {
			auto& dataList = iter->second;

			std::erase_if(dataList, [&](const NiPointer<LegacyFormExtraData>& data)
				{ return data && data->name == name; }
			);

			if (dataList.empty())
				rMap.erase(iter);
		}
	}
	else [[likely]] {
		auto& rMap = g_formExtraDataMap;
		auto iter = rMap.find(form);
		if (iter != rMap.end()) [[likely]] {
			auto& dataList = iter->second;

			std::erase_if(dataList, [&](const NiPointer<FormExtraData>& data)
				{
					if (data && data->GetName() == name) {
						data->OnRemoval(form, FormExtraData::RemovalReason::kManualRequest);
						return true;
					}
					return false;
				}
			);

			if (dataList.empty())
				rMap.erase(iter);
		}
	}
}

void FormExtraDataManager::RemoveByPtr(TESForm* form, FormExtraData* formExtraData, bool legacyMode) noexcept
{
	if (!form || !formExtraData) [[unlikely]]
		return;

	std::unique_lock lock(GetExtraDataLock(legacyMode));
	if (legacyMode) [[unlikely]] {
		auto& rMap = g_legacyFormExtraDataMap;
		auto iter = rMap.find(form);
		if (iter != rMap.end()) [[likely]] {
			auto& dataList = iter->second;

			std::erase_if(dataList, [&](const NiPointer<LegacyFormExtraData>& data) 
				{ return data && static_cast<void*>(data.m_pObject) == formExtraData; }
			);

			if (dataList.empty())
				rMap.erase(iter);
		}
	}
	else [[likely]] {
		auto& rMap = g_formExtraDataMap;
		auto iter = rMap.find(form);
		if (iter != rMap.end()) [[likely]] {
			auto& dataList = iter->second;

			std::erase_if(dataList, [&](const NiPointer<FormExtraData>& data) 
				{
					if (data && data == formExtraData) {
						data->OnRemoval(form, FormExtraData::RemovalReason::kManualRequest);
						return true;
					}
					return false;
				}
			);

			if (dataList.empty())
				rMap.erase(iter);
		}
	}
}

FormExtraData* FormExtraDataManager::Get(const TESForm* form, const char* name, bool legacyMode) noexcept
{
	if (!form || !name) [[unlikely]]
		return nullptr;

	std::shared_lock lock(GetExtraDataLock(legacyMode));
	if (legacyMode) [[unlikely]] {
		auto& rMap = g_legacyFormExtraDataMap;
		auto iter = rMap.find(const_cast<TESForm*>(form));

		if (iter != rMap.end()) [[likely]] {
			for (const auto& data : iter->second) {
				if (data && data->name == name)
					return reinterpret_cast<FormExtraData*>(data.m_pObject);
			}
		}
	}
	else [[likely]] {
		auto& rMap = g_formExtraDataMap;
		auto iter = rMap.find(const_cast<TESForm*>(form));

		if (iter != rMap.end()) [[likely]] {
			for (const auto& data : iter->second) {
				if (data && data->GetName() == name)
					return data;
			}
		}
	}

	return nullptr;
}

UInt32 FormExtraDataManager::GetAll(const TESForm* form, FormExtraData** outData, bool legacyMode) noexcept
{
	UInt32 count = 0;

	if (!form) [[unlikely]]
		return count;

	std::shared_lock lock(GetExtraDataLock(legacyMode));
	if (legacyMode) [[unlikely]] {
		auto& rMap = g_legacyFormExtraDataMap;
		auto iter = rMap.find(const_cast<TESForm*>(form));
		if (iter != rMap.end()) [[likely]] {
			const auto& dataList = iter->second;
			count = static_cast<UInt32>(dataList.size());

			if (outData) {
				for (UInt32 i = 0; i < count; ++i) {
					reinterpret_cast<LegacyFormExtraData**>(outData)[i] = dataList[i];
				}
			}
		}
	}
	else [[likely]] {
		auto& rMap = g_formExtraDataMap;
		auto iter = rMap.find(const_cast<TESForm*>(form));
		if (iter != rMap.end()) [[likely]] {
			const auto& dataList = iter->second;
			count = static_cast<UInt32>(dataList.size());

			if (outData) {
				for (UInt32 i = 0; i < count; ++i) {
					outData[i] = dataList[i];
				}
			}
		}
	}

	return count;
}

bool __fastcall RemoveFromAllFormsMapHook(TESForm* form) noexcept
{
	{	
		// Current map
		{
			std::unique_lock lock(GetExtraDataLock(false));
			auto& rMap = g_formExtraDataMap;
			auto iter = rMap.find(form);
			if (iter != rMap.end())
			{
				for (const auto& data : iter->second) {
					if (data)
						data->OnRemoval(form, FormExtraData::RemovalReason::kFormDeletion);
				}
				rMap.erase(iter);
			}
		}

		// Legacy map
		{
			std::unique_lock lock(GetExtraDataLock(true));
			auto& rMap = g_legacyFormExtraDataMap;
			auto iter = rMap.find(form);
			if (iter != rMap.end()) [[unlikely]] {
				rMap.erase(iter);
			}
		}
	}
	return ThisStdCall<bool>(g_removeFromAllFormMapsAddr, form);
}

void FormExtraDataManager::WriteHooks() noexcept
{
#if RUNTIME
	WriteRelCall(0x483669, &RemoveFromAllFormsMapHook, &g_removeFromAllFormMapsAddr);
#else
	WriteRelCall(0x4FD0C7, &RemoveFromAllFormsMapHook, &g_removeFromAllFormMapsAddr);
#endif
}
