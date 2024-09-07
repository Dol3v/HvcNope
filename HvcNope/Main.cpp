#include "Globals.h"
//#include "Truesight.h"
#include "MockDriver.h"
#include "CallFunction.h"

#include <iostream>

#pragma comment(lib, "ntdll.lib")

// initialize Globals
//Logger g_Logger(Logger::LogLevel::DEBUG);

std::shared_ptr<KernelReadWrite> g_Rw = std::make_shared<MockDriverRw>();
std::shared_ptr<KernelBinary> g_KernelBinary = std::make_shared<KernelBinary>();
std::shared_ptr<KInvoker> g_Invoker = std::make_shared<KInvoker>();

void InitializeGraphicalSubsystem()
{
	LoadLibraryA("user32.dll");
}

int main() 
{
	//HMODULE hNtos = LoadLibraryA("ntoskrnl.exe");
	//std::cout << hNtos << std::endl;
	//if (nullptr == hNtos) return -1;
	//std::cout << GetProcAddress(hNtos, "PsGetProcessId") << std::endl;

	//InitializeGraphicalSubsystem();

	KInvoker invoker;
	//kAddress poolAddress = invoker.CallKernelFunction(
	//	"ExAllocatePool2",
	//	0x41,	// POOL_FLAG_PAGED,
	//	0x100,	// size
	//	'T3st'	// pool tag
	//);

	kAddress base = g_KernelBinary->GetKernelBase();

	kAddress poolAddress = invoker.CallKernelFunction(
		base + 0x3100f0,	// ExAllocatePoolWithTagFromNode
		1,					// PagedPool
		0x100,				// size
		'T3st',				// pool tag
		0x80000000,			// node smth?
		0					// default node? 0 on ExAllocatePoolWithTag lol
	);

	std::cout << std::hex << poolAddress << std::endl;

/*	Dword currentPid = GetCurrentProcessId();
	Dword currentTid = GetCurrentThreadId();

	auto process = Resolves::GetProcessAddress( currentPid );
	LOG_INFO( "Process: 0x%llx", process.value_or( 0 ) );
	DebugBreak();

	auto thread = Resolves::GetThreadAddressInProcess( currentTid, currentPid );
	LOG_INFO( "Thread: 0x%llx", thread.value_or( 0 ) );

	DebugBreak()*/;
}