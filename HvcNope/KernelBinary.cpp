#include "KernelBinary.h"

#include <Psapi.h>

static kAddress TryGetKernelBase() 
{
	//
	// We use EnumDeviceDrivers - the first device driver enumerated is ntoskrnl.exe.
	//

	PVOID deviceDriverBase = nullptr;
	DWORD needed = 0;

	if (!K32EnumDeviceDrivers(&deviceDriverBase, sizeof(PVOID), &needed))
	{
		if (GetLastError() != ERROR_INSUFFICIENT_BUFFER) {
			// handle weird error

			return kNullptr;
		}
	}

	return kAddress(deviceDriverBase);
}

size_t GetSizeOfLoadedModule(HMODULE Module)
{
	MODULEINFO moduleinfo;
	if (GetModuleInformation(
		GetCurrentProcess(),
		Module,
		&moduleinfo,
		sizeof(moduleinfo))) 
	{
		return moduleinfo.SizeOfImage;
	}

	// TODO: die later
	return 0;
}

KernelBinary::KernelBinary()
{
	m_MappedKernel = LoadLibraryA("ntoskrnl.exe");
	m_KernelBase = TryGetKernelBase();
	m_MappedKernelSize = GetSizeOfLoadedModule(m_MappedKernel);

	// load sections
	m_CodeSections = GetSectionsByCharacteristics(
		IMAGE_SCN_CNT_CODE,
		IMAGE_SCN_MEM_DISCARDABLE);

	m_RoDataSections = GetSectionsByCharacteristics(
		IMAGE_SCN_CNT_INITIALIZED_DATA, 
		IMAGE_SCN_MEM_DISCARDABLE | IMAGE_SCN_MEM_EXECUTE | IMAGE_SCN_MEM_WRITE);
}

kAddress KernelBinary::GetKernelBase()
{
	return m_KernelBase;
}

kAddress KernelBinary::ResolveExport(const char* ExportName) const
{
	auto localExportAddress = GetProcAddress(m_MappedKernel, ExportName);
	if (!localExportAddress) {
		// TODO log error
		return 0;
	}

	return MappedToKernel(localExportAddress);
}

optional<const Byte*> FindSignatureInRegions(
	const std::vector<ReadonlyRegion_t>& Regions,
	Sig::Signature_t Signature,
	const Byte* Start) 
{
	for (auto& region : Regions) {
		// skip regions that are below our starting point
		if (region.data() + region.size() <= Start) continue;

		auto searchRegion = region;
		
		// if we should start searching in the middle of 
		// a region, trim it down
		if (searchRegion.data() < Start) {
			auto offset = Qword(Start - searchRegion.data());
			auto length = searchRegion.size() - offset;
			searchRegion = searchRegion.subspan(offset, length);
		}

		auto index = Sig::FindSignatureInBuffer(searchRegion, Signature);
		if (index) {
			// convert index to region start pointer
			return region.data() + index.value();
		}
	}

	return std::nullopt;
}

optional<kAddress> KernelBinary::FindSignature(Sig::Signature_t Signature, Dword Flags) const
{
	auto occurrence = FindSignatureInBinary(Signature, Flags);
	if (occurrence) {
		return MappedToKernel(occurrence.value());
	}
	return std::nullopt;
}

void KernelBinary::ForEveryCodeSignatureOccurrence(
	Sig::Signature_t Signature,
	std::function<bool(const Byte*)> Consumer) const
{
	bool continueSearch = true;
	const Byte* searchStart = nullptr;

	while (continueSearch)
	{
		auto occurrence = FindSignatureInBinary(Signature, InCode, searchStart);
		if (!occurrence) break;

		continueSearch = Consumer(occurrence.value());
		searchStart = occurrence.value() + 1;
	}
}

bool KernelBinary::InKernelBounds(const Byte* PossiblePointer) const
{
	auto* base = (const Byte*)m_MappedKernel;
	return PossiblePointer >= base && PossiblePointer <= base + m_MappedKernelSize;
}

KernelBinary::~KernelBinary()
{
	if (m_MappedKernel) {
		FreeLibrary(m_MappedKernel);
	}
}

std::optional<const Byte*> KernelBinary::FindSignatureInBinary(
	Sig::Signature_t Signature,
	Dword Flags,
	const Byte* Start) const
{
	if (Flags & LocationFlags::InCode)
	{
		auto occurrence = FindSignatureInRegions(m_CodeSections, Signature, Start);
		return occurrence;
	}

	if (Flags & LocationFlags::InRdata)
	{
		auto occurrence = FindSignatureInRegions(m_RoDataSections, Signature, Start);
		return occurrence;
	}

	return std::nullopt;
}

const Byte* KernelBinary::RvaToAddress(Qword Rva) const
{
	return reinterpret_cast<const Byte*>(m_MappedKernel) + Rva;
}

std::vector<ReadonlyRegion_t> KernelBinary::GetSectionsByCharacteristics(Dword Required, Dword RequiredMissing)
 {
	static IMAGE_SECTION_HEADER* s_SectionHeaders = nullptr;
	static Word s_NumberOfSections = 0;

	if (!s_SectionHeaders)
	{
		// load sections
		auto dosHeader = reinterpret_cast<IMAGE_DOS_HEADER*>(m_MappedKernel);
		auto ntHeaders = reinterpret_cast<const IMAGE_NT_HEADERS*>(
			RvaToAddress(dosHeader->e_lfanew));
		s_SectionHeaders = IMAGE_FIRST_SECTION(ntHeaders);
		s_NumberOfSections = ntHeaders->FileHeader.NumberOfSections;
	}

	auto* currentSection = s_SectionHeaders;
	std::vector<ReadonlyRegion_t> sections;

	for (auto i = 0; i < s_NumberOfSections; ++i) {
		if ((currentSection->Characteristics & Required) == Required &&
			(currentSection->Characteristics & RequiredMissing) == 0)
		{
			auto* sectionStart = RvaToAddress(currentSection->VirtualAddress);
			sections.push_back(ReadonlyRegion_t(sectionStart, currentSection->Misc.VirtualSize));
		}

		currentSection++;
	}

	return sections;
 }

kAddress KernelBinary::MappedToKernel(const void* MappedAddress) const
{
	auto rva = Qword(MappedAddress) - Qword(m_MappedKernel);
	return m_KernelBase + rva;
}


