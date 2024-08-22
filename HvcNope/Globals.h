#pragma once

#include "KernelBinary.h"
#include "KernelReadWrite.h"

#include <memory>

extern std::shared_ptr<KernelReadWrite> g_Rw;
extern std::shared_ptr<KernelBinary> g_KernelBinary;

