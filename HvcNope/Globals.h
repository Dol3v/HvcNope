#pragma once

//#include "Log.h"
#include "KernelBinary.h"
#include "KernelReadWrite.h"
#include <memory>

extern std::shared_ptr<KernelReadWrite> g_Rw;
extern std::shared_ptr<KernelBinary> g_KernelBinary;

#include "CallFunction.h"

extern std::shared_ptr<KInvoker> g_Invoker;

