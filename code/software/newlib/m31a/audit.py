#!/usr/bin/env python3
"""Fail-closed inspection of target files. Never executes ELF or BIN.
No Python assert is used: checks remain active under -O/PYTHONOPTIMIZE.
"""
import hashlib
import json
from pathlib import Path
import re
import struct
import subprocess
import sys

class AuditError(Exception):
    pass

def require(condition, message):
    if not condition:
        raise AuditError(message)

def tool(name, *args):
    return subprocess.check_output(['m68k-elf-rosco-'+name, *map(str,args)],text=True)

def parse_elf(raw):
    require(len(raw) >= 52 and raw[:7] == b'\x7fELF\x01\x02\x01', 'invalid ELF32 big-endian header')
    kind, machine, version, entry, phoff, shoff, flags, ehsize, phsize, phnum, shsize, shnum, strindex = struct.unpack_from('>HHIIIIIHHHHHH',raw,16)
    require((kind,machine,version,ehsize,phsize,shsize)==(2,4,1,52,32,40), 'unsupported ELF ABI/table format')
    require(phnum > 0 and shnum > 0 and strindex < shnum, 'missing ELF tables')
    require(phoff >= 52 and phoff+phsize*phnum <= len(raw), 'truncated program headers')
    require(shoff >= 52 and shoff+shsize*shnum <= len(raw), 'truncated section headers')
    ph = [struct.unpack_from('>8I',raw,phoff+i*phsize) for i in range(phnum)]
    sh = [struct.unpack_from('>10I',raw,shoff+i*shsize) for i in range(shnum)]
    st = sh[strindex]
    require(st[1] == 3 and st[4]+st[5] <= len(raw), 'invalid section string table')
    strings = raw[st[4]:st[4]+st[5]]
    sections = {}
    for record in sh:
        name,typ,flg,addr,off,size,link,info,align,entsize = record
        require(name < len(strings), 'invalid section name offset')
        end = strings.find(b'\0',name)
        require(end >= 0, 'unterminated section name')
        text = strings[name:end].decode('ascii')
        require(not text or text not in sections, 'duplicate section name')
        require(align == 0 or align & (align-1) == 0, 'invalid section alignment')
        require(not align or addr % align == 0, 'misaligned section address: '+text)
        require(typ == 8 or off+size <= len(raw), 'truncated section: '+text)
        require(typ == 8 or not align or off % align == 0, 'misaligned section file offset: '+text)
        sections[text] = record
    return entry,ph,sections

def map_owners(text):
    require('Cross Reference Table' in text, 'map lacks cross-reference table')
    result = {}
    for line in text.split('Cross Reference Table',1)[1].splitlines():
        m = re.fullmatch(r'(\S+)\s+(\S+)',line)
        if m and m[1] != 'Symbol':
            require(m[1] not in result, 'ambiguous map owner: '+m[1])
            result[m[1]] = m[2]
    require(result, 'empty map cross-reference table')
    return result

def audit(build, diagnostic=False):
    b=Path(build).resolve();program='runtime-smoke-diag' if diagnostic else 'runtime-smoke';elf=b/(program+'.elf')
    raw=elf.read_bytes();image=(b/(program+'.bin')).read_bytes()
    nm=tool('nm','-n',elf);symbols={};global_names=set()
    for line in nm.splitlines():
        fields=line.split()
        if len(fields)==3:
            if fields[1].isupper():
                require(fields[2] not in global_names, 'duplicate global ELF symbol: '+fields[2])
                global_names.add(fields[2])
                symbols[fields[2]]=int(fields[0],16)
            elif fields[2] not in symbols:
                symbols[fields[2]]=int(fields[0],16)
    require(not tool('nm','-u',elf).strip(), 'unresolved ELF symbols')
    m=(b/(program+'.map')).read_text();owners=map_owners(m)
    for bad in ['librosco.a(', 'libnosys', 'fat_filelib.o', 'dirent.o', 'sdcard.o',
                'libstart_serial', 'libcstdlib', 'libtermios.a', 'libheap','crt0.o']:
        require(bad not in m, 'forbidden map input: '+bad)
    expected={
        'startup.o':['_start','_exit'],
        'syscalls.o':['_sbrk_r','_read_r','_write_r','_open_r','_close_r','_fstat_r','_stat_r','_isatty_r','_lseek_r','_unlink_r','_link_r','_rename_r','_kill_r','_getpid_r'],
        'runtime.o':['m31_runtime_start','m31_run_application','_init','_fini'],
        'termios.o':['rosco_termios_read','rosco_termios_write'],
        'machine.o':['mcGetDevice','mcReadDevice','mcSendDevice']}
    for obj,names in expected.items():
        own=tool('nm','-g','--defined-only',b/obj)
        defs={line.split()[-1] for line in own.splitlines() if len(line.split())==3}
        for name in names:
            require(name in symbols and name in defs, 'missing required definition: '+name)
            require(name in owners and Path(owners[name]).resolve()==b/obj, 'wrong map owner: '+name)
    for name in ['malloc','free','__errno','exit','atexit','__libc_init_array','__libc_fini_array']:
        require(name in symbols and name in owners and re.fullmatch(r'.*/libc\.a\([^)]+\)',owners[name]), 'wrong libc owner: '+name)
    for name in symbols:
        require(not name.startswith(('SD_','fl_','fatfs_','Part_')), 'filesystem symbol: '+name)
    for name in ['_open','_read','_write','_sbrk','_sd_open','_sd_stat']:
        require(name not in symbols, 'unexpected legacy hook: '+name)
    libdir=Path(subprocess.check_output(['m68k-elf-rosco-gcc','-print-file-name=libc.a'],text=True).strip()).parent
    archive=libdir/'librosco.a'
    require(hashlib.sha256(archive.read_bytes()).hexdigest()=='cd839cc509dc4b794790900f6a4b8ef6496f09322b386b834f4595d99ce58fa5','unexpected machine support archive')
    selected=subprocess.check_output(['m68k-elf-rosco-ar','p',str(archive),'machine.o'])
    require(selected==(b/'machine.o').read_bytes(), 'machine.o differs from selected archive member')
    layout_names=['_start','_relocated','_boot_size','_image_size','_tail_size','_data_end','_bss_start','_bss_end','_end']
    require(all(n in symbols for n in layout_names), 'missing layout symbols')
    s=symbols;entry,ph,sections=parse_elf(raw)
    require(entry==s['_start']==0x2000, 'wrong runtime entry')
    require(0<s['_boot_size']<s['_image_size']==len(image), 'invalid BIN/copy size')
    require(s['_tail_size']==s['_image_size']-s['_boot_size'], 'invalid tail size')
    require(s['_data_end']==0x2000+len(image)<=0x40000, 'invalid initialized runtime extent')
    require(s['_bss_start']==s['_data_end']<=s['_bss_end']==s['_end'] and s['_end']%8==0, 'invalid BSS/heap boundary')
    for name in ['.boot','.postboot','.text','.data','.bss']:
        require(name in sections, 'missing section: '+name)
    require(sections['.boot'][3]==0x2000, 'BIN entry is not first section')
    post=sections['.postboot']
    require(post[3]==s['_relocated'] and post[3]+post[5]<=0x2000+s['_boot_size'], 'trampoline not fully copied')
    require(sections['.text'][3]==0x2000+s['_boot_size'], 'tail overwrites trampoline')
    require(len(image)%4==0 and s['_boot_size']%4==0, 'copy alignment')
    loads=[h for h in ph if h[0]==1];stacks=[h for h in ph if h[0]==0x6474e551]
    require(len(loads)==2, 'expected two LOAD segments')
    require(len(ph)==3, 'unexpected program headers')
    require(len(stacks)==1 and stacks[0][6]==6, 'missing/non-RW/non-executable GNU_STACK')
    require(sorted(h[6] for h in loads)==[5,6], 'invalid LOAD permissions / RWX')
    ranges=[]
    for typ,off,vaddr,paddr,filesz,memsz,flg,align in loads:
        require(filesz<=memsz and off+filesz<=len(raw), 'invalid LOAD sizes')
        require(align>=4 and align&(align-1)==0 and off%align==vaddr%align, 'invalid LOAD alignment')
        require(paddr==vaddr+0x3e000, 'wrong BIN load/runtime address relation')
        require(vaddr>=0x2000 and vaddr+memsz<=s['_end'], 'LOAD outside runtime bounds')
        ranges.append((vaddr,vaddr+memsz))
    require(min(h[3] for h in loads)==0x40000, 'wrong BIN load address')
    require(max(h[3]+h[4] for h in loads)==0x40000+len(image), 'wrong BIN load extent')
    ranges.sort();require(ranges[0][1]<=ranges[1][0], 'overlapping LOAD regions')
    checked=0
    for name,record in sections.items():
        _,kind,flg,addr,off,size,*_=record
        if flg&2 and size:
            require(any(h[2]<=addr and addr+size<=h[2]+h[5] for h in loads), 'section outside LOAD: '+name)
            if kind!=8:
                require(0x2000<=addr and addr+size<=s['_data_end'], 'allocated bytes beyond image')
                require(image[addr-0x2000:addr-0x2000+size]==raw[off:off+size], 'ELF/BIN bytes differ: '+name)
                checked+=1
    require(checked>=5, 'insufficient allocated section coverage')
    for a,z in [('_ctors_start','_ctors_end'),('_dtors_start','_dtors_end'),('__preinit_array_start','__preinit_array_end'),('__init_array_start','__init_array_end'),('__fini_array_start','__fini_array_end')]:
        require(a in s and z in s and 0x2000<=s[a]<=s[z]<=s['_data_end'] and (s[z]-s[a])%4==0 and s[a]%4==0, 'invalid lifecycle array: '+a)
    require(s['_ctors_end']>s['_ctors_start'] or s['__init_array_end']>s['__init_array_start'], 'missing smoke constructor')
    require(s['_dtors_end']>s['_dtors_start'] or s['__fini_array_end']>s['__fini_array_start'], 'missing smoke destructor')
    boot=tool('objdump','-d','-j','.boot',elf)
    # D5: the transient slot must not influence the memory planner or startup.
    require('b105d47a' not in boot.lower(), 'stale boot-magic startup gate')
    c_runtime=tool('objdump','-d','--disassemble=m31_runtime_start',elf)
    require(not re.search(r'\b400(?:\s|,)',c_runtime), 'C runtime reads transient ABI slot')
    require('b105d47a' not in tool('objdump','-d','--disassemble=m31_plan',elf).lower(), 'stale C magic gate')
    if not diagnostic:
        require(not re.search(r'\b400(?:\s|,)',boot), 'normal startup reads transient ABI slot')
        require('jsr' not in boot and 'bsr' not in boot and '%sp@' not in boot, 'stack use before validated selection')
        require(not any(n.startswith('m31_diag_') for n in symbols), 'diagnostics in normal profile')
    else:
        # Only the explicit diagnostic profile may use the inherited menu stack.
        # This is a constrained instrumentation exception, not a relaxed normal rule.
        for n in ['m31_diag_mark','m31_diag_error','m31_diag_putchar','m31_diag_hex8']:
            require(n in symbols and 0x2000 <= symbols[n] < 0x2000+s['_boot_size'], 'early diagnostic helper not copied: '+n)
        require(owners.get('m31_diag_mark') == str(b/'startup.o') and owners.get('m31_diag_error') == str(b/'startup.o'), 'wrong diagnostic owner')
        require('trap #14' in boot and 'moveq #4,%d1' in boot, 'wrong firmware diagnostic primitive')
        require('jsr' not in boot, 'absolute early diagnostic call')
        for line in boot.splitlines():
            if 'bsr' in line:
                require('<m31_diag_' in line, 'unexpected early diagnostic call')
        require('m31_diag_mark' in boot and ',%ccr' in boot, 'missing diagnostic preservation')
        require(all(x in image for x in [b' ABI_SLOT=',b' RAM=',b' RESET=',b' SP=',b' SR=']), 'missing measured entry fields')
        require(all(n in symbols for n in ['m31_diag_validate','m31_diag_stop']), 'missing diagnostic control boundaries')
        raw_start=symbols['m31_diag_putchar']-0x2000
        # Exact minimal raw primitive: save volatile registers and SR; TRAP #14/4;
        # restore CCR before flag-neutral MOVEM/RTS. No C/stdio call permitted.
        primitive=bytes.fromhex('48e7c0c040e772044e4e44df4cdf03034e75')
        require(image[raw_start:raw_start+len(primitive)]==primitive, 'diagnostic primitive ABI/CCR changed')
        require(image[:6]==bytes.fromhex('48e7fffe40e7'), 'entry snapshot save frame changed')
        for n in ['m31_diag_mark','m31_diag_error','m31_diag_hex8']:
            start=symbols[n]-0x2000
            require(image[start:start+6]==bytes.fromhex('48e7fffe40e7'), 'diagnostic live-register save changed: '+n)
        validation=boot.split('<m31_diag_validate>:',1)[1].split('<m31_diag_stop>:',1)[0]
        require('m31_diag_reject01' not in symbols, 'retired !01 is still active')
        require('%d2' not in validation, 'raw ABI snapshot participates in validation')
        for num,count in [(2,2),(3,2),(4,1),(5,2),(6,2),(7,2)]:
            require(validation.count('<m31_diag_reject%02d>'%num)==count, 'rejection mapping/count changed: %02d'%num)

    require('moveal %d4,%sp' in boot and 'moveb %a0@+,%a1@+' in boot and boot.index('moveal %d4,%sp')<boot.index('moveb %a0@+,%a1@+'), 'SP switch ordering')
    post=tool('objdump','-d','-j','.postboot',elf)
    require('m31_runtime_start' in post and 'clrb %a0@+' in post, 'missing C/BSS startup operations')
    return {'result':'PASS','ownership':{n:owners[n] for names in expected.values() for n in names},
            'layout':{n:hex(s[n]) for n in layout_names},'allocated_sections_compared':checked,
            'load_permissions':[h[6] for h in loads],'gnu_stack_permissions':stacks[0][6],
            'artifacts':{p.name:{'size':p.stat().st_size,'sha256':hashlib.sha256(p.read_bytes()).hexdigest()} for p in [elf,b/(program+'.bin')]},
            'limitations':['Static inspection does not execute MC68010 instructions or prove interrupt/stack behavior.','Metadata cannot prove physical RAM capacity or running firmware identity.'],
            'input_hashes':{str(p):hashlib.sha256(p.read_bytes()).hexdigest() for p in [libdir/'libc.a',archive,b/'machine.o']},
            'profile':'diagnostic' if diagnostic else 'normal',
            'early_stack_contract':'Inherited valid supervisor-mode menu stack required for early TRAP' if diagnostic else 'No explicit stack use until validation',
            'hardware_executed':False}

def main(argv=None):
    args=sys.argv[1:] if argv is None else argv
    if len(args) not in (1,2) or (len(args)==2 and args[1]!='--diagnostic'):
        print('usage: audit.py BUILD_DIRECTORY [--diagnostic]',file=sys.stderr);return 2
    diagnostic=len(args)==2
    program='runtime-smoke-diag' if diagnostic else 'runtime-smoke'
    b=Path(args[0]).resolve()
    try:
        result=audit(b,diagnostic)
        (b/'symbols.txt').write_text(tool('nm','-n',b/(program+'.elf')))
        (b/'sections.txt').write_text(tool('readelf','-h','-l','-S',b/(program+'.elf')))
        (b/'size.txt').write_text(tool('size','-A',b/(program+'.elf')))
    except (AuditError,OSError,ValueError,struct.error,subprocess.SubprocessError) as exc:
        result={'result':'FAIL','error':str(exc),'hardware_executed':False}
    try:
        (b/'audit.json').write_text(json.dumps(result,indent=2)+'\n')
    except OSError as exc:
        print('audit evidence write failure: '+str(exc),file=sys.stderr);return 2
    print(json.dumps(result,indent=2))
    return 0 if result['result']=='PASS' else 2
if __name__=='__main__':
    raise SystemExit(main())
