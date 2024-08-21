#pragma once

#include <Windows.h>

typedef UINT64 Qword;
typedef UINT32 Dword;
typedef UINT16 Word;
typedef UINT8  Byte;

// A generic kernel pointer
typedef Qword kAddress;

constexpr kAddress kNullptr = 0;

#include "Globals.h"
