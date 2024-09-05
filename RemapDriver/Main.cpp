// NOTE: compiled without CFG

#include <ntifs.h>

extern "C"
NTSTATUS DriverEntry( PDRIVER_OBJECT, PUNICODE_STRING )
{
	//
	// We pick a simple function without any external references 
	// that fits in one page.
	//
	UNICODE_STRING exportName = RTL_CONSTANT_STRING( L"IoIs32bitProcess" );
	PVOID exampleCode = MmGetSystemRoutineAddress( &exportName );
	if (!exampleCode) {
		DbgPrint( "Failed to find export\n" );
		return STATUS_INTERNAL_ERROR;
	}

	auto* mdl = IoAllocateMdl( exampleCode, PAGE_SIZE, false, false, nullptr );
	if (!mdl) {
		DbgPrint( "Failed to allocate MDL\n" );
		return STATUS_INTERNAL_ERROR;
	}

	__try {
		MmProbeAndLockPages(
			mdl,
			KernelMode,
			IoReadAccess
		);
	}
	__except (EXCEPTION_EXECUTE_HANDLER) {
		DbgPrint( "Failed to lock pages\n" );
		IoFreeMdl( mdl );           // Free the MDL on failure
		return GetExceptionCode(); // Return the exception code
	}

	PVOID mappedView = MmGetSystemAddressForMdlSafe(
		mdl,
		NormalPagePriority
	);

	if (!mappedView) {
		DbgPrint( "Failed to map MDL to system view\n" );
		MmUnlockPages( mdl );
		IoFreeMdl( mdl );
		return STATUS_INTERNAL_ERROR;
	}

	DbgPrint( "Successfully mapped code into system view! Mapping: 0x%llx\n", ULONG64( mappedView ) );

	//
	// In here we modify the mappedView PTE to flip the NX bit
	// off. We also note that the PFN is the same as IoIs32bitProcess's
	// PFN, which is a bit odd considering it's part of a large page lol 
	//
	DbgBreakPoint();

	// try executing
	using IoIs32BitProcess_t = bool(*)(void*);

	bool is32bit = (IoIs32BitProcess_t( mappedView ))(nullptr);
	DbgPrint( "Result: %d\n", is32bit );

	MmUnlockPages( mdl );
	IoFreeMdl( mdl );
	return STATUS_INTERNAL_ERROR;
}