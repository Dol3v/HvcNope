#pragma once

#include "Types.h"

#include <map>

//
// Generates kernel function pointers that can invoke our usermode code.
//
class PointerGenerator {
public:

	PointerGenerator();

	kAddress GenerateFunctionPointer(void* UsermodeFunction);

	~PointerGenerator();

private:

	struct UsermodeFunction 
	{	
		// Usermode function pointer to be called from the kernel
		void* FunctionPointer;

		// Process that ownes said pointer
		kAddress Process;
	};

private:

	std::optional<kAddress> RemapLongJmpGadget();

	bool InsertFunctionPointer(kAddress MappedGadget, const UsermodeFunction& Function);

	static bool CraftFunctionPointerStack(kAddress JmpBuf);

	static bool LocateLongjmpGadgetAndBuf();

private:

	static kAddress s_LongJmpGadget;
	static kAddress s_LongJmpBuf;
};