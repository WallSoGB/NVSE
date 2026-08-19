#include "FormExtraData.h"
#include "SafeWrite.h"
#include <shared_mutex>
#include <ranges>

namespace 
{
	template<class T>
	class FormExtraDataMap : public std::unordered_map<const TESForm*, std::vector<NiPointer<T>>> {
	public:
		mutable std::shared_mutex mutex;

		bool __fastcall AddData(TESForm* form, T* formExtraData) noexcept {
			const NiFixedString name = formExtraData->GetName();
			if (!name) [[unlikely]]
				return false;

			std::unique_lock lock(mutex);

			auto iter = this->find(form);
			if (iter != this->end()) [[unlikely]] {
				auto& dataList = iter->second;
				if (std::ranges::any_of(dataList, [&](const NiPointer<T>& data) 
						{ return data && data->GetName() == name; }
					)) [[unlikely]]
				{
					return false; // Already exists
				}
			}

			(*this)[form].emplace_back(formExtraData);
			return true;
		}
	
		void __fastcall RemoveByName(TESForm* form, const char* name) noexcept {
			std::unique_lock lock(mutex);

			auto iter = this->find(form);
			if (iter != this->end()) [[likely]] {
				auto& dataList = iter->second;

				std::erase_if(dataList, [&](const NiPointer<T>& data) {
						if (data && data->GetName() == name) {
							data->OnRemoval(form, FormExtraData::RemovalReason::kManualRequest);
							return true;
						}
						return false;
					}
				);

				if (dataList.empty())
					this->erase(iter);
			}
		}

		void __fastcall RemoveByPtr(TESForm* form, T* formExtraData) noexcept {
			std::unique_lock lock(mutex);

			auto iter = this->find(form);
			if (iter != this->end()) [[likely]] {
				auto& dataList = iter->second;

				std::erase_if(dataList, [&](const NiPointer<T>& data) {
						if (data && data == formExtraData) {
							data->OnRemoval(form, FormExtraData::RemovalReason::kManualRequest);
							return true;
						}
						return false;
					}
				);

				if (dataList.empty())
					this->erase(iter);
			}
		}

		void __fastcall RemoveForForm(TESForm* form) noexcept {
			std::unique_lock lock(mutex);

			auto iter = this->find(form);
			if (iter != this->end()) {
				for (const auto& data : iter->second) {
					if (data)
						data->OnRemoval(form, FormExtraData::RemovalReason::kFormDeletion);
				}
				this->erase(iter);
			}
		}
	
		T* __fastcall Get(const TESForm* form, const char* name) const noexcept {
			std::shared_lock lock(mutex);

			auto iter = this->find(form);
			if (iter != this->end()) [[likely]] {
				for (const auto& data : iter->second) {
					if (data && data->GetName() == name)
						return data;
				}
			}
			return nullptr;
		}

		UInt32 __fastcall GetAll(const TESForm* form, T** outData) const noexcept {
			std::shared_lock lock(mutex);

			UInt32 count = 0;
			auto iter = this->find(form);
			if (iter != this->end()) [[likely]] {
				const auto& dataList = iter->second;
				count = static_cast<UInt32>(dataList.size());

				if (outData)
					memcpy(outData, dataList.data(), count);
			}
			return count;
		}
	};

	using ExtraDataFormMap			= FormExtraDataMap<FormExtraData>;
	using LegacyExtraDataFormMap	= FormExtraDataMap<LegacyFormExtraData>;

	ExtraDataFormMap g_formExtraDataMap;
	LegacyExtraDataFormMap g_legacyFormExtraDataMap;

#if RUNTIME
	UInt32 g_removeFromAllFormMapsAddr = 0x483C70;
#else
	UInt32 g_removeFromAllFormMapsAddr = 0x4FB910;
#endif
}

bool __fastcall FormExtraDataManager::Add(TESForm* form, FormExtraData* formExtraData, bool legacyMode) noexcept
{
	if (!form || !formExtraData) [[unlikely]]
		return false;

	if (legacyMode) [[unlikely]] {
		return g_legacyFormExtraDataMap.AddData(form, reinterpret_cast<LegacyFormExtraData*>(formExtraData));
	}
	else [[likely]] {
#ifdef _DEBUG
		if (formExtraData->GetVersion() > FormExtraData::kVersion)
			DebugBreak();
#endif
		return g_formExtraDataMap.AddData(form, formExtraData);
	}
}

void __fastcall FormExtraDataManager::RemoveByName(TESForm* form, const char* name, bool legacyMode) noexcept
{
	if (!form || !name) [[unlikely]]
		return;

	if (legacyMode) [[unlikely]] {
		g_legacyFormExtraDataMap.RemoveByName(form, name);
	}
	else [[likely]] {
		g_formExtraDataMap.RemoveByName(form, name);
	}
}

void __fastcall FormExtraDataManager::RemoveByPtr(TESForm* form, FormExtraData* formExtraData, bool legacyMode) noexcept
{
	if (!form || !formExtraData) [[unlikely]]
		return;

	if (legacyMode) [[unlikely]] {
		g_legacyFormExtraDataMap.RemoveByPtr(form, reinterpret_cast<LegacyFormExtraData*>(formExtraData));
	}
	else [[likely]] {
		g_formExtraDataMap.RemoveByPtr(form, formExtraData);
	}
}

FormExtraData* __fastcall FormExtraDataManager::Get(const TESForm* form, const char* name, bool legacyMode) noexcept
{
	if (!form || !name) [[unlikely]]
		return nullptr;

	if (legacyMode) [[unlikely]] {
		return reinterpret_cast<FormExtraData*>(g_legacyFormExtraDataMap.Get(form, name));
	}
	else [[likely]] {
		return g_formExtraDataMap.Get(form, name);
	}
}

UInt32 __fastcall FormExtraDataManager::GetAll(const TESForm* form, FormExtraData** outData, bool legacyMode) noexcept
{
	if (!form) [[unlikely]]
		return 0;

	if (legacyMode) [[unlikely]] {
		return g_legacyFormExtraDataMap.GetAll(form, reinterpret_cast<LegacyFormExtraData**>(outData));
	}
	else [[likely]] {
		return g_formExtraDataMap.GetAll(form, outData);
	}
}

static bool __fastcall RemoveFromAllFormsMapHook(TESForm* form) noexcept
{
	g_formExtraDataMap.RemoveForForm(form);
	{	[[unlikely]]
		g_legacyFormExtraDataMap.RemoveForForm(form);
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
