#include <ntddk.h>

#define IOCTL_READ_QWORD CTL_CODE(FILE_DEVICE_UNKNOWN, 0x800, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_WRITE_QWORD CTL_CODE(FILE_DEVICE_UNKNOWN, 0x801, METHOD_BUFFERED, FILE_ANY_ACCESS)

typedef struct _IOCTL_DATA {
    PVOID Address;
    ULONGLONG Data;
} IOCTL_DATA, * PIOCTL_DATA;

NTSTATUS DriverUnsupportedIoctl( PDEVICE_OBJECT DeviceObject, PIRP Irp )
{
    UNREFERENCED_PARAMETER( DeviceObject );
    Irp->IoStatus.Status = STATUS_INVALID_DEVICE_REQUEST;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest( Irp, IO_NO_INCREMENT );
    return STATUS_INVALID_DEVICE_REQUEST;
}

NTSTATUS DriverIoControl( PDEVICE_OBJECT, PIRP Irp )
{
    PIO_STACK_LOCATION stack = IoGetCurrentIrpStackLocation( Irp );
    NTSTATUS status = STATUS_SUCCESS;
    PIOCTL_DATA ioctlData;

    if (stack->Parameters.DeviceIoControl.InputBufferLength < sizeof( IOCTL_DATA )) {
        status = STATUS_BUFFER_TOO_SMALL;
        Irp->IoStatus.Information = 0;
        goto End;
    }

    ioctlData = (PIOCTL_DATA)Irp->AssociatedIrp.SystemBuffer;

    switch (stack->Parameters.DeviceIoControl.IoControlCode) {
    case IOCTL_READ_QWORD:
        __try {
            ioctlData->Data = *(volatile ULONGLONG*)ioctlData->Address;
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            DbgPrintEx( DPFLTR_IHVDRIVER_ID, 1, "Failed to read qword from 0x%llx\n", ioctlData->Address );
            DbgBreakPoint();
            status = GetExceptionCode();
        }
        Irp->IoStatus.Information = sizeof( IOCTL_DATA );
        break;

    case IOCTL_WRITE_QWORD:
        __try {
            *(volatile ULONGLONG*)ioctlData->Address = ioctlData->Data;
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            DbgPrintEx( DPFLTR_IHVDRIVER_ID, 1, "Failed to write qword to 0x%llx\n", ioctlData->Address );
            DbgBreakPoint();
            status = GetExceptionCode();
        }
        Irp->IoStatus.Information = 0;
        break;

    default:
        status = STATUS_INVALID_DEVICE_REQUEST;
        Irp->IoStatus.Information = 0;
        break;
    }

End:
    Irp->IoStatus.Status = status;
    IoCompleteRequest( Irp, IO_NO_INCREMENT );
    return status;
}

NTSTATUS DriverCreateClose( PDEVICE_OBJECT DeviceObject, PIRP Irp )
{
    UNREFERENCED_PARAMETER( DeviceObject );
    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest( Irp, IO_NO_INCREMENT );
    return STATUS_SUCCESS;
}

void DriverUnload( PDRIVER_OBJECT DriverObject )
{
    UNICODE_STRING symLink = RTL_CONSTANT_STRING( L"\\??\\MockDriver" );
    IoDeleteSymbolicLink( &symLink );
    IoDeleteDevice( DriverObject->DeviceObject );
}

extern "C" 
NTSTATUS DriverEntry( PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath )
{
    UNREFERENCED_PARAMETER( RegistryPath );
    UNICODE_STRING devName = RTL_CONSTANT_STRING( L"\\Device\\MockDriver" );
    UNICODE_STRING symLink = RTL_CONSTANT_STRING( L"\\??\\MockDriver" );
    PDEVICE_OBJECT deviceObject = NULL;
    NTSTATUS status;

    status = IoCreateDevice( DriverObject, 0, &devName, FILE_DEVICE_UNKNOWN, 0, FALSE, &deviceObject );
    if (!NT_SUCCESS( status )) {
        return status;
    }

    status = IoCreateSymbolicLink( &symLink, &devName );
    if (!NT_SUCCESS( status )) {
        IoDeleteDevice( deviceObject );
        return status;
    }

    DriverObject->MajorFunction[IRP_MJ_CREATE] = DriverCreateClose;
    DriverObject->MajorFunction[IRP_MJ_CLOSE] = DriverCreateClose;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = DriverIoControl;
    DriverObject->DriverUnload = DriverUnload;

    deviceObject->Flags &= ~DO_DEVICE_INITIALIZING;
    return STATUS_SUCCESS;
}
