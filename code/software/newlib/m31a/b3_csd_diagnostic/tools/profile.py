"""Declarative isolated profile. Importing this file runs no tool or build."""
from pathlib import Path

HERE = Path(__file__).resolve().parents[1]
RUNTIME = HERE.parent
REPO = RUNTIME.parents[3]
PROGRAM = 'm31-csd-failure-diag'
PREFIX = 'm68k-elf-rosco-'
FLAGS = ['-mcpu=68010', '-march=68010', '-mtune=68010', '-std=c11', '-O2', '-g',
         '-Wall', '-Wextra', '-Werror', '-ffreestanding', '-ffunction-sections',
         '-fdata-sections', '-fno-common', '-fno-delete-null-pointer-checks',
         '--param=min-pagesize=0', '-DM31_DIAGNOSTIC', '-DSDRO_CSD_DIAGNOSTIC', '-fstack-usage']
INCLUDES = ['-I'+str(RUNTIME/'public'), '-I'+str(RUNTIME/'include'),
            '-I'+str(RUNTIME/'compat'), '-I'+str(RUNTIME/'fat_ro/include'),
            '-I'+str(RUNTIME/'sd_ro/include'), '-I'+str(RUNTIME/'b3_integration/include'),
            '-iquote', str(REPO/'code/software/libs/src/termios/include'),
            '-iquote', str(REPO/'code/software/libs/src/machine/include')]
SOURCES = {
    'startup.o': RUNTIME/'startup.S', 'memory.o': RUNTIME/'memory.c',
    'runtime.o': RUNTIME/'runtime.c', 'syscalls.o': RUNTIME/'syscalls.c',
    'termios.o': REPO/'code/software/libs/src/termios/termios.c',
    'fat_ro.o': RUNTIME/'fat_ro/src/fat_ro.c', 'sd_ro.o': RUNTIME/'sd_ro/src/sd_ro.c',
    'csd_decode.o': RUNTIME/'sd_ro/src/csd_decode.c',
    'sd_ro_traps.o': RUNTIME/'sd_ro/src/sd_ro_traps.S',
    'b3_integration.o': RUNTIME/'b3_integration/src/b3_integration.c',
    'application.o': RUNTIME/'b3_executable/src/smoke.c',
}
OBJECTS = list(SOURCES) + ['machine.o']
TERM_RENAME = ['-D'+name+'=m31_termios_'+name for name in
               ['tcgetattr', 'tcsetattr', 'tcdrain', 'tcflush']]
LINK_OPTIONS = ['--cref', '--gc-sections', '--warn-common', '--fatal-warnings',
                '-z', 'noexecstack', '-z', 'max-page-size=16', '-z', 'common-page-size=16',
                # Retention proves the same allocator ownership as runtime A.
                # It does NOT claim malloc/free were called by this application.
                '--undefined=malloc', '--undefined=free']
OWNERS = {
 'startup.o': ['_start', '_exit', 'm31_diag_mark', 'm31_diag_error'],
 'runtime.o': ['m31_runtime_start', 'm31_run_application', '_init', '_fini'],
 'memory.o': ['m31_plan', 'm31_heap_move'],
 'syscalls.o': ['_sbrk_r', '_read_r', '_write_r', '_open_r', '_close_r', '_fstat_r',
                '_stat_r', '_isatty_r', '_lseek_r', '_unlink_r', '_link_r', '_rename_r',
                '_kill_r', '_getpid_r', 'm31_heap_init', 'm31_console_init'],
 'termios.o': ['rosco_termios_read', 'rosco_termios_write'],
 'machine.o': ['mcGetDevice', 'mcReadDevice', 'mcSendDevice'],
 'fat_ro.o': ['b1_init', 'b1_shutdown', 'b1_open', 'b1_read', 'b1_seek', 'b1_info', 'b1_close'],
 'sd_ro.o': ['sdro_init', 'sdro_read', 'sdro_context', 'sdro_capacity', 'sdro_shutdown', 'sdro_diagnostics'],
 'csd_decode.o': ['sdro_decode_csd', 'sdro_validate_capacity', 'sdro_check_lba'],
 'sd_ro_traps.o': ['sdro_fw_support', 'sdro_fw_init', 'sdro_fw_read', 'sdro_fw_csd'],
 'b3_integration.o': ['b3_init', 'b3_open', 'b3_read', 'b3_seek', 'b3_info', 'b3_close', 'b3_shutdown', 'b3_diagnostics'],
 'application.o': ['main'],
}
# Existing target disassembly shows validation inlined into both callers;
# --gc-sections may remove only this unused out-of-line public helper.
# Its definition/owner is still mandatory in the input object.
OPTIONAL_FINAL = {'sdro_validate_capacity'}
WRAPPERS = {
 'sdro_fw_support': '70004e4d4e75',
 'sdro_fw_init': '48e700c0226f000c70014e4d4cdf03004e75',
 'sdro_fw_read': '48e740e0226f0014222f0018246f001c70024e4d4cdf07024e75',
 'sdro_fw_csd': '48e740e0226f0014246f0018720970044e4d4cdf07024e75',
}

# Additive diagnostic providers; existing ownership requirements retained.
OWNERS['sd_ro.o'] += ['sdro_csd_snapshot']
OWNERS['csd_decode.o'] += ['sdro_decode_csd_traced']
OWNERS['b3_integration.o'] += ['b3_csd_snapshot']
LINK_OPTIONS += ['--undefined=sdro_decode_csd']
