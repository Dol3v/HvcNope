#include "Globals.h"
#include "Truesight.h"
#include "CallFunction.h"

#include <iostream>

// initialize Globals
Logger g_Logger(Logger::Level::DEBUG);

std::shared_ptr<KernelReadWrite> g_Rw = std::make_shared<TrueSightRw>();
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
	kAddress poolAddress = 
}