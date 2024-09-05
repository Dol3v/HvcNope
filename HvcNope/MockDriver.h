#pragma once

#include "Types.h"
#include "KernelReadWrite.h"

class MockDriverRw : public KernelReadWrite 
{
public:
	MockDriverRw();
	~MockDriverRw();

	// Inherited via KernelReadWrite
	Qword ReadQword( kAddress Address ) override;
	void WriteQword( kAddress Address, Qword Value ) override;

private:
	HANDLE m_DeviceHandle;
};