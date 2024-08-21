#pragma once

#include "KernelBinary.h"
#include "IKernelReadWrite.hpp"

#include <memory>

extern std::shared_ptr<KernelReadWrite> g_Rw;
extern std::shared_ptr<KernelBinary> g_KernelBinary;

