
#include "Resolves.h"

#include "KernelBinary.h"
#include "Offsets.h"
#include "Utils.h"


kAddress Resolves::GetProcessAddress(Dword ProcessId)
{
    static kAddress processListHead = kNullptr;

    if (processListHead == kNullptr) {
        // resolve from export

        processListHead = g_KernelBinary->ResolveExport("PsInitialSystemProcess");
        if (!processListHead) {
            // error
        }
    }

    using namespace Offsets;

    kAddress process = kNullptr;

    Utils::ForEveryListElement(processListHead, EProcess::ActiveProcessLinks,
        [&](kAddress Process) {
            if (g_Rw->ReadQword(Process + EProcess::ProcessId) == ProcessId)
            {
                process = kNullptr;
                return Utils::cStopLoop;
            }

            return Utils::cContinueLoop;
        });

    return process;
}

kAddress TryResolveThreadListHead() 
{
    auto currentProcessId = GetCurrentProcessId();
    kAddress currentProcess = Resolves::GetProcessAddress(currentProcessId);
    if (kNullptr == currentProcess) return kNullptr;

    // try getting the address of a single thread
    auto processThreadList = currentProcess + Offsets::EProcess::ThreadListHead;

    kAddress threadListEntry = kNullptr;
    Utils::ForEveryListEntry(processThreadList,
        [&](kAddress ThreadListEntry) {
            threadListEntry = ThreadListEntry;
            return Utils::cStopLoop; // stop loop immediately
        });

    if (kNullptr == threadListEntry) {
        // TODO: log
        return kNullptr;
    }

    // this is a doubly-linked list, doesn't matter where we're starting
    return threadListEntry;
}

kAddress Resolves::GetThreadAddress(Dword ThreadId)
{
    static kAddress threadListHead = kNullptr;

    if (kNullptr == threadListHead) {
        threadListHead = TryResolveThreadListHead();
        if (kNullptr == threadListHead) {
            // TODO: log
            return kNullptr;
        }
    }

    kAddress threadAddress = kNullptr;
    
    using namespace Offsets;

    Utils::ForEveryListElement(threadListHead, EThread::ListEntry,
        [&](kAddress Thread) {
            kAddress threadCid = Thread + EThread::Cid;

            // threadCid->UniqueThread
            Qword threadId = g_Rw->ReadQword(threadCid + 8);
            if (Dword(threadId) == ThreadId) {
                // found thread
                threadAddress = Thread;
                return Utils::cStopLoop;
            }

            return Utils::cContinueLoop;
        });

    return threadAddress;
}

