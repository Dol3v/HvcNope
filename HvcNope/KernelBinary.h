#pragma once

#include "Types.h"
#include "ByteSignature.h"

class KernelBinary {
public:
	KernelBinary();

	static kAddress GetKernelBase();

	kAddress ResolveExport(const char* ExportName) const;

	enum LocationFlags : Dword 
	{
		None =		0,
		InCode =	1 << 0,
		InRdata =	1 << 1,
		
		// unsupported
		// InData =	1 << 2
	};

	std::optional<kAddress> FindSignature(Sig::Signature_t Signature, Dword Flags = InCode | InRdata);

	~KernelBinary();

private:

	inline const Byte* RvaToAddress(Qword Rva) const;

	std::vector<ReadonlyRegion_t> GetSectionsByCharacteristics(Dword Required, Dword RequiredMissing);

	kAddress MappedToKernel(const void* MappedAddress) const;

private:
	// base address of locally mapped kernel binary
	HMODULE m_MappedKernel;

	// actual base address of loaded kernel
	static kAddress s_KernelBase;

	//
	// Sections in kernel binary
	//

	std::vector<ReadonlyRegion_t> m_CodeSections;
	std::vector<ReadonlyRegion_t> m_RoDataSections;
};