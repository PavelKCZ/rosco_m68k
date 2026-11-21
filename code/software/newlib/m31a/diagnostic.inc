/* Preprocessed GAS macros shared with the original startup, not a second entry.
 * TRAP #14/4 is PRINTCHAR (D0.B). Normal menu entry is supervisor mode.
 * MOVE from SR therefore requires that established supervisor launch contract.
 * MOVEM does not alter CCR. Restore CCR BEFORE MOVEM/RTS (also flag-neutral).
 * All use of the inherited stack is explicit and diagnostic-only. */
#ifdef M31_DIAGNOSTIC
.macro D_SAVE
    movem.l %d0-%d7/%a0-%a6,-(%sp)
    move.w %sr,-(%sp)
.endm
.macro D_RESTORE
    move.w (%sp)+,%ccr
    movem.l (%sp)+,%d0-%d7/%a0-%a6
.endm
.macro D_MARK value
    D_SAVE
    moveq #\value,%d0
    bsr m31_diag_putchar
    moveq #32,%d0
    bsr m31_diag_putchar
    D_RESTORE
.endm
.macro D_ENTRY
    D_SAVE
    lea 62(%sp),%a0
    move.l %a0,%d6
    moveq #0,%d7
    move.w (%sp),%d7
    /* E precedes metadata reads. SP/SR describe the original entry, not a
     * helper frame. No RAM metadata is assumed valid merely to print E. */
    moveq #69,%d0
    bsr m31_diag_putchar
    /* Snapshot once: raw ABI slot for reporting; RAM/reset for validation. */
    move.l 0x400,%d2
    move.l 0x414,%d4
    move.l 0,%d5
    lea m31_diag_abi_slot(%pc),%a0
    bsr m31_diag_string
    move.l %d2,%d0
    bsr m31_diag_hex8
    lea m31_diag_ram(%pc),%a0
    bsr m31_diag_string
    move.l %d4,%d0
    bsr m31_diag_hex8
    lea m31_diag_reset(%pc),%a0
    bsr m31_diag_string
    move.l %d5,%d0
    bsr m31_diag_hex8
    lea m31_diag_sp(%pc),%a0
    bsr m31_diag_string
    move.l %d6,%d0
    bsr m31_diag_hex8
    lea m31_diag_sr(%pc),%a0
    bsr m31_diag_string
    move.l %d7,%d0
    bsr m31_diag_hex8
    bsr m31_diag_newline
    /* Deliver raw slot in d2 and validation inputs in d4/d5. The other
     * incoming registers and original CCR are restored. Slots include SR word. */
    move.l %d2,10(%sp)
    move.l %d4,18(%sp)
    move.l %d5,22(%sp)
    D_RESTORE
.endm
.macro D_REJECT number
    D_SAVE
    moveq #33,%d0
    bsr m31_diag_putchar
    moveq #48,%d0
    bsr m31_diag_putchar
    moveq #(48+\number),%d0
    bsr m31_diag_putchar
    bsr m31_diag_newline
    D_RESTORE
    bra .Lreject
.endm
.set .Lbad_ram,.Lreject02
.set .Lbad_reset,.Lreject03
.set .Lbad_align,.Lreject04
.set .Lbad_heap,.Lreject05
.set .Lbad_source,.Lreject06
.set .Lbad_extent,.Lreject07
#else
.macro D_MARK value
.endm
.macro D_ENTRY
.endm
.set .Lbad_ram,.Lreject
.set .Lbad_reset,.Lreject
.set .Lbad_align,.Lreject
.set .Lbad_heap,.Lreject
.set .Lbad_source,.Lreject
.set .Lbad_extent,.Lreject
#endif
