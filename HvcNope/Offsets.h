#pragma once

#include "Types.h"

//
// All hardcoded offsets we use
//

namespace Offsets 
{
	namespace EProcess 
	{
		constexpr Dword ProcessId = 0x440;
		constexpr Dword ActiveProcessLinks = 0x448;
		constexpr Dword ThreadListHead = 0x5e0;

	}

	namespace EThread
	{
		constexpr Dword InitialStack = 0x28;
		constexpr Dword Cid = 0x4c8;
		constexpr Dword ListEntry = 0x538;
	}
}