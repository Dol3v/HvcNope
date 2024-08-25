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

KernelBinary::KernelBinary()
{
	m_MappedKernel = LoadLibraryA("ntoskrnl.exe");
	m_KernelBase = TryGetKernelBase();

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
	Sig::Signature_t Signature) 
{
	for (auto& region : Regions) {
		auto index = Sig::FindSignatureInBuffer(region, Signature);
		if (index) {
			// convert index to region start pointer
			return region.data() + index.value();
		}
	}

	return std::nullopt;
}

optional<kAddress> KernelBinary::FindSignature(Sig::Signature_t Signature, Dword Flags)
{

	if (Flags & LocationFlags::InCode) 
	{
		auto occurrence = FindSignatureInRegions(m_CodeSections, Signature);
		if (occurrence) {
			return MappedToKernel(occurrence.value());
		}
	}

	if (Flags & LocationFlags::InRdata)
	{
		auto occurrence = FindSignatureInRegions(m_RoDataSections, Signature);
		if (occurrence) {
			return MappedToKernel(occurrence.value());
		}
	}

	return std::nullopt;
}

KernelBinary::~KernelBinary()
{
	if (m_MappedKernel) {
		FreeLibrary(m_MappedKernel);
	}
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


