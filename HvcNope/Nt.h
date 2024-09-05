#pragma once

#include <Windows.h>
#include "Types.h"

#ifndef NT_SUCCESS
#define NT_SUCCESS(Status) (((NTSTATUS)(Status)) >= 0)
#endif


//
// Note the Qword return type - we hijack the execution flow of this syscall,
// and its return value is the one from the kernel function we desire to call.
// 
// See KInvoker for more info.
//
extern "C"
Qword NtWaitForSingleObject(
	HANDLE Object,
	bool Aleratable,
	PLARGE_INTEGER Timeout
);


enum class KTHREAD_STATE : ULONG
{
	Initialized = 0,
	Ready = 1,
	Running = 2,
	Standby = 3,
	Terminated = 4,
	Waiting = 5,
	Transition = 6,
	DeferredReady = 7,
	GateWaitObsolete = 8,
	WaitingForProcessInSwap = 9
};

typedef LONG KPRIORITY;

typedef struct _CLIENT_ID {
	HANDLE UniqueProcess;
	HANDLE UniqueThread;
} CLIENT_ID;

typedef struct _SYSTEM_THREAD_INFORMATION {
	LARGE_INTEGER Reserved1[3];
	ULONG Reserved2;
	PVOID StartAddress;
	CLIENT_ID ClientId;
	KPRIORITY Priority;
	LONG BasePriority;
	ULONG Reserved3;
	ULONG ThreadState;
	ULONG WaitReason;
} SYSTEM_THREAD_INFORMATION, * PSYSTEM_THREAD_INFORMATION;

extern "C"
NTSTATUS NtQueryInformationThread(
	IN HANDLE ThreadHandle,
	IN ULONG ThreadInformationClass,
	OUT PVOID ThreadInformation,
	IN ULONG ThreadInformationLength,
	OUT PULONG ReturnLength OPTIONAL
);

constexpr ULONG ThreadSystemThreadInformation = 40;
