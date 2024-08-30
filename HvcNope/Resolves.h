#pragma once

#include "Types.h"

namespace Resolves {

	std::optional<kAddress> GetThreadAddressInProcess(Dword ThreadId, Dword ProcessId);

	std::optional<kAddress> GetProcessAddress(Dword ProcessId);
}