
#include "Resolves.h"

#include "KernelBinary.h"
#include "Offsets.h"
#include "Utils.h"
#include "Log.h"


kAddress GetSystemProcess()
{
    auto PsInitialSystemProcess = g_KernelBinary->ResolveExport( "PsInitialSystemProcess" );
    if (!PsInitialSystemProcess) {
        // something is seriously off
        FATAL("Failed to resolve PsInitialSystemProcess");
    }

    LOG_DEBUG( "Before: %llx", PsInitialSystemProcess.value() );
    kAddress systemProcess = g_Rw->ReadQword( PsInitialSystemProcess.value() );
    LOG_DEBUG( "After: %llx", systemProcess );

    return systemProcess;
}


std::optional<kAddress> Resolves::GetProcessAddress(Dword ProcessId)
{
    static kAddress processListHead = kNullptr;
    using namespace Offsets;

    if (processListHead == kNullptr) {
        // resolve from export
        auto systemProcess = GetSystemProcess();
        processListHead = systemProcess + EProcess::ActiveProcessLinks;
        LOG_DEBUG( "Resolved process list head: 0x%llx", processListHead );
    }

    std::optional<kAddress> process = std::nullopt;

    Utils::ForEveryListElement( processListHead, EProcess::ActiveProcessLinks,
        [&]( kAddress Process ) {
            if (g_Rw->ReadQword( Process + EProcess::ProcessId ) == ProcessId)
            {
                process = Process;
                return Utils::cStopLoop;
            }

            return Utils::cContinueLoop;
        } );

    LOG_DEBUG( "Address of process %d is 0x%llx", ProcessId, process.value_or( 0 ) );
    return process;
}

std::optional<kAddress> Resolves::GetThreadAddressInProcess( Dword ThreadId, Dword ProcessId )
{
    auto process = Resolves::GetProcessAddress( ProcessId );
    if (!process) return std::nullopt;

    using namespace Offsets;

    kAddress threadListHead = process.value() + EProcess::ThreadListHead;
    std::optional<kAddress> threadAddress = std::nullopt;
        
    Utils::ForEveryListElement(threadListHead, EThread::ListEntry,
        [&](kAddress Thread) {
            kAddress threadCid = Thread + EThread::Cid;

            // threadCid->UniqueThread
            Qword threadId = g_Rw->ReadQword(threadCid + 8);

            if (threadId == ThreadId) {
                // found thread
                threadAddress = Thread;
                return Utils::cStopLoop;
            }

            return Utils::cContinueLoop;
        });

    return threadAddress;
}

