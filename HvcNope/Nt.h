#pragma once

#include <Windows.h>
#include <winternl.h>

//extern "C"
//NTSTATUS NtWaitForSingleObject(
//	HANDLE Object,
//	bool Aleratable,
//	PLARGE_INTEGER Timeout
//);


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

//struct SYSTEM_THREAD_INFORMATION {
//	LARGE_INTEGER KernelTime;
//	LARGE_INTEGER UserTime;
//	LARGE_INTEGER CreateTime;
//	ULONG WaitTime;
//	PVOID StartAddress;
//	struct {
//		HANDLE UniqueProcess;
//		HANDLE UniqueThread;
//	} ClientId;
//	LONG Priority;
//	LONG BasePriority;
//	ULONG ContextSwitches;
//	KTHREAD_STATE ThreadState;
//	ULONG WaitReason;
//};

constexpr ULONG ThreadSystemThreadInformation = 40;
