//
//	vcclr.h - helper code for using the managed extensions to C++
//
//		Copyright (C) 2000-2001 Microsoft Corporation
//		All rights reserved.
//

#if _MSC_VER > 1000
#pragma once
#endif

#if !defined(_INC_VCCLR)
#define _INC_VCCLR

#using <mscorlib.dll>
#include <gcroot.h>

//
// get an interior gc pointer to the first character contained in a System::String object
//
inline const System::Char * PtrToStringChars(const System::String *s) {

	// pin to avoid 1-instruction GC hole in reinterpret_cast
	const System::String __pin*pps = s;
	const System::Byte __pin*bp = reinterpret_cast<const System::Byte *>(s);
	if( bp != 0 ) {
		unsigned offset = System::Runtime::CompilerServices::RuntimeHelpers::OffsetToStringData;
    	bp += offset;
	}
    return reinterpret_cast<const System::Char*>(bp);
}

#endif //_INC_VCCLR
