
#include "Hvcnope.h"

#pragma comment(lib, "ntdll.lib")

std::shared_ptr<KernelReadWrite> g_Rw;
std::shared_ptr<KernelBinary> g_KernelBinary;
std::shared_ptr<KInvoker> g_Invoker;

void HvcNope_Initialize( std::shared_ptr<KernelReadWrite> Rw )
{
	g_Rw = Rw;
	g_KernelBinary = std::make_shared<KernelBinary>();
	g_Invoker = std::make_shared<KInvoker>();
}
