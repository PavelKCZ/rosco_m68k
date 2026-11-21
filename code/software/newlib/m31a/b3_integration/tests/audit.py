#!/usr/bin/env python3
"""Fail-closed source identity, exact owner, dependency and ELF section audit.
No target execution. All generated evidence belongs in the external build dir.
"""
import hashlib,json,subprocess,sys,struct
from pathlib import Path
root=Path(__file__).resolve().parents[1]; repo=root.parents[4]
build=Path(sys.argv[1]).resolve()
def run(*args): return subprocess.check_output(args,text=True,stderr=subprocess.STDOUT)
def require(ok,message):
    if not ok: raise RuntimeError(message)
def digest(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def elf(path):
    data=path.read_bytes();require(data[:7]==b'\x7fELF\x01\x02\x01','ELF32 big-endian required')
    h=struct.unpack_from('>HHIIIIIHHHHHH',data,16)
    require(h[0]==1 and h[1]==4 and h[3]==0 and h[9]==0,'must be entry-free m68k ET_REL without program headers')
    sh=[struct.unpack_from('>IIIIIIIIII',data,h[5]+i*h[10]) for i in range(h[11])]
    strings=data[sh[h[12]][4]:sh[h[12]][4]+sh[h[12]][5]]
    sections={strings[s[0]:].split(b'\0',1)[0].decode():s for s in sh}
    for name,s in sections.items():require(not(s[2]&1 and s[2]&4),'writable executable section '+name)
    if path.name in ('combined.o','sd_ro_traps.o'):require('.note.GNU-stack' in sections and sections['.note.GNU-stack'][2]&4==0,'missing non-exec stack note')
    symbols={}
    for s in sh:
        if s[1]!=2:continue
        st=sh[s[6]]; names=data[st[4]:st[4]+st[5]]
        for off in range(s[4],s[4]+s[5],s[9]):
            n,v,size,info,other,idx=struct.unpack_from('>IIIBBH',data,off)
            name=names[n:].split(b'\0',1)[0].decode()
            if name:symbols[name]=(v,idx)
    return data,sections,sh,symbols
owners={
 'fat_ro.o':'b1_init b1_shutdown b1_open b1_read b1_seek b1_info b1_close'.split(),
 'sd_ro.o':'sdro_init sdro_capacity sdro_card_type sdro_context sdro_read sdro_diagnostics sdro_shutdown'.split(),
 'csd_decode.o':'sdro_decode_csd sdro_check_lba sdro_validate_capacity'.split(),
 'sd_ro_traps.o':'sdro_fw_support sdro_fw_init sdro_fw_read sdro_fw_csd'.split(),
 'b3_integration.o':'b3_init b3_open b3_read b3_seek b3_info b3_close b3_shutdown b3_diagnostics'.split()}
bodies={
 'sdro_fw_support':'70004e4d4e75',
 'sdro_fw_init':'48e700c0226f000c70014e4d4cdf03004e75',
 'sdro_fw_read':'48e740e0226f0014222f0018246f001c70024e4d4cdf07024e75',
 'sdro_fw_csd':'48e740e0226f0014246f0018720970044e4d4cdf07024e75'}
try:
    provenance=json.loads((root/'PROVENANCE.json').read_text())
    for path,sha in provenance['sources'].items():require(digest(repo/path)==sha,'source identity changed: '+path)
    loads=[Path(line.split(maxsplit=1)[1]).resolve() for line in (build/'combined.map').read_text().splitlines() if line.startswith('LOAD ')]
    require(loads==[(build/name).resolve() for name in owners],'unexpected link inputs or order')
    all_names=set();report={}
    for obj,expected in owners.items():
        path=build/obj;require(path.is_file(),'missing '+obj)
        names={line.split()[-1] for line in run('m68k-elf-rosco-nm','-g','--defined-only',str(path)).splitlines()}
        require(names==set(expected),'unexpected definitions/owners '+obj)
        require(not(all_names&names),'duplicate provider');all_names|=names
        elf(path)
        report[obj]={'sha256':digest(path),'globals':sorted(names)}
    for path in [build/'sd_ro_traps.o',build/'combined.o']:
        data,sections,sh,symbols=elf(path)
        for name,body in bodies.items():
            s=sections['.text.'+name];v,idx=symbols[name]
            require(sh[idx]==s and s[8]>=2 and v%2==0,'wrapper membership/alignment '+name)
            require(data[s[4]+v:s[4]+s[5]].hex()==body,'wrapper machine code changed '+name)
    undefined={x.split()[-1] for x in run('m68k-elf-rosco-nm','-u',str(build/'combined.o')).splitlines()}
    expected={'memcmp','memcpy','memset','__modsi3','__muldi3','__mulsi3','__udivdi3','__udivsi3','__umodsi3'}
    require(undefined==expected,'unexpected unresolved dependencies '+str(undefined))
    combined={x.split()[-1] for x in run('m68k-elf-rosco-nm','-g','--defined-only',str(build/'combined.o')).splitlines()}
    require(combined==all_names,'combined object owner mismatch')
    libgcc=run('m68k-elf-rosco-gcc','-mcpu=68010','-print-libgcc-file-name').strip()
    providers=run('m68k-elf-rosco-nm','-A','--defined-only',libgcc)
    for helper in expected-{'memcmp','memcpy','memset'}:require(any(l.split()[-1:]==[helper] for l in providers.splitlines()),'missing helper '+helper)
    # Host binary contains actual implementations and exactly the four firmware mocks.
    host_names={x.split()[-1] for x in run('nm','-g','--defined-only',str(build/'tests')).splitlines()}
    require(all_names<=host_names,'host implementation missing')
    # Compare actual dependency graph with pinned source inventory; compile no alternate FS.
    allowed={str((repo/p).resolve()) for p in provenance['sources']}
    allowed|={str(p.resolve()) for p in root.rglob('*') if p.is_file()}
    dependencies=[]
    for source in [repo/'code/software/newlib/m31a/fat_ro/src/fat_ro.c',repo/'code/software/newlib/m31a/sd_ro/src/sd_ro.c',repo/'code/software/newlib/m31a/sd_ro/src/csd_decode.c',root/'src/b3_integration.c']:
        out=run('cc','-MM','-I'+str(root/'include'),'-I'+str(root.parent/'fat_ro/include'),'-I'+str(root.parent/'sd_ro/include'),str(source))
        deps=out.replace('\\\n',' ').split(':',1)[1].split()
        for dep in deps:require(str(Path(dep).resolve()) in allowed,'unapproved project dependency '+dep)
        dependencies+=deps
    # Independently compile host production units; mocks and test allocator are excluded.
    host_imports=set()
    for obj in owners:
        if obj=='sd_ro_traps.o':continue
        source=repo/'code/software/newlib/m31a'/({'fat_ro.o':'fat_ro/src/fat_ro.c','sd_ro.o':'sd_ro/src/sd_ro.c','csd_decode.o':'sd_ro/src/csd_decode.c','b3_integration.o':'b3_integration/src/b3_integration.c'}[obj])
        output=build/('host-'+obj)
        run('cc','-std=c11','-O2','-Wall','-Wextra','-Werror','-Wpedantic','-I'+str(root/'include'),'-I'+str(root.parent/'fat_ro/include'),'-I'+str(root.parent/'sd_ro/include'),'-c',str(source),'-o',str(output))
        defined={x.split()[-1] for x in run('nm','-g','--defined-only',str(output)).splitlines()}
        require(defined==set(owners[obj]),'host owner mismatch '+obj)
        host_imports|={x.split()[-1] for x in run('nm','-u',str(output)).splitlines()}
    require(host_imports-all_names <= {'memcmp','memcpy','memset','__stack_chk_fail','_GLOBAL_OFFSET_TABLE_'},'unexpected host core imports')
    result={'result':'PASS','source_hashes_checked':len(provenance['sources']),'objects':report,'defined_owners':owners,'undefined':sorted(undefined),'libgcc_provider':libgcc,'wrapper_alignments':2,'wrapper_bytes':'exact F1 bodies; selectors 0/1/2/4; fixed CMD9','host_external_imports':sorted(host_imports-all_names),'dependencies':sorted(set(dependencies)),'limitations':['ET_REL does not prove final addresses or runtime behavior','No physical write-protection claim','No complete interrupt/firmware stack bound','Host mock and target ABI inspections are not hardware execution']}
    (build/'audit.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result,indent=2))
except Exception as exc:
    print('AUDIT FAIL:',exc,file=sys.stderr);sys.exit(1)
