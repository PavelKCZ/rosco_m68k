#include "memory.h"
#include "diagnostic.inc"
/* No stack access or C call until metadata and loaded-image bounds pass.
 * Firmware interrupts retain their inherited SR; memory is flat supervisor RAM. */
.section .boot,"ax"
.global _start
_start:
    D_ENTRY
#ifdef M31_DIAGNOSTIC
m31_diag_validate:
#endif
    /* 0x400 is a transient boot-magic / kernel-API slot, not a RAM invariant. */
#ifndef M31_DIAGNOSTIC
    move.l 0x414,%d4
#endif
    cmpi.l #M31_RAM_MIN,%d4
    blo .Lbad_ram
    cmpi.l #M31_RAM_MAX,%d4
    bhi .Lbad_ram
#ifndef M31_DIAGNOSTIC
    move.l 0,%d5
#endif
    cmpi.l #M31_RAM_MIN,%d5
    blo .Lbad_reset
    cmpi.l #M31_RAM_MAX,%d5
    bhi .Lbad_reset
    move.l %d4,%d0
    or.l %d5,%d0
    andi.l #3,%d0
    bne .Lbad_align
    cmp.l %d5,%d4
    bls .Ltop
    move.l %d5,%d4
.Ltop:
    move.l %d4,%d5
    subi.l #M31_STACK,%d5
    move.l #_end,%d0
    addq.l #7,%d0
    bcs .Lbad_heap
    andi.l #-8,%d0
    cmp.l %d5,%d0
    bhs .Lbad_heap
    lea _start(%pc),%a0
    move.l %a0,%d6
    move.l %d6,%d0
    andi.l #3,%d0
    bne .Lbad_source
    cmpi.l #_data_end,%d6
    blo .Lbad_source
    move.l %d6,%d0
    addi.l #_image_size,%d0
    bcs .Lbad_extent
    cmp.l %d5,%d0
    bhi .Lbad_extent
    D_MARK 86
    /* Retire the menu private stack before relocation can overwrite it.
     * All bounds checks passed; interrupts now use the reserved new stack. */
    move.l %d4,%sp
    D_MARK 83
    /* Copy the boot AND postboot trampoline before jumping into low RAM. */
    move.l #_start,%a1
    move.l #_boot_size,%d0
.Lbootcopy:
    move.b (%a0)+,(%a1)+
    subq.l #1,%d0
    bne .Lbootcopy
    jmp _relocated
#ifdef M31_DIAGNOSTIC
m31_diag_stop:
#endif
.Lreject:
    /* Fail closed without writing RAM or trusting an unvalidated exit pointer. */
    bra .Lreject
#include "diagnostic_helpers.inc"
.section .postboot,"ax"
_relocated:
    D_MARK 82
    move.l #_tail_size,%d0
.Lcopy:
    move.b (%a0)+,(%a1)+
    subq.l #1,%d0
    bne .Lcopy
    lea _bss_start,%a0
    lea _bss_end,%a1
.Lzero:
    cmpa.l %a1,%a0
    bhs .Lready
    clr.b (%a0)+
    bra .Lzero
.Lready:
    D_MARK 90
    /* runtime_start(top, load_address); source image can now be reused. */
    move.l %d6,-(%sp)
    move.l %d4,-(%sp)
    jsr m31_runtime_start
    jmp _exit
.section .text.exit,"ax"
.global _exit
_exit:
    move.l 0x490,%a0
    jmp (%a0)
.section .note.GNU-stack,"",@progbits
