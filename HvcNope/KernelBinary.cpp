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
	s_KernelBase = TryGetKernelBase();
}

kAddress KernelBinary::GetKernelBase()
{
	return s_KernelBase;
}

kAddress KernelBinary::ResolveExport(const char* ExportName) const
{
	auto localExportAddress = GetProcAddress(m_MappedKernel, ExportName);
	if (!localExportAddress) {
		// TODO log error
		return 0;
	}

	return MappedAddressToActual(localExportAddress);
}

KernelBinary::~KernelBinary()
{
	if (m_MappedKernel) {
		FreeLibrary(m_MappedKernel);
	}
}

kAddress KernelBinary::MappedAddressToActual(PVOID MappedAddress) const
{
	auto rva = Qword(MappedAddress) - Qword(m_MappedKernel);
	return s_KernelBase + rva;
}


