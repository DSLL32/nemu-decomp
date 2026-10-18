; Adapted matrix kernels from AMD3D, March 1999.
; /*****************************************************************************
;  *
;  * Copyright (c) 1996-1999 ADVANCED MICRO DEVICES, INC. All Rights reserved.
;  *
;  * This software is unpublished and contains the trade secrets and
;  * confidential proprietary information of AMD. Unless otherwise
;  * provided in the Software Agreement associated herewith, it is
;  * licensed in confidence "AS IS" and is not to be reproduced in
;  * whole or part by any means except for backup. Use, duplication,
;  * or disclosure by the Government is subject to the restrictions
;  * in paragraph(b)(3)(B)of the Rights in Technical Data and
;  * Computer Software clause in DFAR 52.227-7013(a)(Oct 1988).
;  * Software owned by Advanced Micro Devices Inc., One AMD Place
;  * P.O. Box 3453, Sunnyvale, CA 94088-3453.
;  *
; The column-major kernel and entry stubs are recovered from the older DLL.

.586
.mmx
.k3d
_TEXT SEGMENT PAGE PUBLIC USE32 'CODE'
PUBLIC __D3DMul_4x4, __glMul_4x4

PUBLIC __D3DMul_3x3
ALIGN 32
__D3DMul_3x3 PROC
    femms
    mov edx, [esp+12]
    mov eax, [esp+4]
    mov ecx, [esp+8]
    movd mm7, [edx+08h]
    movd mm6, [edx+020h]
    punpckldq mm7, [edx+20]
    movq mm0, [ecx]
    movd mm3, [ecx+08h]
    movq mm1, mm0
    pfmul mm0, mm7
    movq mm2, mm1
    punpckldq mm1, mm1
    pfmul mm1, [edx]
    punpckhdq mm2, mm2
    pfmul mm2, [edx+12]
    pfacc mm0, mm0
    movq mm4, mm3
    punpckldq mm3, mm3
    pfmul mm3, [edx+018h]
    pfadd mm2, mm1
    pfmul mm4, mm6
    movq mm5, [ecx+12]
    pfadd mm2, mm3
    movd mm3, [ecx+20]
    pfadd mm4, mm0
    movq mm1, mm5
    movq [eax], mm2
    pfmul mm5, mm7
    movd [eax+08h], mm4
    movq mm2, mm1
    punpckldq mm1, mm1
    movq mm0, [ecx+018h]
    pfmul mm1, [edx]
    punpckhdq mm2, mm2
    pfmul mm2, [edx+12]
    pfacc mm5, mm5
    movq mm4, mm3
    punpckldq mm3, mm3
    pfmul mm3, [edx+018h]
    pfadd mm2, mm1
    pfmul mm4, mm6
    movq mm1, mm0
    pfadd mm2, mm3
    movd mm3, [ecx+020h]
    pfadd mm4, mm5
    pfmul mm0, mm7
    movq [eax+12], mm2
    movq mm2, mm1
    movd [eax+20], mm4
    punpckldq mm1, mm1
    pfmul mm1, [edx]
    punpckhdq mm2, mm2
    pfmul mm2, [edx+12]
    pfacc mm0, mm0
    pfmul mm6, mm3
    punpckldq mm3, mm3
    pfmul mm3, [edx+018h]
    pfadd mm2, mm1
    pfadd mm6, mm0
    pfadd mm2, mm3
    movd [eax+020h], mm6
    movq [eax+018h], mm2
    femms
    ret
__D3DMul_3x3 ENDP

ALIGN 32
__D3DMul_4x4 PROC
    femms
    mov ecx, [esp+8]
    mov eax, [esp+4]
    mov edx, [esp+12]
    movq mm0, [ecx]
    movq mm1, [ecx+08h]
    movq mm4, [edx]
    punpckhdq mm2, mm0
    movq mm5, [edx+010h]
    punpckhdq mm3, mm1
    movq mm6, [edx+020h]
    punpckldq mm0, mm0
    punpckldq mm1, mm1
    pfmul mm4, mm0
    punpckhdq mm2, mm2
    pfmul mm0, [edx+08h]
    movq mm7, [edx+030h]
    pfmul mm5, mm2
    punpckhdq mm3, mm3
    pfmul mm2, [edx+018h]
    pfmul mm6, mm1
    pfadd mm5, mm4
    pfmul mm1, [edx+028h]
    pfadd mm2, mm0
    pfmul mm7, mm3
    pfadd mm6, mm5
    pfmul mm3, [edx+038h]
    pfadd mm2, mm1
    pfadd mm7, mm6
    movq mm0, [ecx+010h]
    pfadd mm3, mm2
    movq mm1, [ecx+018h]
    movq [eax], mm7
    movq mm4, [edx]
    movq [eax+08h], mm3
    punpckhdq mm2, mm0
    movq mm5, [edx+010h]
    punpckhdq mm3, mm1
    movq mm6, [edx+020h]
    punpckldq mm0, mm0
    punpckldq mm1, mm1
    pfmul mm4, mm0
    punpckhdq mm2, mm2
    pfmul mm0, [edx+08h]
    movq mm7, [edx+030h]
    pfmul mm5, mm2
    punpckhdq mm3, mm3
    pfmul mm2, [edx+018h]
    pfmul mm6, mm1
    pfadd mm5, mm4
    pfmul mm1, [edx+028h]
    pfadd mm2, mm0
    pfmul mm7, mm3
    pfadd mm6, mm5
    pfmul mm3, [edx+038h]
    pfadd mm2, mm1
    pfadd mm7, mm6
    movq mm0, [ecx+020h]
    pfadd mm3, mm2
    movq mm1, [ecx+028h]
    movq [eax+010h], mm7
    movq mm4, [edx]
    movq [eax+018h], mm3
    punpckhdq mm2, mm0
    movq mm5, [edx+010h]
    punpckhdq mm3, mm1
    movq mm6, [edx+020h]
    punpckldq mm0, mm0
    punpckldq mm1, mm1
    pfmul mm4, mm0
    punpckhdq mm2, mm2
    pfmul mm0, [edx+08h]
    movq mm7, [edx+030h]
    pfmul mm5, mm2
    punpckhdq mm3, mm3
    pfmul mm2, [edx+018h]
    pfmul mm6, mm1
    pfadd mm5, mm4
    pfmul mm1, [edx+028h]
    pfadd mm2, mm0
    pfmul mm7, mm3
    pfadd mm6, mm5
    pfmul mm3, [edx+038h]
    pfadd mm2, mm1
    pfadd mm7, mm6
    movq mm0, [ecx+030h]
    pfadd mm3, mm2
    movq mm1, [ecx+038h]
    movq [eax+020h], mm7
    movq mm4, [edx]
    movq [eax+028h], mm3
    punpckhdq mm2, mm0
    movq mm5, [edx+010h]
    punpckhdq mm3, mm1
    movq mm6, [edx+020h]
    punpckldq mm0, mm0
    punpckldq mm1, mm1
    pfmul mm4, mm0
    punpckhdq mm2, mm2
    pfmul mm0, [edx+08h]
    movq mm7, [edx+030h]
    pfmul mm5, mm2
    punpckhdq mm3, mm3
    pfmul mm2, [edx+018h]
    pfmul mm6, mm1
    pfadd mm5, mm4
    pfmul mm1, [edx+028h]
    pfadd mm2, mm0
    pfmul mm7, mm3
    pfadd mm6, mm5
    pfmul mm3, [edx+038h]
    pfadd mm2, mm1
    pfadd mm7, mm6
    pfadd mm3, mm2
    movq [eax+030h], mm7
    movq [eax+038h], mm3
    femms
    ret
__D3DMul_4x4 ENDP



; Column-major multiplication, with a temporary for aliased operands.
ALIGN 32
__glMul_4x4 PROC
    femms 
    mov eax, dword ptr [esp + 8]
    mov ecx, dword ptr [esp + 0ch]
    mov edx, dword ptr [esp + 4]
    sub esp, 040h
    movd mm0, dword ptr [eax + 010h]
    movd mm1, dword ptr [eax]
    movd mm6, dword ptr [eax + 4]
    punpckldq mm1, mm0
    movd mm5, dword ptr [eax + 014h]
    pfmul mm1, qword ptr [ecx]
    punpckldq mm6, mm5
    movd mm7, dword ptr [eax + 024h]
    movd mm5, dword ptr [eax + 034h]
    pfmul mm6, qword ptr [ecx]
    movd mm2, dword ptr [eax + 020h]
    punpckldq mm7, mm5
    movd mm0, dword ptr [eax + 030h]
    pfmul mm7, qword ptr [ecx + 8]
    punpckldq mm2, mm0
    pfadd mm6, mm7
    pfmul mm2, qword ptr [ecx + 8]
    pfacc mm6, mm6
    pfadd mm1, mm2
    movd dword ptr [esp + 4], mm6
    pfacc mm1, mm1
    movd dword ptr [esp], mm1
    movd mm0, dword ptr [eax + 018h]
    movd mm1, dword ptr [eax + 8]
    movd mm6, dword ptr [eax + 0ch]
    punpckldq mm1, mm0
    movd mm5, dword ptr [eax + 01ch]
    pfmul mm1, qword ptr [ecx]
    punpckldq mm6, mm5
    movd mm7, dword ptr [eax + 02ch]
    movd mm5, dword ptr [eax + 03ch]
    pfmul mm6, qword ptr [ecx]
    movd mm2, dword ptr [eax + 028h]
    punpckldq mm7, mm5
    movd mm0, dword ptr [eax + 038h]
    pfmul mm7, qword ptr [ecx + 8]
    punpckldq mm2, mm0
    pfadd mm6, mm7
    pfmul mm2, qword ptr [ecx + 8]
    pfacc mm6, mm6
    pfadd mm1, mm2
    movd dword ptr [esp + 0ch], mm6
    pfacc mm1, mm1
    movd dword ptr [esp + 8], mm1
    movd mm0, dword ptr [eax + 010h]
    movd mm1, dword ptr [eax]
    movd mm6, dword ptr [eax + 4]
    punpckldq mm1, mm0
    movd mm5, dword ptr [eax + 014h]
    pfmul mm1, qword ptr [ecx + 010h]
    punpckldq mm6, mm5
    movd mm7, dword ptr [eax + 024h]
    movd mm5, dword ptr [eax + 034h]
    pfmul mm6, qword ptr [ecx + 010h]
    movd mm2, dword ptr [eax + 020h]
    punpckldq mm7, mm5
    movd mm0, dword ptr [eax + 030h]
    pfmul mm7, qword ptr [ecx + 018h]
    punpckldq mm2, mm0
    pfadd mm6, mm7
    pfmul mm2, qword ptr [ecx + 018h]
    pfacc mm6, mm6
    pfadd mm1, mm2
    movd dword ptr [esp + 014h], mm6
    pfacc mm1, mm1
    movd dword ptr [esp + 010h], mm1
    movd mm0, dword ptr [eax + 018h]
    movd mm1, dword ptr [eax + 8]
    movd mm6, dword ptr [eax + 0ch]
    punpckldq mm1, mm0
    movd mm5, dword ptr [eax + 01ch]
    pfmul mm1, qword ptr [ecx + 010h]
    punpckldq mm6, mm5
    movd mm7, dword ptr [eax + 02ch]
    movd mm5, dword ptr [eax + 03ch]
    pfmul mm6, qword ptr [ecx + 010h]
    movd mm2, dword ptr [eax + 028h]
    punpckldq mm7, mm5
    movd mm0, dword ptr [eax + 038h]
    pfmul mm7, qword ptr [ecx + 018h]
    punpckldq mm2, mm0
    pfadd mm6, mm7
    pfmul mm2, qword ptr [ecx + 018h]
    pfacc mm6, mm6
    pfadd mm1, mm2
    movd dword ptr [esp + 01ch], mm6
    pfacc mm1, mm1
    movd dword ptr [esp + 018h], mm1
    movd mm0, dword ptr [eax + 010h]
    movd mm1, dword ptr [eax]
    movd mm6, dword ptr [eax + 4]
    punpckldq mm1, mm0
    movd mm5, dword ptr [eax + 014h]
    pfmul mm1, qword ptr [ecx + 020h]
    punpckldq mm6, mm5
    movd mm7, dword ptr [eax + 024h]
    movd mm5, dword ptr [eax + 034h]
    pfmul mm6, qword ptr [ecx + 020h]
    movd mm2, dword ptr [eax + 020h]
    punpckldq mm7, mm5
    movd mm0, dword ptr [eax + 030h]
    pfmul mm7, qword ptr [ecx + 028h]
    punpckldq mm2, mm0
    pfadd mm6, mm7
    pfmul mm2, qword ptr [ecx + 028h]
    pfacc mm6, mm6
    pfadd mm1, mm2
    movd dword ptr [esp + 024h], mm6
    pfacc mm1, mm1
    movd dword ptr [esp + 020h], mm1
    movd mm0, dword ptr [eax + 018h]
    movd mm1, dword ptr [eax + 8]
    movd mm6, dword ptr [eax + 0ch]
    punpckldq mm1, mm0
    movd mm5, dword ptr [eax + 01ch]
    pfmul mm1, qword ptr [ecx + 020h]
    punpckldq mm6, mm5
    movd mm7, dword ptr [eax + 02ch]
    movd mm5, dword ptr [eax + 03ch]
    pfmul mm6, qword ptr [ecx + 020h]
    movd mm2, dword ptr [eax + 028h]
    punpckldq mm7, mm5
    movd mm0, dword ptr [eax + 038h]
    pfmul mm7, qword ptr [ecx + 028h]
    punpckldq mm2, mm0
    pfadd mm6, mm7
    pfmul mm2, qword ptr [ecx + 028h]
    pfacc mm6, mm6
    pfadd mm1, mm2
    movd dword ptr [esp + 02ch], mm6
    pfacc mm1, mm1
    movd dword ptr [esp + 028h], mm1
    movd mm0, dword ptr [eax + 010h]
    movd mm1, dword ptr [eax]
    movd mm6, dword ptr [eax + 4]
    punpckldq mm1, mm0
    movd mm5, dword ptr [eax + 014h]
    pfmul mm1, qword ptr [ecx + 030h]
    punpckldq mm6, mm5
    movd mm7, dword ptr [eax + 024h]
    movd mm5, dword ptr [eax + 034h]
    pfmul mm6, qword ptr [ecx + 030h]
    movd mm2, dword ptr [eax + 020h]
    punpckldq mm7, mm5
    movd mm0, dword ptr [eax + 030h]
    pfmul mm7, qword ptr [ecx + 038h]
    punpckldq mm2, mm0
    pfadd mm6, mm7
    pfmul mm2, qword ptr [ecx + 038h]
    pfacc mm6, mm6
    pfadd mm1, mm2
    movd dword ptr [esp + 034h], mm6
    pfacc mm1, mm1
    movd dword ptr [esp + 030h], mm1
    movd mm0, dword ptr [eax + 018h]
    movd mm1, dword ptr [eax + 8]
    movd mm6, dword ptr [eax + 0ch]
    punpckldq mm1, mm0
    movd mm5, dword ptr [eax + 01ch]
    pfmul mm1, qword ptr [ecx + 030h]
    punpckldq mm6, mm5
    movd mm7, dword ptr [eax + 02ch]
    movd mm5, dword ptr [eax + 03ch]
    pfmul mm6, qword ptr [ecx + 030h]
    movd mm2, dword ptr [eax + 028h]
    punpckldq mm7, mm5
    movd mm0, dword ptr [eax + 038h]
    pfmul mm7, qword ptr [ecx + 038h]
    punpckldq mm2, mm0
    pfadd mm6, mm7
    pfmul mm2, qword ptr [ecx + 038h]
    pfacc mm6, mm6
    pfadd mm1, mm2
    movd dword ptr [esp + 03ch], mm6
    pfacc mm1, mm1
    movd dword ptr [esp + 038h], mm1
    movq mm3, qword ptr [esp]
    movq mm4, qword ptr [esp + 8]
    movq qword ptr [edx], mm3
    movq qword ptr [edx + 8], mm4
    movq mm3, qword ptr [esp + 010h]
    movq mm4, qword ptr [esp + 018h]
    movq qword ptr [edx + 010h], mm3
    movq qword ptr [edx + 018h], mm4
    movq mm3, qword ptr [esp + 020h]
    movq mm4, qword ptr [esp + 028h]
    movq qword ptr [edx + 020h], mm3
    movq qword ptr [edx + 028h], mm4
    movq mm3, qword ptr [esp + 030h]
    movq mm4, qword ptr [esp + 038h]
    movq qword ptr [edx + 030h], mm3
    movq qword ptr [edx + 038h], mm4
    add esp, 040h
    femms 
    ret
__glMul_4x4 ENDP
_TEXT ENDS
END
