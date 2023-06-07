#pragma once
#include <Windows.h>
#include <string>
#include <vector>

#define X86_SETNZ 0x0F // 0F 95
#define X86_PUSH  0x68
#define X86_PUSH2 0x6A
#define X86_TEST  0x85
#define X86_EAX   0x87
#define X86_MOV0  0x88
#define X86_MOV1  0x89
#define X86_MOV2  0x8B
#define X86_LEA   0x8D
#define X86_ECX   0x8F
#define X86_MOV3  0xC7
#define X86_EBX   0xCB
#define X86_CALL  0xE8
#define X86_MOVSS 0xF3 // F3 0F
#define X86_NOP   0x90

class CMemAddr
{
public:
	enum class Direction : int
	{
		DOWN = 0,
		UP,
	};

	CMemAddr(void) = default;
	CMemAddr(const uintptr_t ptr) : ptr(ptr) {}
	CMemAddr(const void* ptr) : ptr(uintptr_t(ptr)) {}

	inline operator uintptr_t(void) const
	{
		return ptr;
	}

	inline operator void* (void) const
	{
		return reinterpret_cast<void*>(ptr);
	}

	inline operator bool(void) const
	{
		return ptr != NULL;
	}

	inline bool operator!= (const CMemAddr& addr) const
	{
		return ptr != addr.ptr;
	}

	inline bool operator== (const CMemAddr& addr) const
	{
		return ptr == addr.ptr;
	}

	inline bool operator== (const uintptr_t& addr) const
	{
		return ptr == addr;
	}

	inline uintptr_t GetPtr(void) const
	{
		return ptr;
	}

	template<class T> inline T GetValue(void) const
	{
		return *reinterpret_cast<T*>(ptr);
	}

	template<class T> inline T GetVirtualFunctionIndex(void) const
	{
		return *reinterpret_cast<T*>(ptr) / 8; // Its divided by 8 in x64.
	}

	template<typename T> inline T CCast(void) const
	{
		return (T)ptr;
	}

	template<typename T> inline T RCast(void) const
	{
		return reinterpret_cast<T>(ptr);
	}

	inline CMemAddr Offset(ptrdiff_t offset) const
	{
		return CMemAddr(ptr + offset);
	}

	inline CMemAddr Deref(int deref = 1) const
	{
		uintptr_t reference = ptr;

		while (deref--)
		{
			if (reference)
				reference = *reinterpret_cast<uintptr_t*>(reference);
		}

		return CMemAddr(reference);
	}

	inline CMemAddr WalkVTable(ptrdiff_t vfuncIndex)
	{
		uintptr_t reference = ptr + (8 * vfuncIndex);
		return CMemAddr(reference);
	}

	bool CheckOpCodes(const std::vector<uint8_t> vOpcodeArray) const;
	void Patch(const std::vector<uint8_t> vOpcodeArray) const;
	void PatchString(const std::string& svString) const;
	CMemAddr FindPattern(const std::string& svPattern, const Direction searchDirect = Direction::DOWN, const int opCodesToScan = 512, const ptrdiff_t occurrence = 1) const;
	CMemAddr FollowNearCall(const ptrdiff_t opcodeOffset = 0x1, const ptrdiff_t nextInstructionOffset = 0x5) const;
	CMemAddr ResolveRelativeAddress(const ptrdiff_t registerOffset = 0x0, const ptrdiff_t nextInstructionOffset = 0x4) const;
	std::vector<CMemAddr> FindAllCallReferences(const uintptr_t sectionBase, const size_t sectionSize);
	static void HookVirtualMethod(const uintptr_t virtualTable, const void* pHookMethod, const ptrdiff_t methodIndex, void** ppOriginalMethod);

private:
	uintptr_t ptr = 0;
};

class CModule
{
public:
	struct ModuleSections_t
	{
		ModuleSections_t(void) = default;
		ModuleSections_t(const std::string& svSectionName, uintptr_t pSectionBase, size_t nSectionSize) :
			m_svSectionName(svSectionName), m_pSectionBase(pSectionBase), m_nSectionSize(nSectionSize) {}

		bool IsSectionValid(void) const
		{
			return m_nSectionSize != 0;
		}

		std::string    m_svSectionName;           // Name of section.
		uintptr_t      m_pSectionBase{};          // Start address of section.
		size_t         m_nSectionSize{};          // Size of section.
	};

	CModule(void) = default;
	CModule(const std::string& moduleName);
	CModule(const HMODULE hModule, const std::string& svModuleName);
	CModule(const uintptr_t nModuleBase, const std::string& svModuleName);

	void Init();
	void LoadSections();

	CMemAddr FindPatternSIMD(const std::string& svPattern, const ModuleSections_t* moduleSection = nullptr) const;
	CMemAddr FindString(const std::string& string, const ptrdiff_t occurrence = 1, bool nullTerminator = false) const;
	CMemAddr FindStringReadOnly(const std::string& svString, bool nullTerminator) const;

	CMemAddr          GetVirtualMethodTable(const std::string& svTableName, const uint32_t nRefIndex = 0);
	CMemAddr          GetImportedFunction(const std::string& svModuleName, const std::string& svFunctionName, const bool bGetFunctionReference) const;
	CMemAddr          GetExportedFunction(const std::string& svFunctionName) const;
	ModuleSections_t  GetSectionByName(const std::string& svSectionName) const;
	uintptr_t         GetModuleBase(void) const;
	DWORD             GetModuleSize(void) const;
	std::string       GetModuleName(void) const;
	uintptr_t         GetRVA(const uintptr_t nAddress) const;

	IMAGE_NT_HEADERS64* m_pNTHeaders = nullptr;
	IMAGE_DOS_HEADER* m_pDOSHeader = nullptr;

	ModuleSections_t         m_ExecutableCode;
	ModuleSections_t         m_ExceptionTable;
	ModuleSections_t         m_RunTimeData;
	ModuleSections_t         m_ReadOnlyData;

private:
	CMemAddr FindPatternSIMD(const uint8_t* szPattern, const char* szMask, const ModuleSections_t* moduleSection = nullptr, const uint32_t nOccurrence = 0) const;

	std::string                   m_svModuleName;
	uintptr_t                     m_pModuleBase{};
	DWORD                         m_nModuleSize{};
	std::vector<ModuleSections_t> m_vModuleSections;
};

enum CallingConvention
{
	StdCall,
	FastCall
};

#define CC_STDCALL CallingConvention::StdCall
#define CC_FASTCALL CallingConvention::FastCall

template <CallingConvention, class RetTy, class ...ArgTy>
class CMemFunc;

template <class RetTy, class ...ArgTy>
class CMemFunc<CallingConvention::StdCall, RetTy, ArgTy...>
{
private:
	uintptr_t ptr;

public:
	inline CMemFunc()
	{
		ptr = NULL;
	}

	inline CMemFunc(const CMemAddr& addr)
	{
		ptr = addr.GetPtr();
	}

	inline RetTy operator()(ArgTy... args) const
	{
		return ((RetTy(__stdcall*)(ArgTy...))ptr)(std::forward<ArgTy>(args)...);
	}

	inline void operator=(const CMemAddr& addr)
	{
		this->ptr = addr.GetPtr();
	}

	inline operator uintptr_t() const
	{
		return ptr;
	}

	inline operator void* () const
	{
		return reinterpret_cast<void*>(ptr);
	}

	inline operator void** () const
	{
		return reinterpret_cast<void**>(&ptr);
	}

	inline operator bool() const
	{
		return ptr != NULL;
	}
};

template <class RetTy, class ...ArgTy>
class CMemFunc<CallingConvention::FastCall, RetTy, ArgTy...>
{
private:
	uintptr_t ptr;

public:
	inline CMemFunc()
	{
		ptr = NULL;
	}

	inline CMemFunc(const CMemAddr& addr)
	{
		ptr = addr.GetPtr();
	}

	inline RetTy operator()(ArgTy... args) const
	{
		return ((RetTy(__fastcall*)(ArgTy...))ptr)(std::forward<ArgTy>(args)...);
	}

	inline void operator=(const CMemAddr& addr)
	{
		this->ptr = addr.GetPtr();
	}

	inline operator uintptr_t() const
	{
		return ptr;
	}

	inline operator void* () const
	{
		return reinterpret_cast<void*>(ptr);
	}

	inline operator void** () const
	{
		return reinterpret_cast<void**>(&ptr);
	}

	inline operator bool() const
	{
		return ptr != NULL;
	}
};