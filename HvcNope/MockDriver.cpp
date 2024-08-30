#include "MockDriver.h"
#include "Log.h"

struct IoctlData {
    kAddress Address;
    Qword Data;
};

MockDriverRw::MockDriverRw() : m_DeviceHandle(INVALID_HANDLE_VALUE)
{
    m_DeviceHandle = CreateFile( L"\\\\.\\MockDriver", GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, 0, nullptr );
    if (m_DeviceHandle == INVALID_HANDLE_VALUE) {
        FATAL( "Failed to open mock device, le=%d", GetLastError() );
    }
}

MockDriverRw::~MockDriverRw()
{
    if (m_DeviceHandle != INVALID_HANDLE_VALUE) CloseHandle( m_DeviceHandle );
}

// Io control codes
constexpr ULONG IoctlReadQword = CTL_CODE( FILE_DEVICE_UNKNOWN, 0x800, METHOD_BUFFERED, FILE_ANY_ACCESS );
constexpr ULONG IoctlWriteQword = CTL_CODE( FILE_DEVICE_UNKNOWN, 0x801, METHOD_BUFFERED, FILE_ANY_ACCESS );


Qword MockDriverRw::ReadQword( kAddress Address )
{
    DWORD returned = 0;
    IoctlData inputOutput = { Address, 0 };
    if (!DeviceIoControl( m_DeviceHandle, IoctlReadQword, &inputOutput, sizeof( inputOutput ), &inputOutput, sizeof( inputOutput ), &returned, nullptr )) {
        FATAL( "Failed to IOCTL, le=%d", GetLastError() );
    }
    return inputOutput.Data;
}

void MockDriverRw::WriteQword( kAddress Address, Qword Value )
{
    DWORD returned = 0;
    IoctlData input = { Address, Value };
    if (!DeviceIoControl( m_DeviceHandle, IoctlWriteQword, &input, sizeof( input ), nullptr, 0, &returned, nullptr )) {
        FATAL( "Failed to IOCTL, le=%d", GetLastError() );
    }
}
