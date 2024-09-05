
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

	virtual std::vector<Byte> ReadBuffer(kAddress Address, ULONG Length) {
		auto alignedLength = AlignUpToQword(Length);

		Qword* buffer = new Qword[alignedLength / sizeof( Qword )];

		// copy data into buffer
		for (int i = 0; i < alignedLength / sizeof(Qword); ++i) {
			buffer[i] = this->ReadQword(Address);
			Address += sizeof( Qword );
		}

		// transfer into vector and trim extra bytes, if exist

		auto* start = reinterpret_cast<Byte*>(buffer);
		auto* end = start + alignedLength;

		std::vector<Byte> result( start, end );

		if (alignedLength != Length) {
			auto addedBytes = alignedLength - Length;
			result.erase( result.end() - addedBytes, result.end());
		}
		return result;
	}

	virtual void WriteBuffer( kAddress DestinationAddress, std::span<Qword> Buffer )
	{
		auto current = DestinationAddress;
		for (Qword qword : Buffer) {
			WriteQword( current, qword );
			current += sizeof( Qword );
		}
	}
};