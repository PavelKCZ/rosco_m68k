/* Isolated BIN profile: firmware loads at 0x40000, entry relocates to 0x2000.
 * Same load/run convention as libs/src/start_serial; no shared file changed. */
OUTPUT_FORMAT("elf32-m68k")
OUTPUT_ARCH(m68k)
ENTRY(_start)
PHDRS { code PT_LOAD FLAGS(5); data PT_LOAD FLAGS(6); stack PT_GNU_STACK FLAGS(6); }
SECTIONS {
 . = 0x2000;
 .boot : AT(0x40000) { KEEP(*(.boot)) . = ALIGN(4); } :code
 _boot_size = ADDR(.text) - _start;
 .postboot : AT(ADDR(.postboot) + 0x3e000) { KEEP(*(.postboot)) }
 .text : AT(ADDR(.text) + 0x3e000) { KEEP(*syscalls.o(.text .text.*)) *(.text .text.*) *(.rodata .rodata.*) . = ALIGN(4); }
 .preinit_array : AT(ADDR(.preinit_array) + 0x3e000) {
  __preinit_array_start = .; KEEP(*(.preinit_array)) __preinit_array_end = .;
 }
 .init_array : AT(ADDR(.init_array) + 0x3e000) {
  __init_array_start = .; KEEP(*(SORT_BY_INIT_PRIORITY(.init_array.*))) KEEP(*(.init_array)) __init_array_end = .;
 }
 .fini_array : AT(ADDR(.fini_array) + 0x3e000) {
  __fini_array_start = .; KEEP(*(SORT_BY_INIT_PRIORITY(.fini_array.*))) KEEP(*(.fini_array)) __fini_array_end = .;
 }
 .ctors : AT(ADDR(.ctors) + 0x3e000) {
  _ctors_start = .; KEEP(*(SORT(.ctors.*))) KEEP(*(.ctors)) _ctors_end = .;
 }
 .dtors : AT(ADDR(.dtors) + 0x3e000) {
  _dtors_start = .; KEEP(*(SORT(.dtors.*))) KEEP(*(.dtors)) _dtors_end = .;
 }
 .data : AT(ADDR(.data) + 0x3e000) { *(.data .data.*) . = ALIGN(4); _data_end = .; } :data
 _image_size = _data_end - _start;
 _tail_size = _image_size - _boot_size;
 .bss (NOLOAD) : { _bss_start = .; *(.bss .bss.*) *(COMMON) . = ALIGN(8); _bss_end = .; } :data
 _end = .;
 _TIMER_100HZ = 0x40c;
 /DISCARD/ : { *(.comment) *(.note*) *(.eh_frame*) }
 ASSERT(_start == 0x2000, "BIN entry is not first")
 ASSERT(_image_size > _boot_size, "empty relocation tail")
 ASSERT(_data_end <= 0x40000, "initialized image overlaps normal load address")
 ASSERT((_image_size & 3) == 0, "image alignment")
 ASSERT((_end & 7) == 0, "heap alignment")
}
