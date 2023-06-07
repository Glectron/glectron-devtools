#include<vector>

#include "memory.h"
#include <emmintrin.h>
#include <algorithm>
///////////////////////////////////////////////////////////////////////////////
// For converting a string pattern with wildcards to an array of bytes.
///////////////////////////////////////////////////////////////////////////////
std::vector<int> PatternToBytes(const std::string& svInput)
{
	char* pszPatternStart = const_cast<char*>(svInput.c_str());
	char* pszPatternEnd = pszPatternStart + strlen(svInput.c_str());
	std::vector<int> vBytes = std::vector<int>{ };

	for (char* pszCurrentByte = pszPatternStart; pszCurrentByte < pszPatternEnd; ++pszCurrentByte)
	{
		if (*pszCurrentByte == '?')
		{
			++pszCurrentByte;
			if (*pszCurrentByte == '?')
			{
				++pszCurrentByte; // Skip double wildcard.
			}
			vBytes.push_back(-1); // Push the byte back as invalid.
		}
		else
		{
			vBytes.push_back(strtoul(pszCurrentByte, &pszCurrentByte, 16));
		}
	}
	return vBytes;
};

//-----------------------------------------------------------------------------
// Purpose: check array of opcodes starting from current address
// Input  : vOpcodeArray - 
// Output : true if equal, false otherwise
//-----------------------------------------------------------------------------
bool CMemAddr::CheckOpCodes(const std::vector<uint8_t> vOpcodeArray) const
{
	uintptr_t ref = ptr;

	// Loop forward in the ptr class member.
	for (auto [byteAtCurrentAddress, i] = std::tuple{ uint8_t(), (size_t)0 }; i < vOpcodeArray.size(); i++, ref++)
	{
		byteAtCurrentAddress = *reinterpret_cast<uint8_t*>(ref);

		// If byte at ptr doesn't equal in the byte array return false.
		if (byteAtCurrentAddress != vOpcodeArray[i])
			return false;
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: patch array of opcodes starting from current address
// Input  : vOpcodeArray - 
//-----------------------------------------------------------------------------
void CMemAddr::Patch(const std::vector<uint8_t> vOpcodeArray) const
{
	DWORD oldProt = NULL;

	SIZE_T dwSize = vOpcodeArray.size();
	VirtualProtect(reinterpret_cast<void*>(ptr), dwSize, PAGE_EXECUTE_READWRITE, &oldProt); // Patch page to be able to read and write to it.

	for (size_t i = 0; i < vOpcodeArray.size(); i++)
	{
		*reinterpret_cast<uint8_t*>(ptr + i) = vOpcodeArray[i]; // Write opcodes to Address.
	}

	dwSize = vOpcodeArray.size();
	VirtualProtect(reinterpret_cast<void*>(ptr), dwSize, oldProt, &oldProt); // Restore protection.
}

//-----------------------------------------------------------------------------
// Purpose: patch string constant at current address
// Input  : &svString - 
//-----------------------------------------------------------------------------
void CMemAddr::PatchString(const std::string& svString) const
{
	DWORD oldProt = NULL;

	SIZE_T dwSize = svString.size();
	const std::vector<char> bytes(svString.begin(), svString.end());

	VirtualProtect(reinterpret_cast<void*>(ptr), dwSize, PAGE_EXECUTE_READWRITE, &oldProt); // Patch page to be able to read and write to it.

	for (size_t i = 0; i < svString.size(); i++)
	{
		*reinterpret_cast<uint8_t*>(ptr + i) = bytes[i]; // Write string to Address.
	}

	dwSize = svString.size();
	VirtualProtect(reinterpret_cast<void*>(ptr), dwSize, oldProt, &oldProt); // Restore protection.
}

//-----------------------------------------------------------------------------
// Purpose: find array of bytes in process memory
// Input  : *szPattern - 
//			searchDirect - 
//			opCodesToScan - 
//			occurrence - 
// Output : CMemAddr
//-----------------------------------------------------------------------------
CMemAddr CMemAddr::FindPattern(const std::string& svPattern, const Direction searchDirect, const int opCodesToScan, const ptrdiff_t occurrence) const
{
	uint8_t* pScanBytes = reinterpret_cast<uint8_t*>(ptr); // Get the base of the module.

	const std::vector<int> PatternBytes = PatternToBytes(svPattern); // Convert our pattern to a byte array.
	const std::pair bytesInfo = std::make_pair(PatternBytes.size(), PatternBytes.data()); // Get the size and data of our bytes.
	ptrdiff_t occurrences = 0;

	for (long i = 01; i < opCodesToScan + bytesInfo.first; i++)
	{
		bool bFound = true;
		int nMemOffset = searchDirect == Direction::DOWN ? i : -i;

		for (DWORD j = 0ul; j < bytesInfo.first; j++)
		{
			// If either the current byte equals to the byte in our pattern or our current byte in the pattern is a wildcard
			// our if clause will be false.
			uint8_t currentByte = *(pScanBytes + nMemOffset + j);
			_mm_prefetch(reinterpret_cast<const CHAR*>(static_cast<int64_t>(currentByte + nMemOffset + 64)), _MM_HINT_T0); // precache some data in L1.
			if (currentByte != bytesInfo.second[j] && bytesInfo.second[j] != -1)
			{
				bFound = false;
				break;
			}
		}

		if (bFound)
		{
			occurrences++;
			if (occurrence == occurrences)
			{
				return CMemAddr(&*(pScanBytes + nMemOffset));
			}
		}
	}

	return CMemAddr();
}

//-----------------------------------------------------------------------------
// Purpose: ResolveRelativeAddress wrapper
// Input  : opcodeOffset - 
//			nextInstructionOffset - 
// Output : CMemAddr
//-----------------------------------------------------------------------------
CMemAddr CMemAddr::FollowNearCall(const ptrdiff_t opcodeOffset, const ptrdiff_t nextInstructionOffset) const
{
	return ResolveRelativeAddress(opcodeOffset, nextInstructionOffset);
}

//-----------------------------------------------------------------------------
// Purpose: resolves the relative pointer to offset
// Input  : registerOffset - 
//			nextInstructionOffset - 
// Output : CMemAddr
//-----------------------------------------------------------------------------
CMemAddr CMemAddr::ResolveRelativeAddress(const ptrdiff_t registerOffset, const ptrdiff_t nextInstructionOffset) const
{
	// Skip register.
	const uintptr_t skipRegister = ptr + registerOffset;

	// Get 4-byte long relative Address.
	const int32_t relativeAddress = *reinterpret_cast<int32_t*>(skipRegister);

	// Get location of next instruction.
	const uintptr_t nextInstruction = ptr + nextInstructionOffset;

	// Get function location via adding relative Address to next instruction.
	return CMemAddr(nextInstruction + relativeAddress);
}

//-----------------------------------------------------------------------------
// Purpose: resolve all 'call' references to ptr 
// (This is very slow only use for mass patching.)
// Input  : sectionBase - 
//			sectionSize - 
// Output : vector<CMemAddr>
//-----------------------------------------------------------------------------
std::vector<CMemAddr> CMemAddr::FindAllCallReferences(const uintptr_t sectionBase, const size_t sectionSize)
{
	std::vector<CMemAddr> referencesInfo = {};

	uint8_t* pTextStart = reinterpret_cast<uint8_t*>(sectionBase);
	for (size_t i = 0ull; i < sectionSize - 0x5; i++, _mm_prefetch(reinterpret_cast<const char*>(pTextStart + 64), _MM_HINT_NTA))
	{
		if (pTextStart[i] == X86_CALL)
		{
			CMemAddr memAddr = CMemAddr(&pTextStart[i]);
			if (!memAddr.Offset(0x1).CheckOpCodes({ 0x00, 0x00, 0x00, 0x00 })) // Check if its not a dynamic resolved call.
			{
				if (memAddr.FollowNearCall() == *this)
					referencesInfo.push_back(memAddr);
			}
		}
	}

	return referencesInfo;
}

//-----------------------------------------------------------------------------
// Purpose: patch virtual method to point to a user set function
// Input  : virtualTable - 
//			pHookMethod - 
//          methodIndex -
//          pOriginalMethod -
// Output : void** via pOriginalMethod
//-----------------------------------------------------------------------------
void CMemAddr::HookVirtualMethod(const uintptr_t virtualTable, const void* pHookMethod, const ptrdiff_t methodIndex, void** ppOriginalMethod)
{
	DWORD oldProt = NULL;

	// Calculate delta to next virtual method.
	const uintptr_t virtualMethod = virtualTable + (methodIndex * sizeof(ptrdiff_t));

	// Preserve original function.
	const uintptr_t originalFunction = *reinterpret_cast<uintptr_t*>(virtualMethod);

	// Set page for current virtual method to execute n read n write.
	VirtualProtect(reinterpret_cast<void*>(virtualMethod), sizeof(virtualMethod), PAGE_EXECUTE_READWRITE, &oldProt);

	// Set virtual method to our hook.
	*reinterpret_cast<uintptr_t*>(virtualMethod) = reinterpret_cast<uintptr_t>(pHookMethod);

	// Restore original page.
	VirtualProtect(reinterpret_cast<void*>(virtualMethod), sizeof(virtualMethod), oldProt, &oldProt);

	// Move original function into argument.
	*ppOriginalMethod = reinterpret_cast<void*>(originalFunction);
}

///////////////////////////////////////////////////////////////////////////////
// For converting a string pattern with wildcards to an array of bytes and mask.
///////////////////////////////////////////////////////////////////////////////
std::pair<std::vector<uint8_t>, std::string> PatternToMaskedBytes(const std::string& svInput)
{
	char* pszPatternStart = const_cast<char*>(svInput.c_str());
	char* pszPatternEnd = pszPatternStart + strlen(svInput.c_str());
	std::vector<uint8_t> vBytes = std::vector<uint8_t>{ };
	std::string svMask = std::string();

	for (char* pszCurrentByte = pszPatternStart; pszCurrentByte < pszPatternEnd; ++pszCurrentByte)
	{
		if (*pszCurrentByte == '?')
		{
			++pszCurrentByte;
			if (*pszCurrentByte == '?')
			{
				++pszCurrentByte; // Skip double wildcard.
			}
			vBytes.push_back(0); // Push the byte back as invalid.
			svMask += '?';
		}
		else
		{
			vBytes.push_back(strtoul(pszCurrentByte, &pszCurrentByte, 16));
			svMask += 'x';
		}
	}
	return make_pair(vBytes, svMask);
};

///////////////////////////////////////////////////////////////////////////////
// For converting a string to an array of bytes.
///////////////////////////////////////////////////////////////////////////////
std::vector<int> StringToBytes(const std::string& svInput, bool bNullTerminator)
{
	char* pszStringStart = const_cast<char*>(svInput.c_str());
	char* pszStringEnd = pszStringStart + strlen(svInput.c_str());
	std::vector<int> vBytes = std::vector<int>{ };

	for (char* pszCurrentByte = pszStringStart; pszCurrentByte < pszStringEnd; ++pszCurrentByte)
	{
		// Dereference character and push back the byte.
		vBytes.push_back(*pszCurrentByte);
	}

	if (bNullTerminator)
	{
		vBytes.push_back('\0');
	}
	return vBytes;
};

///////////////////////////////////////////////////////////////////////////////
// For converting a string to an array of bytes.
///////////////////////////////////////////////////////////////////////////////
std::pair<std::vector<uint8_t>, std::string> StringToMaskedBytes(const std::string& svInput, bool bNullTerminator)
{
	char* pszStringStart = const_cast<char*>(svInput.c_str());
	char* pszStringEnd = pszStringStart + strlen(svInput.c_str());
	std::vector<uint8_t> vBytes = std::vector<uint8_t>{ };
	std::string svMask = std::string();

	for (char* pszCurrentByte = pszStringStart; pszCurrentByte < pszStringEnd; ++pszCurrentByte)
	{
		// Dereference character and push back the byte.
		vBytes.push_back(*pszCurrentByte);
		svMask += 'x';
	}

	if (bNullTerminator)
	{
		vBytes.push_back(0x0);
		svMask += 'x';
	}
	return make_pair(vBytes, svMask);
};

//-----------------------------------------------------------------------------
// Purpose: constructor
// Input  : *svModuleName
//-----------------------------------------------------------------------------
CModule::CModule(const std::string& svModuleName) : m_svModuleName(svModuleName)
{
	m_pModuleBase = reinterpret_cast<uintptr_t>(GetModuleHandleA(svModuleName.c_str()));

	Init();
	LoadSections();
}

//-----------------------------------------------------------------------------
// Purpose: constructor
// Input  : hModule
//-----------------------------------------------------------------------------
CModule::CModule(const HMODULE hModule, const std::string& svModuleName) : m_svModuleName(svModuleName)
{
	m_pModuleBase = reinterpret_cast<uintptr_t>(hModule);

	Init();
	LoadSections();
}

//-----------------------------------------------------------------------------
// Purpose: constructor
// Input  : nModuleBase
//-----------------------------------------------------------------------------
CModule::CModule(const uintptr_t nModuleBase, const std::string& svModuleName) : m_svModuleName(svModuleName), m_pModuleBase(nModuleBase)
{
	Init();
	LoadSections();
}

//-----------------------------------------------------------------------------
// Purpose: initializes module descriptors
//-----------------------------------------------------------------------------
void CModule::Init()
{
	m_pDOSHeader = reinterpret_cast<IMAGE_DOS_HEADER*>(m_pModuleBase);
	m_pNTHeaders = reinterpret_cast<IMAGE_NT_HEADERS64*>(m_pModuleBase + m_pDOSHeader->e_lfanew);
	m_nModuleSize = static_cast<size_t>(m_pNTHeaders->OptionalHeader.SizeOfImage);

	const IMAGE_SECTION_HEADER* hSection = IMAGE_FIRST_SECTION(m_pNTHeaders); // Get first image section.

	for (WORD i = 0; i < m_pNTHeaders->FileHeader.NumberOfSections; i++) // Loop through the sections.
	{
		const IMAGE_SECTION_HEADER& hCurrentSection = hSection[i]; // Get current section.
		m_vModuleSections.push_back(ModuleSections_t(reinterpret_cast<const char*>(hCurrentSection.Name),
			static_cast<uintptr_t>(m_pModuleBase + hCurrentSection.VirtualAddress), hCurrentSection.SizeOfRawData)); // Push back a struct with the section data.
	}
}

//-----------------------------------------------------------------------------
// Purpose: initializes the default executable segments
//-----------------------------------------------------------------------------
void CModule::LoadSections()
{
	m_ExecutableCode = GetSectionByName((".text"));
	m_ExceptionTable = GetSectionByName((".pdata"));
	m_RunTimeData = GetSectionByName((".data"));
	m_ReadOnlyData = GetSectionByName((".rdata"));
}


//-----------------------------------------------------------------------------
// Purpose: find array of bytes in process memory using SIMD instructions
// Input  : *szPattern - 
//          *szMask - 
// Output : CMemAddr
//-----------------------------------------------------------------------------
CMemAddr CModule::FindPatternSIMD(const uint8_t* szPattern, const char* szMask, const ModuleSections_t* moduleSection, const uint32_t nOccurrence) const
{
	if (!m_ExecutableCode.IsSectionValid())
		return CMemAddr();

	const bool bSectionValid = moduleSection ? moduleSection->IsSectionValid() : false;

	const uintptr_t nBase = bSectionValid ? moduleSection->m_pSectionBase : m_ExecutableCode.m_pSectionBase;
	const uintptr_t nSize = bSectionValid ? moduleSection->m_nSectionSize : m_ExecutableCode.m_nSectionSize;

	const size_t nMaskLen = strlen(szMask);
	const uint8_t* pData = reinterpret_cast<uint8_t*>(nBase);
	const uint8_t* pEnd = pData + nSize - nMaskLen;

	int nOccurrenceCount = 0;
	int nMasks[64]; // 64*16 = enough masks for 1024 bytes.
	const int iNumMasks = static_cast<int>(ceil(static_cast<float>(nMaskLen) / 16.f));

	memset(nMasks, '\0', iNumMasks * sizeof(int));
	for (intptr_t i = 0; i < iNumMasks; ++i)
	{
		for (intptr_t j = strnlen(szMask + i * 16, 16) - 1; j >= 0; --j)
		{
			if (szMask[i * 16 + j] == 'x')
			{
				_bittestandset(reinterpret_cast<LONG*>(&nMasks[i]), static_cast<LONG>(j));
			}
		}
	}
	const __m128i xmm1 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(szPattern));
	__m128i xmm2, xmm3, msks;
	for (; pData != pEnd; _mm_prefetch(reinterpret_cast<const char*>(++pData + 64), _MM_HINT_NTA))
	{
		if (szPattern[0] == pData[0])
		{
			xmm2 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(pData));
			msks = _mm_cmpeq_epi8(xmm1, xmm2);
			if ((_mm_movemask_epi8(msks) & nMasks[0]) == nMasks[0])
			{
				for (uintptr_t i = 1; i < static_cast<uintptr_t>(iNumMasks); ++i)
				{
					xmm2 = _mm_loadu_si128(reinterpret_cast<const __m128i*>((pData + i * 16)));
					xmm3 = _mm_loadu_si128(reinterpret_cast<const __m128i*>((szPattern + i * 16)));
					msks = _mm_cmpeq_epi8(xmm2, xmm3);
					if ((_mm_movemask_epi8(msks) & nMasks[i]) == nMasks[i])
					{
						if ((i + 1) == iNumMasks)
						{
							if (nOccurrenceCount == nOccurrence)
							{
								return static_cast<CMemAddr>(const_cast<uint8_t*>(pData));
							}
							nOccurrenceCount++;
						}
					}
					else
					{
						goto cont;
					}
				}
				if (nOccurrenceCount == nOccurrence)
				{
					return static_cast<CMemAddr>((&*(const_cast<uint8_t*>(pData))));
				}
				nOccurrenceCount++;
			}
		}cont:;
	}
	return CMemAddr();
}


//-----------------------------------------------------------------------------
// Purpose: find a string pattern in process memory using SIMD instructions
// Input  : &svPattern
//			&moduleSection
// Output : CMemAddr
//-----------------------------------------------------------------------------
CMemAddr CModule::FindPatternSIMD(const std::string& svPattern, const ModuleSections_t* moduleSection) const
{
	const std::pair patternInfo = PatternToMaskedBytes(svPattern);
	const CMemAddr memory = FindPatternSIMD(patternInfo.first.data(), patternInfo.second.c_str(), moduleSection);
	return memory;
}

//-----------------------------------------------------------------------------
// Purpose: find address of input string constant in read only memory
// Input  : *svString - 
//          bNullTerminator - 
// Output : CMemAddr
//-----------------------------------------------------------------------------
CMemAddr CModule::FindStringReadOnly(const std::string& svString, bool bNullTerminator) const
{
	if (!m_ReadOnlyData.IsSectionValid())
		return CMemAddr();

	const std::vector<int> vBytes = StringToBytes(svString, bNullTerminator); // Convert our string to a byte array.
	const std::pair bytesInfo = std::make_pair(vBytes.size(), vBytes.data()); // Get the size and data of our bytes.

	const uint8_t* pBase = reinterpret_cast<uint8_t*>(m_ReadOnlyData.m_pSectionBase); // Get start of .rdata section.

	for (size_t i = 0ull; i < m_ReadOnlyData.m_nSectionSize - bytesInfo.first; i++)
	{
		bool bFound = true;

		// If either the current byte equals to the byte in our pattern or our current byte in the pattern is a wildcard
		// our if clause will be false.
		for (size_t j = 0ull; j < bytesInfo.first; j++)
		{
			if (pBase[i + j] != bytesInfo.second[j] && bytesInfo.second[j] != -1)
			{
				bFound = false;
				break;
			}
		}

		if (bFound)
		{
			CMemAddr result = CMemAddr(&pBase[i]);
			return result;
		}
	}

	return CMemAddr();
}

//-----------------------------------------------------------------------------
// Purpose: find address of reference to string constant in executable memory
// Input  : *svString - 
//          bNullTerminator - 
// Output : CMemAddr
//-----------------------------------------------------------------------------
CMemAddr CModule::FindString(const std::string& svString, const ptrdiff_t nOccurrence, bool bNullTerminator) const
{
	if (!m_ExecutableCode.IsSectionValid())
		return CMemAddr();

	std::string svPackedString = svString + std::to_string(nOccurrence);

	const CMemAddr stringAddress = FindStringReadOnly(svString, bNullTerminator); // Get Address for the string in the .rdata section.

	if (!stringAddress)
		return CMemAddr();

	uint8_t* pLatestOccurrence = nullptr;
	uint8_t* pTextStart = reinterpret_cast<uint8_t*>(m_ExecutableCode.m_pSectionBase); // Get the start of the .text section.
	ptrdiff_t dOccurrencesFound = 0;
	CMemAddr resultAddress;

	for (size_t i = 0ull; i < m_ExecutableCode.m_nSectionSize - 0x5; i++)
	{
		char byte = pTextStart[i];
		if (byte == X86_LEA)
		{
			const CMemAddr skipOpCode = CMemAddr(reinterpret_cast<uintptr_t>(&pTextStart[i])).Offset(0x2); // Skip next 2 opcodes, those being the instruction and the register.
			const int32_t relativeAddress = skipOpCode.GetValue<int32_t>();                                  // Get 4-byte long string relative Address
			const uintptr_t nextInstruction = skipOpCode.Offset(0x4).GetPtr();                               // Get location of next instruction.
			const CMemAddr potentialLocation = CMemAddr(nextInstruction + relativeAddress);                    // Get potential string location.

			if (potentialLocation == stringAddress)
			{
				dOccurrencesFound++;
				if (nOccurrence == dOccurrencesFound)
				{
					resultAddress = CMemAddr(&pTextStart[i]);
					return resultAddress;
				}

				pLatestOccurrence = &pTextStart[i]; // Stash latest occurrence.
			}
		}
	}

	resultAddress = CMemAddr(pLatestOccurrence);
	return resultAddress;
}

//-----------------------------------------------------------------------------
// Purpose: get address of a virtual method table by rtti type descriptor name.
// Input  : *svTableName - 
//			nRefIndex - 
// Output : CMemAddr
//-----------------------------------------------------------------------------
CMemAddr CModule::GetVirtualMethodTable(const std::string& svTableName, const uint32_t nRefIndex)
{
	std::string svPackedTableName = svTableName + std::to_string(nRefIndex);

	ModuleSections_t moduleSection = { (".data"), m_RunTimeData.m_pSectionBase, m_RunTimeData.m_nSectionSize };

	const auto tableNameInfo = StringToMaskedBytes(svTableName, false);
	CMemAddr rttiTypeDescriptor = FindPatternSIMD(tableNameInfo.first.data(), tableNameInfo.second.c_str(), &moduleSection).Offset(-0x10);
	if (!rttiTypeDescriptor)
		return CMemAddr();

	uintptr_t scanStart = m_ReadOnlyData.m_pSectionBase; // Get the start address of our scan.

	const uintptr_t scanEnd = (m_ReadOnlyData.m_pSectionBase + m_ReadOnlyData.m_nSectionSize) - 0x4; // Calculate the end of our scan.
	const uintptr_t rttiTDRva = rttiTypeDescriptor.GetPtr() - m_pModuleBase; // The RTTI gets referenced by a 4-Byte RVA address. We need to scan for that address.
	while (scanStart < scanEnd)
	{
		moduleSection = { (".rdata"), scanStart, m_ReadOnlyData.m_nSectionSize };
		CMemAddr reference = FindPatternSIMD(reinterpret_cast<const unsigned char*>(&rttiTDRva), ("xxxx"), &moduleSection, nRefIndex);
		if (!reference)
			break;

		CMemAddr referenceOffset = reference.Offset(-0xC);
		if (referenceOffset.GetValue<int32_t>() != 1) // Check if we got a RTTI Object Locator for this reference by checking if -0xC is 1, which is the 'signature' field which is always 1 on x64.
		{
			scanStart = reference.Offset(0x4).GetPtr(); // Set location to current reference + 0x4 so we avoid pushing it back again into the vector.
			continue;
		}

		moduleSection = { (".rdata"), m_ReadOnlyData.m_pSectionBase, m_ReadOnlyData.m_nSectionSize };
		CMemAddr vfTable = FindPatternSIMD(reinterpret_cast<const unsigned char*>(&referenceOffset), ("xxxxxxxx"), &moduleSection).Offset(0x8);
		return vfTable;
	}

	return CMemAddr();
}


CMemAddr CModule::GetImportedFunction(const std::string& svModuleName, const std::string& svFunctionName, const bool bGetFunctionReference) const
{
	if (!m_pDOSHeader || m_pDOSHeader->e_magic != IMAGE_DOS_SIGNATURE) // Is dosHeader valid?
		return CMemAddr();

	if (!m_pNTHeaders || m_pNTHeaders->Signature != IMAGE_NT_SIGNATURE) // Is ntHeader valid?
		return CMemAddr();

	// Get the location of IMAGE_IMPORT_DESCRIPTOR for this module by adding the IMAGE_DIRECTORY_ENTRY_IMPORT relative virtual address onto our module base address.
	IMAGE_IMPORT_DESCRIPTOR* pImageImportDescriptors = reinterpret_cast<IMAGE_IMPORT_DESCRIPTOR*>(m_pModuleBase + m_pNTHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress);
	if (!pImageImportDescriptors)
		return CMemAddr();

	for (IMAGE_IMPORT_DESCRIPTOR* pIID = pImageImportDescriptors; pIID->Name != 0; pIID++)
	{
		// Get virtual relative Address of the imported module name. Then add module base Address to get the actual location.
		std::string svImportedModuleName = reinterpret_cast<char*>(reinterpret_cast<DWORD*>(m_pModuleBase + pIID->Name));

		// Convert all characters to lower case because KERNEL32.DLL sometimes is kernel32.DLL, sometimes KERNEL32.dll.
		std::transform(svImportedModuleName.begin(), svImportedModuleName.end(), svImportedModuleName.begin(), static_cast<int (*)(int)>(std::tolower));

		if (svImportedModuleName.compare(svModuleName) == 0) // Is this our wanted imported module?.
		{
			// Original First Thunk to get function name.
			IMAGE_THUNK_DATA* pOgFirstThunk = reinterpret_cast<IMAGE_THUNK_DATA*>(m_pModuleBase + pIID->OriginalFirstThunk);

			// To get actual function address.
			IMAGE_THUNK_DATA* pFirstThunk = reinterpret_cast<IMAGE_THUNK_DATA*>(m_pModuleBase + pIID->FirstThunk);
			for (; pOgFirstThunk->u1.AddressOfData; ++pOgFirstThunk, ++pFirstThunk)
			{
				// Get image import by name.
				const IMAGE_IMPORT_BY_NAME* pImageImportByName = reinterpret_cast<IMAGE_IMPORT_BY_NAME*>(m_pModuleBase + pOgFirstThunk->u1.AddressOfData);

				// Get import function name.
				const std::string svImportedFunctionName = pImageImportByName->Name;
				if (svImportedFunctionName.compare(svFunctionName) == 0) // Is this our wanted imported function?
				{
					// Grab function address from firstThunk.
					uintptr_t* pFunctionAddress = (uintptr_t*)&pFirstThunk->u1.Function;

					// Reference or address?
					return bGetFunctionReference ? CMemAddr(pFunctionAddress) : CMemAddr(*pFunctionAddress); // Return as CMemAddr class.
				}
			}
		}
	}
	return CMemAddr();
}

//-----------------------------------------------------------------------------
// Purpose: get address of exported function in this module
// Input  : *svFunctionName - 
//          bNullTerminator - 
// Output : CMemAddr
//-----------------------------------------------------------------------------
CMemAddr CModule::GetExportedFunction(const std::string& svFunctionName) const
{
	if (!m_pDOSHeader || m_pDOSHeader->e_magic != IMAGE_DOS_SIGNATURE) // Is dosHeader valid?
		return CMemAddr();

	if (!m_pNTHeaders || m_pNTHeaders->Signature != IMAGE_NT_SIGNATURE) // Is ntHeader valid?
		return CMemAddr();

	// Get the location of IMAGE_EXPORT_DIRECTORY for this module by adding the IMAGE_DIRECTORY_ENTRY_EXPORT relative virtual address onto our module base address.
	const IMAGE_EXPORT_DIRECTORY* pImageExportDirectory = reinterpret_cast<IMAGE_EXPORT_DIRECTORY*>(m_pModuleBase + m_pNTHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress);
	if (!pImageExportDirectory)
		return CMemAddr();

	// Are there any exported functions?
	if (!pImageExportDirectory->NumberOfFunctions)
		return CMemAddr();

	// Get the location of the functions via adding the relative virtual address from the struct into our module base address.
	const DWORD* pAddressOfFunctions = reinterpret_cast<DWORD*>(m_pModuleBase + pImageExportDirectory->AddressOfFunctions);
	if (!pAddressOfFunctions)
		return CMemAddr();

	// Get the names of the functions via adding the relative virtual address from the struct into our module base Address.
	const DWORD* pAddressOfName = reinterpret_cast<DWORD*>(m_pModuleBase + pImageExportDirectory->AddressOfNames);
	if (!pAddressOfName)
		return CMemAddr();

	// Get the ordinals of the functions via adding the relative virtual Address from the struct into our module base address.
	DWORD* pAddressOfOrdinals = reinterpret_cast<DWORD*>(m_pModuleBase + pImageExportDirectory->AddressOfNameOrdinals);
	if (!pAddressOfOrdinals)
		return CMemAddr();

	for (DWORD i = 0; i < pImageExportDirectory->NumberOfFunctions; i++) // Iterate through all the functions.
	{
		// Get virtual relative Address of the function name. Then add module base Address to get the actual location.
		std::string ExportFunctionName = reinterpret_cast<char*>(reinterpret_cast<DWORD*>(m_pModuleBase + pAddressOfName[i]));

		if (ExportFunctionName.compare(svFunctionName) == 0) // Is this our wanted exported function?
		{
			// Get the function ordinal. Then grab the relative virtual address of our wanted function. Then add module base address so we get the actual location.
			return CMemAddr(m_pModuleBase + pAddressOfFunctions[reinterpret_cast<WORD*>(pAddressOfOrdinals)[i]]); // Return as CMemAddr class.
		}
	}
	return CMemAddr();
}

//-----------------------------------------------------------------------------
// Purpose: get the module section by name (example: '.rdata', '.text')
// Input  : *svModuleName - 
// Output : ModuleSections_t
//-----------------------------------------------------------------------------
CModule::ModuleSections_t CModule::GetSectionByName(const std::string& svSectionName) const
{
	for (const ModuleSections_t& section : m_vModuleSections)
	{
		if (section.m_svSectionName == svSectionName)
			return section;
	}

	return ModuleSections_t();
}

//-----------------------------------------------------------------------------
// Purpose: returns the module base
//-----------------------------------------------------------------------------
uintptr_t CModule::GetModuleBase(void) const
{
	return m_pModuleBase;
}

//-----------------------------------------------------------------------------
// Purpose: returns the module size
//-----------------------------------------------------------------------------
DWORD CModule::GetModuleSize(void) const
{
	return m_nModuleSize;
}

//-----------------------------------------------------------------------------
// Purpose: returns the module name
//-----------------------------------------------------------------------------
std::string CModule::GetModuleName(void) const
{
	return m_svModuleName;
}

//-----------------------------------------------------------------------------
// Purpose: returns the RVA of given address
//-----------------------------------------------------------------------------
uintptr_t CModule::GetRVA(const uintptr_t nAddress) const
{
	return (nAddress - GetModuleBase());
}