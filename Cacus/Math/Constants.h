/*=============================================================================
	Constants.h
	Author: Fernando Velázquez

	Defines constants for Cacus Math.

	By including this file, the compiler will store them in your program/lib's.
	You must only include this header once in your project.
=============================================================================*/

CIVector4 CFVector4::MASK_ABSOLUTE = CIVector4( 0x7FFFFFFF, 0x7FFFFFFF, 0x7FFFFFFF, 0x7FFFFFFF);
CIVector4 CFVector4::MASK_SIGNBITS = CIVector4( 0x80000000, 0x80000000, 0x80000000, 0x80000000);
CIVector4 CIVector4::MASK_3D =       CIVector4( 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0x00000000);

