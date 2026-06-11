;; matrasm.nasm - NASM/elf32 port of MATH/MATRASM.ASM (original MASM, 32-bit).
;; Faithful 1:1 translation. Two register-argument routines, called via the
;; GCC inline-asm wrappers in MATRIX.CPP (BOB_LINUX branch).
;;
;;   XASMTransform:   eax=MATRIX_PTR  edx=&x  ebx=&y  ecx=&z   -> eax=ScaleFactor
;;                    (x/y/z transformed in place)
;;   XASMDoBigXProd:  (eax,edx)=v1.dx,dy  (ebx,ecx)=v2.dx,dy   -> al=sign test
;;
;; Linux ELF32 has no leading underscore, so the MASM "_X<name>" public symbol
;; becomes "X<name>" here.

bits 32

section .bss
align 4
Arse:        resd 1      ; dummy - never used (kept for layout parity)
LocalX:      resd 1
LocalY:      resd 1
LocalZ:      resd 1
ScaleFactor: resd 1
BodyXl:      resd 1
BodyXh:      resd 1
BodyYl:      resd 1
BodyYh:      resd 1
BodyZl:      resd 1
BodyZh:      resd 1

section .text

;; ----------------------------------------------------------------------------
global XASMTransform
XASMTransform:
        push    esi
        push    edi
        push    ebp

        push    edx
        push    ebx
        mov     edx,[edx]
        push    ecx
        mov     ebx,[ebx]
        mov     ecx,[ecx]

        mov     esi,eax

        mov     [LocalX],edx
        mov     ebp,ebx
        mov     eax,ecx
        mov     edi,edx
        mov     [LocalY],ebx
        sar     eax,0x1f
        sar     ebp,0x1f
        mov     [LocalZ],ecx
        sar     edi,0x1f
        xor     ecx,eax
        xor     edx,edi
        xor     ebx,ebp
        sub     ecx,eax
        sub     edx,edi
        mov     eax,ecx
        sub     ebx,ebp
        or      eax,edx
        or      eax,ebx
        cmp     eax,0x00007FFF
        jl      .Quick
        bsr     ecx,eax
        sub     ecx,0x0E
        mov     ebx,[LocalY]
        mov     eax,[LocalX]
        sar     ebx,cl
        mov     edx,[LocalZ]
        sar     eax,cl
        mov     [LocalY],ebx
        sar     edx,cl
        mov     [LocalX],eax
        mov     [LocalZ],edx
        mov     [ScaleFactor],ecx
        jmp     .DoneScale
.Quick:
        mov     dword [ScaleFactor],0
.DoneScale:

        ;; Calculate body x
        movsx   eax,word [esi]
        mov     ebx,[LocalX]
        mov     ecx,[LocalY]
        imul    ebx
        add     esi,2
        mov     ebp,eax
        mov     edi,edx
        movsx   eax,word [esi]
        mov     ebx,[LocalZ]
        imul    ecx
        add     esi,2
        add     ebp,eax
        adc     edi,edx
        movsx   eax,word [esi]
        imul    ebx
        add     esi,2
        add     ebp,eax
        adc     edi,edx
        add     ebp,ebp
        adc     edi,edi
        mov     [BodyXl],ebp
        mov     [BodyXh],edi

        movsx   eax,word [esi]
        mov     ebx,[LocalX]
        mov     ecx,[LocalY]
        imul    ebx
        add     esi,2
        mov     ebp,eax
        mov     edi,edx
        movsx   eax,word [esi]
        mov     ebx,[LocalZ]
        imul    ecx
        add     esi,2
        add     ebp,eax
        adc     edi,edx
        movsx   eax,word [esi]
        imul    ebx
        add     esi,2
        add     ebp,eax
        adc     edi,edx
        add     ebp,ebp
        adc     edi,edi
        mov     [BodyYl],ebp
        mov     [BodyYh],edi

        movsx   eax,word [esi]
        mov     ebx,[LocalX]
        mov     ecx,[LocalY]
        imul    ebx
        add     esi,2
        mov     ebp,eax
        mov     edi,edx
        movsx   eax,word [esi]
        mov     ebx,[LocalZ]
        imul    ecx
        add     esi,2
        add     ebp,eax
        adc     edi,edx
        movsx   eax,word [esi]
        imul    ebx
        add     ebp,eax
        adc     edi,edx
        add     ebp,ebp
        adc     edi,edi
        mov     [BodyZl],ebp
        mov     [BodyZh],edi

        ;; Final test for overflows during the additions above
        mov     eax,[BodyXl+2]
        mov     ebx,[BodyYl+2]
        mov     edx,eax
        mov     esi,ebx
        mov     ecx,[BodyZl+2]
        sar     edx,0x1f
        mov     edi,ecx
        sar     esi,0x1f
        sar     edi,0x1f
        xor     eax,edx
        xor     ebx,esi
        xor     ecx,edi
        sub     eax,edx
        sub     ebx,esi
        sub     ecx,edi
        or      eax,ebx
        or      eax,ecx
        cmp     eax,0x00007FFF
        ja      .OFlowFix
.DoneRescale:
        mov     eax,[BodyZl]
        pop     ecx
        mov     [ecx],eax
        pop     ebx
        mov     eax,[BodyYl]
        pop     edx
        mov     [ebx],eax
        mov     eax,[BodyXl]
        mov     [edx],eax
        mov     eax,[ScaleFactor]
        pop     ebp
        pop     edi
        pop     esi
        ret
.OFlowFix:
        bsr     ecx,eax
        sub     ecx,0x0E
        add     [ScaleFactor],ecx
        mov     eax,[BodyXl]
        mov     edx,[BodyXh]
        shrd    eax,edx,cl
        sar     edx,cl
        mov     [BodyXl],eax
        mov     [BodyXh],edx

        mov     eax,[BodyYl]
        mov     edx,[BodyYh]
        shrd    eax,edx,cl
        sar     edx,cl
        mov     [BodyYl],eax
        mov     [BodyYh],edx

        mov     eax,[BodyZl]
        mov     edx,[BodyZh]
        shrd    eax,edx,cl
        sar     edx,cl
        mov     [BodyZl],eax
        mov     [BodyZh],edx
        jmp     .DoneRescale

;; ----------------------------------------------------------------------------
global XASMDoBigXProd
XASMDoBigXProd:
        push    ebp
        mov     ebp,edx
        mov     edx,eax
        sar     edx,0x1F
        imul    ecx
        mov     edi,edx
        mov     esi,eax

        mov     edx,ebp
        mov     eax,ebp
        sar     edx,0x1F
        imul    ebx

        sub     esi,eax
        sbb     edi,edx
        mov     al,0
        js      .Fail
        mov     al,1
.Fail:
        pop     ebp
        ret

section .note.GNU-stack noalloc noexec nowrite progbits
