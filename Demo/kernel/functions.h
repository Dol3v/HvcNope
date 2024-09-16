
#include "types.h"

//
// Object related functions
//

NTSTATUS 
ObOpenObjectByName(
    POBJECT_ATTRIBUTES ObjectAttributes,
    void* ObjectType,
    KPROCESSOR_MODE AccessMode,
    void* AccessState,
    ACCESS_MASK DesiredAccess,
    PVOID ParseContext,
    PHANDLE Handle
);

NTSTATUS
ObReferenceObjectByHandle(
    HANDLE Handle,
    ACCESS_MASK DesiredAccess,
    PVOID ObjectType,
    KPROCESSOR_MODE AccessMode,
    PVOID *Object,
    PVOID HandleInformation
);

void ObDereferenceObject(PVOID Object);

//
// IRP related functions
//

PIRP IoAllocateIrp(CCHAR StackSize, BOOLEAN ChargeQuota);

//
// Misc
//
kAddress KeGetCurrentThread();
