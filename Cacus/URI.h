/*=============================================================================
	URI.h
	Author: Fernando Velázquez

	POD styled URI designed for easy implementation and high compatibility.

	//TODO: Update to RFC 3986
	// https://www.ietf.org/rfc/rfc3986.txt
=============================================================================*/

#ifndef USES_CACUS_URI
#define USES_CACUS_URI

/** RFC 2396 notes - https://www.ietf.org/rfc/rfc2396.txt

3.1 >> Schemes treat upper case letter as lower case
3.1 >> Relative URI's inherit scheme from base URI
3.2 >> Authority (auth@host:port) requires of "//" at start.
Cacus >> Interpreter can handle cases where leading "//" are missing

*/


class CACUS_API URI
{
public:
	friend class PropertyURI;

	URI();
	URI( const char* Address);
	URI( const URI& rhs);
	URI( const URI& BaseURI, const char* Address);
	#ifndef NO_MOVE_CONSTRUCTORS
		URI( URI&& rhs); //Old C++ compilers don't support this
	#endif
	~URI();

	URI& operator=( const URI& rhs);

	bool operator==( const URI& rhs);
	bool operator!=( const URI& rhs);

	const char* operator*() const; //Uses internal String buffer

	const char*     Scheme() const;
	const char*     Authority() const;
	const char*     Auth() const;
	const char*     Hostname() const;
	unsigned short  Port() const;
	const char*     Path() const;
	const char*     Query() const;
	const char*     Fragment() const;

	void setScheme( const char* Val);
	void setAuthority( const char* Val);
	void setAuth( const char* Val);
	void setHostname( const char* Val);
	void setPort( const char* Val);
	void setPath( const char* Val);
	void setQuery( const char* Val);
	void setFragment( const char* Val);


	static unsigned short DefaultPort( const char* Scheme);
private:

	const char*    scheme;
	const char*    auth;
	const char*    hostname;
	unsigned short port;
	const char*    path;
	const char*    query;
	const char*    fragment;

	void Parse( const char* Address);
};

#if USES_CACUS_FIELD
class CACUS_API PropertyURI : public CProperty
{
	DECLARE_FIELD(PropertyURI,CProperty)
	SET_PRIMITIVE(URI)

	PropertyURI( const char* InName, CStruct* InParent, int32 InArrayDim, size_t InOffset, uint32 InPropertyFlags)
		: CProperty( InName, InParent, InArrayDim, sizeof(URI), InOffset, InPropertyFlags | PF_Destructible) {}

	bool Parse( void* Into, const char* From) const;
	bool Booleanize( void* Object) const;
	bool IsText() const;
	void DestroyValue( void* Object) const;
	const char* String( void* Object) const;
};

template<> inline CProperty* CreateProperty<URI>( const char* Name, CStruct* Parent, URI& Prop, uint32 PropertyFlags)
{
	return CP_CREATE(PropertyURI);
}
#endif


inline const char* URI::Scheme() const    { return scheme ? scheme : ""; }
inline const char* URI::Auth() const      { return auth ? auth : ""; }
inline const char* URI::Hostname() const  { return hostname ? hostname : ""; }
inline const char* URI::Path() const      { return path ? path : ""; }
inline const char* URI::Query() const     { return query ? query : ""; }
inline const char* URI::Fragment() const  { return fragment ? fragment : ""; }


#endif
