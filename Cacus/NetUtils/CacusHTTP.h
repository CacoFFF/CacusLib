/*=============================================================================
	NetUtils/CacusHTTP.h
	Author: Fernando Vel�zquez

	Simple HTTP browser.
=============================================================================*/

#ifndef USES_CACUS_HTTP
#define USES_CACUS_HTTP

#include "../URI.h"

class CACUS_API CacusHTTP
{
public:
	URI RequestURI;
	CScopeMem ResponseHeaderData;
	CScopeMem ResponseData;

public:
	double Timeout;
	int32 ContentLength;

public:
	CacusHTTP();

	bool Browse( const char* BrowseURI); // TODO: Take URI as parameter too

};

#endif