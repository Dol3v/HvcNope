
#include "Resolves.h"
#include "KernelBinary.h"
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

    constexpr ULONG ActiveProcessLinksOffset = 0x448;
    constexpr ULONG ProcessIdOffset = 0x440;

    kAddress process = kNullptr;

    Utils::ForEveryListElement(processListHead, ActiveProcessLinksOffset,
        [&](kAddress Process) {
            if (g_Rw->ReadQword(Process + ProcessIdOffset) == ProcessId)
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

    const Dword cProcessThreadListHeadOffset = 0x5e0;

    // try getting the address of a single thread
    auto processThreadList = currentProcess + cProcessThreadListHeadOffset;

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

    const Dword cThreadListEntryOffset  = 0x4e8;
    const Dword cThreadCidOffset        = 0x478;

    kAddress threadAddress = kNullptr;
    using namespace Utils;

    ForEveryListElement(threadListHead, cThreadListEntryOffset,
        [&](kAddress Thread) {
            kAddress threadCid = Thread + cThreadCidOffset;

            // threadCid->UniqueThread
            Qword threadId = g_Rw->ReadQword(threadCid + 8);
            if (Dword(threadId) == ThreadId) {
                // found thread
                threadAddress = Thread;
                return cStopLoop;
            }

            return cContinueLoop;
        });

    return threadAddress;
}

