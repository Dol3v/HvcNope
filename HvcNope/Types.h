#pragma once

#include <Windows.h>
#include <optional>

typedef UINT64 Qword;
typedef UINT32 Dword;
typedef UINT16 Word;
typedef UINT8  Byte;

// A generic kernel pointer
typedef Qword kAddress;

// we're using it a lot
using std::optional;

using ReadonlyRegion_t = std::span<const Byte>;

constexpr kAddress kNullptr = 0;

#include "Globals.h"
