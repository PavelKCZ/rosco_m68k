#!/usr/bin/env python3
"""Maintained D3 explicit build. No compiler, filesystem writes or queries at import."""
import sys
sys.dont_write_bytecode = True
import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
from profile import HERE, REPO, RUNTIME, PROGRAM, PREFIX, FLAGS, INCLUDES, SOURCES, OBJECTS, TERM_RENAME, LINK_OPTIONS

INJECTION = ('CPATH','C_INCLUDE_PATH','CPLUS_INCLUDE_PATH','LIBRARY_PATH','COMPILER_PATH','GCC_EXEC_PREFIX','PYTHONPATH')

def sha(p):
    return hashlib.sha256(Path(p).read_bytes()).hexdigest()

def clean_environment():
    bad = [n for n in INJECTION if os.environ.get(n)]
    if bad:
        raise ValueError('ENV_INJECTION:'+','.join(bad))
    return dict(os.environ, PYTHONDONTWRITEBYTECODE='1', LC_ALL='C')

def safe_path(value):
    if not value or not Path(value).is_absolute() or any(c.isspace() or c == ',' for c in str(value)):
        raise ValueError('INVALID_PATH')
    return Path(value).resolve()

def external(value):
    p = safe_path(value)
    if Path('/tmp') not in p.parents or p == Path('/tmp') or REPO == p or REPO in p.parents:
        raise ValueError('EXTERNAL_BUILD_REQUIRED')
    return p

def pinned_file(root, name, digest):
    rel = Path(name)
    if rel.is_absolute() or '..' in rel.parts or not rel.parts:
        raise ValueError('PIN_PATH:'+name)
    p = root/rel
    if p.is_symlink() or not p.is_file() or root.resolve() not in p.resolve().parents or sha(p) != digest:
        raise ValueError('INPUT_HASH:'+name)
    return p

def verify_sources():
    safe_path(str(REPO))
    entries = {}
    for line in (HERE/'SHA256SUMS').read_text().splitlines():
        digest, name = line.split('  ', 1)
        if name in entries or name == 'SHA256SUMS' or len(digest) != 64:
            raise ValueError('MANIFEST_FORMAT')
        pinned_file(HERE, name, digest)
        entries[name] = digest
    actual = {str(p.relative_to(HERE)) for p in HERE.rglob('*') if p.is_file() and p != HERE/'SHA256SUMS'}
    if actual != set(entries):
        raise ValueError('MANIFEST_INVENTORY')
    v = json.loads((HERE/'PROVENANCE.json').read_text())
    if v['objects'] != OBJECTS or v['flags'] != FLAGS:
        raise ValueError('PROFILE_CONTRACT')
    if v['production_sources'] != {k:str(p.relative_to(REPO)) for k,p in SOURCES.items()}:
        raise ValueError('SOURCE_CONTRACT')
    for name,digest in v['protected_inputs'].items():
        pinned_file(REPO,name,digest)
    return v

def checked_tool(path, name, provenance):
    p = safe_path(str(path))
    if not p.is_file() or sha(p) != provenance['tools'][name]['sha256']:
        raise ValueError('TOOL_IDENTITY:'+name)
    return p

def libraries_match(archives, provenance):
    if set(archives) != {'libc.a','libgcc.a','librosco.a'}:
        raise ValueError('ARCHIVE_ROLES')
    for name,p in archives.items():
        if not Path(p).is_file() or sha(p) != provenance['archives'][name]:
            raise ValueError('ARCHIVE_IDENTITY:'+name)

def check_dependencies(deps, roots, provenance):
    for name,digest in deps.items():
        p = Path(name).resolve()
        if not p.is_file() or sha(p) != digest:
            raise ValueError('DEPENDENCY_CHANGED:'+name)
        if REPO in p.parents:
            expected = provenance['protected_inputs'].get(str(p.relative_to(REPO)))
        else:
            keys = [role+'/'+str(p.relative_to(Path(root))) for role,root in roots.items() if Path(root) in p.parents]
            expected = provenance['toolchain_headers'].get(keys[0]) if len(keys)==1 else None
        if expected != digest:
            raise ValueError('UNAPPROVED_DEPENDENCY:'+name)

def verify_historical_fixture(path):
    p = Path(path)
    if not p.is_file() or p.stat().st_size != 44980 or sha(p) != '066e667d849623ac682c8153542af1f688209f28c9f09a0bedaafba6943ebeb5':
        raise ValueError('BLOCKED: historical runtime BIN')
    return p.resolve()

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--build', required=True)
    parser.add_argument('--toolchain-bin')
    parser.add_argument('action',choices=['compile','link','binary','inspect'])
    args = parser.parse_args()
    env = clean_environment()
    provenance = verify_sources()
    b = external(args.build)
    if args.action == 'compile' and b.exists():
        raise ValueError('NEW_BUILD_DIRECTORY_REQUIRED')
    marker = b/'profile-state.json'
    contract = {'profile':PROGRAM,'repo':str(REPO),'manifest_sha256':sha(HERE/'SHA256SUMS')}
    if args.action != 'compile' and (not marker.is_file() or json.loads(marker.read_text()) != contract):
        raise ValueError('BUILD_OWNER_OR_INPUTS_CHANGED')
    directory = safe_path(args.toolchain_bin) if args.toolchain_bin else None
    tools = {}
    for name in provenance['tools']:
        path = directory/(PREFIX+name) if directory else shutil.which(PREFIX+name)
        if not path:
            raise ValueError('MISSING_TOOL:'+name)
        tools[name] = str(checked_tool(path,name,provenance))
    def query(argv):
        return subprocess.check_output(list(map(str,argv)),env=env,text=True).strip()
    versions = {n:query([p,'--version']) for n,p in tools.items()}
    if '13.3.1' not in versions['gcc'] or query([tools['gcc'],'-dumpmachine']) != 'm68k-elf-rosco':
        raise ValueError('COMPILER_VERSION')
    for n,v in versions.items():
        if n!='gcc' and '2.42' not in v:
            raise ValueError('BINUTILS_VERSION:'+n)
    cc = tools['gcc']
    libc = Path(query([cc,'-print-file-name=libc.a'])).resolve()
    libgcc = Path(query([cc,'-print-libgcc-file-name'])).resolve()
    archives = {'libc.a':str(libc),'libgcc.a':str(libgcc),'librosco.a':str(libc.parent/'librosco.a')}
    libraries_match(archives,provenance)
    roots = {'gcc_include':str(Path(query([cc,'-print-file-name=include'])).resolve()),'newlib_include':str((libc.parent.parent/'include').resolve())}
    if args.action == 'compile':
        b.mkdir(parents=True,exist_ok=False)
        marker.write_text(json.dumps(contract,indent=2)+'\n')
        record = {'origin_build':str(b),'profile':PROGRAM,'objects':{},'commands':[],'flags':FLAGS,
                  'compiler_version':versions['gcc'],'tools':{n:{'path':p,'sha256':sha(p),'version':versions[n]} for n,p in tools.items()},
                  'archives':{p:sha(p) for p in archives.values()},'archive_roles':archives,'toolchain_roots':roots,'input_manifest_sha256':contract['manifest_sha256']}
    else:
        record = json.loads((b/'build-record.json').read_text())
        if record['archive_roles'] != archives or record['tools'] != {n:{'path':p,'sha256':sha(p),'version':versions[n]} for n,p in tools.items()}:
            raise ValueError('BUILD_TOOLCHAIN_CHANGED')
        if set(record['objects']) != set(OBJECTS):
            raise ValueError('OBJECT_INVENTORY')
        for obj,item in record['objects'].items():
            if sha(b/obj) != item['sha256']:
                raise ValueError('OBJECT_HASH:'+obj)
            check_dependencies(item.get('dependencies',{}),roots,provenance)
    def command(argv, output=None):
        done = subprocess.run(list(map(str,argv)),stdout=subprocess.PIPE,stderr=subprocess.PIPE,env=env)
        record['commands'].append({'argv':list(map(str,argv)),'returncode':done.returncode,'stderr':done.stderr.decode(errors='replace'),'stdout_sha256':hashlib.sha256(done.stdout).hexdigest()})
        (b/'build-record.json').write_text(json.dumps(record,indent=2)+'\n')
        if done.returncode:
            raise ValueError('COMMAND_FAILED:'+str(argv[0])+':'+done.stderr.decode(errors='replace'))
        if output is not None:
            output.write_bytes(done.stdout)
    if args.action == 'compile':
        for obj,source in SOURCES.items():
            argv = [cc,*FLAGS,*INCLUDES]
            if obj=='termios.o':argv += TERM_RENAME
            argv += ['-MD','-MP','-MF',str(b/(obj+'.d')),'-c',str(source),'-o',str(b/obj)]
            command(argv)
            first = (b/(obj+'.d')).read_text().replace('\\\n',' ').splitlines()[0].split(':',1)[1]
            deps = {str(Path(n).resolve()):sha(n) for n in first.split()}
            check_dependencies(deps,roots,provenance)
            record['objects'][obj] = {'source':str(source),'source_sha256':sha(source),'sha256':sha(b/obj),'dependencies':deps,'argv':argv}
        command([tools['ar'],'p',archives['librosco.a'],'machine.o'],b/'machine.o')
        record['objects']['machine.o'] = {'archive':archives['librosco.a'],'member':'machine.o','sha256':sha(b/'machine.o')}
    elif args.action=='link':
        opts = ['-T',str(RUNTIME/'link.ld'),'-Map',str(b/(PROGRAM+'.map')),*LINK_OPTIONS]
        command([cc,*FLAGS,'-nostdlib','-Wl,'+','.join(opts),'-o',str(b/(PROGRAM+'.elf')),*[str(b/n) for n in OBJECTS],'-Wl,--start-group',str(libc),str(libgcc),'-Wl,--end-group'])
    elif args.action=='binary':
        command([tools['objcopy'],'-O','binary',str(b/(PROGRAM+'.elf')),str(b/(PROGRAM+'.bin'))])
    else:
        for tool,opts,suffix in [('objdump',['-dr'],'dis'),('readelf',['-h','-l','-S','-s','-r','-W'],'elf.txt'),('nm',['-n'],'symbols.txt'),('size',['-A'],'sizes.txt')]:
            command([tools[tool],*opts,str(b/(PROGRAM+'.elf'))],b/(PROGRAM+'.'+suffix))
    record['outputs'] = {str(p.relative_to(b)):{'sha256':sha(p),'bytes':p.stat().st_size} for p in [b/(PROGRAM+'.'+e) for e in ['elf','bin','map']] if p.is_file()}
    (b/'build-record.json').write_text(json.dumps(record,indent=2)+'\n')

if __name__=='__main__':
    try:main()
    except Exception as exc:
        print('BUILD BLOCKED: '+str(exc),file=sys.stderr)
        sys.exit(2)
