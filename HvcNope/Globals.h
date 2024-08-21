#pragma once

#include "KOffsetManager.h"
#include "IKernelReadWrite.hpp"

#include <memory>

extern std::shared_ptr<KernelReadWrite> g_Rw;
extern std::shared_ptr<KOffsetManager> g_OffsetManager;
