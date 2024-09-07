#pragma once

extern "C" {

	//
	// MSR intrinsics
	//

	void Hvcnope_writemsr(unsigned long, unsigned __int64);
	unsigned __int64 Hvcnope_readmsr(unsigned long);

	//
	// In I/O operation intrinsics
	//

	unsigned char Hvcnope__inbyte( unsigned short );
	void Hvcnope__inbytestring( unsigned short, unsigned char*, unsigned long );
	unsigned short Hvcnope__inword( unsigned short );
	void Hvcnope__inwordstring( unsigned short, unsigned short*, unsigned long );
	unsigned long Hvcnope__indword( unsigned short );
	void Hvcnope__indwordstring( unsigned short, unsigned long*, unsigned long );

	//
	// Out I/O operation intrinsics
	//

	void Hvcnope__outbyte( unsigned short, unsigned char );
	void Hvcnope__outbytestring( unsigned short, unsigned char*, unsigned long );
	void Hvcnope__outdword( unsigned short, unsigned long );
	void Hvcnope__outdwordstring( unsigned short, unsigned long*, unsigned long );
	void Hvcnope__outword( unsigned short, unsigned short );
	void Hvcnope__outwordstring( unsigned short, unsigned short*, unsigned long );

	//
	// GS intrinsics
	//

	unsigned char Hvcnope__readgsbyte( unsigned long Offset );
	unsigned short Hvcnope__readgsword( unsigned long Offset );
	unsigned long Hvcnope__readgsdword( unsigned long Offset );
	unsigned long long Hvcnope__readgsqword( unsigned long Offset );

	void Hvcnope__writegsbyte( unsigned long Offset, unsigned char Data );
	void Hvcnope__writegsword( unsigned long Offset, unsigned short Data );
	void Hvcnope__writegsdword( unsigned long Offset, unsigned long Data );
	void Hvcnope__writegsqword( unsigned long Offset, unsigned long long Data );

	void Hvcnope__incgsbyte( unsigned long Offset );
	void Hvcnope__incgsword( unsigned long Offset );
	void Hvcnope__incgsdword( unsigned long Offset );
	void Hvcnope__incgsqword( unsigned long Offset );

	void Hvcnope__addgsbyte( unsigned long Offset, unsigned char Value );
	void Hvcnope__addgsword( unsigned long Offset, unsigned short Value );
	void Hvcnope__addgsdword( unsigned long Offset, unsigned long Value );
	void Hvcnope__addgsqword( unsigned long Offset, unsigned long long Value );
}