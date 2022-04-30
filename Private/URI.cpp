
#include "CacusLibPrivate.h"

#include "CacusTemplate.h"
#include "TCharBuffer.h"
#include "CacusField.h"
#include "URI.h"
//#include "StackUnwinder.h"
#include "DebugCallback.h"


//Specialized lightweight helpers
static void SetText( const char*& Val, const char* NewVal);
static char* CopyText( const char* Stream, size_t Length);

//*****************************************************
//*********************** URI *************************
//*****************************************************


/** Global parsing notes

For relative cases
- "//" forces authority parsing
- "/", "." force path parsing
- "?" forces query parsing
- "#" forces fragment parsing
*/


//========= ParseScheme - begin ==========//
//
// RFC 3986 Section 3.1 states that:
//   scheme = alpha *( alpha | digit | "+" | "-" | "." )
//
static const char* ParseScheme( const char*& Pos)
{
	const char* Cur = Pos;
	if ( CGetCharType(*Cur) & CHTYPE_Alpha )
	{
		while ( (CGetCharType(*Cur) & CHTYPE_AlNum) || CStrchr("+-.", *Cur) )
			Cur++;
		if ( *Cur == ':' )
		{
			char* Result = (char*)CMalloc( (Cur-Pos) + 1 );
			char* rCur = Result;
			while ( Pos < Cur ) //Copy and normalize
			{
				if ( CGetCharType(*Pos) & CHTYPE_Upper )
					*rCur = *Pos + ('a' - 'A');
				else
					*rCur = *Pos;
				Pos++;
				rCur++;
			}
			*rCur = '\0';
			Pos = Cur+1;
			return Result;
		}
	}
	return nullptr; //Failure
}
//========= ParseScheme - end ==========//


//========= ParseAuthority - begin ==========//
//
//    RFC 3986 Section 3.2 states that:
// The authority component is preceded by a double slash ("//") and is
// terminated by the next slash ("/"), question mark ("?"), or number
// sign ("#") character, or by the end of the URI.
//
static const char* ParseAuthority( const char*& Pos)
{
	if ( CStrncmp( Pos, "//") )
		return nullptr;
	const char* Cur = Pos+2;
	const char* Start = Cur;
	while ( *Cur && !CStrchr("/?#", *Cur) )
		Cur++;
	const char* Result = CopyText( Start, Cur-Start);
	Pos = Cur;
	return Result;
}
//========= ParseAuthority - end ==========//


//========= ParsePath - begin ==========//
//
//    RFC 3986 Section 3.3 states that:
// The path component is terminated by the first question mark ("?")
// or number sign ("#") character, or by the end of the URI.
//
static const char* ParsePath( const char*& Pos, const char* BasePath=nullptr)
{
	const char* Start = Pos;
	const char* End = Start;
	AdvanceTo( End, "?#");
	char* Result = CopyText( Start, End - Start);
	Pos = End;
	return Result;
}
//========= ParsePath - end ==========//


//========= ParseQuery - begin ==========//
//
//    RFC 3986 Section 3.4 states that:
// The query component is indicated by the first question mark ("?")
// character and terminated by a number sign ("#") character or by
// the end of the URI.
//
// Note: a string containing only a null character is a valid return
// This allows the parser to discard the base Query
//
static const char* ParseQuery( const char*& Pos)
{
	if ( *Pos != '?' ) //Not a query string
		return nullptr;
	const char* Start = Pos+1;
	const char* End = Start;
	AdvanceTo( End, '#');
	char* Result = (char*)CMalloc(End-Start);
	memcpy( Result, Start, End-Start);
	Result[End-Start] = '\0';
	Pos = End;
	return Result;
}
//========= ParseQuery - end ==========//


//========= ParseFragment - begin ==========//
//
//    RFC 3986 Section 3.5 states that:
// The fragment identifier component is indicated by the presence of
// a number sign ("#") character and terminated by the end of the URI
//
// Note: a string containing only a null character is a valid return
// This allows the parser to discard the base Fragment identifier
//
static const char* ParseFragment( const char*& Pos)
{
	if ( *Pos != '#' ) //Not a fragment identifier
		return nullptr;
	const char* Start = Pos+1;
	const char* End = Start;
	while ( *End ) End++;
	char* Result = (char*)CMalloc(End-Start);
	memcpy( Result, Start, End-Start);
	Result[End-Start] = '\0';
	Pos = End;
	return Result;
}
//========= ParseFragment - end ==========//



//========= RemoveDotSegments - begin ==========//
//
//    RFC 3986 Section 5.2.4
//
static void RemoveLastSegment( char* Output)
{
	if ( Output )
	{
		size_t OutLen = CStrlen( Output);
		if ( OutLen <= 2 )
			OutLen = 0;
		else
		{
			while ( (OutLen > 0) && Output[OutLen] != '/' )
				OutLen--;
		}
		Output[OutLen] = '\0';
	}
}

static void RemoveDotSegments( const char*& Path)
{
	if ( !Path )
		return;
	char* Input = CopyToBuffer( Path);
	char* Output = nullptr; //Requested on demand

	while ( Input && *Input )
	{
		if      ( !CStrncmp(Input,"./" ) )
			Input += _len("./"); //Remove
		else if ( !CStrncmp(Input,"../") )
			Input += _len("../"); //Remove
		else if ( !CStrcmp (Input,"/." ) )
			Input[1] = '\0'; //Remove dot
		else if ( !CStrncmp(Input,"/./") )
			Input += _len("/."); //Remove one slash and dot
		else if ( !CStrcmp( Input,"/..") )
		{
			Input[1] = '\0'; //Remove dots, leave /
			RemoveLastSegment( Output);
		}
		else if ( !CStrncmp(Input,"/../") )
		{
			Input += _len("/.."); //Remove one slash and two dots
			RemoveLastSegment( Output);
		}
		else if ( !CStrcmp(Input,".") || !CStrcmp(Input,"..") )
			Input[0] = '\0'; //Clear
		else
		{
			if ( !Output ) //Setup Output
			{
				Output = CharBuffer<char>( CStrlen(Input));
				Output[0] = '\0';
			}
			char* Slash = CStrchr( Input+1, '/');
			if ( Slash )
			{
				*Slash = '\0'; //Temporarily split in two
				strcat( Output, Input);
				*Slash = '/';
			}
			else
				strcat( Output, Input);
			Input = Slash;
		}
	}
	CFree( (void*)Path);
	Path = nullptr;
	SetText( Path, Output);
}
//========= RemoveDotSegments - begin ==========//


//========= MergePaths - begin ==========//
//
// Path concatenation
// RefTransform = Base + RefTransform
//
static void MergePaths( const char*& RefTransform, const char* Base)
{
	size_t Len_r = CStrlen( RefTransform);
	size_t Len_b = CStrlen( Base);
	char* Transform = (char*)CMalloc( Len_r + Len_b + 1);
	memcpy( Transform, Base, Len_b);
	memcpy( Transform + Len_b, RefTransform, Len_r);
	Transform[ Len_b + Len_r ] = '\0';
	CFree( (void*)RefTransform);
	RefTransform = Transform;
}
//========= MergePaths - begin ==========//




URI::URI()
{
	memset(this,0,sizeof(URI));
}

URI::URI( const char* InURI)
	: URI()
{
	Parse( InURI);
}

URI::URI( const URI& rhs)
	: URI()
{
	setScheme( rhs.scheme );
	setAuth( rhs.auth );
	setHostname( rhs.hostname );
	port = rhs.port;
	setPath( rhs.path );
	setQuery( rhs.query );
	setFragment( rhs.fragment );
}


URI::URI( const URI& rhs, const char* Address)
	: URI(rhs)
{
	if ( Address )
		Parse( Address);
}

URI::URI( URI&& rhs)
{
	memcpy( this, &rhs, sizeof(URI));
	memset( &rhs, 0, sizeof(URI));
}

URI::~URI()
{
	if ( scheme )    CFree( (void*)scheme);
	if ( auth )      CFree( (void*)auth);
	if ( hostname )  CFree( (void*)hostname);
	if ( path )      CFree( (void*)path);
	if ( query )     CFree( (void*)query);
	if ( fragment )  CFree( (void*)fragment);
	memset( this, 0, sizeof(URI) );
}

URI& URI::operator=(const URI & rhs)
{
	setScheme( rhs.scheme );
	setAuth( rhs.auth );
	setHostname( rhs.hostname );
	port = rhs.port;
	setPath( rhs.path );
	setQuery( rhs.query );
	setFragment( rhs.fragment );
	return *this;
}

bool URI::operator==( const URI& rhs)
{
	return !strcmp( Scheme(), rhs.Scheme())
		&& !strcmp( Auth(), rhs.Auth())
		&& !_stricmp( Hostname(), rhs.Hostname())
		&& (Port() == rhs.Port())
		&& !_stricmp( Path(), rhs.Path())
		&& !_stricmp( Query(), rhs.Query())
		&& !_stricmp( Fragment(), rhs.Fragment());
}

bool URI::operator!=( const URI& rhs)
{
	return !((*this) == rhs);
}

//========= URI::operator* - begin ==========//
//
// Exports URI as plain text contained in the string buffer 
//
const char* URI::operator*() const //TODO: NOT TESTED
{
	size_t Len_Scheme   = scheme   ? CStrlen(scheme)   : 0;
	size_t Len_Auth     = auth     ? CStrlen(auth)     : 0;
	size_t Len_Hostname = hostname ? CStrlen(hostname) : 0;
	size_t Len_Port     = 0;
	for ( uint32 nport=port ; nport>0 ; nport /= 10, Len_Port++);
	size_t Len_Path     = path     ? CStrlen(path)     : 0;
	size_t Len_Query    = query    ? CStrlen(query)    : 0;
	size_t Len_Fragment = fragment ? CStrlen(fragment) : 0;

	size_t Len_Extra = (size_t)(Len_Scheme   != 0)
	                 + (size_t)(Len_Auth + Len_Hostname != 0)
	                 + (size_t)(Len_Auth     != 0)
	                 + (size_t)(Len_Port     != 0)
	                 + (size_t)(Len_Path     != 0)
	                 + (size_t)(Len_Query    != 0)
	                 + (size_t)(Len_Fragment != 0)
	                 + 8; //At least 8 bytes in place of null terminator (just in case)

	char* Result = CharBuffer<char>( (uint32)(Len_Scheme + Len_Auth + Len_Hostname + Len_Port + Len_Path + Len_Query + Len_Fragment + Len_Extra));
	if ( Result )
	{
		Result[0] = 0;
		size_t Pos = 0;
		if ( Len_Scheme   )
		{
			strcpy( Result + Pos                , scheme  );
			strcpy( Result + Pos + Len_Scheme   , ":"     );
			Pos += Len_Scheme + 1;
		}
		if ( Len_Auth || Len_Hostname )
		{
			strcpy( Result + Pos                , "//"  );
			Pos += 2;
		}
		if ( Len_Auth )
		{
			strcpy( Result + Pos                , auth    );
			strcpy( Result + Pos + Len_Auth     , "@"     );
			Pos += Len_Auth + 1;
		}
		if ( Len_Hostname )
		{
			strcpy( Result + Pos                , hostname);
			Pos += Len_Hostname;
		}
		if ( Len_Port )
		{
			sprintf( Result + Pos, ":%i"        , (int)port);
			Pos += Len_Port + 1;
		}
		if ( Len_Path )
		{
			if ( *path != '/' )
				strcpy( Result + Pos++          , "/"     );
			strcpy( Result + Pos                , path    );
			Pos += Len_Path;
		}
		if ( Len_Query )
		{
			strcpy( Result + Pos++              , "?"     );
			strcpy( Result + Pos                , query   );
			Pos += Len_Query;
		}
		if ( Len_Fragment )
		{
			strcpy( Result + Pos++              , "#"     );
			strcpy( Result + Pos                , fragment);
			Pos += Len_Fragment;
		}
		return Result;
	}
	return "";
}
//========= URI::operator* - end ==========//

unsigned short URI::Port() const
{
	return (port != 0) ? port : DefaultPort(Scheme());
}

void URI::setScheme( const char* Val)
{
	SetText( scheme, Val);
	if ( scheme )
		TransformLowerCase( (char*)scheme);
}

void URI::setAuth( const char* Val)
{
	SetText( auth, Val);
}

void URI::setHostname( const char* Val)
{
	SetText( hostname, Val);
}

void URI::setPort( const char* Val)
{
	int NewVal = atoi( Val);
	if ( NewVal & 0xFFFF0000 )
		port = 0;
	else
		port = (unsigned short)NewVal;
}

void URI::setPath( const char* Val)
{
	SetText( path, Val);
}

void URI::setQuery( const char* Val)
{
	SetText( query, Val);
}

void URI::setFragment( const char* Val)
{
	SetText( fragment, Val);
}


//========= URI::setAuthority - begin ==========//
//
// Splits the Authority component into the sub components
// and assigns them to the URI.
//
void URI::setAuthority( const char* Val)
{
	if ( !Val )
		Val = "";
	setAuth(nullptr);
	setHostname(nullptr);
	port = 0;
	char* Separator = CStrchr( Val, '@');
	if ( Separator ) //UserInfo
	{
		auth = CopyText( Val, Separator-Val);
		Val = Separator + 1;
	}
	Separator = CStrchr( Val, ':'); //Port separator
	if ( !Separator )
		hostname = CopyText( Val, CStrlen(Val));
	else
	{
		hostname = CopyText( Val, Separator - Val);
		setPort( Separator + 1);
	}
}
//========= URI::setAuthority - end ==========//





//========= URI::DefaultPort - begin ==========//
//
// Returns a known default port for Scheme
//
unsigned short URI::DefaultPort( const char* Scheme)
{
	struct port_entry
	{
		unsigned short port;
		const char* name;
		port_entry( unsigned short in_port, const char* in_name)
			: port(in_port), name(in_name) {}
	};
	static port_entry entries[] = 
	{
		port_entry(   21, "ftp"),
		port_entry(   22, "ssh"),
		port_entry(   23, "telnet"),
		port_entry(   80, "http"),
		port_entry(  119, "nntp"),
		port_entry(  389, "ldap"),
		port_entry(  443, "https"),
		port_entry(  554, "rtsp"),
		port_entry(  636, "ldaps"),
		port_entry( 5060, "sip"),
		port_entry( 5061, "sips"),
		port_entry( 5222, "xmpp"),
		port_entry( 7777, "unreal"),
		port_entry( 9418, "git"),
	};

	TChar8Buffer<16> s = Scheme;
	if ( s.Len() < 8 ) //Our largest entry is 7 chars long, this is a quick filter
	{
		TransformLowerCase( *s);
		for ( uint32 i=0 ; i<ARRAY_COUNT(entries) ; i++ )
			if ( !strcmp( *s, entries[i].name) )
				return entries[i].port;
	}
	return 0;
}
//========= URI::DefaultPort - end ==========//


//========= URI::Parse - begin ==========//
//
// Imports a plain text URI into this object
//
void URI::Parse( const char* Address)
{
//	guard(URI::Parse)
	// Move the contents of this URI into 'Base'
	// This will temporarily become the 'Reference URI'
	// Until it's time to become the 'Transform URI' (result of parsing)
	// The Base URI will be deleted at end of function, so we can safely move elements to Reference using 'Exchange'
	DebugCallback("== Parsing URI:", CACUS_CALLBACK_URI | CACUS_CALLBACK_DEBUGONLY);
	DebugCallback(Address, CACUS_CALLBACK_URI | CACUS_CALLBACK_DEBUGONLY);
	URI Base;
	memcpy( &Base, this, sizeof(URI) );
	memset( this, 0, sizeof(URI) );

	// Skip spaces
	const char* Pos = Address;
	while ( CGetCharType(*Pos) & CHTYPE_TokSp ) Pos++;

	// The reference URI is blank, safe to assign values without worrying about old data
	const char* authority;

	scheme = ParseScheme(Pos);
	authority = ParseAuthority(Pos);
	path = ParsePath(Pos);
	query = ParseQuery(Pos);
	fragment = ParseFragment(Pos);

	// TODO: https://tools.ietf.org/html/rfc3986#section-5.2.1  Pre-parse the Base URI

	// RFC 3986 section 5.2.2:  Transform References
	// Here 'Reference' (this) becomes 'Transform', make necessary adjustments
	if ( scheme )
	{
		setAuthority( authority);
		RemoveDotSegments( path);
	}
	else
	{
		if ( authority )
		{
			setAuthority( authority);
			RemoveDotSegments( path);
		}
		else
		{
			if ( !path )
			{
				Exchange( path, Base.path);
				if ( !query )
					Exchange( query, Base.query);
			}
			else
			{
				if ( *path == '/' )
					RemoveDotSegments( path);
				else
				{
					MergePaths( path, Base.path);
					RemoveDotSegments( path);
				}
			}
			Exchange( auth, Base.auth);
			Exchange( hostname, Base.hostname);
			port = Base.port;
		}
		Exchange( scheme, Base.scheme);
	}
	if ( authority )
		CFree( (void*)authority);
	//Base and it's remaining elements is destructed
//	unguard;
}
//========= URI::Parse - end ==========//


//*****************************************************
//*************** LIGHTWEIGHT HELPERS *****************
//*****************************************************

static char* CopyText( const char* Stream, size_t Length)
{
	char* Buf = nullptr;
	if ( Length )
	{
		Buf = (char*)CMalloc( Length+1);
		memcpy( Buf, Stream, Length);
		Buf[Length] = '\0';
	}
	return Buf;
}


static void SetText( const char*& Val, const char* NewVal)
{
	if ( Val == NewVal )
		return;
	if ( Val )
		CFree( (void*)Val);
	size_t Length = NewVal ? CStrlen( NewVal ) : 0;
	Val = CopyText( NewVal, Length);
}


//*****************************************************
//************* PROPERTY IMPLEMENTATION ***************
//*****************************************************

#if USES_CACUS_FIELD
bool PropertyURI::Parse( void* Into, const char* From) const
{
	Primitive& Prop = GetProp<Primitive>(Into);
	Prop = URI( From);
	return true;
}

bool PropertyURI::Booleanize( void* Object) const
{
	Primitive& Prop = GetProp<Primitive>(Object);
	return Prop.hostname != nullptr
		|| Prop.path != nullptr
		|| Prop.query != nullptr
		|| Prop.fragment != nullptr;
}

bool PropertyURI::IsText() const
{
	return true;
}

void PropertyURI::DestroyValue( void* Object) const
{
	Primitive& Prop = GetProp<Primitive>(Object);
	Prop.~URI();
}

const char* PropertyURI::String( void* Object) const
{
	return GetProp<Primitive>(Object).operator*();
}
#endif
