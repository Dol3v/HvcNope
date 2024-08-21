#pragma once

#include "Types.h"


class KernelBinary {
public:
	KernelBinary();

	static kAddress GetKernelBase();

	kAddress ResolveExport(const char* ExportName) const;

	~KernelBinary();

private:

	kAddress MappedAddressToActual(PVOID MappedAddress) const;

private:
	// base address of locally mapped kernel binary
	HMODULE m_MappedKernel;

	// actual base address of loaded kernel
	static kAddress s_KernelBase;
};