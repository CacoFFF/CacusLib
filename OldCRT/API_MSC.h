/**
	API_MSC.h
	Author: Fernando Velázquez

	Microsoft Visual Studio 2015 specific code.
	The purpose of this is to eliminate superfluous code,
	linking, globals and ultimately reducing DLL size.
	
	The trick consists on dynamically linking against MSVCRT.LIB
	from Microsoft Visual C++ 6 and patching any missing stuff
	the new compiler wants to add to it.

	This file must be included once.
*/


#pragma comment (lib, "OLDCRT\\MSVCRT.LIB")
#pragma comment (lib, "OLDCRT\\MSVCRT_OLD.LIB")
//Above needed for:
// _vscprintf
// _vscwprintf
#pragma comment (linker, "/NODEFAULTLIB:msvcrt.lib")
#pragma comment (linker, "/merge:.CRT=.rdata")

extern "C"
{

void __declspec(dllimport) *__CxxFrameHandler;
void  __declspec(naked) __CxxFrameHandler3(void)
{
	// Jump indirect: Jumps to __CxxFrameHandler
	_asm jmp __CxxFrameHandler ; Trampoline bounce
}

//Conversion of double to int64 >> hardcoded by compiler
void __declspec(naked) _ftol2_sse()
{
	__asm
	{
		push    ebp
		mov     ebp, esp
		sub     esp, 8
		and     esp, 0FFFFFFF8h
		fstp    [esp]
		cvttsd2si eax, [esp]
		leave
		retn
	}
}



}