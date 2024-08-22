
#include "Types.h"
#include "Truesight.h"

#include <iostream>


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

	InitializeGraphicalSubsystem();

	g_Rw = std::make_shared<TrueSightRw>();
	g_KernelBinary = std::make_shared<KernelBinary>();
}