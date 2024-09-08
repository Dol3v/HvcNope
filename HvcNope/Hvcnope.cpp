
#include "Hvcnope.h"
#include "MockDriver.h"

#pragma comment(lib, "ntdll.lib")

std::shared_ptr<KernelReadWrite> g_Rw = std::make_shared<MockDriverRw>();
std::shared_ptr<KernelBinary> g_KernelBinary = std::make_shared<KernelBinary>();
std::shared_ptr<KInvoker> g_Invoker = std::make_shared<KInvoker>();