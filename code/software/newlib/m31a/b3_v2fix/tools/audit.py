#!/usr/bin/env python3
"""Maintained D3 audit fork. Historical D2 scripts remain unchanged.

Reads evidence and runs binutils only. No compilation, target execution or output
files. Manual review is an explicit mandatory gate, never auto-generated PASS.
"""
import sys
sys.dont_write_bytecode = True
import hashlib
import importlib.util
import json
import re
import struct
import subprocess
from pathlib import Path
from profile import (HERE, RUNTIME, REPO, PREFIX, PROGRAM, FLAGS, INCLUDES,
                     SOURCES, OBJECTS, TERM_RENAME, OWNERS, WRAPPERS, OPTIONAL_FINAL)
from build import verify_sources, external, checked_tool, libraries_match, check_dependencies, clean_environment

_TOOLS = {}

class Rejected(Exception):
    pass

def need(condition, code):
    if not condition:
        raise Rejected(code)

def sha(p):
    return hashlib.sha256(p.read_bytes()).hexdigest()

def tool(name, *args, binary=False):
    output = subprocess.check_output([_TOOLS[name], *map(str, args)], stderr=subprocess.PIPE, env=clean_environment())
    return output if binary else output.decode()

def object_elf(raw):
    need(len(raw) >= 52 and raw[:7] == b'\x7fELF\x01\x02\x01', 'OBJECT_ELF')
    h = struct.unpack_from('>HHIIIIIHHHHHH', raw, 16)
    need(h[0:3] == (1, 4, 1) and h[10] == 40 and h[11] > 0 and h[12] < h[11], 'OBJECT_ABI')
    need(h[5]+h[10]*h[11] <= len(raw), 'OBJECT_TABLE')
    sh = [struct.unpack_from('>10I', raw, h[5]+i*h[10]) for i in range(h[11])]
    st = sh[h[12]]
    strings = raw[st[4]:st[4]+st[5]]
    sections = {}
    for entry in sh:
        need(entry[0] < len(strings), 'OBJECT_NAME')
        name = strings[entry[0]:].split(b'\0', 1)[0].decode()
        need(entry[1] == 8 or entry[4]+entry[5] <= len(raw), 'OBJECT_EXTENT')
        sections[name] = entry
    return sections

def definitions(p):
    return {line.split()[-1] for line in tool('nm', '-g', '--defined-only', p).splitlines()
            if len(line.split()) == 3}

def member_definitions(raw):
    """Read actual symbol definitions; an undefined _write_r reference is legal."""
    object_elf(raw)
    h = struct.unpack_from('>HHIIIIIHHHHHH', raw, 16)
    sh = [struct.unpack_from('>10I', raw, h[5]+i*h[10]) for i in range(h[11])]
    names_out = set()
    for sec in sh:
        if sec[1] != 2: continue
        need(sec[9] == 16 and sec[6] < len(sh), 'MEMBER_SYMBOL_TABLE')
        st = sh[sec[6]]; names = raw[st[4]:st[4]+st[5]]
        for offset in range(sec[4], sec[4]+sec[5], 16):
            n, value, size, info, other, index = struct.unpack_from('>IIIBBH', raw, offset)
            if index != 0 and n and info>>4 in (1,2):
                need(n < len(names), 'MEMBER_SYMBOL_NAME')
                names_out.add(names[n:].split(b'\0',1)[0].decode())
    return names_out

def audit(build, machine_only=False):
    b = external(str(build))
    provenance = verify_sources()
    record = json.loads((b/'build-record.json').read_text())
    need(record['input_manifest_sha256'] == sha(HERE/'SHA256SUMS'), 'INPUT_MANIFEST')
    global _TOOLS
    need(set(record['tools']) == set(provenance['tools']), 'TOOL_INVENTORY')
    _TOOLS = {n:str(checked_tool(item['path'],n,provenance)) for n,item in record['tools'].items()}
    libraries_match(record['archive_roles'],provenance)
    need(record['profile'] == PROGRAM and set(record['objects']) == set(OBJECTS), 'OBJECT_INVENTORY')
    origin = external(record['origin_build'])
    need(record['flags'] == FLAGS and '13.3.1' in record['compiler_version'], 'COMPILER_PROFILE')
    archives = {Path(n).resolve(): digest for n, digest in record['archives'].items()}
    need({Path(p).name:h for p,h in archives.items()} == provenance['archives'] and
         set(map(str, archives)) == set(record['archive_roles'].values()), 'ARCHIVE_IDENTITIES')
    for p, digest in archives.items():
        need(p.is_file() and sha(p) == digest, 'ARCHIVE_HASH')
    libc = next(p for p in archives if p.name == 'libc.a')
    gcc = next(p for p in archives if p.name == 'libgcc.a')
    rosco = next(p for p in archives if p.name == 'librosco.a')
    objects = {}; syscall_providers = {}
    for name in OBJECTS:
        p = b/name; item = record['objects'][name]
        need(p.is_file() and sha(p) == item.get('sha256'), 'OBJECT_HASH:'+name)
        objects[name] = definitions(p)
        object_elf(p.read_bytes())
        if name in SOURCES:
            source = SOURCES[name]
            need(item['source'] == str(source) and item['source_sha256'] == sha(source), 'SOURCE_OWNER:'+name)
            argv = [_TOOLS['gcc'], *FLAGS, *INCLUDES]
            if name == 'termios.o': argv += TERM_RENAME
            argv += ['-MD', '-MP', '-MF', str(origin/(name+'.d')), '-c', str(source), '-o', str(origin/name)]
            need(item['argv'] == argv and str(source) in item['dependencies'], 'COMPILE_COMMAND:'+name)
            check_dependencies(item['dependencies'],record['toolchain_roots'],provenance)
            for dep, digest in item['dependencies'].items():
                need(sha(Path(dep)) == digest, 'DEPENDENCY_HASH:'+dep)
        else:
            need(item['archive'] == str(rosco) and item['member'] == 'machine.o', 'MACHINE_OWNER')
            need(tool('ar', 'p', rosco, 'machine.o', binary=True) == p.read_bytes(), 'MACHINE_BYTES')
        for symbol in objects[name]:
            if symbol in OWNERS['syscalls.o']:
                syscall_providers.setdefault(symbol, []).append(name)
    for symbol in OWNERS['syscalls.o']:
        need(syscall_providers.get(symbol) == ['syscalls.o'], 'SYSCALL_PROVIDER:'+symbol)
    need(objects['application.o'] == {'main'}, 'APPLICATION_EXPORTS')
    # Reuse tested pure ELF/map parsers from the pinned, UNMODIFIED runtime audit.
    # Do NOT call its profile-specific audit() or main() functions.
    spec = importlib.util.spec_from_file_location('m31_base_audit', RUNTIME/'audit.py')
    old = importlib.util.module_from_spec(spec); spec.loader.exec_module(old)
    elf = b/(PROGRAM+'.elf'); image = (b/(PROGRAM+'.bin')).read_bytes(); raw = elf.read_bytes()
    entry, ph, sections = old.parse_elf(raw)
    maptext = (b/(PROGRAM+'.map')).read_text(); owners = old.map_owners(maptext)
    loaded = [Path(line.split(maxsplit=1)[1]).resolve() for line in maptext.splitlines() if line.startswith('LOAD ')]
    need(loaded == [origin/o for o in OBJECTS]+[libc, gcc], 'LINK_INPUTS')
    for bad in ['libnosys', 'libstart_serial', 'libcstdlib', 'libheap', 'crt0.o',
                'librosco.a(', 'fat_filelib.o', 'dirent.o', 'sdcard.o', 'sdcard_asm.o']:
        need(bad not in maptext, 'FORBIDDEN_INPUT:'+bad)
    symbols = {}; global_symbols = set()
    for line in tool('nm', '-n', elf).splitlines():
        fields = line.split()
        if len(fields) == 3:
            address, kind, name = fields
            if kind.isupper():
                need(name not in global_symbols, 'DUPLICATE_GLOBAL:'+name)
                global_symbols.add(name); symbols[name] = int(address, 16)
            elif name not in symbols:
                symbols[name] = int(address, 16)
    need(not tool('nm', '-u', elf).strip(), 'UNRESOLVED_SYMBOLS')
    for obj, required in OWNERS.items():
        for name in required:
            need(name in objects[obj], 'OBJECT_DEFINITION:'+name)
            if name in OPTIONAL_FINAL and name not in symbols: continue
            need(name in symbols and owners.get(name) == str(origin/obj), 'OWNER:'+name)
    for name in ['malloc','free','__errno','exit','atexit','__libc_init_array','__libc_fini_array']:
        need(name in symbols and owners.get(name, '').startswith(str(libc)+'('), 'LIBC_OWNER:'+name)
    forbidden_prefix = ('SD_', 'fl_', 'fatfs_', 'Part_')
    for name in global_symbols:
        need(not name.startswith(forbidden_prefix), 'LEGACY_SYMBOL:'+name)
    for name in ['_open','_read','_write','_sbrk','_sd_open','_sd_stat']:
        need(name not in global_symbols, 'LEGACY_HOOK:'+name)
    # Audit every selected archive member, not just a symbol-name blacklist.
    members = set()
    for owner in owners.values():
        match = re.fullmatch(r'(.+\.a)\(([^()]+)\)', owner)
        if match:
            archive, member = Path(match[1]).resolve(), match[2]
            need(archive in (libc, gcc), 'ARCHIVE_PROVIDER:'+owner)
            need(member in provenance['archive_members_allowlist'][archive.name], 'UNREVIEWED_MEMBER:'+member)
            members.add((archive, member))
        elif owner.endswith('.o'):
            need(owner in {str(origin/o) for o in OBJECTS}, 'UNKNOWN_OBJECT_OWNER:'+owner)
    need(members, 'ARCHIVE_MEMBER_EVIDENCE')
    member_hashes = {}; archive_defs = {}
    for archive, member in sorted(members):
        data = tool('ar', 'p', archive, member, binary=True)
        need(data[:7] == b'\x7fELF\x01\x02\x01', 'ARCHIVE_MEMBER_ABI')
        member_hashes[str(archive)+'('+member+')'] = hashlib.sha256(data).hexdigest()
        defs = member_definitions(data)
        archive_defs[str(archive)+'('+member+')'] = defs
        need(not (defs & set(OWNERS['syscalls.o'])) and
             not any(n.startswith(('SD_','fl_','fatfs_','Part_','_sd_')) for n in defs),
             'FORBIDDEN_ARCHIVE_MEMBER:'+member)
    linker_symbols = {'_start','_relocated','_boot_size','_image_size','_tail_size','_data_end',
                      '_bss_start','_bss_end','_end','_TIMER_100HZ','_ctors_start','_ctors_end',
                      '_dtors_start','_dtors_end','__preinit_array_start','__preinit_array_end',
                      '__init_array_start','__init_array_end','__fini_array_start','__fini_array_end'}
    for name in global_symbols-linker_symbols:
        owner = owners.get(name)
        if owner in {str(origin/o) for o in OBJECTS}:
            need(name in objects[Path(owner).name], 'GLOBAL_OBJECT_OWNER:'+name)
        else:
            need(owner in archive_defs and name in archive_defs[owner], 'GLOBAL_ARCHIVE_OWNER:'+name)
    for name in ['__modsi3','__muldi3','__mulsi3','__udivdi3','__udivsi3','__umodsi3',
                 '__divsi3','__divsi3_internal','__udivsi3_internal','__mulsi3_internal','__clz_tab']:
        if name in symbols:
            need(owners.get(name, '').startswith(str(gcc)+'('), 'HELPER_OWNER:'+name)
    for name in ['memcmp','memcpy','memset']:
        need(name in symbols and owners.get(name, '').startswith(str(libc)+'('), 'MEMORY_OWNER:'+name)

    layout = ['_start','_relocated','_boot_size','_image_size','_tail_size','_data_end',
              '_bss_start','_bss_end','_end']
    need(all(n in symbols for n in layout), 'LAYOUT_SYMBOLS')
    s = symbols
    need(entry == s['_start'] == 0x2000, 'ENTRY')
    need(0 < s['_boot_size'] < s['_image_size'] == len(image), 'IMAGE_BOUNDARY')
    need(s['_tail_size'] == s['_image_size']-s['_boot_size'], 'TAIL_SIZE')
    need(s['_data_end'] == 0x2000+len(image) <= 0x40000, 'IMAGE_BOUNDARY')
    need(s['_bss_start'] == s['_data_end'] <= s['_bss_end'] == s['_end'] and s['_end']%8 == 0, 'BSS_END')
    need(len(image)%4 == 0 and s['_boot_size']%4 == 0, 'IMAGE_ALIGNMENT')
    for name in ['.boot','.postboot','.text','.data','.bss']:
        need(name in sections, 'MISSING_SECTION:'+name)
    need(sections['.boot'][3] == 0x2000 and sections['.text'][3] == 0x2000+s['_boot_size'], 'BOOT_EXTENT')
    post = sections['.postboot']
    need(post[3] == s['_relocated'] and post[3]+post[5] <= 0x2000+s['_boot_size'], 'TRAMPOLINE')
    loads = [p for p in ph if p[0] == 1]; stacks = [p for p in ph if p[0] == 0x6474e551]
    need(len(ph) == 3 and len(loads) == 2 and sorted(p[6] for p in loads) == [5,6], 'LOAD_PERMISSIONS')
    need(len(stacks) == 1 and stacks[0][6] == 6, 'GNU_STACK')
    ranges = []
    for typ, off, vma, lma, filesz, memsz, flags, align in loads:
        need(filesz <= memsz and off+filesz <= len(raw), 'LOAD_SIZE')
        need(align >= 4 and not align&(align-1) and off%align == vma%align, 'LOAD_ALIGNMENT')
        need(lma == vma+0x3e000, 'LOAD_DELTA')
        need(0x2000 <= vma <= vma+memsz <= s['_end'], 'LOAD_MEMORY')
        ranges.append((vma, vma+memsz))
    ranges.sort(); need(ranges[0][1] <= ranges[1][0], 'LOAD_OVERLAP')
    need(min(p[3] for p in loads) == 0x40000 and max(p[3]+p[4] for p in loads) == 0x40000+len(image), 'LOAD_EXTENT')
    for name, sec in sections.items():
        _, kind, flags, addr, off, size, *_ = sec
        if flags&2 and size:
            need(flags&5 != 5, 'SECTION_WX:'+name)
            need(any(p[2] <= addr and addr+size <= p[2]+p[5] for p in loads), 'SECTION_LOAD:'+name)
            if kind != 8:
                need(0x2000 <= addr and addr+size <= s['_data_end'], 'SECTION_IMAGE:'+name)
                need(image[addr-0x2000:addr-0x2000+size] == raw[off:off+size], 'BIN_BYTES:'+name)
    for a, z in [('_ctors_start','_ctors_end'),('_dtors_start','_dtors_end'),
                 ('__preinit_array_start','__preinit_array_end'),('__init_array_start','__init_array_end'),
                 ('__fini_array_start','__fini_array_end')]:
        need(a in s and z in s and 0x2000 <= s[a] <= s[z] <= s['_data_end'] and
             s[a]%4 == 0 and (s[z]-s[a])%4 == 0, 'LIFECYCLE_ARRAY:'+a)
    need(s['_ctors_end'] > s['_ctors_start'] or s['__init_array_end'] > s['__init_array_start'], 'CONSTRUCTOR')
    need(s['_dtors_end'] > s['_dtors_start'] or s['__fini_array_end'] > s['__fini_array_start'], 'DESTRUCTOR')
    # Validate a concrete minimum-RAM plan, not an invented running-board size.
    heap = (s['_end']+7)&~7
    need(heap < 0x80000-32768 and 0x40000+len(image) <= 0x80000-32768, 'MINIMUM_RAM_PLAN')
    # Target B2 layout established in B3-I1: descriptor0, CSD24, bounce40,
    # diagnostic552, total576; final placement must retain that alignment/extent.
    need('adapter' in s and s['adapter']%4 == 0 and
         sections['.data'][3] <= s['adapter'] and
         s['adapter']+576 <= sections['.data'][3]+sections['.data'][5], 'ADAPTER_PLACEMENT')
    adapter_section = object_elf((b/'sd_ro.o').read_bytes())['.data.adapter']
    need(adapter_section[5] == 576 and adapter_section[8] >= 4, 'ADAPTER_OBJECT_LAYOUT')
    for name, body in WRAPPERS.items():
        address = s[name]; need(address%2 == 0, 'WRAPPER_ODD:'+name)
        need(image[address-0x2000:address-0x2000+len(bytes.fromhex(body))] == bytes.fromhex(body), 'WRAPPER_BODY:'+name)
    wrapper_sections = object_elf((b/'sd_ro_traps.o').read_bytes())
    wrapper_raw = (b/'sd_ro_traps.o').read_bytes()
    for name in WRAPPERS:
        sec = wrapper_sections['.text.'+name]
        need(sec[8] >= 2, 'WRAPPER_SECTION_ALIGNMENT:'+name)
        need(wrapper_raw[sec[4]:sec[4]+sec[5]] == bytes.fromhex(WRAPPERS[name]), 'WRAPPER_OBJECT_BODY:'+name)
    dis = tool('objdump', '-d', elf)
    traps = [int(m[1],16) for line in dis.splitlines() if 'trap #13' in line
             and (m := re.match(r'\s*([0-9a-f]+):', line))]
    wanted_traps = sorted(s[n]+bytes.fromhex(body).index(bytes.fromhex('4e4d')) for n,body in WRAPPERS.items())
    need(sorted(traps) == wanted_traps, 'TRAP_SELECTORS')
    boot = tool('objdump', '-d', '-j', '.boot', elf)
    need('b105d47a' not in boot.lower() and 'jsr' not in boot, 'EARLY_ABI')
    need('moveal %d4,%sp' in boot and 'moveb %a0@+,%a1@+' in boot and
         boot.index('moveal %d4,%sp') < boot.index('moveb %a0@+,%a1@+'), 'STACK_SWITCH')
    for line in boot.splitlines():
        if 'bsr' in line: need('<m31_diag_' in line, 'EARLY_CALL')
    need(image[:6] == bytes.fromhex('48e7fffe40e7'), 'ENTRY_SNAPSHOT')
    primitive = bytes.fromhex('48e7c0c040e772044e4e44df4cdf03034e75')
    start = s['m31_diag_putchar']-0x2000
    need(image[start:start+len(primitive)] == primitive, 'PRINTCHAR_PRIMITIVE')
    for name in ['m31_diag_mark','m31_diag_error','m31_diag_hex8']:
        start = s[name]-0x2000
        need(0x2000 <= s[name] < 0x2000+s['_boot_size'] and
             image[start:start+6] == bytes.fromhex('48e7fffe40e7'), 'DIAGNOSTIC_SAVE:'+name)
    need(all(label in image for label in [b' ABI_SLOT=',b' RAM=',b' RESET=',b' SP=',b' SR=']), 'ABI_FIELDS')
    need('m31_diag_reject01' not in s, 'RETIRED_MAGIC_GATE')
    validation = boot.split('<m31_diag_validate>:',1)[1].split('<m31_diag_stop>:',1)[0]
    need('%d2' not in validation, 'RAW_SLOT_VALIDATION')
    for number, count in [(2,2),(3,2),(4,1),(5,2),(6,2),(7,2)]:
        need(validation.count('<m31_diag_reject%02d>'%number) == count, 'REJECTION_MAPPING')
    post_dis = tool('objdump', '-d', '-j', '.postboot', elf)
    need('m31_runtime_start' in post_dis and 'clrb %a0@+' in post_dis, 'POSTBOOT')
    runtime_dis = tool('objdump', '-d', '--disassemble=m31_runtime_start', elf)
    need(not re.search(r'\b400(?:\s|,)', runtime_dis), 'C_RAW_SLOT')
    need('b105d47a' not in tool('objdump','-d','--disassemble=m31_plan',elf).lower(), 'C_MAGIC_GATE')
    # Additional diagnostic-only gates. Existing runtime/read-only checks above
    # are retained verbatim, including exact F1 wrappers and archive allowlist.
    evidence = object_elf((b/'sd_ro.o').read_bytes()).get('.bss.csd_evidence')
    need(evidence is not None and evidence[5] == 48 and evidence[8] >= 2, 'CSD_RECORD_LAYOUT')
    need('csd_evidence' in s and s['csd_evidence']%2 == 0 and
         s['_bss_start'] <= s['csd_evidence'] and s['csd_evidence']+48 <= s['_end'], 'CSD_RECORD_PLACEMENT')
    for marker in [b'CSDD1 VALID=', b'CSDD1 SUP=', b'CSDD1 RAW=', b'CSDD1 REJ=']:
        need(marker in image, 'CSD_OUTPUT_MARKER')
    result = {'result':'MACHINE_PASS', 'machine_checks':'PASS',
              'scope':'automatic static checks only; NOT release or hardware approval',
              'elf_sha256':sha(elf), 'bin_sha256':sha(b/(PROGRAM+'.bin')),
              'layout':{n:hex(s[n]) for n in layout}, 'archive_members':member_hashes,
              'limitations':['No CPU execution or SD/CRC integrity proof',
                            'Stack/IRQ acceptance needs manual engineering evidence, not dynamic proof',
                            'Build provenance is a recorded local chain, not cryptographic build attestation']}
    if machine_only:
        return result
    # Assembly and -mcpu flags alone do not prove all linked-library instructions
    # or nested firmware/IRQ stack use. This review must reference THIS image.
    need((b/'manual-review.json').is_file(), 'MANUAL_REQUIRED')
    review = json.loads((b/'manual-review.json').read_text())
    need(review['elf_sha256'] == sha(elf) and review['bin_sha256'] == sha(b/(PROGRAM+'.bin')), 'MANUAL_IMAGE')
    need(bool(review['reviewer'].strip()) and bool(review['notes'].strip()), 'MANUAL_IDENTITY')
    pending = []
    for name in ['mc68010_instructions','stack_irq_budget','firmware_abi_assumptions','b3_memory_budget']:
        value = review['checks'][name]
        need(value in ('PASS','UNKNOWN','CONDITIONAL'), 'MANUAL_REQUIRED:'+name)
        if value != 'PASS': pending.append(name)
    result.update(result='CONDITIONAL' if pending else 'PASS', manual_pending=pending,
                  scope='static checks plus honest manual review; NOT hardware approval')
    return result

def main():
    try:
        need(len(sys.argv) in (2,3), 'USAGE: audit.py BUILD [--machine-only]')
        need(len(sys.argv) == 2 or sys.argv[2] == '--machine-only', 'USAGE')
        result = audit(Path(sys.argv[1]), len(sys.argv) == 3)
        code = 3 if result['result'] == 'CONDITIONAL' else 0
    except Exception as exc:
        result = {'result':'FAIL', 'error':str(exc), 'hardware_executed':False}; code = 2
    print(json.dumps(result, indent=2)); return code

if __name__ == '__main__':
    sys.exit(main())
