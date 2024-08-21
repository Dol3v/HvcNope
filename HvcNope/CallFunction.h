#pragma once

#include "Resolves.h"

#include <queue>
#include <functional>

extern HANDLE g_NotifyCall;
extern HANDLE g_WaitForCallEnd;

extern "C"
NTSTATUS NtSignalAndWaitForSingleObject(
	HANDLE ObjectToSignal,
	HANDLE ObjectToWaitOn,
	bool Aleratable,
	DWORD Timeout
);

extern "C"
NTSTATUS NtWaitForSingleObject(
	HANDLE Object,
	bool Aleratable,
	DWORD Timeout
);

class KInvoker 
{

	static kAddress GetNtWaitForSingleObjectReturnAddress(kAddress CallingThread)
	{
		//
		// We target the last return address before returning to UserMode. The call stack after
		// calling NtWaitForSingleObject is something like the following:
		// - nt!NtWaitForSingleObject
		// - nt!KiSystemServiceCopyEnd+C
		// - ntdll!NtWaitForSingleObject
		//
		// Note that we're switching to UM stack after KiSystemServiceCopyEnd, hence it makes sense to target it.
		// We'll start scanning from the initial stack 
		//

		constexpr ULONG c_InitialStackOffset = 0x28;
		kAddress initialStack = g_Rw->ReadQword(CallingThread + c_InitialStackOffset);

		constexpr ULONG c_TrapFrameSize = 0x190;
		kAddress returnAddress = g_Rw->ReadQword(initialStack - c_TrapFrameSize - sizeof(kAddress));
		return returnAddress;
	}

	template <typename... Args>
	static void OverrideThreadStackWithFunctionCall(
		_In_ kAddress Function,
		_In_ kAddress CallingThread,
		_Inout_ Qword* ReturnValue,
		_In_ std::tuple<Args...>&& Args
	) {
		kAddress returnAddress = GetNtWaitForSingleObjectReturnAddress(CallingThread);
		g_Rw->WriteQword()
	}

	/*
	Kernel callbacks: we'll put
	
	*/

public:
	template <typename... Args>
	Qword CallKernelFunction(
		kAddress Function,
		Args... Arguments) 
	{
		auto args = std::make_tuple(std::forward<Args>(Arguments)...);
		HANDLE event = CreateEventA(nullptr, true, false, nullptr);
		DWORD currentTid = GetCurrentThreadId();
		Qword returnValue = 0;

		m_TaskQueue.push([args, event, Function, currentTid, &returnValue]
			{
				kAddress thread = Resolves::GetThreadAddress(currentTid);
				OverrideThreadStackWithFunctionCall(Function, thread, &returnValue, args);
				SetEvent(event);
			});

		NtWaitForSingleObject(event, false, INFINITE);
	}

private:
	std::queue<std::function<void()>> m_TaskQueue;

};



void WorkerThread(PVOID Context) {

}