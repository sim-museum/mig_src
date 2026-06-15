; Mig Alley — native Linux port: real implementations of the XASM_* software-rasterizer
; primitives that GRAFPRIM.CPP calls via `__asm__("call XASM_...")` (Watcom register
; convention: args in eax/edx/ebx/ecx, return in eax). The originals are in the TASM
; GRAFPASM.ASM (6756 lines, MASM syntax — can't assemble with nasm), which BoB left dead.
; This file ports the primitives, starting with the PALETTE subset (foundation for every
; 8-bit-indexed -> 16-bit-565 blit). Faithful to GRAFPASM.ASM's ASM_* routines:
;   palette_buffer = 8 palettes x 256 x u16 (the master store the game fills)
;   palette_table  = the active 256 x u16 8->16-bit LUT (used by blits)
; Build: nasm -f elf32. The remaining XASM_* (line/pixel drawers) stay as no-op stubs in
; port_link_stubs.cpp until ported (next step).
;
; assemble: nasm -f elf32 SRC/GRAPHICS/ma_xasm.nasm -o port/build/obj/ma_xasm.o

section .bss
align 4
palette_buffer: resw 256*8        ; 8 palettes of 256 entries (matches GRAFPASM)
palette_table:  resw 256          ; active palette / 8->16-bit LUT

section .text

; void* XASM_GetPaletteTable(void)  -> eax = &palette_buffer  (the 8-palette master store)
global XASM_GetPaletteTable
XASM_GetPaletteTable:
    lea     eax, [palette_buffer]
    ret

; UWord XASM_GetPaletteEntry(eax=index) -> eax = palette_table[index]  (zero-extended u16)
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
    shl     eax, 9                ; *512 bytes  (= 256 entries * 2 bytes)
    lea     esi, [palette_buffer + eax]
    lea     edi, [palette_table]
    mov     ecx, 128              ; 128 dwords = 256 words
    cld
    rep     movsd
    pop     ecx
    pop     edi
    pop     esi
    ret
