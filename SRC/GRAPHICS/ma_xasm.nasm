; Mig Alley — native Linux port: real implementations of the XASM_* software-rasterizer
; primitives that GRAFPRIM.CPP calls via `__asm__("call XASM_...")` (Watcom register
; convention: args in eax/edx/ebx/ecx, return in eax). The originals are in the TASM
; GRAFPASM.ASM (6756 lines, MASM syntax — can't assemble with nasm), which BoB left dead.
;
; This file ports, faithfully, the subset needed for a first FLAT-SHADED 3D frame:
;   - palette primitives (8-bit-indexed -> 16-bit-565 LUT)
;   - accessors: SetColour, SetPixelWidth, GetTransparency, Get{Land,Horizon}FadeTable
;   - the span-filler dispatch (HoriLineAddr + horiline_data1/2/4 tables)
;   - the SOLID-colour span fillers PlainHoriLine1/2 (the ones that plot flat-shaded pixels)
; The gouraud/textured fillers referenced by the dispatch tables are NO-OP `ret` stubs for
; now (a textured/gouraud poly simply doesn't paint yet — no crash); they get ported next.
;
; Shared module data (palette_table, colour_data, PixelWidth, fade tables, the dispatch
; tables) is defined ONCE here and referenced directly by the routines in this same TU.
; Build: nasm -f elf32 SRC/GRAPHICS/ma_xasm.nasm -o port/build/obj/ma_xasm.o

; --- colourdata STRUC offsets (GRAFPASM.ASM:42-53, natural-packed) ---
%define cd_imageptr     0
%define cd_alphaptr     4
%define cd_imagexmask   8
%define cd_imageymask   12
%define cd_aliastblptr  16
%define cd_lumtblptr    20
%define cd_col          24
%define cd_imageyshift  25
; --- vertex STRUC: sx (7th DD, 16.16 fixed screen-x) at offset 24 ---
%define vertex_sx       24

;==============================================================================
section .data
align 4
PixelWidth: dw 4          ; current pixel-replication width (1,2,4); inits to 4

; span-filler dispatch tables, indexed by SCANLINETYPE (GRAFPASM.ASM:70-116).
; For the flat-shaded frame only the Plain entries point at real code; the rest are stubs.
align 4
horiline_data1:
    dd  XASM_PlainHoriLine1, XASM_GouraudHoriLine1, XASM_PlainHoriLine1, XASM_ImageHoriLine1
    dd  XASM_MImageHoriLine1, XASM_SImageHoriLine1, XASM_ImageHoriLine1, XASM_ImageHoriLine1
    dd  XASM_ImageHoriLine1, XASM_MImageHoriLine1, XASM_TFImageHoriline1, XASM_AImageHoriline1
    dd  XASM_CImageHoriLine1
horiline_data2:
    dd  XASM_PlainHoriLine1, XASM_GouraudHoriLine2, XASM_PlainHoriLine2, XASM_ImageHoriLine2
    dd  XASM_MImageHoriLine2, XASM_SImageHoriLine2, XASM_ImageHoriLine2, XASM_ImageHoriLine2
    dd  XASM_ImageHoriLine2, XASM_MImageHoriLine2, XASM_TFImageHoriline2, XASM_AImageHoriline2
    dd  XASM_CImageHoriLine2
horiline_data4:
    dd  XASM_PlainHoriLine1, XASM_GouraudHoriLine4, XASM_PlainHoriLine2, XASM_ImageHoriLine4
    dd  XASM_MImageHoriLine4, XASM_SImageHoriLine4, XASM_ImageHoriLine4, XASM_ImageHoriLine4
    dd  XASM_ImageHoriLine4, XASM_MImageHoriLine4, XASM_TFImageHoriline4, XASM_AImageHoriline4
    dd  XASM_CImageHoriLine4
nullscan_data:
    dd  XASM_NullScanLine

;==============================================================================
section .bss
align 4
palette_buffer: resw 256*8        ; 8 palettes of 256 entries (master store)
palette_table:  resw 256          ; active palette / 8->16-bit 565 LUT
colour_data:    resb 28           ; struct colourdata (the active fill descriptor)
Trns3DEnabled:  resw 1            ; transparency-enable flag (0 = off)
LandFadeData:   resb 8196         ; land/terrain fade LUT  (uint8_t[4098*2])
SkyFadeData:    resb 8196         ; sky fade LUT
HorizonFadeData: resb 16*2        ; 16-entry u16 horizon fade LUT

;==============================================================================
section .text

; ---- palette primitives -------------------------------------------------------
; void* XASM_GetPaletteTable(void) -> eax = &palette_buffer
global XASM_GetPaletteTable
XASM_GetPaletteTable:
    lea     eax, [palette_buffer]
    ret

; UWord XASM_GetPaletteEntry(eax=index) -> eax = palette_table[index] (zero-extended)
global XASM_GetPaletteEntry
XASM_GetPaletteEntry:
    and     eax, 0xFF
    movzx   eax, word [palette_table + eax*2]
    ret

; void XASM_SetPaletteEntry(eax=index, edx=value) -> palette_table[index] = (u16)value
global XASM_SetPaletteEntry
XASM_SetPaletteEntry:
    and     eax, 0xFF
    mov     [palette_table + eax*2], dx
    ret

; void XASM_SelectPalette(eax=palNum) -> copy palette_buffer[palNum*256] into palette_table
global XASM_SelectPalette
XASM_SelectPalette:
    push    esi
    push    edi
    push    ecx
    movsx   eax, ax
    shl     eax, 9                ; *512 bytes
    lea     esi, [palette_buffer + eax]
    lea     edi, [palette_table]
    mov     ecx, 128
    cld
    rep     movsd
    pop     ecx
    pop     edi
    pop     esi
    ret

; ---- accessors ----------------------------------------------------------------
; void XASM_SetColour(eax=colour, edx=img.width, ebx=img.height, ecx=img.data)
; colour<=0xFF -> colour_data.col = al (flat-shade). >0xFF -> alphaptr (textured).
global XASM_SetColour
XASM_SetColour:
    push    eax
    push    esi
    push    edi
    push    ecx
    mov     esi, -1
.L1:
    shr     edx, 1
    inc     esi
    or      edx, edx
    jnz     short .L1
    mov     edx, esi
    mov     esi, -1
.L2:
    shr     ebx, 1
    inc     esi
    or      ebx, ebx
    jnz     short .L2
    mov     ebx, esi
    mov     esi, ecx
    cmp     eax, 0xFF
    lea     edi, [colour_data]
    jbe     short .colour
    mov     [edi + cd_alphaptr], eax
    jmp     short .lq
.colour:
    mov     [edi + cd_col], al
.lq:
    mov     eax, 0x00000001
    mov     cl, dl
    shl     eax, cl
    dec     eax
    shl     eax, 16
    mov     [edi + cd_imagexmask], eax
    mov     al, 16
    sub     al, dl
    mov     [edi + cd_imageyshift], al
    mov     eax, 0x00000001
    mov     cl, bl
    shl     eax, cl
    dec     eax
    shl     eax, 16
    mov     [edi + cd_imageymask], eax
    mov     [edi + cd_imageptr], esi
    pop     ecx
    pop     edi
    pop     esi
    pop     eax
    ret

; SWord XASM_SetPixelWidth(eax=new width) -> ax = previous PixelWidth
global XASM_SetPixelWidth
XASM_SetPixelWidth:
    xchg    ax, [PixelWidth]
    ret

; SWord XASM_GetTransparency(void) -> ax = Trns3DEnabled
global XASM_GetTransparency
XASM_GetTransparency:
    mov     ax, [Trns3DEnabled]
    ret

; void* XASM_GetLandFadeTable(void) -> eax = &LandFadeData, edx = &SkyFadeData
global XASM_GetLandFadeTable
XASM_GetLandFadeTable:
    mov     eax, LandFadeData
    mov     edx, SkyFadeData
    ret

; void* XASM_GetHorizonFadeTable(void) -> eax = &HorizonFadeData
global XASM_GetHorizonFadeTable
XASM_GetHorizonFadeTable:
    lea     eax, [HorizonFadeData]
    ret

; ---- span-filler dispatch -----------------------------------------------------
; HoriLineRtnPtr XASM_HoriLineAddr(eax=SCANLINETYPE) -> eax = filler addr (per PixelWidth)
global XASM_HoriLineAddr
XASM_HoriLineAddr:
    push    ebx
    push    esi
    push    edi
    push    ebp
    push    edx
    push    ecx
    lea     ebx, [horiline_data1]
    cmp     word [PixelWidth], 1
    jz      .ok
    lea     ebx, [horiline_data2]
    cmp     word [PixelWidth], 2
    jz      .ok
    lea     ebx, [horiline_data4]
.ok:
    mov     eax, [ebx + 4*eax]
    pop     ecx
    pop     edx
    pop     ebp
    pop     edi
    pop     esi
    pop     ebx
    ret

; ---- solid-colour span fillers (the flat-shade pixel plotters) -----------------
; void XASM_PlainHoriLine1(eax=screen base, edx=&left VERTEX, ebx=&right VERTEX)
global XASM_PlainHoriLine1
XASM_PlainHoriLine1:
    push    ecx
    push    edi
    mov     ecx, [ebx + vertex_sx]
    mov     edi, [edx + vertex_sx]
    sar     ecx, 16
    sar     edi, 16
    sub     ecx, edi
    inc     ecx
    jle     .out
    add     edi, edi
    add     edi, eax
    xor     eax, eax
    mov     al, [colour_data + cd_col]
    add     ax, ax
    mov     ax, word [palette_table + eax]
    rep     stosw
.out:
    pop     edi
    pop     ecx
    ret

; void XASM_PlainHoriLine2(eax=screen base, edx=&left VERTEX, ebx=&right VERTEX)  (width 2)
global XASM_PlainHoriLine2
XASM_PlainHoriLine2:
    push    ecx
    push    edi
    push    edx
    mov     ecx, [ebx + vertex_sx]
    mov     edi, [edx + vertex_sx]
    add     ecx, 0x20000
    sar     edi, 17
    sar     ecx, 17
    add     eax, edi
    sub     ecx, edi
    jle     .out
    add     edi, eax
    xor     eax, eax
    mov     al, [colour_data + cd_col]
    add     ax, ax
    mov     ax, [palette_table + eax]
    mov     dx, ax
    shl     eax, 16
    mov     ax, dx
    rep     stosd
.out:
    pop     edx
    pop     edi
    pop     ecx
    ret

; ---- gouraud/textured span fillers (ported from GRAFPASM.ASM via workflow) ----

; ======== shared data for the gouraud/textured span fillers (defined ONCE) ========
; vertex STRUC offsets (GRAFPASM.ASM:22-40; each real field is a DD padded by a dummy DD)
%define vertex_sy        32
%define vertex_ix        40
%define vertex_iy        48
%define vertex_intensity 56

section .data
align 2
JITTER:  dw 0, 0                 ; GRAFPASM.ASM:166 (4 bytes)
FUP:     db 5,1,7,3,4,0,6,2      ; dither phase table (GRAFPASM.ASM:167)
TOFF:    db 0
TSHIFT:  db 0
align 1
TransparencyData:                ; 8x8 screen-door dither matrix (GRAFPASM.ASM:127-135)
    db 00000000b,00000000b,00000000b,00000000b,00000000b,00000000b,00000000b,00000000b
    db 10000000b,01000000b,00100000b,00010000b,00001000b,00000100b,00000010b,00000001b
    db 10001000b,01000100b,00100010b,00010001b,10001000b,01000100b,00100010b,00010001b
    db 10001001b,01010100b,00101010b,01010001b,10101000b,01010100b,00101010b,00010101b
    db 10011001b,01010101b,01101010b,01010011b,10101010b,01010101b,10101010b,01010101b
    db 11011001b,01011101b,01101110b,01011011b,11101010b,01010111b,10111010b,01110101b
    db 11011101b,11011101b,01101111b,01111011b,11101110b,01110111b,10111011b,01111101b
    db 11011111b,11111101b,01111111b,11111011b,11111110b,11110111b,10111111b,11111101b

section .bss
align 4
IX:      resd 1
DIX:     resd 1
IY:      resd 1
DIY:     resd 1
INTENS:  resd 1
DINTENS: resd 1

section .text
; ---- XASM_GouraudHoriLine1 ----
global XASM_GouraudHoriLine1
XASM_GouraudHoriLine1:
    push    eax
    push    ecx
    push    edx
    push    esi
    push    edi
    push    ebp
    mov     esi, edx
    mov     ecx, [ebx + vertex_sx]       ; right vertex sx (16.16)
    mov     edi, [esi + vertex_sx]       ; left vertex sx (16.16)
    sar     ecx, 16
    sar     edi, 16
    sub     ecx, edi
    inc     ecx
    jle     .out
    add     edi, edi                     ; left_col * 2 (bytes)
    add     edi, eax                     ; start screen address
    mov     ebp, [esi + vertex_sy]
    mov     eax, [esi + vertex_intensity]
    and     ebp, 0xFF
    mov     [INTENS], eax
    lea     ebp, [LandFadeData + ebp]
    sub     eax, [ebx + vertex_intensity]
    movzx   ebp, word [ebp]
    neg     eax
    mov     word [JITTER], bp
    mov     edx, eax
    sar     edx, 0x1F
    idiv    ecx                          ; edx:eax / ecx -> DINTENS step
    dec     edi
    dec     edi
    mov     [DINTENS], eax
.loop:
    xor     eax, eax
    mov     edx, [INTENS]
    movzx   ebp, word [JITTER]
    add     edx, ebp
    inc     edi
    inc     edi
    shr     edx, 16
    add     bp, 0x9136
    cmp     edx, 0x0F
    jbe     short .okay
    mov     edx, 0x0F
.okay:
    ror     bp, 3
    mov     ax, word [HorizonFadeData + 2*edx]
    mov     edx, [DINTENS]
    mov     word [edi], ax
    add     dword [INTENS], edx
    dec     ecx
    mov     word [JITTER], bp
    jnz     short .loop
.out:
    pop     ebp
    pop     edi
    pop     esi
    pop     edx
    pop     ecx
    pop     eax
    ret

; ---- XASM_GouraudHoriLine2 ----
global XASM_GouraudHoriLine2
XASM_GouraudHoriLine2:
    push    eax
    push    ecx
    push    edx
    push    esi
    push    edi
    push    ebp
    mov     esi, edx
    mov     ecx, [ebx + vertex_sx]       ; right vertex sx (16.16)
    mov     edi, [esi + vertex_sx]       ; left vertex sx  (16.16)
    add     ecx, 0x20000
    sar     ecx, 17                      ; right integer column (width-2 rounding)
    sar     edi, 17                      ; left  integer column
    sub     ecx, edi
    inc     ecx                          ; span pixel count
    jle     .out
    add     edi, edi
    add     edi, edi
    add     edi, eax                     ; start screen addr = base + col*4 (4 bytes/texel @ width2)
    mov     ebp, [esi + vertex_sy]
    mov     eax, [esi + vertex_intensity]
    and     ebp, 0xff
    mov     [INTENS], eax                ; INTENS = left intensity
    lea     ebp, [LandFadeData + ebp]
    sub     eax, [ebx + vertex_intensity]
    movzx   ebp, word [ebp]              ; JITTER seed = LandFadeData[sy&0xFF]
    neg     eax                          ; eax = right.intensity - left.intensity
    mov     word [JITTER], bp
    mov     edx, eax
    sar     edx, 0x1f                    ; sign-extend into edx for idiv
    idiv    ecx                          ; DINTENS = (right-left)/width
    sub     edi, 4
    mov     [DINTENS], eax
.loop:
    xor     eax, eax
    mov     edx, dword [INTENS]
    movzx   ebp, word [JITTER]
    add     edx, ebp                     ; dither current intensity
    add     edi, 4
    shr     edx, 16
    add     bp, 0x9136                   ; advance dither accumulator
    cmp     edx, 0xf
    jbe     short .okay
    mov     edx, 0xf                     ; clamp shade index to 0x0F
.okay:
    ror     bp, 3
    mov     ax, word [HorizonFadeData + 2*edx]
    mov     edx, dword [DINTENS]
    mov     word [edi], ax               ; replicate pixel (width 2)
    mov     word [edi+2], ax
    add     dword [INTENS], edx          ; step intensity
    dec     ecx
    mov     word [JITTER], bp
    jnz     short .loop
.out:
    pop     ebp
    pop     edi
    pop     esi
    pop     edx
    pop     ecx
    pop     eax
    ret

; ---- XASM_GouraudHoriLine4 ----
global XASM_GouraudHoriLine4
XASM_GouraudHoriLine4:
    push    eax
    push    ecx
    push    edx
    push    esi
    push    edi
    push    ebp
    mov     esi, edx
    mov     ecx, [ebx + vertex_sx]      ; right vertex sx (16.16)
    mov     edi, [esi + vertex_sx]      ; left vertex sx
    add     ecx, 40000h                 ; x4: round up before sar 18
    sar     ecx, 18
    sar     edi, 18
    sub     ecx, edi                    ; span width (cols)
    inc     ecx                         ; inclusive
    jle     .Out
    add     edi, edi
    add     edi, edi
    add     edi, edi                    ; left_col * 8 (4-wide => 8 bytes/col)
    add     edi, eax                    ; start screen address
    mov     ebp, [esi + vertex_sy]
    mov     eax, [esi + vertex_intensity]
    and     ebp, 0ffh                   ; sy & 0xFF
    mov     [INTENS], eax               ; INTENS = left intensity
    lea     ebp, [LandFadeData + ebp]
    sub     eax, [ebx + vertex_intensity]
    movzx   ebp, word [ebp]             ; LandFadeData[sy&0xFF] -> dither seed
    neg     eax                         ; eax = right.int - left.int
    mov     word [JITTER], bp           ; seed dither accumulator
    mov     edx, eax
    sar     edx, 1fh                    ; sign-extend eax into edx for idiv
    idiv    ecx                         ; DINTENS = (right.int-left.int)/width
    sub     edi, 8                      ; pre-decrement (loop adds 8 first)
    mov     [DINTENS], eax
.Loop:
    xor     eax, eax
    mov     edx, dword [INTENS]
    movzx   ebp, word [JITTER]
    add     edx, ebp                    ; INTENS + dither
    add     edi, 8                      ; advance one col (4 px * 2 bytes)
    shr     edx, 16
    add     bp, 9136h                   ; dither step
    cmp     edx, 0fh
    jbe     short .Okay
    mov     edx, 0fh                    ; clamp shade index to 0x0F
.Okay:
    ror     bp, 3                       ; ordered-dither rotate
    mov     ax, word [HorizonFadeData + 2*edx]
    mov     edx, dword [DINTENS]
    mov     word [edi], ax              ; replicate pixel x4
    mov     word [edi+2], ax
    mov     word [edi+4], ax
    mov     word [edi+6], ax
    add     dword [INTENS], edx         ; step intensity
    dec     ecx
    mov     word [JITTER], bp           ; store dither accumulator
    jnz     short .Loop
.Out:
    pop     ebp
    pop     edi
    pop     esi
    pop     edx
    pop     ecx
    pop     eax
    ret

; ---- XASM_ImageHoriLine1 ----
global XASM_ImageHoriLine1
XASM_ImageHoriLine1:
    push    ecx
    push    edi
    push    ebp
    mov     ecx, [ebx + vertex_sx]      ; right vertex sx (16.16)
    mov     edi, [edx + vertex_sx]      ; left  vertex sx (16.16)
    add     ecx, 0x10000
    sar     edi, 16
    sar     ecx, 16
    mov     esi, edx
    sub     ecx, edi                    ; span width
    jle     .out
    add     edi, edi
    add     edi, eax                    ; start screen address (2 bytes/col)
    mov     eax, [esi + vertex_ix]      ; left  vertex ix (16.16 U)
    mov     edx, [ebx + vertex_ix]      ; right vertex ix
    mov     [IX], eax
    sub     edx, eax
    mov     ebp, [esi + vertex_iy]      ; left  V
    mov     eax, edx
    sar     edx, 31
    idiv    ecx                         ; DIX = (right.ix-left.ix)/width
    mov     [IY], ebp
    mov     [DIX], eax                  ; image x delta
    mov     edx, [ebx + vertex_iy]      ; right vertex iy
    mov     eax, ebp
    sub     edx, ebp
    jz      .horizontal
    cmp     edx, 0xfffeff9a
    jl      .nothorizontal
    cmp     edx, 0x00010066
    jle     .horizontal
.nothorizontal:
    mov     eax, edx
    mov     esi, [colour_data + cd_imageptr]
    sar     edx, 31
    idiv    ecx                         ; DIY = (right.iy-left.iy)/width
    dec     edi
    dec     edi
    mov     [DIY], eax                  ; image y delta
    mov     edx, [IX]

;; edi = screen ptr, esi = image map ptr, ecx = line width, edx=U, ebp=V
.loop:
    push    ecx
    mov     ebx, [colour_data + cd_imagexmask]
    mov     eax, [DIX]
    and     ebx, edx
    add     edx, eax                    ; step U
    mov     ecx, [colour_data + cd_imageymask]
    mov     eax, ebp
    mov     cl, [colour_data + cd_imageyshift]
    shr     ebx, 16                     ; texel column
    and     eax, ecx
    add     ebx, esi
    shr     eax, cl                     ; texel row byte offset
    inc     edi
    inc     edi
    mov     ecx, [DIY]
    add     ebx, eax
    add     ebp, ecx                    ; step V
    pop     ecx
    mov     eax, [ebx]                  ; fetch texel dword (low byte used)
    and     eax, 0xFF
    add     ax, ax
    dec     ecx
    mov     ax, [palette_table + eax]   ; 565 pixel
    mov     word [edi], ax
    jnz     .loop
.out:
    pop     ebp
    pop     edi
    pop     ecx
    ret

.horizontal:
    mov     esi, [colour_data + cd_imageptr]
    or      ecx, [colour_data + cd_imagexmask]
    mov     edx, [IX]
    push    ecx
    and     ebp, [colour_data + cd_imageymask]
    mov     cl, [colour_data + cd_imageyshift]
    shr     ebp, cl
    pop     ecx
    add     esi, ebp                    ; row-fixed base ptr
    dec     edi
    dec     edi
    mov     ebp, [DIX]
.loop2:
    mov     ebx, edx
    add     edx, ebp                    ; step U
    and     ebx, ecx                    ; ecx holds imagexmask | width here
    inc     edi
    inc     edi
    shr     ebx, 16
    mov     bl, byte [esi + ebx]        ; texel
    and     ebx, 0xFF
    add     bx, bx
    dec     cx                          ; decrement low word of width (per original)
    mov     bx, [palette_table + ebx]
    mov     word [edi], bx
    jnz     .loop2
    pop     ebp
    pop     edi
    pop     ecx
    ret

; ---- XASM_ImageHoriLine2 ----
global XASM_ImageHoriLine2
XASM_ImageHoriLine2:
    push    ecx
    push    edi
    push    ebp
    mov     ecx, [ebx + vertex_sx]          ; right vertex sx (16.16)
    mov     edi, [edx + vertex_sx]          ; left vertex sx (16.16)
    add     ecx, 0x20000                    ; width-2 rounding
    sar     edi, 17
    sar     ecx, 17
    mov     esi, edx
    sub     ecx, edi                        ; span width in columns
    jle     .Out
    add     edi, edi
    add     edi, edi                        ; *4 bytes/column (width 2 => 4 bytes)
    add     edi, eax                        ; start screen address
    mov     eax, [esi + vertex_ix]          ; left vertex ix
    mov     edx, [ebx + vertex_ix]          ; right vertex ix
    mov     [IX], eax
    sub     edx, eax
    mov     ebp, [esi + vertex_iy]
    mov     eax, edx
    sar     edx, 31
    idiv    ecx
    mov     [IY], ebp
    mov     [DIX], eax                      ; image x (U) delta
    mov     edx, [ebx + vertex_iy]          ; right vertex iy
    mov     eax, ebp
    sub     edx, ebp
    jz      .Horizontal
    cmp     edx, 0xfffeff9a
    jl      .NotHorizontal
    cmp     edx, 0x00010066
    jle     .Horizontal
.NotHorizontal:
    mov     eax, edx
    mov     esi, [colour_data + cd_imageptr]
    sar     edx, 31
    idiv    ecx
    sub     edi, 4
    mov     [DIY], eax                      ; image y (V) delta
    mov     edx, [IX]

;; edi = screen ptr  esi = image map data ptr  ecx = line width
.Loop:
    push    ecx
    mov     ebx, [colour_data + cd_imagexmask]
    mov     eax, [DIX]
    and     ebx, edx
    add     edx, eax
    mov     ecx, [colour_data + cd_imageymask]
    mov     eax, ebp
    mov     cl, [colour_data + cd_imageyshift]
    shr     ebx, 16
    and     eax, ecx
    add     ebx, esi
    shr     eax, cl
    add     edi, 4
    mov     ecx, [DIY]
    add     ebx, eax
    add     ebp, ecx
    pop     ecx
    mov     eax, [ebx]
    and     eax, 0xFF
    add     ax, ax
    dec     ecx
    mov     ax, [palette_table + eax]
    mov     word [edi], ax
    mov     word [edi+2], ax
    jnz     .Loop
.Out:
    pop     ebp
    pop     edi
    pop     ecx
    ret
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

.Horizontal:
    mov     esi, [colour_data + cd_imageptr]
    or      ecx, [colour_data + cd_imagexmask]
    mov     edx, [IX]
    push    ecx
    and     ebp, [colour_data + cd_imageymask]
    mov     cl, [colour_data + cd_imageyshift]
    shr     ebp, cl
    pop     ecx
    add     esi, ebp
    sub     edi, 4
    mov     ebp, [DIX]
.Loop2:
    mov     ebx, edx
    add     edx, ebp
    and     ebx, ecx
    add     edi, 4
    shr     ebx, 16
    mov     bl, [esi + ebx]
    and     ebx, 0xFF
    add     bx, bx
    dec     cx
    mov     bx, [palette_table + ebx]
    mov     word [edi], bx
    mov     word [edi+2], bx
    jnz     .Loop2
    pop     ebp
    pop     edi
    pop     ecx
    ret

; ---- XASM_ImageHoriLine4 ----
global XASM_ImageHoriLine4
XASM_ImageHoriLine4:
    push    ecx
    push    edi
    push    ebp
    mov     ecx, [ebx + vertex_sx]      ; right vertex
    mov     edi, [edx + vertex_sx]      ; left vertex
    add     ecx, 0x40000
    sar     edi, 18
    sar     ecx, 18
    mov     esi, edx
    sub     ecx, edi
    jle     .out
    add     edi, edi
    add     edi, edi
    add     edi, edi
    add     edi, eax                    ; start scr. adr.
    mov     eax, [esi + vertex_ix]      ; left vertex
    mov     edx, [ebx + vertex_ix]      ; right vertex
    mov     [IX], eax
    sub     edx, eax
    mov     ebp, [esi + vertex_iy]
    mov     eax, edx
    sar     edx, 31
    idiv    ecx
    mov     [IY], ebp
    mov     [DIX], eax                  ; image x delta
    mov     edx, [ebx + vertex_iy]      ; right vertex
    mov     eax, ebp
    sub     edx, ebp
    jz      .horizontal
    cmp     edx, 0xfffeff9a
    jl      .nothorizontal
    cmp     edx, 0x00010066
    jle     .horizontal
.nothorizontal:
    mov     eax, edx
    mov     esi, [colour_data + cd_imageptr]
    sar     edx, 31
    idiv    ecx
    sub     edi, 8
    mov     [DIY], eax                  ; image y delta
    mov     edx, [IX]

; edi = screen ptr / esi = image map data ptr / ecx = line width
.loop:
    push    ecx
    mov     ebx, [colour_data + cd_imagexmask]
    mov     eax, [DIX]
    and     ebx, edx
    add     edx, eax
    mov     ecx, [colour_data + cd_imageymask]
    mov     eax, ebp
    mov     cl, [colour_data + cd_imageyshift]
    shr     ebx, 16
    and     eax, ecx
    add     ebx, esi
    shr     eax, cl
    add     edi, 8
    mov     ecx, [DIY]
    add     ebx, eax
    add     ebp, ecx
    pop     ecx
    mov     eax, [ebx]
    and     eax, 0xFF
    add     ax, ax
    dec     ecx
    mov     ax, [palette_table + eax]
    mov     word [edi], ax
    mov     word [edi + 2], ax
    mov     word [edi + 4], ax
    mov     word [edi + 6], ax
    jnz     .loop
.out:
    pop     ebp
    pop     edi
    pop     ecx
    ret

.horizontal:
    mov     esi, [colour_data + cd_imageptr]
    or      ecx, [colour_data + cd_imagexmask]
    mov     edx, [IX]
    push    ecx
    and     ebp, [colour_data + cd_imageymask]
    mov     cl, [colour_data + cd_imageyshift]
    shr     ebp, cl
    pop     ecx
    add     esi, ebp
    sub     edi, 8
    mov     ebp, [DIX]
.loop2:
    mov     ebx, edx
    add     edx, ebp
    and     ebx, ecx
    add     edi, 8
    shr     ebx, 16
    mov     bl, [esi + ebx]
    and     ebx, 0xFF
    add     bx, bx
    dec     cx
    mov     bx, [palette_table + ebx]
    mov     word [edi], bx
    mov     word [edi + 2], bx
    mov     word [edi + 4], bx
    mov     word [edi + 6], bx
    jnz     .loop2
    pop     ebp
    pop     edi
    pop     ecx
    ret

; ---- XASM_MImageHoriLine1 ----
global XASM_MImageHoriLine1
XASM_MImageHoriLine1:
    push    ecx
    push    edi
    mov     ecx, [ebx + vertex_sx]          ; right vertex sx (16.16)
    mov     esi, edx
    mov     edi, [edx + vertex_sx]          ; left vertex sx (16.16)
    add     ecx, 0x10000
    sar     edi, 16
    sar     ecx, 16
    sub     ecx, edi                        ; span width
    jle     .Out
    push    ebp
    add     edi, edi
    add     edi, eax                        ; start screen address (2 bytes/col)
    mov     eax, [esi + vertex_ix]          ; left vertex ix (16.16 U)
    mov     edx, [ebx + vertex_ix]          ; right vertex ix
    mov     [IX], eax
    sub     edx, eax
    mov     eax, edx
    sar     edx, 31                         ; sign-extend into edx for idiv
    idiv    ecx
    mov     [DIX], eax                      ; image x delta = dU/width
    mov     ebp, [esi + vertex_iy]          ; left vertex iy (V)
    mov     edx, [ebx + vertex_iy]          ; right vertex iy
    mov     eax, ebp
    mov     [IY], ebp
    sub     edx, eax
    jz      .Horizontal
    cmp     edx, 0xfffeff9a                 ; near-zero dV band -> treat as horizontal
    jl      .NotHorizontal
    cmp     edx, 0x00010066
    jle     .Horizontal
.NotHorizontal:
    mov     eax, edx
    sar     edx, 31
    idiv    ecx
    or      ecx, [colour_data + cd_imagexmask]
    mov     esi, [colour_data + cd_imageptr]
    mov     [DIY], eax                      ; image y delta = dV/width
    mov     edx, [IX]
    mov     ebp, [IY]

; edi = screen ptr, esi = image map data ptr, ecx = (width | imagexmask)
    dec     edi
    dec     edi
.Loop:
    push    ecx
    mov     eax, ebp
    mov     ebx, edx
    and     eax, [colour_data + cd_imageymask]
    and     ebx, ecx                        ; wrap U (low word = mask, high = width loop)
    add     edx, [DIX]
    shr     ebx, 16                         ; texel column
    mov     cl, [colour_data + cd_imageyshift]
    add     ebp, [DIY]
    shr     eax, cl                         ; texel row byte offset
    inc     edi
    inc     edi
    add     ebx, eax
    mov     al, [esi + ebx]                 ; fetch texel byte
    cmp     al, 0xFE                        ; transparent colour-key?
    pop     ecx
    jz      .MaskByte                       ; yes -> skip write
    and     eax, 0xff
    add     ax, ax
    mov     ax, [palette_table + eax]       ; 8 -> 16bpp 565
    mov     word [edi], ax
.MaskByte:
    dec     cx
    jnz     .Loop
    pop     ebp
.Out:
    pop     edi
    pop     ecx
    ret
; ---- constant-V fast path (dV ~ 0): precompute row base, step U only ----
.Horizontal:
    mov     esi, [colour_data + cd_imageptr]
    mov     edx, [IX]
    or      ecx, [colour_data + cd_imagexmask]
    push    ecx
    and     ebp, [colour_data + cd_imageymask]
    mov     cl, [colour_data + cd_imageyshift]
    shr     ebp, cl
    pop     ecx
    add     esi, ebp                        ; image ptr += constant row offset
    dec     edi
    dec     edi
    mov     ebp, [DIX]                       ; ebp = U step (free reg now)
.Loop2:
    mov     ebx, edx
    add     edx, ebp
    and     ebx, ecx
    inc     edi
    inc     edi
    shr     ebx, 16
    mov     bl, [esi + ebx]
    cmp     bl, 0xfe
    jz      .mb
    and     ebx, 0xff
    add     bx, bx
    mov     bx, [palette_table + ebx]
    mov     word [edi], bx
.mb:
    dec     cx
    jnz     .Loop2
    pop     ebp
    pop     edi
    pop     ecx
    ret

; ---- XASM_MImageHoriLine2 ----
global XASM_MImageHoriLine2
XASM_MImageHoriLine2:
    push    ecx
    push    edi
    mov     ecx, [ebx + vertex_sx]          ; right vertex sx (16.16)
    mov     esi, edx
    mov     edi, [edx + vertex_sx]          ; left vertex sx
    add     ecx, 0x20000
    sar     edi, 17                          ; PD 08Feb96
    sar     ecx, 17                          ; PD 08Feb96
    sub     ecx, edi
    jle     .Out
    push    ebp
    add     edi, edi
    add     edi, edi
    add     edi, eax                        ; start screen address
    mov     eax, [esi + vertex_ix]          ; left vertex ix
    mov     edx, [ebx + vertex_ix]          ; right vertex ix
    mov     [IX], eax
    sub     edx, eax
    mov     eax, edx
    sar     edx, 31
    idiv    ecx
    mov     [DIX], eax                       ; image x delta
    mov     ebp, [esi + vertex_iy]           ; left vertex iy
    mov     edx, [ebx + vertex_iy]           ; right vertex iy
    mov     eax, ebp
    mov     [IY], ebp
    sub     edx, eax
    jz      .Horizontal
    cmp     edx, 0xfffeff9a
    jl      .NotHorizontal
    cmp     edx, 0x00010066
    jle     .Horizontal
.NotHorizontal:
    mov     eax, edx
    sar     edx, 31
    idiv    ecx
    or      ecx, [colour_data + cd_imagexmask]
    mov     esi, [colour_data + cd_imageptr]
    mov     [DIY], eax                       ; image y delta
    mov     edx, [IX]
    mov     ebp, [IY]

    ; edi = screen ptr, esi = image map data ptr, ecx = line width
    sub     edi, 4
.Loop:
    push    ecx
    mov     eax, ebp
    mov     ebx, edx
    and     eax, [colour_data + cd_imageymask]
    and     ebx, ecx
    add     edx, [DIX]
    shr     ebx, 16
    mov     cl, [colour_data + cd_imageyshift]
    add     ebp, [DIY]
    shr     eax, cl
    add     edi, 4
    add     ebx, eax
    mov     al, [esi + ebx]
    cmp     al, 0xFE
    pop     ecx
    jz      .MaskByte
    and     eax, 0xff
    add     ax, ax
    mov     ax, [palette_table + eax]
    mov     word [edi], ax
    mov     word [edi + 2], ax
.MaskByte:
    dec     cx
    jnz     .Loop
    pop     ebp
.Out:
    pop     edi
    pop     ecx
    ret
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
.Horizontal:
    mov     esi, [colour_data + cd_imageptr]
    mov     edx, [IX]
    or      ecx, [colour_data + cd_imagexmask]
    push    ecx
    and     ebp, [colour_data + cd_imageymask]
    mov     cl, [colour_data + cd_imageyshift]
    shr     ebp, cl
    pop     ecx
    add     esi, ebp
    sub     edi, 4
    mov     ebp, [DIX]
.Loop2:
    mov     ebx, edx
    add     edx, ebp
    and     ebx, ecx
    add     edi, 4
    shr     ebx, 16
    mov     bl, [esi + ebx]
    cmp     bl, 0xfe
    jz      .mb
    and     ebx, 0xff
    add     bx, bx
    mov     bx, [palette_table + ebx]
    mov     word [edi], bx
    mov     word [edi + 2], bx
.mb:
    dec     cx
    jnz     .Loop2
    pop     ebp
    pop     edi
    pop     ecx
    ret

; ---- XASM_MImageHoriLine4 ----
global XASM_MImageHoriLine4
XASM_MImageHoriLine4:
    push    ecx
    push    edi
    mov     ecx, [ebx + vertex_sx]      ; right vertex screen-x (16.16)
    mov     esi, edx
    mov     edi, [edx + vertex_sx]      ; left vertex screen-x (16.16)
    add     ecx, 0x40000
    sar     edi, 18
    sar     ecx, 18
    sub     ecx, edi
    jle     .Out
    push    ebp
    add     edi, edi
    add     edi, edi
    add     edi, edi                    ; left_col * 8 (bytes, x4 @ 2bpp)
    add     edi, eax                    ; start screen address
    mov     eax, [esi + vertex_ix]      ; left U (16.16)
    mov     edx, [ebx + vertex_ix]      ; right U
    mov     [IX], eax
    sub     edx, eax
    mov     eax, edx
    sar     edx, 31
    idiv    ecx
    mov     [DIX], eax                  ; image x delta
    mov     ebp, [esi + vertex_iy]      ; left V
    mov     edx, [ebx + vertex_iy]      ; right V
    mov     eax, ebp
    mov     [IY], ebp
    sub     edx, eax
    jz      .Horizontal
    cmp     edx, 0xfffeff9a
    jl      .NotHorizontal
    cmp     edx, 0x00010066
    jle     .Horizontal
.NotHorizontal:
    mov     eax, edx
    sar     edx, 31
    idiv    ecx
    or      ecx, [colour_data + cd_imagexmask]
    mov     esi, [colour_data + cd_imageptr]
    mov     [DIY], eax                  ; image y delta
    mov     edx, [IX]
    mov     ebp, [IY]

    sub     edi, 8
.Loop:
    push    ecx
    mov     eax, ebp
    mov     ebx, edx
    and     eax, [colour_data + cd_imageymask]
    and     ebx, ecx
    add     edx, [DIX]
    shr     ebx, 16
    mov     cl, [colour_data + cd_imageyshift]
    add     ebp, [DIY]
    shr     eax, cl
    add     edi, 8
    add     ebx, eax
    mov     al, [esi + ebx]
    cmp     al, 0xFE
    pop     ecx
    jz      .MaskByte
    and     eax, 0xff
    add     ax, ax
    mov     ax, [palette_table + eax]
    mov     word [edi], ax
    mov     word [edi + 2], ax
    mov     word [edi + 4], ax
    mov     word [edi + 6], ax
.MaskByte:
    dec     cx
    jnz     .Loop
    pop     ebp
.Out:
    pop     edi
    pop     ecx
    ret
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
.Horizontal:
    mov     esi, [colour_data + cd_imageptr]
    mov     edx, [IX]
    or      ecx, [colour_data + cd_imagexmask]
    push    ecx
    and     ebp, [colour_data + cd_imageymask]
    mov     cl, [colour_data + cd_imageyshift]
    shr     ebp, cl
    pop     ecx
    add     esi, ebp
    sub     edi, 8
    mov     ebp, [DIX]
.Loop2:
    mov     ebx, edx
    add     edx, ebp
    and     ebx, ecx
    add     edi, 8
    shr     ebx, 16
    mov     bl, byte [esi + ebx]
    cmp     bl, 0xfe
    jz      .mb
    and     ebx, 0xff
    add     bx, bx
    mov     bx, [palette_table + ebx]
    mov     word [edi], bx
    mov     word [edi + 2], bx
    mov     word [edi + 4], bx
    mov     word [edi + 6], bx
.mb:
    dec     cx
    jnz     .Loop2
    pop     ebp
    pop     edi
    pop     ecx
    ret

; ---- XASM_SImageHoriLine1 ----
global XASM_SImageHoriLine1
XASM_SImageHoriLine1:
    push    ecx
    push    edi
    push    ebp
    mov     ecx, [ebx + vertex_sx]          ; right vertex sx (16.16)
    sar     ecx, 16
    mov     esi, edx
    mov     edi, [esi + vertex_sx]          ; left vertex sx (16.16)
    sar     edi, 16
    sub     ecx, edi                        ; span width
    inc     ecx
    jle     .Out

    mov     ebp, [esi + vertex_sy]
    add     edi, edi
    add     edi, eax                        ; start screen address
    and     ebp, 0FFh
    mov     eax, [esi + vertex_intensity]
    lea     ebp, [LandFadeData + ebp]
    mov     [INTENS], eax
    mov     bp, word [ebp]                  ; JITTER seed = LandFadeData[sy&0xFF]
    sub     eax, [ebx + vertex_intensity]
    mov     word [JITTER], bp
    mov     edx, eax
    xor     ax, ax
    test    eax, eax
    jz      .Fast
    neg     edx
    mov     eax, edx
    sar     edx, 31
    idiv    ecx
    mov     [DINTENS], eax

    mov     eax, [esi + vertex_ix]          ; left vertex ix
    mov     [IX], eax
    sub     eax, [ebx + vertex_ix]          ; right vertex ix
    neg     eax
    mov     edx, eax
    sar     edx, 31
    idiv    ecx
    mov     [DIX], eax                      ; image x delta

    mov     eax, [esi + vertex_iy]          ; left vertex iy
    mov     [IY], eax
    sub     eax, [ebx + vertex_iy]          ; right vertex iy
    neg     eax
    mov     edx, eax
    sar     edx, 31
    idiv    ecx
    mov     [DIY], eax                      ; image y delta

    mov     esi, [colour_data + cd_imageptr]
    mov     edx, [IX]
    mov     ebp, [IY]

    or      ecx, [colour_data + cd_imagexmask]
    dec     edi
    dec     edi
.Loop:
    push    ecx
    mov     ebx, edx
    mov     eax, ebp
    and     ebx, ecx
    add     ebp, [DIY]
    shr     ebx, 16
    and     eax, [colour_data + cd_imageymask]
    mov     cl, [colour_data + cd_imageyshift]
    inc     edi
    inc     edi
    shr     eax, cl
    add     edx, [DIX]
    add     ebx, eax                        ; texel offset = U_col + V_row
    mov     eax, [INTENS]
    xor     cx, cx
    add     eax, [JITTER]                   ; dword read (JITTER spans two words)
    mov     ch, byte [esi + ebx]            ; texel byte
    shr     eax, 16
    shr     cx, 4                           ; texel hi-nibble -> cx
    cmp     eax, 0Fh
    mov     ebx, [DINTENS]
    jbe     short .OKay
    mov     eax, 0Fh
.OKay:
    or      ax, cx                          ; index = (intensity clamp) | hi-nibble
    add     [INTENS], ebx
    mov     ax, [LandFadeData + eax*2]      ; shaded 16bpp pixel
    mov     bx, [JITTER]
    pop     ecx
    add     bx, 9136h                       ; ordered-dither step
    ror     bx, 3
    mov     word [edi], ax
    dec     cx
    mov     word [JITTER], bx
    jnz     short .Loop
.Out:
    pop     ebp
    pop     edi
    pop     ecx
    ret
.Fast:                                      ; intensity delta == 0: skip DINTENS step
    mov     eax, [esi + vertex_ix]          ; left vertex ix
    mov     [IX], eax
    sub     eax, [ebx + vertex_ix]          ; right vertex ix
    neg     eax
    mov     edx, eax
    sar     edx, 31
    idiv    ecx
    mov     [DIX], eax                      ; image x delta

    mov     eax, [esi + vertex_iy]          ; left vertex iy
    mov     [IY], eax
    sub     eax, [ebx + vertex_iy]          ; right vertex iy
    neg     eax
    mov     edx, eax
    sar     edx, 31
    idiv    ecx
    mov     [DIY], eax                      ; image y delta

    mov     esi, [colour_data + cd_imageptr]
    mov     edx, [IX]
    mov     ebp, [IY]

    or      ecx, [colour_data + cd_imagexmask]
    dec     edi
    dec     edi
.FLoop:
    push    ecx
    mov     ebx, edx
    mov     eax, ebp
    and     ebx, ecx
    add     ebp, [DIY]
    shr     ebx, 16
    and     eax, [colour_data + cd_imageymask]
    mov     cl, [colour_data + cd_imageyshift]
    inc     edi
    inc     edi
    shr     eax, cl
    add     edx, [DIX]
    add     ebx, eax
    mov     eax, [INTENS]
    xor     cx, cx
    add     eax, [JITTER]                   ; dword read
    mov     ch, byte [esi + ebx]
    shr     eax, 16
    shr     cx, 4
    cmp     eax, 0Fh
    mov     bx, [JITTER]
    jbe     short .OKay2
    mov     eax, 0Fh
.OKay2:
    add     bx, 9136h
    or      ax, cx
    ror     bx, 3
    mov     ax, [LandFadeData + 2*eax]
    mov     word [JITTER], bx
    pop     ecx
    mov     word [edi], ax
    dec     cx
    jnz     short .FLoop
    pop     ebp
    pop     edi
    pop     ecx
    ret

; ---- XASM_SImageHoriLine2 ----
global XASM_SImageHoriLine2

;==============================================================================
; XASM_SImageHoriLine2 — shaded-textured span, 2x pixel replication.
;   eax = screen ptr, edx = &left vertex, ebx = &right vertex.  void return.
;   Preserves ecx, edi, ebp (per original push/pop discipline). Forward DF.
;   Shared symbols (palette_table unused here; colour_data, LandFadeData) are
;   defined elsewhere in ma_xasm.nasm. Locals IX/DIX/IY/DIY/INTENS/DINTENS
;   (section .bss, resd 1 each) and JITTER (section .data, dw 0,0) are the
;   shared per-module interpolator scratch — add once to ma_xasm if absent.
;   Requires %defines: vertex_sx=24, vertex_sy=32, vertex_ix=40, vertex_iy=48,
;   vertex_intensity=56, cd_imageptr=0, cd_imagexmask=8, cd_imageymask=12,
;   cd_imageyshift=25.
;==============================================================================
XASM_SImageHoriLine2:
    push    ecx
    push    edi
    push    ebp
    mov     ecx, [ebx + vertex_sx]          ; right vertex
    sar     ecx, 17
    mov     esi, edx
    mov     edi, [esi + vertex_sx]          ; left vertex
    sar     edi, 17
    sub     ecx, edi
    inc     ecx
    jle     .Out

    mov     ebp, [esi + vertex_sy]
    add     edi, edi
    add     edi, edi
    add     edi, eax                        ; start scr. adr.
    and     ebp, 0FFh
    mov     eax, [esi + vertex_intensity]
    lea     ebp, [LandFadeData + ebp]
    mov     [INTENS], eax
    mov     bp, word [ebp]
    sub     eax, [ebx + vertex_intensity]
    mov     [JITTER], bp
    mov     edx, eax
    xor     ax, ax
    test    eax, eax
    jz      .Fast
    neg     edx
    mov     eax, edx
    sar     edx, 31
    idiv    ecx
    mov     [DINTENS], eax

    mov     eax, [esi + vertex_ix]          ; left vertex
    mov     [IX], eax
    sub     eax, [ebx + vertex_ix]          ; right vertex
    neg     eax
    mov     edx, eax
    sar     edx, 31
    idiv    ecx
    mov     [DIX], eax                      ; image x delta

    mov     eax, [esi + vertex_iy]          ; left vertex
    mov     [IY], eax
    sub     eax, [ebx + vertex_iy]          ; right vertex
    neg     eax
    mov     edx, eax
    sar     edx, 31
    idiv    ecx
    mov     [DIY], eax                      ; image y delta

    mov     esi, [colour_data + cd_imageptr]
    mov     edx, [IX]
    mov     ebp, [IY]

    or      ecx, [colour_data + cd_imagexmask]
    sub     edi, 4
.Loop:
    push    ecx
    mov     ebx, edx
    mov     eax, ebp
    and     ebx, ecx
    add     ebp, [DIY]
    shr     ebx, 16
    and     eax, [colour_data + cd_imageymask]
    mov     cl, [colour_data + cd_imageyshift]
    add     edi, 4
    shr     eax, cl
    add     edx, [DIX]
    add     ebx, eax
    mov     eax, [INTENS]
    xor     cx, cx
    add     eax, [JITTER]
    mov     ch, byte [esi + ebx]
    shr     eax, 16
    shr     cx, 4
    cmp     eax, 0Fh
    mov     ebx, [DINTENS]
    jbe     short .OKay
    mov     eax, 0Fh
.OKay:
    or      ax, cx
    add     [INTENS], ebx
    mov     ax, [LandFadeData + eax*2]
    mov     bx, [JITTER]
    pop     ecx
    add     bx, 9136h
    ror     bx, 3
    mov     word [edi], ax
    mov     word [edi+2], ax
    dec     cx
    mov     [JITTER], bx
    jnz     .Loop
.Out:
    pop     ebp
    pop     edi
    pop     ecx
    ret
.Fast:
    mov     eax, [esi + vertex_ix]          ; left vertex
    mov     [IX], eax
    sub     eax, [ebx + vertex_ix]          ; right vertex
    neg     eax
    mov     edx, eax
    sar     edx, 31
    idiv    ecx
    mov     [DIX], eax                      ; image x delta

    mov     eax, [esi + vertex_iy]          ; left vertex
    mov     [IY], eax
    sub     eax, [ebx + vertex_iy]          ; right vertex
    neg     eax
    mov     edx, eax
    sar     edx, 31
    idiv    ecx
    mov     [DIY], eax                      ; image y delta

    mov     esi, [colour_data + cd_imageptr]
    mov     edx, [IX]
    mov     ebp, [IY]

    or      ecx, [colour_data + cd_imagexmask]
    sub     edi, 4
.FLoop:
    push    ecx
    mov     ebx, edx
    mov     eax, ebp
    and     ebx, ecx
    add     ebp, [DIY]
    shr     ebx, 16
    and     eax, [colour_data + cd_imageymask]
    mov     cl, [colour_data + cd_imageyshift]
    add     edi, 4
    shr     eax, cl
    add     edx, [DIX]
    add     ebx, eax
    mov     eax, [INTENS]
    xor     cx, cx
    add     eax, [JITTER]
    mov     ch, byte [esi + ebx]
    shr     eax, 16
    shr     cx, 4
    cmp     eax, 0Fh
    mov     bx, [JITTER]
    jbe     short .OKay2
    mov     eax, 0Fh
.OKay2:
    add     bx, 9136h
    or      ax, cx
    ror     bx, 3
    mov     ax, [LandFadeData + eax*2]
    mov     [JITTER], bx
    pop     ecx
    mov     word [edi], ax
    mov     word [edi+2], ax
    dec     cx
    jnz     short .FLoop
    pop     ebp
    pop     edi
    pop     ecx
    ret

; ---- XASM_SImageHoriLine4 ----
global XASM_SImageHoriLine4
XASM_SImageHoriLine4:
    push    ecx
    push    edi
    push    ebp
    mov     ecx, [ebx + vertex_sx]       ; right vertex
    sar     ecx, 18
    mov     esi, edx
    mov     edi, [esi + vertex_sx]       ; left vertex
    sar     edi, 18
    sub     ecx, edi
    inc     ecx
    jle     .Out

    mov     ebp, [esi + vertex_sy]
    add     edi, edi
    add     edi, edi
    add     edi, edi
    add     edi, eax                     ; start scr. adr.
    and     ebp, 0xFF
    mov     eax, [esi + vertex_intensity]
    lea     ebp, [LandFadeData + ebp]
    mov     [INTENS], eax
    mov     bp, word [ebp]
    sub     eax, [ebx + vertex_intensity]
    mov     [JITTER], bp
    mov     edx, eax
    xor     ax, ax
    test    eax, eax
    jz      .Fast
    neg     edx
    mov     eax, edx
    sar     edx, 31
    idiv    ecx
    mov     [DINTENS], eax

    mov     eax, [esi + vertex_ix]       ; left vertex
    mov     [IX], eax
    sub     eax, [ebx + vertex_ix]       ; right vertex
    neg     eax
    mov     edx, eax
    sar     edx, 31
    idiv    ecx
    mov     [DIX], eax                   ; image x delta

    mov     eax, [esi + vertex_iy]       ; left vertex
    mov     [IY], eax
    sub     eax, [ebx + vertex_iy]       ; right vertex
    neg     eax
    mov     edx, eax
    sar     edx, 31
    idiv    ecx
    mov     [DIY], eax                   ; image y delta

    mov     esi, [colour_data + cd_imageptr]
    mov     edx, [IX]
    mov     ebp, [IY]

    ; edi = screen ptr, esi = image map data ptr, ecx = line width
    or      ecx, [colour_data + cd_imagexmask]
    sub     edi, 8
.Loop:
    push    ecx
    mov     ebx, edx
    mov     eax, ebp
    and     ebx, ecx
    add     ebp, [DIY]
    shr     ebx, 16
    and     eax, [colour_data + cd_imageymask]
    mov     cl, [colour_data + cd_imageyshift]
    add     edi, 8
    shr     eax, cl
    add     edx, [DIX]
    add     ebx, eax
    mov     eax, [INTENS]
    xor     cx, cx
    add     eax, [JITTER]
    mov     ch, byte [esi + ebx]
    shr     eax, 16
    shr     cx, 4
    cmp     eax, 0x0F
    mov     ebx, [DINTENS]
    jbe     short .OKay
    mov     eax, 0x0F
.OKay:
    or      ax, cx
    add     [INTENS], ebx
    mov     ax, [LandFadeData + eax*2]
    mov     bx, [JITTER]
    pop     ecx
    add     bx, 0x9136
    ror     bx, 3
    mov     word [edi], ax
    mov     word [edi+2], ax
    mov     word [edi+4], ax
    mov     word [edi+6], ax
    dec     cx
    mov     [JITTER], bx
    jnz     .Loop
.Out:
    pop     ebp
    pop     edi
    pop     ecx
    ret
.Fast:
    mov     eax, [esi + vertex_ix]       ; left vertex
    mov     [IX], eax
    sub     eax, [ebx + vertex_ix]       ; right vertex
    neg     eax
    mov     edx, eax
    sar     edx, 31
    idiv    ecx
    mov     [DIX], eax                   ; image x delta

    mov     eax, [esi + vertex_iy]       ; left vertex
    mov     [IY], eax
    sub     eax, [ebx + vertex_iy]       ; right vertex
    neg     eax
    mov     edx, eax
    sar     edx, 31
    idiv    ecx
    mov     [DIY], eax                   ; image y delta

    mov     esi, [colour_data + cd_imageptr]
    mov     edx, [IX]
    mov     ebp, [IY]

    or      ecx, [colour_data + cd_imagexmask]
    sub     edi, 8
.FLoop:
    push    ecx
    mov     ebx, edx
    mov     eax, ebp
    and     ebx, ecx
    add     ebp, [DIY]
    shr     ebx, 16
    and     eax, [colour_data + cd_imageymask]
    mov     cl, [colour_data + cd_imageyshift]
    add     edi, 8
    shr     eax, cl
    add     edx, [DIX]
    add     ebx, eax
    mov     eax, [INTENS]
    xor     cx, cx
    add     eax, [JITTER]
    mov     ch, byte [esi + ebx]
    shr     eax, 16
    shr     cx, 4
    cmp     eax, 0x0F
    mov     bx, [JITTER]
    jbe     short .OKay2
    mov     eax, 0x0F
.OKay2:
    add     bx, 0x9136
    or      ax, cx
    ror     bx, 3
    mov     ax, [LandFadeData + eax*2]
    mov     [JITTER], bx
    pop     ecx
    mov     word [edi], ax
    mov     word [edi+2], ax
    mov     word [edi+4], ax
    mov     word [edi+6], ax
    dec     cx
    jnz     .FLoop
    pop     ebp
    pop     edi
    pop     ecx
    ret

; ---- XASM_TFImageHoriline1 ----
global XASM_TFImageHoriline1
XASM_TFImageHoriline1:
    push    ecx
    push    edi
    push    ebp
    mov     ecx, [ebx + vertex_sx]      ; right vertex sx (16.16)
    sar     ecx, 10h
    mov     esi, edx
    mov     edi, [esi + vertex_sx]      ; left vertex sx
    sar     edi, 16
    push    eax
    mov     ax, di
    and     eax, 07h
    mov     al, [FUP + eax]             ; sx&7 -> initial TSHIFT phase
    mov     [TSHIFT], al
    pop     eax
    sub     ecx, edi
    inc     ecx                         ; span width (cols)
    jle     .Out
    mov     edx, [esi + vertex_sy]
    and     dl, 07h
    mov     [TOFF], dl                  ; sy&7 -> dither row offset
    add     edi, edi
    add     edi, eax                    ; start screen address (word ptr)
    mov     eax, [esi + vertex_intensity]
    mov     [INTENS], eax
    sub     eax, [ebx + vertex_intensity]
    mov     edx, eax
    neg     edx
    mov     eax, edx
    sar     edx, 31
    idiv    ecx
    mov     [DINTENS], eax              ; intensity delta / width
    mov     eax, [esi + vertex_ix]      ; left vertex U
    mov     [IX], eax
    sub     eax, [ebx + vertex_ix]      ; right vertex U
    neg     eax
    mov     edx, eax
    sar     edx, 31
    idiv    ecx
    mov     [DIX], eax                  ; image x delta
    mov     eax, [esi + vertex_iy]      ; left vertex V
    mov     [IY], eax
    sub     eax, [ebx + vertex_iy]      ; right vertex V
    neg     eax
    mov     edx, eax
    sar     edx, 31
    idiv    ecx
    mov     [DIY], eax                  ; image y delta
    mov     esi, [colour_data + cd_imageptr]
    mov     edx, [IX]                   ; running U (16.16)
    mov     ebp, [IY]                   ; running V
    ; edi = screen ptr, esi = image map ptr, ecx = line width
    or      ecx, [colour_data + cd_imagexmask]   ; pack xmask into hi bits, cx=count
    dec     edi
    dec     edi
.Loop:
    push    ecx
    mov     ebx, edx
    mov     eax, ebp
    and     ebx, ecx                    ; U & xmask
    add     ebp, [DIY]
    shr     ebx, 16
    and     eax, [colour_data + cd_imageymask]   ; V & ymask
    mov     cl, [colour_data + cd_imageyshift]
    inc     edi
    inc     edi
    shr     eax, cl                     ; -> row byte offset
    add     edx, [DIX]
    add     ebx, eax                    ; texel index
    mov     eax, [INTENS]
    xor     cx, cx
    mov     ch, [esi + ebx]             ; ch = texel colour index
    shr     eax, 14
    and     eax, 078h
    cmp     eax, 038h
    mov     ebx, [DINTENS]
    jbe     short .OKay
    mov     eax, 038h                   ; clamp intensity level
.OKay:
    ; ch = colour index, eax = intensity level (0x00..0x38)
    add     [INTENS], ebx
    mov     cl, [TSHIFT]
    inc     cl
    test    cl, 08h
    jz      short .noreload
    mov     cl, [TOFF]
    inc     cl
    and     cl, 07h
    mov     [TOFF], cl
    xor     cl, cl
.noreload:
    mov     [TSHIFT], cl
    or      al, [TOFF]                  ; level | current transp offset
    mov     al, [TransparencyData + eax]
    shr     al, cl                      ; select dither bit by TSHIFT phase
    test    al, 01h
    jz      short .nowrite
    ; ch = colour index -> 565 pixel via palette_table (ecx = index*2)
    and     ecx, 0000FF00h
    shr     ecx, 7
    mov     ax, [palette_table + ecx]
    mov     [edi], ax
.nowrite:
    pop     ecx
    dec     cx
    jnz     .Loop
.Out:
    pop     ebp
    pop     edi
    pop     ecx
    ret

; ---- XASM_TFImageHoriline2 ----
global XASM_TFImageHoriline2
XASM_TFImageHoriline2:
    push    ecx
    push    edi
    push    ebp
    mov     ecx, [ebx + vertex_sx]      ; right vertex screen-x (16.16 fixed)
    sar     ecx, 0x11                   ; 2x: >>17 -> integer column
    mov     esi, edx
    mov     edi, [esi + vertex_sx]      ; left vertex screen-x
    sar     edi, 17
    push    eax
    mov     ax, di
    and     eax, 0x07
    mov     al, [FUP + eax]
    mov     [TSHIFT], al
    pop     eax
    sub     ecx, edi
    inc     ecx                         ; span width (+1)
    jle     .Out
    mov     edx, [esi + vertex_sy]
    and     dl, 0x07
    mov     [TOFF], dl
    add     edi, edi
    add     edi, edi
    add     edi, eax                    ; start screen address (4 bytes/texel)
    mov     eax, [esi + vertex_intensity]
    mov     [INTENS], eax
    sub     eax, [ebx + vertex_intensity]
    mov     edx, eax
    neg     edx
    mov     eax, edx
    sar     edx, 31
    idiv    ecx
    mov     [DINTENS], eax
    mov     eax, [esi + vertex_ix]      ; left vertex U
    mov     [IX], eax
    sub     eax, [ebx + vertex_ix]      ; right vertex U
    neg     eax
    mov     edx, eax
    sar     edx, 31
    idiv    ecx
    mov     [DIX], eax                  ; image x delta (16.16)
    mov     eax, [esi + vertex_iy]      ; left vertex V
    mov     [IY], eax
    sub     eax, [ebx + vertex_iy]      ; right vertex V
    neg     eax
    mov     edx, eax
    sar     edx, 31
    idiv    ecx
    mov     [DIY], eax                  ; image y delta
    mov     esi, [colour_data + cd_imageptr]
    mov     edx, [IX]
    mov     ebp, [IY]
    ; edi=screen ptr, esi=image map ptr, ecx=line width
    or      ecx, [colour_data + cd_imagexmask]
    sub     edi, 4
.Loop:
    push    ecx
    mov     ebx, edx
    mov     eax, ebp
    and     ebx, ecx
    add     ebp, [DIY]
    shr     ebx, 16
    and     eax, [colour_data + cd_imageymask]
    mov     cl, [colour_data + cd_imageyshift]
    add     edi, 4
    shr     eax, cl
    add     edx, [DIX]
    add     ebx, eax
    mov     eax, [INTENS]
    xor     cx, cx
    mov     ch, byte [esi + ebx]        ; ch = texel colour index
    shr     eax, 14
    and     eax, 0x78
    cmp     eax, 0x38
    mov     ebx, [DINTENS]
    jbe     short .OKay
    mov     eax, 0x38
.OKay:
    ; ch = colour index, eax = intensity level (0x00..0x38)
    add     [INTENS], ebx
    mov     cl, [TSHIFT]
    inc     cl
    test    cl, 0x08
    jz      short .noreload
    mov     cl, [TOFF]
    inc     cl
    and     cl, 0x07
    mov     [TOFF], cl
    xor     cl, cl
.noreload:
    mov     [TSHIFT], cl
    or      al, [TOFF]                  ; current transp offset (level | TOFF)
    mov     al, [TransparencyData + eax]
    shr     al, cl                      ; bit-test via TSHIFT
    test    al, 0x01
    jz      short .nowrite
    ; ch = colour index -> 565 pixel, written x2 (2x replication)
    and     ecx, 0x0000FF00
    shr     ecx, 7
    mov     ax, [palette_table + ecx]
    mov     word [edi], ax
    mov     word [edi + 2], ax
.nowrite:
    pop     ecx
    dec     cx
    jnz     .Loop
.Out:
    pop     ebp
    pop     edi
    pop     ecx
    ret

; ---- XASM_TFImageHoriline4 ----
global XASM_TFImageHoriline4
XASM_TFImageHoriline4:
    push    ecx
    push    edi
    push    ebp
    mov     ecx, [ebx + vertex_sx]          ; right vertex sx
    sar     ecx, 18                          ; 12h: 16.16 -> column (x4)
    mov     esi, edx
    mov     edi, [esi + vertex_sx]          ; left vertex sx
    sar     edi, 18
    push    eax
    mov     ax, di
    and     eax, 7
    mov     al, [FUP + eax]                  ; TSHIFT seed = FUP[left_col & 7]
    mov     [TSHIFT], al
    pop     eax
    sub     ecx, edi                         ; span width-1
    inc     ecx
    jle     .Out
    mov     edx, [esi + vertex_sy]
    and     dl, 7
    mov     [TOFF], dl                       ; TOFF = sy & 7
    add     edi, edi
    add     edi, edi
    add     edi, edi                         ; left_col * 8 (8 bytes/texel)
    add     edi, eax                         ; + screen base = start scr. adr.
    mov     eax, [esi + vertex_intensity]
    mov     [INTENS], eax
    sub     eax, [ebx + vertex_intensity]
    mov     edx, eax
    neg     edx
    mov     eax, edx
    sar     edx, 31
    idiv    ecx
    mov     [DINTENS], eax                   ; intensity delta / width
    mov     eax, [esi + vertex_ix]          ; left vertex
    mov     [IX], eax
    sub     eax, [ebx + vertex_ix]          ; right vertex
    neg     eax
    mov     edx, eax
    sar     edx, 31
    idiv    ecx
    mov     [DIX], eax                       ; image x delta
    mov     eax, [esi + vertex_iy]          ; left vertex
    mov     [IY], eax
    sub     eax, [ebx + vertex_iy]          ; right vertex
    neg     eax
    mov     edx, eax
    sar     edx, 31
    idiv    ecx
    mov     [DIY], eax                       ; image y delta
    mov     esi, [colour_data + cd_imageptr]
    mov     edx, [IX]
    mov     ebp, [IY]
    ; edi = screen ptr, esi = image map data ptr, ecx = line width
    or      ecx, [colour_data + cd_imagexmask]
    sub     edi, 8
.Loop:
    push    ecx
    mov     ebx, edx
    mov     eax, ebp
    and     ebx, ecx                         ; U & (imagexmask | width-in-loword)
    add     ebp, [DIY]
    shr     ebx, 16                          ; U texel column
    and     eax, [colour_data + cd_imageymask]
    mov     cl, [colour_data + cd_imageyshift]
    add     edi, 8
    shr     eax, cl                          ; V row byte offset
    add     edx, [DIX]
    add     ebx, eax                         ; texel index = U + V
    mov     eax, [INTENS]
    xor     cx, cx
    mov     ch, byte [esi + ebx]             ; ch = colour index (texel)
    shr     eax, 14
    and     eax, 0x78
    cmp     eax, 0x38
    mov     ebx, [DINTENS]
    jbe     short .OKay
    mov     eax, 0x38                         ; clamp intensity level
.OKay:
    ; ch = colour index, eax = intensity level (0x00..0x38)
    add     [INTENS], ebx                     ; step intensity
    mov     cl, [TSHIFT]
    inc     cl
    test    cl, 0x08
    jz      short .noreload
    mov     cl, [TOFF]                        ; TSHIFT rolled over -> bump TOFF
    inc     cl
    and     cl, 7
    mov     [TOFF], cl
    xor     cl, cl
.noreload:
    mov     [TSHIFT], cl
    or      al, [TOFF]                        ; current transp offset
    mov     al, [TransparencyData + eax]
    shr     al, cl
    test    al, 1
    jz      short .nowrite
    ; ch = colour index
    and     ecx, 0x0000FF00                   ; isolate texel (ch)
    shr     ecx, 7                            ; texel * 2 = palette byte offset
    mov     ax, [palette_table + ecx]
    mov     [edi], ax
    mov     [edi + 2], ax
    mov     [edi + 4], ax
    mov     [edi + 6], ax
.nowrite:
    pop     ecx
    dec     cx
    jnz     .Loop
.Out:
    pop     ebp
    pop     edi
    pop     ecx
    ret

; ---- XASM_AImageHoriline1 ----
global XASM_AImageHoriline1
XASM_AImageHoriline1:
    push    ecx
    push    edi
    push    ebp
    mov     ecx, [ebx + vertex_sx]      ; right vertex sx (16.16)
    mov     edi, [edx + vertex_sx]      ; left vertex sx
    add     ecx, 10000h
    sar     edi, 16
    push    eax
    mov     ax, di
    and     eax, 07h
    mov     al, [FUP + eax]
    mov     [TSHIFT], al
    pop     eax
    sar     ecx, 16
    mov     esi, edx
    sub     ecx, edi                    ; span width
    jle     .Out
    add     edi, edi
    add     edi, eax                    ; start screen address
    mov     eax, [esi + vertex_ix]      ; left ix
    mov     edx, [ebx + vertex_ix]      ; right ix
    mov     [IX], eax
    sub     edx, eax
    mov     ebp, [esi + vertex_iy]
    mov     eax, [esi + vertex_sy]
    and     al, 07h
    mov     [TOFF], al
    mov     eax, edx
    sar     edx, 31
    idiv    ecx
    mov     [IY], ebp
    mov     [DIX], eax                  ; image x delta (DIX)
    mov     edx, [ebx + vertex_iy]      ; right iy
    mov     eax, ebp
    sub     edx, ebp
    mov     eax, edx
    mov     esi, [colour_data + cd_imageptr]
    sar     edx, 31
    idiv    ecx
    dec     edi
    dec     edi
    mov     [DIY], eax                  ; image y delta (DIY)
    mov     edx, [IX]                   ; edx = running U accumulator

    ; edi = screen ptr, ebp = running V, edx = running U, ecx = width
.Loop:
    push    ecx
    mov     ebx, [colour_data + cd_imagexmask]
    mov     eax, [DIX]
    and     ebx, edx
    add     edx, eax
    mov     ecx, [colour_data + cd_imageymask]
    mov     eax, ebp
    mov     cl, [colour_data + cd_imageyshift]
    shr     ebx, 16
    and     eax, ecx
    shr     eax, cl
    inc     edi
    inc     edi
    mov     ecx, [DIY]
    add     ebx, eax                    ; ebx = texel index (U_col + V_row)
    add     ebp, ecx
    mov     esi, [colour_data + cd_alphaptr]
    mov     eax, [esi + ebx]            ; alpha dword
    sar     eax, 5-3                    ; >>2
    and     eax, 38h                    ; transparency level
    mov     cl, [TSHIFT]
    inc     cl
    test    cl, 08h
    jz      .noreload
    mov     cl, [TOFF]
    inc     cl
    and     cl, 07h
    mov     [TOFF], cl
    xor     cl, cl
.noreload:
    mov     [TSHIFT], cl
    or      al, [TOFF]
    mov     al, [TransparencyData + eax]
    shr     al, cl
    test    al, 01h
    pop     ecx
    jz      .nowrite
    mov     esi, [colour_data + cd_imageptr]
    mov     eax, [esi + ebx]
    and     eax, 0FFh
    add     ax, ax
    mov     ax, [palette_table + eax]
    mov     word [edi], ax
.nowrite:
    dec     ecx
    jnz     .Loop
.Out:
    pop     ebp
    pop     edi
    pop     ecx
    ret

; ---- XASM_AImageHoriline2 ----
global XASM_AImageHoriline2
XASM_AImageHoriline2:
    push    ecx
    push    edi
    push    ebp
    mov     ecx, [ebx + vertex_sx]      ; right vertex sx (16.16)
    mov     edi, [edx + vertex_sx]      ; left  vertex sx (16.16)
    add     ecx, 0x20000
    sar     edi, 17                     ; left integer column (x2 width)
    push    eax
    mov     ax, di
    and     eax, 0x07
    mov     al, [FUP + eax]
    mov     [TSHIFT], al
    pop     eax
    sar     ecx, 17                     ; right integer column (x2 width)
    mov     esi, edx                    ; esi = left vertex
    sub     ecx, edi                    ; span width
    jle     .out
    add     edi, edi
    add     edi, edi                    ; edi = leftcol*4  (4 bytes/texel for x2)
    add     edi, eax                    ; + screen base = start screen address
    mov     eax, [esi + vertex_ix]      ; left ix
    mov     edx, [ebx + vertex_ix]      ; right ix
    mov     [IX], eax
    sub     edx, eax
    mov     ebp, [esi + vertex_iy]
    mov     eax, [esi + vertex_sy]
    and     al, 0x07
    mov     [TOFF], al
    mov     eax, edx
    sar     edx, 31
    idiv    ecx                         ; DIX = (right.ix-left.ix)/width
    mov     [IY], ebp
    mov     [DIX], eax
    mov     edx, [ebx + vertex_iy]      ; right iy
    mov     eax, ebp
    sub     edx, ebp
    mov     eax, edx
    mov     esi, [colour_data + cd_imageptr]
    sar     edx, 31
    idiv    ecx                         ; DIY = (right.iy-left.iy)/width
    sub     edi, 4
    mov     [DIY], eax
    mov     edx, [IX]                   ; edx = running U accumulator (16.16)

; edi = screen ptr, esi = image map data ptr, ecx = line width
.loop:
    push    ecx
    mov     ebx, [colour_data + cd_imagexmask]
    mov     eax, [DIX]
    and     ebx, edx
    add     edx, eax                    ; step U
    mov     ecx, [colour_data + cd_imageymask]
    mov     eax, ebp
    mov     cl, [colour_data + cd_imageyshift]
    shr     ebx, 16                     ; texel column
    and     eax, ecx
    shr     eax, cl                     ; texel row byte offset
    add     edi, 4
    mov     ecx, [DIY]
    add     ebx, eax                    ; texel index
    add     ebp, ecx                    ; step V
    mov     esi, [colour_data + cd_alphaptr]
    mov     eax, [esi + ebx]            ; alpha byte (dword load, low byte used)
    sar     eax, 5-3                    ; >>2
    and     eax, 0x38                   ; transparency level (0,8,...,0x38)
    mov     cl, [TSHIFT]
    inc     cl
    test    cl, 0x08
    jz      short .noreload
    mov     cl, [TOFF]
    inc     cl
    and     cl, 0x07
    mov     [TOFF], cl
    xor     cl, cl
.noreload:
    mov     [TSHIFT], cl
    or      al, [TOFF]
    mov     al, [TransparencyData + eax]
    shr     al, cl
    test    al, 0x01
    pop     ecx
    jz      short .nowrite
    mov     esi, [colour_data + cd_imageptr]
    mov     eax, [esi + ebx]            ; texel byte (dword load)
    and     eax, 0xFF
    add     ax, ax
    mov     ax, [palette_table + eax]
    mov     word [edi], ax
    mov     word [edi+2], ax            ; 2x horizontal replicate
.nowrite:
    dec     ecx
    jnz     .loop
.out:
    pop     ebp
    pop     edi
    pop     ecx
    ret

; ---- XASM_AImageHoriline4 ----
global XASM_AImageHoriline4
XASM_AImageHoriline4:
    push    ecx
    push    edi
    push    ebp
    mov     ecx, [ebx + vertex_sx]          ; right vertex sx
    mov     edi, [edx + vertex_sx]          ; left  vertex sx
    add     ecx, 0x40000
    sar     edi, 18
    push    eax
    mov     ax, di
    and     eax, 0x07
    mov     al, [FUP + eax]
    mov     [TSHIFT], al
    pop     eax
    sar     ecx, 18
    mov     esi, edx
    sub     ecx, edi
    jle     .Out
    add     edi, edi
    add     edi, edi
    add     edi, edi
    add     edi, eax                        ; start screen address
    mov     eax, [esi + vertex_ix]          ; left vertex ix
    mov     edx, [ebx + vertex_ix]          ; right vertex ix
    mov     [IX], eax
    sub     edx, eax
    mov     ebp, [esi + vertex_iy]
    mov     eax, [esi + vertex_sy]
    and     al, 0x07
    mov     [TOFF], al
    mov     eax, edx
    sar     edx, 31
    idiv    ecx
    mov     [IY], ebp
    mov     [DIX], eax                      ; image x delta
    mov     edx, [ebx + vertex_iy]          ; right vertex iy
    mov     eax, ebp
    sub     edx, ebp
    mov     eax, edx
    mov     esi, [colour_data + cd_imageptr]
    sar     edx, 31
    idiv    ecx
    sub     edi, 8
    mov     [DIY], eax                      ; image y delta
    mov     edx, [IX]

; edi = screen ptr ; esi = image map data ptr ; ecx = line width
.Loop:
    push    ecx
    mov     ebx, [colour_data + cd_imagexmask]
    mov     eax, [DIX]
    and     ebx, edx
    add     edx, eax
    mov     ecx, [colour_data + cd_imageymask]
    mov     eax, ebp
    mov     cl, [colour_data + cd_imageyshift]
    shr     ebx, 16
    and     eax, ecx
    shr     eax, cl
    add     edi, 8
    mov     ecx, [DIY]
    add     ebx, eax
    add     ebp, ecx
    mov     esi, [colour_data + cd_alphaptr]
    mov     eax, [esi + ebx]
    sar     eax, 5-3
    and     eax, 0x38
    mov     cl, [TSHIFT]
    inc     cl
    test    cl, 0x08
    jz      short .noreload
    mov     cl, [TOFF]
    inc     cl
    and     cl, 0x07
    mov     [TOFF], cl
    xor     cl, cl
.noreload:
    mov     [TSHIFT], cl
    or      al, [TOFF]
    mov     al, [TransparencyData + eax]
    shr     al, cl
    test    al, 0x01
    pop     ecx
    jz      short .nowrite
    mov     esi, [colour_data + cd_imageptr]
    mov     eax, [esi + ebx]
    and     eax, 0xFF
    add     ax, ax
    mov     ax, [palette_table + eax]
    mov     word [edi], ax
    mov     word [edi+2], ax
    mov     word [edi+4], ax
    mov     word [edi+6], ax
.nowrite:
    dec     ecx
    jnz     .Loop
.Out:
    pop     ebp
    pop     edi
    pop     ecx
    ret

; ---- XASM_CImageHoriLine1 ----
global XASM_CImageHoriLine1
XASM_CImageHoriLine1:
    push    ecx
    push    edi
    push    ebp
    mov     ecx, [ebx + vertex_sx]          ; right vertex screen-x (16.16)
    sar     ecx, 16
    mov     esi, edx
    mov     edi, [esi + vertex_sx]          ; left vertex screen-x
    sar     edi, 16
    sub     ecx, edi
    inc     ecx                             ; span width = right_col - left_col + 1
    jle     .Out

    mov     ebp, [esi + vertex_sy]
    add     edi, edi
    add     edi, eax                        ; start screen address (eax + 2*left_col)
    and     ebp, 0xFF
    mov     eax, [esi + vertex_intensity]
    lea     ebp, [SkyFadeData + ebp]
    mov     [INTENS], eax
    mov     bp, word [ebp]                  ; dither seed = SkyFadeData[sy&0xFF]
    sub     eax, [ebx + vertex_intensity]
    mov     [JITTER], bp
    mov     edx, eax
    xor     ax, ax
    test    eax, eax
    jz      .Fast                           ; intensity delta == 0 -> fast path
    neg     edx
    mov     eax, edx
    sar     edx, 31
    idiv    ecx
    mov     [DINTENS], eax

    mov     eax, [esi + vertex_ix]          ; left vertex U
    mov     [IX], eax
    sub     eax, [ebx + vertex_ix]          ; right vertex U
    neg     eax
    mov     edx, eax
    sar     edx, 31
    idiv    ecx
    mov     [DIX], eax                      ; image x delta

    mov     eax, [esi + vertex_iy]          ; left vertex V
    mov     [IY], eax
    sub     eax, [ebx + vertex_iy]          ; right vertex V
    neg     eax
    mov     edx, eax
    sar     edx, 31
    idiv    ecx
    mov     [DIY], eax                      ; image y delta

    mov     esi, [colour_data + cd_imageptr]
    mov     edx, [IX]
    mov     ebp, [IY]

    ; edi = screen ptr, esi = image map ptr, ecx = line width
    or      ecx, [colour_data + cd_imagexmask]
    dec     edi
    dec     edi
.Loop:
    push    ecx
    mov     ebx, edx
    mov     eax, ebp
    and     ebx, ecx
    add     ebp, [DIY]
    shr     ebx, 16                         ; texel column
    and     eax, [colour_data + cd_imageymask]
    mov     cl, [colour_data + cd_imageyshift]
    inc     edi
    inc     edi
    shr     eax, cl                         ; texel row byte offset
    add     edx, [DIX]
    add     ebx, eax
    mov     eax, [INTENS]
    xor     cx, cx
    add     eax, [JITTER]
    mov     ch, byte [esi + ebx]            ; texel byte
    shr     eax, 16
    shr     cx, 4                           ; texel hi-nibble -> cl
    cmp     eax, 0x0F
    mov     ebx, [DINTENS]
    jbe     short .OKay
    mov     eax, 0x0F                       ; clamp intensity index to 0x0F
.OKay:
    or      ax, cx                          ; index = intensity | texelHiNibble
    add     [INTENS], ebx
    mov     ax, word [SkyFadeData + eax*2]  ; sky-shaded 16bpp pixel
    mov     bx, [JITTER]
    pop     ecx
    add     bx, 0x9136                      ; ordered-dither step
    ror     bx, 3
    mov     word [edi], ax
    dec     cx
    mov     [JITTER], bx
    jnz     short .Loop
.Out:
    pop     ebp
    pop     edi
    pop     ecx
    ret
.Fast:
    mov     eax, [esi + vertex_ix]          ; left vertex U
    mov     [IX], eax
    sub     eax, [ebx + vertex_ix]          ; right vertex U
    neg     eax
    mov     edx, eax
    sar     edx, 31
    idiv    ecx
    mov     [DIX], eax                      ; image x delta

    mov     eax, [esi + vertex_iy]          ; left vertex V
    mov     [IY], eax
    sub     eax, [ebx + vertex_iy]          ; right vertex V
    neg     eax
    mov     edx, eax
    sar     edx, 31
    idiv    ecx
    mov     [DIY], eax                      ; image y delta

    mov     esi, [colour_data + cd_imageptr]
    mov     edx, [IX]
    mov     ebp, [IY]

    or      ecx, [colour_data + cd_imagexmask]
    dec     edi
    dec     edi
.FLoop:
    push    ecx
    mov     ebx, edx
    mov     eax, ebp
    and     ebx, ecx
    add     ebp, [DIY]
    shr     ebx, 16
    and     eax, [colour_data + cd_imageymask]
    mov     cl, [colour_data + cd_imageyshift]
    inc     edi
    inc     edi
    shr     eax, cl
    add     edx, [DIX]
    add     ebx, eax
    mov     eax, [INTENS]                   ; constant intensity (delta==0)
    xor     cx, cx
    add     eax, [JITTER]
    mov     ch, byte [esi + ebx]
    shr     eax, 16
    shr     cx, 4
    cmp     eax, 0x0F
    mov     bx, [JITTER]
    jbe     short .OKay2
    mov     eax, 0x0F
.OKay2:
    add     bx, 0x9136
    or      ax, cx
    ror     bx, 3
    mov     ax, word [SkyFadeData + 2*eax]
    mov     [JITTER], bx
    pop     ecx
    mov     word [edi], ax
    dec     cx
    jnz     short .FLoop
    pop     ebp
    pop     edi
    pop     ecx
    ret

; ---- XASM_CImageHoriLine2 ----
global XASM_CImageHoriLine2
XASM_CImageHoriLine2:
    push    ecx
    push    edi
    push    ebp
    mov     ecx, [ebx + vertex_sx]      ; right vertex sx (16.16)
    sar     ecx, 17                     ; x2: >>17 => integer column
    mov     esi, edx
    mov     edi, [esi + vertex_sx]      ; left vertex sx
    sar     edi, 17
    sub     ecx, edi
    inc     ecx                         ; span width (+1)
    jle     .Out

    mov     ebp, [esi + vertex_sy]
    add     edi, edi
    add     edi, edi                    ; left_col*4 (2 px * 2 bytes)
    add     edi, eax                    ; start screen address
    and     ebp, 0FFh
    mov     eax, [esi + vertex_intensity]
    lea     ebp, [SkyFadeData + ebp]
    mov     [INTENS], eax
    mov     bp, word [ebp]              ; per-line jitter seed = SkyFadeData[sy&0xFF]
    sub     eax, [ebx + vertex_intensity]
    mov     [JITTER], bp
    mov     edx, eax
    xor     ax, ax
    test    eax, eax
    jz      .Fast
    neg     edx
    mov     eax, edx
    sar     edx, 31
    idiv    ecx
    mov     [DINTENS], eax

    mov     eax, [esi + vertex_ix]      ; left vertex ix
    mov     [IX], eax
    sub     eax, [ebx + vertex_ix]      ; right vertex ix
    neg     eax
    mov     edx, eax
    sar     edx, 31
    idiv    ecx
    mov     [DIX], eax                  ; image x delta

    mov     eax, [esi + vertex_iy]      ; left vertex iy
    mov     [IY], eax
    sub     eax, [ebx + vertex_iy]      ; right vertex iy
    neg     eax
    mov     edx, eax
    sar     edx, 31
    idiv    ecx
    mov     [DIY], eax                  ; image y delta

    mov     esi, [colour_data + cd_imageptr]
    mov     edx, [IX]
    mov     ebp, [IY]

    or      ecx, [colour_data + cd_imagexmask]
    sub     edi, 4
.Loop:
    push    ecx
    mov     ebx, edx
    mov     eax, ebp
    and     ebx, ecx
    add     ebp, [DIY]
    shr     ebx, 16
    and     eax, [colour_data + cd_imageymask]
    mov     cl, [colour_data + cd_imageyshift]
    add     edi, 4
    shr     eax, cl
    add     edx, [DIX]
    add     ebx, eax
    mov     eax, [INTENS]
    xor     cx, cx
    add     eax, [JITTER]
    mov     ch, byte [esi + ebx]        ; texel byte -> ch (hi nibble used)
    shr     eax, 16
    shr     cx, 4
    cmp     eax, 0Fh
    mov     ebx, [DINTENS]
    jbe     short .OKay
    mov     eax, 0Fh                    ; clamp intensity index to 0x0F
.OKay:
    or      ax, cx                      ; index = (intens) | (texelHiNibble)
    add     [INTENS], ebx
    mov     ax, [SkyFadeData + eax*2]   ; shaded 16bpp pixel
    mov     bx, [JITTER]
    pop     ecx
    add     bx, 9136h
    ror     bx, 3                       ; ordered-dither advance
    mov     word [edi], ax
    mov     word [edi+2], ax            ; double-pixel write
    dec     cx
    mov     [JITTER], bx
    jnz     .Loop
.Out:
    pop     ebp
    pop     edi
    pop     ecx
    ret
.Fast:                                  ; intensity delta == 0 path
    mov     eax, [esi + vertex_ix]      ; left vertex ix
    mov     [IX], eax
    sub     eax, [ebx + vertex_ix]      ; right vertex ix
    neg     eax
    mov     edx, eax
    sar     edx, 31
    idiv    ecx
    mov     [DIX], eax                  ; image x delta

    mov     eax, [esi + vertex_iy]      ; left vertex iy
    mov     [IY], eax
    sub     eax, [ebx + vertex_iy]      ; right vertex iy
    neg     eax
    mov     edx, eax
    sar     edx, 31
    idiv    ecx
    mov     [DIY], eax                  ; image y delta

    mov     esi, [colour_data + cd_imageptr]
    mov     edx, [IX]
    mov     ebp, [IY]

    or      ecx, [colour_data + cd_imagexmask]
    sub     edi, 4
.FLoop:
    push    ecx
    mov     ebx, edx
    mov     eax, ebp
    and     ebx, ecx
    add     ebp, [DIY]
    shr     ebx, 16
    and     eax, [colour_data + cd_imageymask]
    mov     cl, [colour_data + cd_imageyshift]
    add     edi, 4
    shr     eax, cl
    add     edx, [DIX]
    add     ebx, eax
    mov     eax, [INTENS]
    xor     cx, cx
    add     eax, [JITTER]
    mov     ch, byte [esi + ebx]
    shr     eax, 16
    shr     cx, 4
    cmp     eax, 0Fh
    mov     bx, [JITTER]
    jbe     short .OKay2
    mov     eax, 0Fh
.OKay2:
    add     bx, 9136h
    or      ax, cx
    ror     bx, 3
    mov     ax, [SkyFadeData + eax*2]
    mov     [JITTER], bx
    pop     ecx
    mov     word [edi], ax
    mov     word [edi+2], ax
    dec     cx
    jnz     short .FLoop
    pop     ebp
    pop     edi
    pop     ecx
    ret

; ---- XASM_CImageHoriLine4 ----
global XASM_CImageHoriLine4
; void XASM_CImageHoriLine4(void)
;   eax = screen scanline pixel ptr (565 16bpp)
;   edx = &LEFT vertex   ebx = &RIGHT vertex
; x4 pixel-replication; sky-shaded textured span (== SImage but SkyFadeData).
; Steps sx, ix, iy, intensity. Preserves ecx, edi, ebp (as the original proc);
; ebx is scratch. (GRAFPASM.ASM ASM_CImageHoriLine4)
XASM_CImageHoriLine4:
    push    ecx
    push    edi
    push    ebp

    mov     ecx, [ebx + vertex_sx]      ; right vertex sx (16.16)
    sar     ecx, 18                      ; x4 -> >>18 integer column
    mov     esi, edx                     ; esi = left vertex
    mov     edi, [esi + vertex_sx]       ; left vertex sx
    sar     edi, 18
    sub     ecx, edi
    inc     ecx
    jle     .Out

    mov     ebp, [esi + vertex_sy]
    add     edi, edi
    add     edi, edi
    add     edi, edi                      ; left_col * 8 (x4 -> 8 bytes/texel)
    add     edi, eax                      ; start screen address
    and     ebp, 0xFF
    mov     eax, [esi + vertex_intensity]
    lea     ebp, [SkyFadeData + ebp]
    mov     [INTENS], eax
    mov     bp, word [ebp]                ; SkyFadeData[sy&0xFF] (jitter seed)
    sub     eax, [ebx + vertex_intensity]
    mov     [JITTER], bp
    mov     edx, eax
    xor     ax, ax
    test    eax, eax
    jz      .Fast

    neg     edx
    mov     eax, edx
    sar     edx, 31
    idiv    ecx
    mov     [DINTENS], eax

    mov     eax, [esi + vertex_ix]        ; left vertex ix
    mov     [IX], eax
    sub     eax, [ebx + vertex_ix]        ; right vertex ix
    neg     eax
    mov     edx, eax
    sar     edx, 31
    idiv    ecx
    mov     [DIX], eax                    ; image x delta

    mov     eax, [esi + vertex_iy]        ; left vertex iy
    mov     [IY], eax
    sub     eax, [ebx + vertex_iy]        ; right vertex iy
    neg     eax
    mov     edx, eax
    sar     edx, 31
    idiv    ecx
    mov     [DIY], eax                    ; image y delta

    mov     esi, [colour_data + cd_imageptr]
    mov     edx, [IX]
    mov     ebp, [IY]

    or      ecx, [colour_data + cd_imagexmask]
    sub     edi, 8
.Loop:
    push    ecx
    mov     ebx, edx
    mov     eax, ebp
    and     ebx, ecx
    add     ebp, [DIY]
    shr     ebx, 16
    and     eax, [colour_data + cd_imageymask]
    mov     cl, [colour_data + cd_imageyshift]
    add     edi, 8
    shr     eax, cl
    add     edx, [DIX]
    add     ebx, eax
    mov     eax, [INTENS]
    xor     cx, cx
    add     eax, [JITTER]
    mov     ch, byte [esi + ebx]
    shr     eax, 16
    shr     cx, 4
    cmp     eax, 0x0F
    mov     ebx, [DINTENS]
    jbe     short .OKay
    mov     eax, 0x0F
.OKay:
    or      ax, cx
    add     [INTENS], ebx
    mov     ax, [SkyFadeData + eax*2]
    mov     bx, [JITTER]
    pop     ecx
    add     bx, 0x9136
    ror     bx, 3
    mov     word [edi], ax
    mov     word [edi+2], ax
    mov     word [edi+4], ax
    mov     word [edi+6], ax
    dec     cx
    mov     [JITTER], bx
    jnz     .Loop
.Out:
    pop     ebp
    pop     edi
    pop     ecx
    ret

.Fast:
    mov     eax, [esi + vertex_ix]        ; left vertex ix
    mov     [IX], eax
    sub     eax, [ebx + vertex_ix]        ; right vertex ix
    neg     eax
    mov     edx, eax
    sar     edx, 31
    idiv    ecx
    mov     [DIX], eax                    ; image x delta

    mov     eax, [esi + vertex_iy]        ; left vertex iy
    mov     [IY], eax
    sub     eax, [ebx + vertex_iy]        ; right vertex iy
    neg     eax
    mov     edx, eax
    sar     edx, 31
    idiv    ecx
    mov     [DIY], eax                    ; image y delta

    mov     esi, [colour_data + cd_imageptr]
    mov     edx, [IX]
    mov     ebp, [IY]

    or      ecx, [colour_data + cd_imagexmask]
    sub     edi, 8
.FLoop:
    push    ecx
    mov     ebx, edx
    mov     eax, ebp
    and     ebx, ecx
    add     ebp, [DIY]
    shr     ebx, 16
    and     eax, [colour_data + cd_imageymask]
    mov     cl, [colour_data + cd_imageyshift]
    add     edi, 8
    shr     eax, cl
    add     edx, [DIX]
    add     ebx, eax
    mov     eax, [INTENS]
    xor     cx, cx
    add     eax, [JITTER]
    mov     ch, byte [esi + ebx]
    shr     eax, 16
    shr     cx, 4
    cmp     eax, 0x0F
    mov     bx, [JITTER]
    jbe     short .OKay2
    mov     eax, 0x0F
.OKay2:
    add     bx, 0x9136
    or      ax, cx
    ror     bx, 3
    mov     ax, [SkyFadeData + eax*2]
    mov     [JITTER], bx
    pop     ecx
    mov     word [edi], ax
    mov     word [edi+2], ax
    mov     word [edi+4], ax
    mov     word [edi+6], ax
    dec     cx
    jnz     .FLoop
    pop     ebp
    pop     edi
    pop     ecx
    ret

; ---- XASM_NullScanLine ----
global XASM_NullScanLine
; ASM_NullScanLine (GRAFPASM.ASM:6732) — invalid-scanline-type trap.
; Original body is "int 3 / ret" (a debug breakpoint). Ported as a bare
; ret (no int3) per the port spec. No vertex/colour_data reads, no
; interpolation, no register use; preserves the call ABI (void, ret).
XASM_NullScanLine:
	ret

