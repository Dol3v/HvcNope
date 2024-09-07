#pragma once

extern "C" {

	void Hvcnope_writemsr(unsigned long, unsigned __int64);
	unsigned __int64 Hvcnope_readmsr(unsigned long);

	unsigned char Hvcnope__inbyte( unsigned short );
	void Hvcnope__inbytestring( unsigned short, unsigned char*, unsigned long );
	unsigned short Hvcnope__inword( unsigned short );
	void Hvcnope__inwordstring( unsigned short, unsigned short*, unsigned long );
	unsigned long Hvcnope__indword( unsigned short );
	void Hvcnope__indwordstring( unsigned short, unsigned long*, unsigned long );

	void Hvcnope__outbyte( unsigned short, unsigned char );
	void Hvcnope__outbytestring( unsigned short, unsigned char*, unsigned long );
	void Hvcnope__outdword( unsigned short, unsigned long );
	void Hvcnope__outdwordstring( unsigned short, unsigned long*, unsigned long );
	void Hvcnope__outword( unsigned short, unsigned short );
	void Hvcnope__outwordstring( unsigned short, unsigned short*, unsigned long );
}