#include "KOffsetManager.h"

kAddress TryGetKernelBase() 
{
	
}

KOffsetManager::KOffsetManager()
{
	m_MappedKernel = LoadLibraryA("ntoskrnl.exe");
	m_KernelBase = TryGetKernelBase();
}

kAddress KOffsetManager::GetKernelBase()
{
	return m_KernelBase;
}

kAddress KOffsetManager::ResolveExport(const char* ExportName)
{
	auto localExportAddress = (Qword)GetProcAddress(m_MappedKernel, ExportName);
	if (!localExportAddress) {
		// TODO log error
		return 0;
	}

	return m_KernelBase + Qword(localExportAddress - m_KernelBase);
}

KOffsetManager::~KOffsetManager()
{
	if (m_MappedKernel) {
		FreeLibrary(m_MappedKernel);
	}
}


