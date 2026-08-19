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
	
		bool __fastcall RemoveByName(TESForm* form, const char* name) noexcept {
			NiPointer<T> storedData;
			{
				std::unique_lock lock(mutex);

				auto iter = this->find(form);
				if (iter != this->end()) [[likely]] {
					auto& dataList = iter->second;

					std::erase_if(dataList, [&](const NiPointer<T>& data) {
						if (data && data->GetName() == name) {
							storedData = data;
							return true;
						}
						return false;
						}
					);

					if (dataList.empty())
						this->erase(iter);
				}
			}
			if (storedData)
				storedData->OnRemoval(form, FormExtraData::RemovalReason::kManualRequest);

			return storedData;
		}

		bool __fastcall RemoveByPtr(TESForm* form, T* formExtraData) noexcept {
			NiPointer<T> storedData;
			{
				std::unique_lock lock(mutex);

				auto iter = this->find(form);
				if (iter != this->end()) [[likely]] {
					auto& dataList = iter->second;

					std::erase_if(dataList, [&](const NiPointer<T>& data) {
						if (data && data == formExtraData) {
							storedData = data;
							return true;
						}
						return false;
						}
					);

					if (dataList.empty())
						this->erase(iter);
				}
			}
			if (storedData)
				storedData->OnRemoval(form, FormExtraData::RemovalReason::kManualRequest);

			return storedData;
		}

		void __fastcall RemoveForForm(TESForm* form, FormExtraData::RemovalReason reason) noexcept {
			std::vector<NiPointer<T>> storedData;
			{
				std::unique_lock lock(mutex);

				auto iter = this->find(form);
				if (iter != this->end()) {
					storedData = std::move(iter->second);
					this->erase(iter);
				}
			}
			for (const auto& data : storedData) {
				if (data)
					data->OnRemoval(form, reason);
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

		template<class arrayItem>
		UInt32 __fastcall GetAll(const TESForm* form, arrayItem* outData) const noexcept {
			std::shared_lock lock(mutex);

			UInt32 count = 0;
			auto iter = this->find(form);
			if (iter != this->end()) [[likely]] {
				const auto& dataList = iter->second;
				count = static_cast<UInt32>(dataList.size());

				if (count && outData) {
					for (UInt32 i = 0; i < count; ++i) {
						outData[i] = dataList[i];
					}
				}
					
			}
			return count;
		}
	};

	using ExtraDataFormMap			= FormExtraDataMap<FormExtraData>;
	using LegacyExtraDataFormMap	= FormExtraDataMap<LegacyFormExtraData>;

	ExtraDataFormMap g_formExtraDataMap;
	LegacyExtraDataFormMap g_legacyFormExtraDataMap;
}

bool __fastcall FormExtraDataManager::Add(TESForm* form, FormExtraData* formExtraData, bool legacyMode) noexcept
{
	if (!form || !formExtraData) [[unlikely]]
		return false;

	if (legacyMode) [[unlikely]] {
		return g_legacyFormExtraDataMap.AddData(form, reinterpret_cast<LegacyFormExtraData*>(formExtraData));
	}
	else [[likely]] {
		if (formExtraData->GetVersion() > FormExtraData::kVersion) [[unlikely]] {
#ifdef _DEBUG
			_DMESSAGE("Tried to add FormExtraData with a version newer than supported! (Got %i, max supported is %i)", formExtraData->GetVersion(), FormExtraData::kVersion);
			DebugBreak();
#endif
			return false;
		}
		return g_formExtraDataMap.AddData(form, formExtraData);
	}
}

bool __fastcall FormExtraDataManager::RemoveByName(TESForm* form, const char* name, bool legacyMode) noexcept
{
	if (!form || !name) [[unlikely]]
		return false;

	if (legacyMode) [[unlikely]] {
		return g_legacyFormExtraDataMap.RemoveByName(form, name);
	}
	else [[likely]] {
		return g_formExtraDataMap.RemoveByName(form, name);
	}
}

bool __fastcall FormExtraDataManager::RemoveByPtr(TESForm* form, FormExtraData* formExtraData, bool legacyMode) noexcept
{
	if (!form || !formExtraData) [[unlikely]]
		return false;

	if (legacyMode) [[unlikely]] {
		return g_legacyFormExtraDataMap.RemoveByPtr(form, reinterpret_cast<LegacyFormExtraData*>(formExtraData));
	}
	else [[likely]] {
		return g_formExtraDataMap.RemoveByPtr(form, formExtraData);
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

UInt32 __fastcall FormExtraDataManager::GetAll(const TESForm* form, NiPointer<FormExtraData>* outData) noexcept
{
	if (!form) [[unlikely]]
		return 0;

	return g_formExtraDataMap.GetAll<NiPointer<FormExtraData>>(form, outData);
}

UInt32 __fastcall FormExtraDataManager::LegacyGetAll(const TESForm* form, LegacyFormExtraData** outData) noexcept
{
	if (!form) [[unlikely]]
		return 0;

	return g_legacyFormExtraDataMap.GetAll<LegacyFormExtraData*>(form, outData);
}

namespace Hooks {

	namespace {
		__declspec(noinline) void __fastcall RemoveForm(TESForm* form, FormExtraData::RemovalReason reason) {
			g_formExtraDataMap.RemoveForForm(form, reason);
			{
				[[unlikely]]
				g_legacyFormExtraDataMap.RemoveForForm(form, reason);
			}
		}
	}

	template <UInt32 address, FormExtraData::RemovalReason reason>
	class RemoveFromAllFormsMapHook {
		static inline UInt32 replacedAddress = 0;

		static bool __fastcall Hook(TESForm* form) noexcept {
			RemoveForm(form, reason);
			return ThisStdCall<bool>(replacedAddress, form);
		}

	public:
		RemoveFromAllFormsMapHook() {
			WriteRelCall(address, &RemoveFromAllFormsMapHook::Hook, &replacedAddress);
		}
	};

	void InitHooks() 
	{
#if RUNTIME
		RemoveFromAllFormsMapHook<0x483669, FormExtraData::RemovalReason::kFormDeletion>();
		RemoveFromAllFormsMapHook<0x8680A4, FormExtraData::RemovalReason::kTrashedReference>();
#else
		RemoveFromAllFormsMapHook<0x4FD0C7, FormExtraData::RemovalReason::kFormDeletion>();
		// GECK has no garbage collector
#endif
	}

}



void FormExtraDataManager::WriteHooks() noexcept
{
	Hooks::InitHooks();
}
