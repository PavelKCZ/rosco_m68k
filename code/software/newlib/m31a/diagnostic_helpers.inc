/* Included in .boot after the rejection stop loop. No absolute code/data
 * references. Helpers execute at load address, then at relocated run address. */
#ifdef M31_DIAGNOSTIC
/* !01 retired: boot magic is not a persistent ABI signature. */
m31_diag_reject02:
.Lreject02: D_REJECT 2
m31_diag_reject03:
.Lreject03: D_REJECT 3
m31_diag_reject04:
.Lreject04: D_REJECT 4
m31_diag_reject05:
.Lreject05: D_REJECT 5
m31_diag_reject06:
.Lreject06: D_REJECT 6
m31_diag_reject07:
.Lreject07: D_REJECT 7
/* PRINTCHAR's documented interface preserves other registers. Additionally
 * preserve its argument/scratch registers and CCR, so string/hex loops and
 * surrounding startup flags cannot be changed by this diagnostic call. */
.type m31_diag_putchar,@function
m31_diag_putchar:
    movem.l %d0-%d1/%a0-%a1,-(%sp)
    move.w %sr,-(%sp)
    moveq #4,%d1
    trap #14
    move.w (%sp)+,%ccr
    movem.l (%sp)+,%d0-%d1/%a0-%a1
    rts
.size m31_diag_putchar,.-m31_diag_putchar
.type m31_diag_string,@function
m31_diag_string:
    move.b (%a0)+,%d0
    beq 1f
    bsr m31_diag_putchar
    bra m31_diag_string
1:  rts
.size m31_diag_string,.-m31_diag_string
.type m31_diag_hex8,@function
m31_diag_hex8:
    D_SAVE
    move.l %d0,%d3
    moveq #7,%d2
1:  rol.l #4,%d3
    move.l %d3,%d0
    andi.l #15,%d0
    addi.b #48,%d0
    cmpi.b #57,%d0
    bls 2f
    addq.b #7,%d0
2:  bsr m31_diag_putchar
    dbra %d2,1b
    D_RESTORE
    rts
.size m31_diag_hex8,.-m31_diag_hex8
.type m31_diag_newline,@function
m31_diag_newline:
    moveq #13,%d0
    bsr m31_diag_putchar
    moveq #10,%d0
    bsr m31_diag_putchar
    rts
.size m31_diag_newline,.-m31_diag_newline
.global m31_diag_mark
.type m31_diag_mark,@function
m31_diag_mark:
    D_SAVE
    move.l 66(%sp),%d0
    bsr m31_diag_putchar
    moveq #32,%d0
    bsr m31_diag_putchar
    D_RESTORE
    rts
.size m31_diag_mark,.-m31_diag_mark
.global m31_diag_error
.type m31_diag_error,@function
m31_diag_error:
    D_SAVE
    moveq #33,%d0
    bsr m31_diag_putchar
    moveq #67,%d0
    bsr m31_diag_putchar
    move.l 66(%sp),%d0
    bsr m31_diag_hex8
    bsr m31_diag_newline
    D_RESTORE
    rts
.size m31_diag_error,.-m31_diag_error
m31_diag_abi_slot: .asciz " ABI_SLOT="
m31_diag_ram:   .asciz " RAM="
m31_diag_reset: .asciz " RESET="
m31_diag_sp:    .asciz " SP="
m31_diag_sr:    .asciz " SR="
.balign 4
#endif
