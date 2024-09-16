

#include <iostream>
#include <ntstatus.h>
#include <Hvcnope.h>

#include "kernel/functions.h"

unsigned char EicarSignatureXored[] = {
    0x58 ^ 0x37, 0x35 ^ 0x37, 0x4F ^ 0x37, 0x21 ^ 0x37, 0x50 ^ 0x37, 0x25 ^ 0x37, 0x40 ^ 0x37, 0x41 ^ 0x37,
    0x50 ^ 0x37, 0x5B ^ 0x37, 0x34 ^ 0x37, 0x5C ^ 0x37, 0x50 ^ 0x37, 0x5A ^ 0x37, 0x58 ^ 0x37, 0x35 ^ 0x37,
    0x34 ^ 0x37, 0x28 ^ 0x37, 0x50 ^ 0x37, 0x5E ^ 0x37, 0x29 ^ 0x37, 0x37 ^ 0x37, 0x43 ^ 0x37, 0x43 ^ 0x37,
    0x29 ^ 0x37, 0x37 ^ 0x37, 0x7D ^ 0x37, 0x24 ^ 0x37, 0x45 ^ 0x37, 0x49 ^ 0x37, 0x43 ^ 0x37, 0x41 ^ 0x37,
    0x52 ^ 0x37, 0x2D ^ 0x37, 0x53 ^ 0x37, 0x54 ^ 0x37, 0x41 ^ 0x37, 0x4E ^ 0x37, 0x44 ^ 0x37, 0x41 ^ 0x37,
    0x52 ^ 0x37, 0x44 ^ 0x37, 0x2D ^ 0x37, 0x41 ^ 0x37, 0x4E ^ 0x37, 0x54 ^ 0x37, 0x49 ^ 0x37, 0x56 ^ 0x37,
    0x49 ^ 0x37, 0x52 ^ 0x37, 0x55 ^ 0x37, 0x53 ^ 0x37, 0x2D ^ 0x37, 0x54 ^ 0x37, 0x45 ^ 0x37, 0x53 ^ 0x37,
    0x54 ^ 0x37, 0x2D ^ 0x37, 0x46 ^ 0x37, 0x49 ^ 0x37, 0x4C ^ 0x37, 0x45 ^ 0x37, 0x21 ^ 0x37, 0x24 ^ 0x37,
    0x48 ^ 0x37, 0x2B ^ 0x37, 0x48 ^ 0x37, 0x2A ^ 0x37
};


const char* DetectableFileName = "Test1.txt";
const char* UndetectableFileName = "Test2.txt";

int RunEicarTestWithUsermodeIo() 
{
    std::cout << "[*] Running first test - writing EICAR to file with normal APIs\n";

    auto* eicar = new BYTE[sizeof(EicarSignatureXored)];

    memcpy(eicar, EicarSignatureXored, sizeof(EicarSignatureXored));
    for (size_t i = 0; i < sizeof(EicarSignatureXored); ++i) {
        eicar[i] ^= 0x37;
    }

    // create new file and write to it

    HANDLE maliciousFile = CreateFileA(
        DetectableFileName,
        GENERIC_ALL,
        0,
        NULL,
        CREATE_ALWAYS, 
        FILE_ATTRIBUTE_NORMAL,
        0
    );

    if (maliciousFile == INVALID_HANDLE_VALUE) {
        delete[] eicar;
        return FALSE;
    }

    //
    // write EICAR to file - we might be stopped at the write itself,
    // but we should expect the file to be deleted anyways.
    //
    
    DWORD writtenBytes = 0;
    BOOL written = WriteFile(
        maliciousFile,
        eicar,
        sizeof(EicarSignatureXored),
        &writtenBytes,
        NULL
    );

    if (!written || writtenBytes < sizeof(EicarSignatureXored)) {
        std::cerr << "[*] File failed to be written to, le=" <<  GetLastError() << std::endl;

        CloseHandle(maliciousFile);
        delete[] eicar;
        return FALSE;
    }

    // Wait for defender to delete the file
    Sleep(100);

    if (PathFileExistsA(DetectableFileName)) {
        std::cerr << "[-] Test failed - defender didn't delete the file" << std::endl;;
    } else {
        std::cout << "[+] Defender deleted the file!" << std::endl;
    }

    CloseHandle(maliciousFile);
    free(eicar);
    return TRUE;    
}

kAddress OpenObjectByName(const wchar_t* Name, ACCESS_MASK AccessMask)
{
    UNICODE_STRING unicodeName = {0};
    (void)RtlInitUnicodeString(&unicodeName, Name);

    OBJECT_ATTRIBUTES attributes = RTL_CONSTANT_OBJECT_ATTRIBUTES(&unicodeName, OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE);
    HANDLE handle = 0;

    NTSTATUS status = ObOpenObjectByName(
        &attributes,
        NULL,
        KernelMode,
        nullptr,
        AccessMask,
        nullptr,
        &handle
    );

    if (!NT_SUCCESS(status)) {
        std::cerr << "[-] Failed to open object by name=" << Name << ", status=0x" << std::hex << status << std::endl;
        return 0;
    }

    // kernel handle, can be converted to pointer
    return kAddress(handle);
}

PDRIVER_DISPATCH NtfsFsdOpen = nullptr;
PDRIVER_DISPATCH NtfsFsdWrite = nullptr;
PDRIVER_DISPATCH NtfsFsdClose = nullptr;

NTSTATUS ResolveNtfsPointers()
{
    KernelPtr<DRIVER_OBJECT> ntfsDriver = OpenObjectByName(L"\\Filesystem\\Ntfs", GENERIC_READ);
    if (!ntfsDriver.get()) {
        std::cerr << "[-] Failed to open NTFS driver object" << std::endl;
        return STATUS_INTERNAL_ERROR;
    }

    NtfsFsdOpen  = reinterpret_cast<PDRIVER_DISPATCH>(ntfsDriver->MajorFunction[IRP_MJ_CREATE]);
    NtfsFsdWrite = reinterpret_cast<PDRIVER_DISPATCH>(ntfsDriver->MajorFunction[IRP_MJ_WRITE]);
    NtfsFsdClose = reinterpret_cast<PDRIVER_DISPATCH>(ntfsDriver->MajorFunction[IRP_MJ_CLOSE]);

    std::cout << "[+] Resolved NTFS pointers, NtfsFsdOpen=0x" << std::hex << kAddress(NtfsFsdOpen) << std::endl;

    return STATUS_SUCCESS;
}

//
// I/O Parameters passed to an IRP handler
//
struct IoParameters {
    
    //
    // Either METHOD_UNBUFFERED or METHOD_BUFFED, METHOD_DIRECT_Xx is not supported
    //
    enum Method {
        Buffered,
        Unbuffered
    };

    PVOID InputBuffer;
    size_t InputLength;

    PVOID OutputBuffer;
    size_t OutputLength;
};


NTSTATUS SendIrpToNtfs(
    const wchar_t* FilePath,
    ULONG MajorFunction,
    const IO_STACK_LOCATION::Parameters_t& Parameters,
    const IoParameters& Io)
{
    // Open file object from FilePath
    KernelPtr<FILE_OBJECT> file = OpenObjectByName(FilePath, GENERIC_READ);
    if (!file.get()) {
        std::wcerr << L"Failed to open file, path=" << FilePath << std::endl;
        return STATUS_NOT_FOUND;
    }
    
    // get device object, see fltmgr!FltpLegacyProcessingAfterPreCallbacksCompleted
    KernelPtr<FILE_OBJECT> related = file->RelatedFileObject;
    PVOID deviceObject = related->DeviceObject;

    // build IRP
    KernelPtr<IRP> irp = IoAllocateIrp(1, false);
    irp->UserBuffer = 0; // TODO

    IO_STATUS_BLOCK iosb = {0};
    irp->UserIosb = &iosb;

    // setup stack location
    KernelPtr<IO_STACK_LOCATION> currentStackLocation = irp->Tail.Overlay.CurrentStackLocation;
    currentStackLocation->MajorFunction = MajorFunction;
    currentStackLocation->MinorFunction = 0;
    currentStackLocation->DeviceObject = deviceObject;
    currentStackLocation->FileObject = file.get();
    currentStackLocation->Parameters = Parameters;

    // setup input/output
    
}

_DEVICE_OBJECT* GetVcbDeviceObject( const KernelPtr<FILE_OBJECT>& FileObject ) 
{
    // get VPB (volume parameter block)
    KernelPtr<VPB> vpb = FileObject->Vpb;
    return vpb->DeviceObject;
}

KernelPtr<IRP> CraftWriteIrp(
    PFILE_OBJECT FileObject,
    void* InputBuffer,
    size_t InputSize,
    _DEVICE_OBJECT* DeviceObject,
    IO_STATUS_BLOCK& Iosb) 
{
    constexpr CCHAR StackSize = 10; 

    KernelPtr<IRP> irp = IoAllocateIrp(StackSize, /*ChargeQuota=*/ false);
    irp->UserIosb = &Iosb;

    //
    // Even though the docs state that IRP_MJ_WRITE receives the buffer
    // in SystemBuffer, NTFS itself seems to use UserBuffer
    //
    irp->UserBuffer = InputBuffer;

    irp->Tail.Overlay.OriginalFileObject = (PVOID) FileObject;
    irp->Tail.Overlay.Thread =  (PVOID) KeGetCurrentThread();

    KernelPtr<IO_STACK_LOCATION> stackLocation = irp->Tail.Overlay.CurrentStackLocation;
    stackLocation->MajorFunction = IRP_MJ_WRITE;
    stackLocation->DeviceObject = DeviceObject;
    stackLocation->FileObject = FileObject;

    stackLocation->Parameters.Write.ByteOffset.QuadPart = 0;
    stackLocation->Parameters.Write.Length = InputSize;

    return irp;
}

NTSTATUS CreateFileKernelIo(PUNICODE_STRING FilePath) 
{
    std::cout << "[*] Running kernel I/O Test" << std::endl;

    //
    // Start off normally - get a usermode handle to the file
    //

    
}

int RunEicarTestWithKernelIo() 
{
    KernelPtr<IRP> createFileIrp = 
}


int main() 
{
    //
    //
    //

    ObOpenObjectByName()
}