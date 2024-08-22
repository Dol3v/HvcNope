
#pragma once

#include "Types.h"
#include <vector>

//
// Kernel read/write interface
//
class KernelReadWrite {
public:
	virtual Qword ReadQword(kAddress Address) = 0;

	virtual void WriteQword(kAddress Address, Qword Value) = 0;

#define AlignUpToQword(Size) ((Size & ~(sizeof(Qword))) + sizeof(Qword))

	virtual std::vector<BYTE> ReadBuffer(kAddress Address, ULONG Length) {
		auto alignedLength = AlignUpToQword(Length);
		auto* buffer = new Qword[alignedLength / sizeof(Qword)];

		// copy data into buffer

		for (int i = 0; i < alignedLength / sizeof(Qword); ++i) {
			buffer[i] = this->ReadQword(Address);
		}

		// copy into vector and trim extra entries

		std::vector<BYTE> result((BYTE*)buffer, (BYTE*)(buffer)+alignedLength);
		result.erase(result.begin() + (alignedLength - Length), result.end());
		return result;
	}
};