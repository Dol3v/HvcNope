#pragma once

#include "Types.h"


class KOffsetManager {
public:
	KOffsetManager();

	kAddress GetKernelBase();

	kAddress ResolveExport(const char* ExportName);

	~KOffsetManager();

private:
	// base address of locally mapped kernel binary
	HMODULE m_MappedKernel;

	// actual base address of loaded kernel
	kAddress m_KernelBase;
};