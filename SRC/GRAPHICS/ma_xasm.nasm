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

; ---- deferred fillers: no-op stubs (gouraud / textured) ------------------------
; Referenced by the dispatch tables; a poly of these types simply isn't painted yet.
; Ported next (GRAFPASM/GRAFJIM gouraud + image span loops).
%macro STUBFILL 1
global %1
%1:
    ret
%endmacro
STUBFILL XASM_GouraudHoriLine1
STUBFILL XASM_GouraudHoriLine2
STUBFILL XASM_GouraudHoriLine4
STUBFILL XASM_ImageHoriLine1
STUBFILL XASM_ImageHoriLine2
STUBFILL XASM_ImageHoriLine4
STUBFILL XASM_MImageHoriLine1
STUBFILL XASM_MImageHoriLine2
STUBFILL XASM_MImageHoriLine4
STUBFILL XASM_SImageHoriLine1
STUBFILL XASM_SImageHoriLine2
STUBFILL XASM_SImageHoriLine4
STUBFILL XASM_TFImageHoriline1
STUBFILL XASM_TFImageHoriline2
STUBFILL XASM_TFImageHoriline4
STUBFILL XASM_AImageHoriline1
STUBFILL XASM_AImageHoriline2
STUBFILL XASM_AImageHoriline4
STUBFILL XASM_CImageHoriLine1
STUBFILL XASM_CImageHoriLine2
STUBFILL XASM_CImageHoriLine4
STUBFILL XASM_NullScanLine
