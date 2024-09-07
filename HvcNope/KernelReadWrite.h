
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

	virtual Byte ReadByte( kAddress address ) {
		Qword qword = ReadQword( (kAddress)((uintptr_t)address & ~0x7) );
		size_t byteOffset = (uintptr_t)address & 0x7;
		return (Byte)(qword >> (byteOffset * 8));
	}

	virtual void WriteByte( kAddress address, Byte data ) {
		kAddress alignedAddress = (kAddress)((uintptr_t)address & ~0x7);
		Qword qword = ReadQword( alignedAddress );
		size_t byteOffset = (uintptr_t)address & 0x7;

		qword &= ~(0xFFULL << (byteOffset * 8));
		qword |= ((Qword)data << (byteOffset * 8));

		WriteQword( alignedAddress, qword );
	}

	virtual Word ReadWord( kAddress address ) {
		Qword qword = ReadQword( (kAddress)((uintptr_t)address & ~0x7) );
		size_t byteOffset = (uintptr_t)address & 0x7;
		return (Word)(qword >> (byteOffset * 8));
	}

	virtual void WriteWord( kAddress address, Word data ) {
		kAddress alignedAddress = (kAddress)((uintptr_t)address & ~0x7);
		Qword qword = ReadQword( alignedAddress );
		size_t byteOffset = (uintptr_t)address & 0x7;

		qword &= ~(0xFFFFULL << (byteOffset * 8));
		qword |= ((Qword)data << (byteOffset * 8));

		WriteQword( alignedAddress, qword );
	}

	virtual Dword ReadDword( kAddress address ) {
		Qword qword = ReadQword( (kAddress)((uintptr_t)address & ~0x7) );
		size_t byteOffset = (uintptr_t)address & 0x7;
		return (Dword)(qword >> (byteOffset * 8));
	}

	virtual void WriteDword( kAddress address, Dword data ) {
		kAddress alignedAddress = (kAddress)((uintptr_t)address & ~0x7);
		Qword qword = ReadQword( alignedAddress );
		size_t byteOffset = (uintptr_t)address & 0x7;

		qword &= ~(0xFFFFFFFFULL << (byteOffset * 8));
		qword |= ((Qword)data << (byteOffset * 8));

		WriteQword( alignedAddress, qword );
	}
};