/* Isolated GCC/MC68010 C wrappers, based on local sdfat/sdcard_asm.S.
 * Host build NEVER compiles or executes this file. D0 returns raw firmware status.
 * No arbitrary register/CMD API: CSD command is fixed here to CMD9.
 */
.section .text.sdro_fw_support,"ax"
.balign 2
.globl sdro_fw_support
sdro_fw_support:
    moveq #0,%d0
    trap #13
    rts
.section .text.sdro_fw_init,"ax"
.balign 2
.globl sdro_fw_init
sdro_fw_init:
    movem.l %a0-%a1,-(%sp)
    move.l 12(%sp),%a1
    moveq #1,%d0
    trap #13
    movem.l (%sp)+,%a0-%a1
    rts
.section .text.sdro_fw_read,"ax"
.balign 2
.globl sdro_fw_read
sdro_fw_read:
    movem.l %d1/%a0-%a2,-(%sp)
    move.l 20(%sp),%a1
    move.l 24(%sp),%d1
    move.l 28(%sp),%a2
    moveq #2,%d0
    trap #13
    movem.l (%sp)+,%d1/%a0-%a2
    rts
.section .text.sdro_fw_csd,"ax"
.balign 2
.globl sdro_fw_csd
sdro_fw_csd:
    movem.l %d1/%a0-%a2,-(%sp)
    move.l 20(%sp),%a1
    move.l 24(%sp),%a2
    moveq #9,%d1
    moveq #4,%d0
    trap #13
    movem.l (%sp)+,%d1/%a0-%a2
    rts
.section .note.GNU-stack,"",@progbits
