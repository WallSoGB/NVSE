#pragma once

#include "NiTypes.h"

class FormExtraDataManager 
{
protected:
	friend class FormExtraData;
	friend class LegacyFormExtraData;

	__declspec(noinline) static bool Add(TESForm* form, FormExtraData* formExtraData, bool legacyMode) noexcept;

	__declspec(noinline) static void RemoveByName(TESForm* form, const char* name, bool legacyMode) noexcept;
	__declspec(noinline) static void RemoveByPtr(TESForm* form, FormExtraData* formExtraData, bool legacyMode) noexcept;

	__declspec(noinline) static FormExtraData* Get(const TESForm* form, const char* name, bool legacyMode) noexcept;

	__declspec(noinline) static UInt32 GetAll(const TESForm* form, FormExtraData** outData, bool legacyMode) noexcept;

public:

	static void WriteHooks() noexcept;
};

class FormExtraData
{
public:
	enum {
		kVersion = 1
	};

	enum RemovalReason {
		kManualRequest = 0,
		kFormDeletion  = 1,
	};

	UInt32	nvseReserved = 0;
	UInt32	refCount = 0;

	FormExtraData() : nvseReserved(0), refCount(0) {}
	virtual ~FormExtraData() {};
	virtual void DeleteThis() {
		this->~FormExtraData();
		FormHeap_Free(this);
	};
	virtual const NiFixedString& GetName() const = 0;

	virtual UInt32 GetVersion() const { return kVersion; };

	virtual bool OnRemoval(TESForm* removedFrom, UInt32 removalReason) { return true; };

	virtual UInt32 Reserved0(void*, void*) { return 0; };
	virtual UInt32 Reserved1(void*, void*) { return 0; };
	virtual UInt32 Reserved2(void*, void*) { return 0; };
	virtual UInt32 Reserved3(void*, void*) { return 0; };

	void IncRefCount() {
		InterlockedIncrement(&refCount);
	}

	void DecRefCount() {
		if (InterlockedDecrement(&refCount) == 0) {
			DeleteThis();
		}
	}

	static bool Add(TESForm* form, FormExtraData* formExtraData) { return FormExtraDataManager::Add(form, formExtraData, false); }

	static void RemoveByName(TESForm* form, const char* name) { FormExtraDataManager::RemoveByName(form, name, false); }
	static void RemoveByPtr(TESForm* form, FormExtraData* formExtraData) { FormExtraDataManager::RemoveByPtr(form, formExtraData, false); };

	static FormExtraData* Get(const TESForm* form, const char* name) { return FormExtraDataManager::Get(form, name, false); }

	static UInt32 GetAll(const TESForm* form, FormExtraData** outData) { return FormExtraDataManager::GetAll(form, outData, false); }
};

// Deprecated, kept only for backwards compatibility
class LegacyFormExtraData 
{
public:
	NiFixedString	name;
	UInt32			refCount = 0;

	LegacyFormExtraData(const NiFixedString& aName) : name(aName), refCount(0) {}
	virtual ~LegacyFormExtraData() {};
	virtual void DeleteThis() {
		this->~LegacyFormExtraData();
		FormHeap_Free(this);
	};

	void IncRefCount() {
		InterlockedIncrement(&refCount);
	}

	void DecRefCount() {
		if (InterlockedDecrement(&refCount) == 0) {
			DeleteThis();
		}
	}

	static bool Add(TESForm* form, FormExtraData* formExtraData) { return FormExtraDataManager::Add(form, formExtraData, true); }

	static void RemoveByName(TESForm* form, const char* name) { FormExtraDataManager::RemoveByName(form, name, true); }
	static void RemoveByPtr(TESForm* form, FormExtraData* formExtraData) { FormExtraDataManager::RemoveByPtr(form, formExtraData, true); };

	static FormExtraData* Get(const TESForm* form, const char* name) { return FormExtraDataManager::Get(form, name, true); }

	static UInt32 GetAll(const TESForm* form, FormExtraData** outData) { return FormExtraDataManager::GetAll(form, outData, true); }
};