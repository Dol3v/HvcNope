#pragma once

// #include <windows.h>

#define _AMD64_
#include <ntdef.h>

#include "attrs.h"

typedef unsigned long ACCESS_MASK;

struct IO_STATUS_BLOCK {
    union {
        LONG Status;
        void* Pointer;
    };
    ULONGLONG Information;
};

typedef IO_STATUS_BLOCK* PIO_STATUS_BLOCK;

enum KPROCESSOR_MODE {
    KernelMode = 0,
    UserMode
};

struct _DEVICE_OBJECT;


#define TIMER_TOLERABLE_DELAY_BITS      6
#define TIMER_EXPIRED_INDEX_BITS        6
#define TIMER_PROCESSOR_INDEX_BITS      5


typedef struct _DISPATCHER_HEADER {
    union {
        union {
            volatile LONG Lock;
            LONG LockNV;
        } DUMMYUNIONNAME;

        struct {                            // Events, Semaphores, Gates, etc.
            UCHAR Type;                     // All (accessible via KOBJECT_TYPE)
            UCHAR Signalling;
            UCHAR Size;
            UCHAR Reserved1;
        } DUMMYSTRUCTNAME;

        struct {                            // Timer
            UCHAR TimerType;
            union {
                UCHAR TimerControlFlags;
                struct {
                    UCHAR Absolute : 1;
                    UCHAR Wake : 1;
                    UCHAR EncodedTolerableDelay : TIMER_TOLERABLE_DELAY_BITS;
                } DUMMYSTRUCTNAME;
            };

            UCHAR Hand;
            union {
                UCHAR TimerMiscFlags;
                struct {

#if !defined(KENCODED_TIMER_PROCESSOR)

                    UCHAR Index : TIMER_EXPIRED_INDEX_BITS;

#else

                    UCHAR Index : 1;
                    UCHAR Processor : TIMER_PROCESSOR_INDEX_BITS;

#endif

                    UCHAR Inserted : 1;
                    volatile UCHAR Expired : 1;
                } DUMMYSTRUCTNAME;
            } DUMMYUNIONNAME;
        } DUMMYSTRUCTNAME2;

        struct {                            // Timer2
            UCHAR Timer2Type;
            union {
                UCHAR Timer2Flags;
                struct {
                    UCHAR Timer2Inserted : 1;
                    UCHAR Timer2Expiring : 1;
                    UCHAR Timer2CancelPending : 1;
                    UCHAR Timer2SetPending : 1;
                    UCHAR Timer2Running : 1;
                    UCHAR Timer2Disabled : 1;
                    UCHAR Timer2ReservedFlags : 2;
                } DUMMYSTRUCTNAME;
            } DUMMYUNIONNAME;

            UCHAR Timer2ComponentId;
            UCHAR Timer2RelativeId;
        } DUMMYSTRUCTNAME3;

        struct {                            // Queue
            UCHAR QueueType;
            union {
                UCHAR QueueControlFlags;
                struct {
                    UCHAR Abandoned : 1;
                    UCHAR DisableIncrement : 1;
                    UCHAR QueueReservedControlFlags : 6;
                } DUMMYSTRUCTNAME;
            } DUMMYUNIONNAME;

            UCHAR QueueSize;
            UCHAR QueueReserved;
        } DUMMYSTRUCTNAME4;

        struct {                            // Thread
            UCHAR ThreadType;
            UCHAR ThreadReserved;

            union {
                UCHAR ThreadControlFlags;
                struct {
                    UCHAR CycleProfiling : 1;
                    UCHAR CounterProfiling : 1;
                    UCHAR GroupScheduling : 1;
                    UCHAR AffinitySet : 1;
                    UCHAR Tagged : 1;
                    UCHAR EnergyProfiling: 1;
                    UCHAR SchedulerAssist: 1;

#if !defined(_X86_)

                    UCHAR ThreadReservedControlFlags : 1;

#else

                    UCHAR Instrumented : 1;

#endif

                } DUMMYSTRUCTNAME;
            } DUMMYUNIONNAME;

            union {
                UCHAR DebugActive;

#if !defined(_X86_)

                struct {
                    BOOLEAN ActiveDR7 : 1;
                    BOOLEAN Instrumented : 1;
                    BOOLEAN Minimal : 1;
                    BOOLEAN Reserved4 : 2;
                    BOOLEAN AltSyscall : 1;
                    BOOLEAN Emulation : 1;
                    BOOLEAN Reserved5 : 1;
                } DUMMYSTRUCTNAME;

#endif

            } DUMMYUNIONNAME2;
        } DUMMYSTRUCTNAME5;

        struct {                         // Mutant
            UCHAR MutantType;
            UCHAR MutantSize;
            BOOLEAN DpcActive;
            UCHAR MutantReserved;
        } DUMMYSTRUCTNAME6;
    } DUMMYUNIONNAME;

    LONG SignalState;                   // Object lock
    LIST_ENTRY WaitListHead;            // Object lock
} DISPATCHER_HEADER, *PDISPATCHER_HEADER;


//
// Event object
//

typedef struct _KEVENT {
    DISPATCHER_HEADER Header;
} KEVENT, *PKEVENT, *PRKEVENT;


typedef struct _FILE_OBJECT {
    CSHORT Type;
    CSHORT Size;
    PVOID DeviceObject;
    PVOID Vpb;
    PVOID FsContext;
    PVOID FsContext2;
    PVOID SectionObjectPointer;
    PVOID PrivateCacheMap;
    NTSTATUS FinalStatus;
    struct _FILE_OBJECT *RelatedFileObject;
    BOOLEAN LockOperation;
    BOOLEAN DeletePending;
    BOOLEAN ReadAccess;
    BOOLEAN WriteAccess;
    BOOLEAN DeleteAccess;
    BOOLEAN SharedRead;
    BOOLEAN SharedWrite;
    BOOLEAN SharedDelete;
    ULONG Flags;
    UNICODE_STRING FileName;
    LARGE_INTEGER CurrentByteOffset;
    __volatile ULONG Waiters;
    __volatile ULONG Busy;
    PVOID LastLock;
    KEVENT Lock;
    KEVENT Event;
    __volatile PVOID CompletionContext;
    ULONG_PTR IrpListLock;
    LIST_ENTRY IrpList;
    __volatile PVOID FileObjectExtension;
} FILE_OBJECT;
typedef struct _FILE_OBJECT *PFILE_OBJECT; 


#define POINTER_ALIGNMENT alignas(8)

typedef struct _IO_STACK_LOCATION {
    UCHAR MajorFunction;
    UCHAR MinorFunction;
    UCHAR Flags;
    UCHAR Control;

    //
    // The following user parameters are based on the service that is being
    // invoked.  Drivers and file systems can determine which set to use based
    // on the above major and minor function codes.
    //

    // NOTE: local change, we created a type name for
    // the parameters for later usage
    union Parameters_t {

        //
        // System service parameters for:  NtCreateFile
        //

        struct {
            PVOID SecurityContext;
            ULONG Options;
            USHORT FileAttributes POINTER_ALIGNMENT;
            USHORT ShareAccess;
            ULONG EaLength POINTER_ALIGNMENT;
        } Create;

        //
        // System service parameters for:  NtReadFile
        //

        struct {
            ULONG Length;
            ULONG Key POINTER_ALIGNMENT;
#if defined(_WIN64)
            ULONG Flags;
#endif
            LARGE_INTEGER ByteOffset;
        } Read;

        //
        // System service parameters for:  NtWriteFile
        //

        struct {
            ULONG Length;
            ULONG Key POINTER_ALIGNMENT;
#if defined(_WIN64)
            ULONG Flags;
#endif
            LARGE_INTEGER ByteOffset;
        } Write;

        //
        // Others - driver-specific
        //

        struct {
            PVOID Argument1;
            PVOID Argument2;
            PVOID Argument3;
            PVOID Argument4;
        } Others;

    } Parameters;

    //
    // Save a pointer to this device driver's device object for this request
    // so it can be passed to the completion routine if needed.
    //

    _DEVICE_OBJECT* DeviceObject;

    //
    // The following location contains a pointer to the file object for this
    // request.
    //

    FILE_OBJECT* FileObject;

    //
    // The following routine is invoked depending on the flags in the above
    // flags field.
    //

    PVOID CompletionRoutine;

    //
    // The following is used to store the address of the context parameter
    // that should be passed to the CompletionRoutine.
    //

    PVOID Context;

} IO_STACK_LOCATION, *PIO_STACK_LOCATION;

typedef struct _KAPC {                  
    UCHAR Type;                         
        UCHAR AllFlags;                 
    UCHAR Size;                         
    UCHAR SpareByte1;                   
    ULONG SpareLong0;                   
    struct _KTHREAD *Thread;            
    LIST_ENTRY ApcListEntry;            
    PVOID Reserved[3];                  
    PVOID NormalContext;                
    PVOID SystemArgument1;              
    PVOID SystemArgument2;              
    CCHAR ApcStateIndex;                
    KPROCESSOR_MODE ApcMode;            
    BOOLEAN Inserted;                   
} KAPC, *PKAPC, *PRKAPC;     

typedef struct DECLSPEC_ALIGN(MEMORY_ALLOCATION_ALIGNMENT) _IRP {
    short Type;
    USHORT Size;


    //
    // Define the common fields used to control the IRP.
    //

    //
    // Define a pointer to the Memory Descriptor List (MDL) for this I/O
    // request.  This field is only used if the I/O is "direct I/O".
    //

    PVOID MdlAddress;

    //
    // Flags word - used to remember various flags.
    //

    ULONG Flags;

    //
    // The following union is used for one of three purposes:
    //
    //    1. This IRP is an associated IRP.  The field is a pointer to a master
    //       IRP.
    //
    //    2. This is the master IRP.  The field is the count of the number of
    //       IRPs which must complete (associated IRPs) before the master can
    //       complete.
    //
    //    3. This operation is being buffered and the field is the address of
    //       the system space buffer.
    //

    union {
        struct _IRP *MasterIrp;
        __volatile LONG IrpCount;
        PVOID SystemBuffer;
    } AssociatedIrp;

    //
    // Thread list entry - allows queuing the IRP to the thread pending I/O
    // request packet list.
    //

    LIST_ENTRY ThreadListEntry;

    //
    // I/O status - final status of operation.
    //

    IO_STATUS_BLOCK IoStatus;

    //
    // Requester mode - mode of the original requester of this operation.
    //

    KPROCESSOR_MODE RequestorMode;

    //
    // Pending returned - TRUE if pending was initially returned as the
    // status for this packet.
    //

    BOOLEAN PendingReturned;

    //
    // Stack state information.
    //

    CHAR StackCount;
    CHAR CurrentLocation;

    //
    // Cancel - packet has been canceled.
    //

    BOOLEAN Cancel;

    //
    // Cancel Irql - Irql at which the cancel spinlock was acquired.
    //

    KIRQL CancelIrql;

    //
    // ApcEnvironment - Used to save the APC environment at the time that the
    // packet was initialized.
    //

    CCHAR ApcEnvironment;

    //
    // Allocation control flags.
    //

    UCHAR AllocationFlags;

    //
    // User parameters.
    //

    union {
        PIO_STATUS_BLOCK UserIosb;

        //
        // Context used when the Irp is managed by IoRing and is used by IoRing.
        // UserIosb is used to cancel an Irp, so sharing space with UserIosb
        // let IoRing cancel an Irp based on its context.
        //

        PVOID IoRingContext;
    };

    PVOID UserEvent;
    union {
        struct {
            union {
                PVOID UserApcRoutine;
                PVOID IssuingProcess;
            };
            union {
                PVOID UserApcContext;

                //
                // IoRing object that rolled this Irp, if any.  The completion
                // is processed through this IoRing object.  UserApcRoutine and
                // UserApcContext is not supported when issuing IOs through an
                // IoRing so we union this with UserApcContext.  We did not use
                // UserApcRoutine because IssuingProcess use the same location
                // and is used when an Irp is queued to FileObject and when the
                // Irp is managed by IoRing it is queued to the FileObject.
                //

                struct _IORING_OBJECT* IoRing;
            };
        } AsynchronousParameters;
        LARGE_INTEGER AllocationSize;
    } Overlay;

    //
    // CancelRoutine - Used to contain the address of a cancel routine supplied
    // by a device driver when the IRP is in a cancelable state.
    //

    __volatile PVOID CancelRoutine;

    //
    // Note that the UserBuffer parameter is outside of the stack so that I/O
    // completion can copy data back into the user's address space without
    // having to know exactly which service was being invoked.  The length
    // of the copy is stored in the second half of the I/O status block. If
    // the UserBuffer field is NULL, then no copy is performed.
    //

    PVOID UserBuffer;

    //
    // Kernel structures
    //
    // The following section contains kernel structures which the IRP needs
    // in order to place various work information in kernel controller system
    // queues.  Because the size and alignment cannot be controlled, they are
    // placed here at the end so they just hang off and do not affect the
    // alignment of other fields in the IRP.
    //

    union {

        struct {

            union {

                //
                // DeviceQueueEntry - The device queue entry field is used to
                // queue the IRP to the device driver device queue.
                //

                // KDEVICE_QUEUE_ENTRY DeviceQueueEntry;

                struct {

                    //
                    // The following are available to the driver to use in
                    // whatever manner is desired, while the driver owns the
                    // packet.
                    //

                    PVOID DriverContext[4];

                } ;

            } ;

            //
            // Thread - pointer to caller's Thread Control Block.
            //

            PVOID Thread;

            //
            // Auxiliary buffer - pointer to any auxiliary buffer that is
            // required to pass information to a driver that is not contained
            // in a normal buffer.
            //

            PCHAR AuxiliaryBuffer;

            //
            // The following unnamed structure must be exactly identical
            // to the unnamed structure used in the minipacket header used
            // for completion queue entries.
            //

            struct {

                //
                // List entry - used to queue the packet to completion queue, among
                // others.
                //

                LIST_ENTRY ListEntry;

                union {

                    //
                    // Current stack location - contains a pointer to the current
                    // IO_STACK_LOCATION structure in the IRP stack.  This field
                    // should never be directly accessed by drivers.  They should
                    // use the standard functions.
                    //

                    struct _IO_STACK_LOCATION *CurrentStackLocation;

                    //
                    // Minipacket type.
                    //

                    ULONG PacketType;
                };
            };

            //
            // Original file object - pointer to the original file object
            // that was used to open the file.  This field is owned by the
            // I/O system and should not be used by any other drivers.
            //

            PVOID OriginalFileObject;

        } Overlay;

        //
        // APC - This APC control block is used for the special kernel APC as
        // well as for the caller's APC, if one was specified in the original
        // argument list.  If so, then the APC is reused for the normal APC for
        // whatever mode the caller was in and the "special" routine that is
        // invoked before the APC gets control simply deallocates the IRP.
        //

        KAPC Apc;

        //
        // CompletionKey - This is the key that is used to distinguish
        // individual I/O operations initiated on a single file handle.
        //

        PVOID CompletionKey;

    } Tail;

} IRP;

typedef IRP *PIRP;

typedef NTSTATUS (*PDRIVER_DISPATCH)(void*, PIRP) KERNEL;

#define IRP_MJ_CREATE                   0x00
#define IRP_MJ_CREATE_NAMED_PIPE        0x01
#define IRP_MJ_CLOSE                    0x02
#define IRP_MJ_READ                     0x03
#define IRP_MJ_WRITE                    0x04
#define IRP_MJ_QUERY_INFORMATION        0x05
#define IRP_MJ_SET_INFORMATION          0x06
#define IRP_MJ_QUERY_EA                 0x07
#define IRP_MJ_SET_EA                   0x08
#define IRP_MJ_FLUSH_BUFFERS            0x09
#define IRP_MJ_QUERY_VOLUME_INFORMATION 0x0a
#define IRP_MJ_SET_VOLUME_INFORMATION   0x0b
#define IRP_MJ_DIRECTORY_CONTROL        0x0c
#define IRP_MJ_FILE_SYSTEM_CONTROL      0x0d
#define IRP_MJ_DEVICE_CONTROL           0x0e
#define IRP_MJ_INTERNAL_DEVICE_CONTROL  0x0f
#define IRP_MJ_SHUTDOWN                 0x10
#define IRP_MJ_LOCK_CONTROL             0x11
#define IRP_MJ_CLEANUP                  0x12
#define IRP_MJ_CREATE_MAILSLOT          0x13
#define IRP_MJ_QUERY_SECURITY           0x14
#define IRP_MJ_SET_SECURITY             0x15
#define IRP_MJ_POWER                    0x16
#define IRP_MJ_SYSTEM_CONTROL           0x17
#define IRP_MJ_DEVICE_CHANGE            0x18
#define IRP_MJ_QUERY_QUOTA              0x19
#define IRP_MJ_SET_QUOTA                0x1a
#define IRP_MJ_PNP                      0x1b
#define IRP_MJ_PNP_POWER                IRP_MJ_PNP      // Obsolete....
#define IRP_MJ_MAXIMUM_FUNCTION         0x1b

typedef struct _DRIVER_OBJECT {
    CSHORT Type;
    CSHORT Size;

    //
    // The following links all of the devices created by a single driver
    // together on a list, and the Flags word provides an extensible flag
    // location for driver objects.
    //

    PVOID DeviceObject;
    ULONG Flags;

    //
    // The following section describes where the driver is loaded.  The count
    // field is used to count the number of times the driver has had its
    // registered reinitialization routine invoked.
    //

    PVOID DriverStart;
    ULONG DriverSize;
    PVOID DriverSection;
    PVOID DriverExtension;

    //
    // The driver name field is used by the error log thread
    // determine the name of the driver that an I/O request is/was bound.
    //

    UNICODE_STRING DriverName;

    //
    // The following section is for registry support.  This is a pointer
    // to the path to the hardware information in the registry
    //

    PUNICODE_STRING HardwareDatabase;

    //
    // The following section contains the optional pointer to an array of
    // alternate entry points to a driver for "fast I/O" support.  Fast I/O
    // is performed by invoking the driver routine directly with separate
    // parameters, rather than using the standard IRP call mechanism.  Note
    // that these functions may only be used for synchronous I/O, and when
    // the file is cached.
    //

    PVOID FastIoDispatch;

    //
    // The following section describes the entry points to this particular
    // driver.  Note that the major function dispatch table must be the last
    // field in the object so that it remains extensible.
    //

    PVOID DriverInit;
    PVOID DriverStartIo;
    PVOID DriverUnload;
    PVOID MajorFunction[IRP_MJ_MAXIMUM_FUNCTION + 1];

} DRIVER_OBJECT;

struct _VPB
{
    SHORT Type;                                                             //0x0
    SHORT Size;                                                             //0x2
    USHORT Flags;                                                           //0x4
    USHORT VolumeLabelLength;                                               //0x6
    struct _DEVICE_OBJECT* DeviceObject;                                    //0x8
    struct _DEVICE_OBJECT* RealDevice;                                      //0x10
    ULONG SerialNumber;                                                     //0x18
    ULONG ReferenceCount;                                                   //0x1c
    WCHAR VolumeLabel[32];                                                  //0x20
}; 

typedef _VPB VPB;
typedef _VPB* PVPB;
