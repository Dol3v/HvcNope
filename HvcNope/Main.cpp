
#include "Types.h"
#include <iostream>

int main() {
	HMODULE hNtos = LoadLibraryA("ntoskrnl.exe");
	std::cout << hNtos << std::endl;
	if (nullptr == hNtos) return -1;
	std::cout << GetProcAddress(hNtos, "PsGetProcessId") << std::endl;
}