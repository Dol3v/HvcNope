#include "Intrinsics.h"

#include "Types.h"
#include "Globals.h"
#include "Log.h"

kAddress FindKdpSysWriteMsr()
{
	auto KdpSysWriteMsr = Sig::FromHex(
		// xor r8d, r8d
		"45 33 C0" \
		// mov rdx, [rdx]
		"48 8B 12" \
		// mov rax, rdx
		"48 8B C2" \
		// shr rdx, 0x20
		"48 C1 EA 20" \
		// wrmsr
		"0F 30");

	auto maybeKdpSysWriteMsr = g_KernelBinary->FindSignature( KdpSysWriteMsr, KernelBinary::InCode );
	if (!maybeKdpSysWriteMsr) {
		LOG_FAIL( "Failed to find KdpSysWriteMsr by signature" );
		throw std::exception();
	}

	return maybeKdpSysWriteMsr.value();
}

void Hvcnope_writemsr( unsigned long Address, unsigned __int64 Value )
{
	static kAddress KdpSysWriteMsr = kNullptr;
	if (!KdpSysWriteMsr) {
		KdpSysWriteMsr = FindKdpSysWriteMsr();
	}

	unsigned __int64 value = Value;
	NTSTATUS status = g_Invoker->CallKernelFunction(
		KdpSysWriteMsr,
		Address,
		&value
	);

	if (!NT_SUCCESS( status )) {
		LOG_WARN( "MSR Write failed, KdpSysWriteMsr returned status 0x%x", status );
	}
}

kAddress FindKdpSysReadMsr()
{
	auto KdpSysReadMsrSignature = Sig::FromHex(
		// mov [rsp+arg_8], rdx
		"48 89 54 24 10" \
		// mov r9, rdx
		"4C 8B CA" \
		// xor r8d, r8d
		"45 33 C0" \
		// rdmsr
		"0F 32" );

	auto maybeKdpSysReadMsr = g_KernelBinary->FindSignature( KdpSysReadMsrSignature, KernelBinary::InCode );
	if (!maybeKdpSysReadMsr) {
		LOG_FAIL( "Failed to find KdpSysReadMsr by signature" );
		throw std::exception();
	}

	return maybeKdpSysReadMsr.value();
}

unsigned __int64 Hvcnope_readmsr( unsigned long Address)
{
	static kAddress KdpSysReadMsr = kNullptr;
	if (!KdpSysReadMsr) {
		KdpSysReadMsr = FindKdpSysReadMsr();
	}

	unsigned __int64 value = 0;
	NTSTATUS status = g_Invoker->CallKernelFunction(
		KdpSysReadMsr,
		Address,
		&value
	);

	if (!NT_SUCCESS( status )) {
		LOG_WARN( "MSR Read failed, KdpSysReadMsr returned status 0x%x", status );
	}

	return value;
}

//
// Returns (KdpSysReadIoSpace, KdpSysWriteIoSpace)
//
std::tuple<kAddress, kAddress> ResolveKdpSysIoSpaceFunctions()
{
	static kAddress KdpSysReadIoSpace = kNullptr;
	static kAddress KdpSysWriteIoSpace = kNullptr;

	if (KdpSysReadIoSpace && KdpSysWriteIoSpace) {
		return std::make_tuple( KdpSysReadIoSpace, KdpSysWriteIoSpace );
	}

	// Present at the start of both KdpSysReadIoSpace and KdpSysWriteIoSpace
	std::vector<Sig::SigByte> KdpSysIoPrefix = Sig::FromHex(
		// xor r10d, r10d
		"45 33 D2"
		// lea r11d, [r10+1]
		"45 8D 5A 01"
		// cmp ecx, r11d
		"41 3B CB"
	);
	
	//
	// We scan down from the prefix until we find either a read from I/O or a write
	//

	// in ax, dx
	std::vector<Sig::SigByte> ReadFromIo = Sig::FromHex( "66 ED" );
	
	// out ax, dx
	std::vector<Sig::SigByte> WriteToIo = Sig::FromHex( "66 EF" );

	g_KernelBinary->ForEveryCodeSignatureOccurrence(
		Sig::FromVector( KdpSysIoPrefix ),
		[&]( const Byte* Prefix ) -> bool 
		{
			const Dword ScanDistance = 0x100;
			auto scanRegion = ReadonlyRegion_t( Prefix, ScanDistance );

			auto ioRead = Sig::FindSignatureInBuffer( scanRegion, ReadFromIo );
			if (ioRead) {
				KdpSysReadIoSpace = g_KernelBinary->MappedToKernel( Prefix );
				LOG_DEBUG( "Found KdpSysReadIoSpace at 0x%llx", KdpSysReadIoSpace );
				return true;
			}

			auto ioWrite = Sig::FindSignatureInBuffer( scanRegion, WriteToIo );
			if (ioWrite) {
				KdpSysWriteIoSpace = g_KernelBinary->MappedToKernel( Prefix );
				LOG_DEBUG( "Found KdpSysWriteIoSpace at 0x%llx", KdpSysWriteIoSpace );
				return true;
			}

			LOG_WARN( "Found prefix without I/O read or write, mapped address %p", Prefix );
			return true;
		}
	);

	if (!KdpSysReadIoSpace || !KdpSysWriteIoSpace) {
		LOG_FAIL( "Failed to resolve I/O functions" );
		throw std::exception();
	}

	return std::make_tuple( KdpSysReadIoSpace, KdpSysWriteIoSpace );
}

kAddress GetKdpSysReadIoSpace() 
{
	static kAddress KdpSysReadIoSpace = kNullptr;
	if (!KdpSysReadIoSpace) {
		KdpSysReadIoSpace = std::get<0>(ResolveKdpSysIoSpaceFunctions());
	}

	return KdpSysReadIoSpace;
}

template <typename NumericType>
NumericType ReadIoSpace( unsigned short Port )
{
	auto KdpSysReadIoSpace = GetKdpSysReadIoSpace();
	Dword bytesRead = 0;

	NumericType value{ 0 };

	NTSTATUS status = g_Invoker->CallKernelFunction(
		KdpSysReadIoSpace,
		1,
		0,
		1,
		Port,
		&value,
		sizeof( value ),
		&bytesRead
	);

	if (!NT_SUCCESS( status )) {
		LOG_FAIL( "Failed to call KdpSysReadIoSpace, status=0x%x", status );
		throw std::exception();
	}

	return value;
}

unsigned char Hvcnope__inbyte( unsigned short Port )
{
	return ReadIoSpace<unsigned char>( Port );
}

void Hvcnope__inbytestring( unsigned short Port, unsigned char* Buffer, unsigned long Count )
{
	for (auto i = 0; i < Count; ++i) {
		Buffer[i] = Hvcnope__inbyte( Port++ );
	}
}

unsigned short Hvcnope__inword( unsigned short Port )
{
	return ReadIoSpace<unsigned short>( Port );
}

void Hvcnope__inwordstring( unsigned short Port, unsigned short* Buffer, unsigned long Count )
{
	for (auto i = 0; i < Count; ++i) {
		Buffer[i] = Hvcnope__inword( Port );
		Port += sizeof( unsigned short );
	}
}

unsigned long Hvcnope__indword( unsigned short Port )
{
	return ReadIoSpace<unsigned long>( Port );
}

void Hvcnope__indwordstring( unsigned short Port, unsigned long* Buffer, unsigned long Count )
{
	for (auto i = 0; i < Count; ++i) {
		Buffer[i] = Hvcnope__indword( Port );
		Port += sizeof( unsigned long );
	}
}

kAddress GetKdpSysWriteIoSpace()
{
	static kAddress KdpSysWriteIoSpace = kNullptr;
	if (!KdpSysWriteIoSpace) {
		KdpSysWriteIoSpace = std::get<1>( ResolveKdpSysIoSpaceFunctions() );
	}

	return KdpSysWriteIoSpace;
}

template <typename NumericType>
void WriteIoSpace( unsigned short Port, NumericType Value )
{
	auto KdpSysWriteIoSpace = GetKdpSysWriteIoSpace();
	Dword bytesWritten = 0;

	auto value = Value;

	NTSTATUS status = g_Invoker->CallKernelFunction(
		KdpSysWriteIoSpace,
		1,
		0,
		1,
		Port,
		&value,
		sizeof( value ),
		&bytesWritten
	);

	if (!NT_SUCCESS( status )) {
		LOG_FAIL( "Failed to call KdpSysWriteIoSpace, status=0x%x", status );
		throw std::exception();
	}
}

void Hvcnope__outbyte( unsigned short Port, unsigned char Value )
{
	WriteIoSpace<unsigned char>( Port, Value );
}

void Hvcnope__outbytestring( unsigned short Port, unsigned char* Buffer, unsigned long Count )
{
	for (auto i = 0; i < Count; ++i) {
		Hvcnope__outbyte( Port, Buffer[i] );
		Port += sizeof( unsigned char );
	}
}

void Hvcnope__outdword( unsigned short Port, unsigned long Value )
{
	WriteIoSpace<unsigned long>( Port, Value );
}

void Hvcnope__outdwordstring( unsigned short Port, unsigned long* Buffer, unsigned long Count )
{
	for (auto i = 0; i < Count; ++i) {
		Hvcnope__outdword( Port, Buffer[i] );
		Port += sizeof( unsigned long );
	}
}

void Hvcnope__outword( unsigned short Port , unsigned short Value )
{
	WriteIoSpace<unsigned short>( Port, Value );
}

void Hvcnope__outwordstring( unsigned short Port, unsigned short* Buffer, unsigned long Count )
{
	for (auto i = 0; i < Count; ++i) {
		Hvcnope__outword( Port, Buffer[i] );
		Port += sizeof( unsigned short );
	}
}

//
// GS Intrinsics
//

kAddress GetGsBase() 
{
	constexpr unsigned long IA32_KERNEL_GS_BASE = 0xC0000102;
	return kAddress( Hvcnope_readmsr( IA32_KERNEL_GS_BASE ) );
}

unsigned char Hvcnope__readgsbyte( unsigned long Offset ) {
	return g_Rw->ReadByte( GetGsBase() + Offset );
}

unsigned short Hvcnope__readgsword( unsigned long Offset ) {
	return g_Rw->ReadWord( GetGsBase() + Offset );
}

unsigned long Hvcnope__readgsdword( unsigned long Offset ) {
	return g_Rw->ReadDword( GetGsBase() + Offset );
}

unsigned long long Hvcnope__readgsqword( unsigned long Offset ) {
	return g_Rw->ReadQword( GetGsBase() + Offset );
}

void Hvcnope__writegsbyte( unsigned long Offset, unsigned char Data ) {
	g_Rw->WriteByte( GetGsBase() + Offset, Data );
}

void Hvcnope__writegsword( unsigned long Offset, unsigned short Data ) {
	g_Rw->WriteWord( GetGsBase() + Offset, Data );
}

void Hvcnope__writegsdword( unsigned long Offset, unsigned long Data ) {
	g_Rw->WriteDword( GetGsBase() + Offset, Data );
}

void Hvcnope__writegsqword( unsigned long Offset, unsigned long long Data ) {
	g_Rw->WriteQword( GetGsBase() + Offset, Data );
}

void Hvcnope__incgsbyte( unsigned long Offset ) {
	Byte value = Hvcnope__readgsbyte( Offset );
	value++;
	Hvcnope__writegsbyte( Offset, value );
}

void Hvcnope__incgsword( unsigned long Offset ) {
	Word value = Hvcnope__readgsword( Offset );
	value++;
	Hvcnope__writegsword( Offset, value );
}

void Hvcnope__incgsdword( unsigned long Offset ) {
	Dword value = Hvcnope__readgsdword( Offset );
	value++;
	Hvcnope__writegsdword( Offset, value );
}

void Hvcnope__incgsqword( unsigned long Offset ) {
	Qword value = Hvcnope__readgsqword( Offset );
	value++;
	Hvcnope__writegsqword( Offset, value );
}

void Hvcnope__addgsbyte( unsigned long Offset, unsigned char Value ) {
	Byte result = Hvcnope__readgsbyte( Offset ) + Value;
	Hvcnope__writegsbyte( Offset, result );
}

void Hvcnope__addgsword( unsigned long Offset, unsigned short Value ) {
	Word result = Hvcnope__readgsword( Offset ) + Value;
	Hvcnope__writegsword( Offset, result );
}

void Hvcnope__addgsdword( unsigned long Offset, unsigned long Value ) {
	Dword result = Hvcnope__readgsdword( Offset ) + Value;
	Hvcnope__writegsdword( Offset, result );
}

void Hvcnope__addgsqword( unsigned long Offset, unsigned long long Value ) {
	Qword result = Hvcnope__readgsqword( Offset ) + Value;
	Hvcnope__writegsqword( Offset, result );
}
