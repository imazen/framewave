/*
Copyright (c) 2006-2009 Advanced Micro Devices, Inc. All Rights Reserved.
This software is subject to the Apache v2.0 License.
*/

#include "fwdev.h"
#include "fwSignal.h"

template<class TD>
TD* SigMalloc(int len)
{
	Fw32u size = sizeof(TD) * len;
	TD* memPtr = (TD*)fwMalloc(size);
	
	return memPtr;
}

Fw8u*   STDCALL fwsMalloc_8u(int len) { return SigMalloc<Fw8u>(len); }
Fw16u*  STDCALL fwsMalloc_16u(int len) { return SigMalloc<Fw16u>(len); }
Fw32u*  STDCALL fwsMalloc_32u(int len) { return SigMalloc<Fw32u>(len); }
Fw8s*   STDCALL fwsMalloc_8s(int len) { return SigMalloc<Fw8s>(len); }
Fw16s*  STDCALL fwsMalloc_16s(int len) { return SigMalloc<Fw16s>(len); }
Fw32s*  STDCALL fwsMalloc_32s(int len) { return SigMalloc<Fw32s>(len); }
Fw64s*  STDCALL fwsMalloc_64s(int len) { return SigMalloc<Fw64s>(len); }
Fw32f*  STDCALL fwsMalloc_32f(int len) { return SigMalloc<Fw32f>(len); }
Fw64f*  STDCALL fwsMalloc_64f(int len) { return SigMalloc<Fw64f>(len); }
Fw16sc* STDCALL fwsMalloc_16sc(int len) { return SigMalloc<Fw16sc>(len); }
Fw32sc* STDCALL fwsMalloc_32sc(int len) { return SigMalloc<Fw32sc>(len); }
Fw64sc* STDCALL fwsMalloc_64sc(int len) { return SigMalloc<Fw64sc>(len); }
Fw32fc* STDCALL fwsMalloc_32fc(int len) { return SigMalloc<Fw32fc>(len); }
Fw64fc* STDCALL fwsMalloc_64fc(int len) { return SigMalloc<Fw64fc>(len); }

void STDCALL fwsFree(void* ptr)			 { fwFree(ptr); }

