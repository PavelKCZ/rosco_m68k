#!/usr/bin/env python3
"""Future explicit build driver. NEVER invoked in T2-I1.

All compiler/query/extraction/link/output operations happen only in main().
"""
import sys
sys.dont_write_bytecode = True
import argparse
import hashlib
import json
import os
import shutil
import subprocess
from pathlib import Path
from profile import (HERE, REPO, RUNTIME, PROGRAM, PREFIX, FLAGS, INCLUDES,
                     SOURCES, OBJECTS, TERM_RENAME, LINK_OPTIONS)

def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()

def external(value):
    if not value or not Path(value).is_absolute() or any(c.isspace() for c in value):
        raise ValueError('BUILD must be an explicit absolute, whitespace-free /tmp path')
    p = Path(value).resolve()
    if Path('/tmp') not in p.parents or p == Path('/tmp') or REPO in p.parents:
        raise ValueError('BUILD must resolve to a dedicated external directory below /tmp')
    return p

def verify_sources():
    manifest = HERE/'SHA256SUMS'
    entries = {}
    for line in manifest.read_text().splitlines():
        digest, name = line.split('  ', 1)
        if name in entries or Path(name).is_absolute() or '..' in Path(name).parts:
            raise ValueError('malformed source manifest')
        entries[name] = digest
        p = HERE/name
        if p.is_symlink() or not p.is_file() or sha(p) != digest:
            raise ValueError('profile source changed: '+name)
    actual = {str(p.relative_to(HERE)) for p in HERE.rglob('*') if p.is_file() and p != manifest}
    if actual != set(entries):
        raise ValueError('profile manifest inventory mismatch')
    provenance = json.loads((HERE/'PROVENANCE.json').read_text())
    for name, digest in provenance['protected_inputs'].items():
        if sha(REPO/name) != digest:
            raise ValueError('protected source identity changed: '+name)
    return provenance

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--build', required=True)
    parser.add_argument('action', choices=['compile', 'link', 'binary', 'inspect', 'audit', 'negative'])
    args = parser.parse_args()
    b = external(args.build)
    provenance = verify_sources()
    marker = b/'profile-state.json'
    if b.exists() and not marker.exists() and any(b.iterdir()):
        raise ValueError('refuse nonempty directory not owned by this profile')
    b.mkdir(parents=True, exist_ok=True)
    identity = {'profile': PROGRAM, 'source': str(HERE)}
    if marker.exists() and json.loads(marker.read_text()) != identity:
        raise ValueError('different build profile owner')
    marker.write_text(json.dumps(identity, indent=2)+'\n')
    record_path = b/'build-record.json'
    record = json.loads(record_path.read_text()) if record_path.exists() else {
        'origin_build': str(b), 'objects': {}, 'commands': [], 'profile': PROGRAM}

    def command(argv, output=None):
        done = subprocess.run(list(map(str, argv)), stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                              env=dict(os.environ, PYTHONDONTWRITEBYTECODE='1'))
        record['commands'].append({'argv': list(map(str, argv)), 'returncode': done.returncode,
                                   'stdout_sha256': hashlib.sha256(done.stdout).hexdigest(),
                                   'stderr': done.stderr.decode(errors='replace')})
        record_path.write_text(json.dumps(record, indent=2)+'\n')
        if output is not None:
            output.write_bytes(done.stdout)
        if done.returncode:
            sys.stderr.buffer.write(done.stderr)
            raise ValueError('tool returned '+str(done.returncode)+': '+str(argv[0])+
                             '; stdout evidence='+str(output))
        return done.stdout.decode(errors='replace') if output is None else ''

    cc = PREFIX+'gcc'
    libc = Path(command([cc, '-print-file-name=libc.a']).strip()).resolve()
    libgcc = Path(command([cc, '-print-libgcc-file-name']).strip()).resolve()
    librosco = libc.parent/'librosco.a'
    if not libc.is_file() or not libgcc.is_file() or not librosco.is_file():
        raise ValueError('missing installed runtime archives')
    record['archives'] = {str(p): sha(p) for p in [libc, libgcc, librosco]}
    if record['archives'] != provenance['archives']:
        raise ValueError('installed archive identity changed')
    record['flags'] = FLAGS
    record['compiler_version'] = command([cc, '--version'])
    if '13.3.1' not in record['compiler_version']:
        raise ValueError('unexpected compiler version')
    record['tools'] = {}
    for name in ['gcc','as','ld','ar','objcopy','objdump','readelf','nm','size']:
        executable = Path(shutil.which(PREFIX+name) or '')
        if not executable.is_file():
            raise ValueError('missing target tool: '+name)
        record['tools'][name] = {'path':str(executable.resolve()), 'sha256':sha(executable),
                                 'version':command([str(executable), '--version'])}
    if args.action == 'compile':
        record['objects'] = {}
        for obj, source in SOURCES.items():
            deps = b/(obj+'.d')
            argv = [cc, *FLAGS, *INCLUDES]
            if obj == 'termios.o':
                argv += TERM_RENAME
            argv += ['-MD', '-MP', '-MF', str(deps), '-c', str(source), '-o', str(b/obj)]
            command(argv)
            first = deps.read_text().replace('\\\n', ' ').splitlines()[0].split(':', 1)[1]
            headers = {str(Path(n).resolve()): sha(Path(n)) for n in first.split()}
            record['objects'][obj] = {'source': str(source), 'source_sha256': sha(source),
                                      'sha256': sha(b/obj), 'dependencies': headers, 'argv': argv}
        command([PREFIX+'ar', 'p', str(librosco), 'machine.o'], b/'machine.o')
        record['objects']['machine.o'] = {'archive': str(librosco), 'member': 'machine.o',
                                          'sha256': sha(b/'machine.o')}
    else:
        if set(record['objects']) != set(OBJECTS):
            raise ValueError('compile record incomplete')
        for obj, item in record['objects'].items():
            if sha(b/obj) != item['sha256']:
                raise ValueError('object changed: '+obj)
            for name, digest in item.get('dependencies', {}).items():
                if sha(Path(name)) != digest:
                    raise ValueError('dependency changed; recompile: '+name)
        if args.action == 'link':
            opts = ['-T', str(RUNTIME/'link.ld'), '-Map', str(b/(PROGRAM+'.map')), *LINK_OPTIONS]
            argv = [cc, *FLAGS, '-nostdlib', '-Wl,'+','.join(opts), '-o', str(b/(PROGRAM+'.elf'))]
            argv += [str(b/o) for o in OBJECTS]
            argv += ['-Wl,--start-group', str(libc), str(libgcc), '-Wl,--end-group']
            command(argv)
        elif args.action == 'binary':
            command([PREFIX+'objcopy', '-O', 'binary', str(b/(PROGRAM+'.elf')), str(b/(PROGRAM+'.bin'))])
        elif args.action == 'inspect':
            for tool, options, suffix in [('objdump', ['-dr'], 'dis'), ('readelf', ['-h','-l','-S','-s','-r','-W'], 'elf.txt'),
                                          ('nm', ['-n'], 'symbols.txt'), ('size', ['-A'], 'sizes.txt')]:
                command([PREFIX+tool, *options, str(b/(PROGRAM+'.elf'))], b/(PROGRAM+'.'+suffix))
        elif args.action in ('audit', 'negative'):
            tool = 'audit.py' if args.action == 'audit' else 'negative_audit.py'
            command([sys.executable, str(HERE/'tools'/tool), str(b)], b/(args.action+'.json'))
    record_path.write_text(json.dumps(record, indent=2)+'\n')

if __name__ == '__main__':
    try:
        main()
    except Exception as exc:
        print('BUILD STOP:', exc, file=sys.stderr)
        sys.exit(2)
