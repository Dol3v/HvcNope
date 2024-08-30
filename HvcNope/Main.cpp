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
	kAddress poolAddress = invoker.CallKernelFunction(
		"ExAllocatePool2",
		0x41,	// POOL_FLAG_PAGED,
		0x100, // size
		'T3st' // pool tag
	);

	std::cout << std::hex << poolAddress << std::endl;
}