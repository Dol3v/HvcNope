#pragma once

#include "Types.h"
#include "ByteSignature.h"
#include <functional>

class KernelBinary 
{
public:
	KernelBinary();

	kAddress GetKernelBase();

	kAddress ResolveExport(const char* ExportName) const;

	enum LocationFlags : Dword 
	{
		None =		0,
		InCode =	1 << 0,
		InRdata =	1 << 1,
		
		// unsupported
		// InData =	1 << 2
	};

	std::optional<kAddress> FindSignature(Sig::Signature_t Signature, Dword Flags = InCode | InRdata) const;

	std::optional<const Byte*> FindSignatureInBinary(
		Sig::Signature_t Signature,
		Dword Flags = InCode | InRdata,
		const Byte* Start = nullptr) const;

	void ForEveryCodeSignatureOccurrence(
		Sig::Signature_t Signature,
		std::function<bool(const Byte*)> Consumer) const;

	bool InKernelBounds(const Byte* PossiblePointer) const;

	kAddress MappedToKernel(const void* MappedAddress) const;

	~KernelBinary();

private:

	inline const Byte* RvaToAddress(Qword Rva) const;

	std::vector<ReadonlyRegion_t> GetSectionsByCharacteristics(Dword Required, Dword RequiredMissing);
private:
	// base address of locally mapped kernel binary
	HMODULE m_MappedKernel;

	size_t m_MappedKernelSize;

	// actual base address of loaded kernel
	kAddress m_KernelBase;

	//
	// Sections in kernel binary
	//

	std::vector<ReadonlyRegion_t> m_CodeSections;
	std::vector<ReadonlyRegion_t> m_RoDataSections;
};