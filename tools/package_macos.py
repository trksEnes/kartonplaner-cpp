#!/usr/bin/env python3
"""Bundle an already-built app with its runtime, then verify every load path."""
from pathlib import Path
import os
import plistlib
import shutil
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]
QT = Path(os.environ.get('QT_ROOT', '/opt/homebrew/opt/qt'))
SOURCE = ROOT / 'KartonPlaner.app'
STAGE = ROOT / 'build/package/KartonPlaner.app'

def run(*args):
    subprocess.run([str(a) for a in args], check=True)

def mach_o(path):
    if not path.is_file() or path.is_symlink():
        return False
    with path.open('rb') as handle:
        return handle.read(4) in (b'\xcf\xfa\xed\xfe', b'\xfe\xed\xfa\xcf', b'\xca\xfe\xba\xbe')

if sys.platform != 'darwin':
    raise SystemExit('Dieses Werkzeug bündelt nur die macOS-App.')
if not (SOURCE / 'Contents/MacOS/KartonPlaner').exists():
    raise SystemExit('Zuerst tools/build.sh app ausführen.')
if STAGE.exists():
    shutil.rmtree(STAGE)
(STAGE / 'Contents').mkdir(parents=True)
for entry in ('MacOS', 'Resources'):
    src = SOURCE / 'Contents' / entry
    if src.exists():
        shutil.copytree(src, STAGE / 'Contents' / entry)
shutil.copy2(SOURCE / 'Contents/Info.plist', STAGE / 'Contents/Info.plist')
info = STAGE / 'Contents/Info.plist'
values = plistlib.loads(info.read_bytes())
values.update(LSMinimumSystemVersion='14.0', CFBundleDevelopmentRegion='de', CFBundleShortVersionString='1.0.0')
info.write_bytes(plistlib.dumps(values))
resources = STAGE / 'Contents/Resources'
resources.mkdir(exist_ok=True)
if (ROOT / 'licenses').exists():
    shutil.copytree(ROOT / 'licenses', resources / 'licenses', dirs_exist_ok=True)
for notice in ('THIRD_PARTY_NOTICES.md', 'LICENSE'):
    if (ROOT / notice).exists():
        shutil.copy2(ROOT / notice, resources / notice)
    else:
        (resources / notice).unlink(missing_ok=True)
# Re-running packaging must also work on an executable previously rewritten by macdeployqt.
executable = STAGE / 'Contents/MacOS/KartonPlaner'
for line in subprocess.check_output(['otool', '-L', str(executable)], text=True).splitlines()[1:]:
    dependency = line.strip().split(' (')[0]
    prefix = next((prefix for prefix in ('@rpath/', '@executable_path/../Frameworks/') if dependency.startswith(prefix + 'Qt')), None)
    if prefix:
        original = QT / 'lib' / dependency[len(prefix):]
        if original.exists():
            run('install_name_tool', '-change', dependency, original, executable)
# Only the two platform plugins used by this Widgets app are necessary.
run(QT / 'bin/macdeployqt', STAGE, '-no-plugins', f'-libpath={QT / "lib"}')
plugins = STAGE / 'Contents/PlugIns/platforms'
plugins.mkdir(parents=True, exist_ok=True)
for name in ('libqcocoa.dylib', 'libqoffscreen.dylib'):
    dest = plugins / name
    shutil.copy2(QT / 'share/qt/plugins/platforms' / name, dest)
    run('install_name_tool', '-add_rpath', '@loader_path/../../Frameworks', dest)
frameworks = STAGE / 'Contents/Frameworks'
# Some Homebrew transitive dylibs use @rpath although macdeployqt does not copy
# the referenced file. Resolve these before validating the finished bundle.
for _ in range(8):
    added = False
    for binary in list(p for p in STAGE.rglob('*') if mach_o(p)):
        for line in subprocess.check_output(['otool', '-L', str(binary)], text=True).splitlines()[1:]:
            dependency = line.strip().split(' (')[0]
            absolute = dependency.startswith(('/opt/', '/usr/local/'))
            if dependency.startswith('@rpath/') or absolute:
                if '.framework/' in dependency:
                    relative = dependency[dependency.rfind('/', 0, dependency.index('.framework/')) + 1:] if absolute else dependency[7:]
                else:
                    relative = Path(dependency).name if absolute else dependency[7:]
                target = frameworks / relative
                if not target.exists():
                    source = Path(dependency) if absolute else next((base / relative for base in (QT / 'lib', Path('/opt/homebrew/lib')) if (base / relative).exists()), None)
                    if source and source.exists():
                        if '.framework/' in relative:
                            framework_name = relative.split('/')[0]
                            source_framework = next(parent for parent in source.parents if parent.name == framework_name)
                            shutil.copytree(source_framework, frameworks / framework_name, symlinks=True)
                        else:
                            shutil.copy2(source, target)
                        added = True
                ids = subprocess.check_output(['otool', '-D', str(binary)], text=True).splitlines()[1:]
                if absolute and target.exists() and dependency not in [s.strip() for s in ids]:
                    run('install_name_tool', '-change', dependency, '@rpath/' + relative, binary)
    if not added:
        break
for binary in (p for p in STAGE.rglob('*') if mach_o(p)):
    listing = subprocess.check_output(['otool', '-L', str(binary)], text=True).splitlines()[1:]
    ids = subprocess.check_output(['otool', '-D', str(binary)], text=True).splitlines()[1:]
    own_id = ids[0].strip() if ids else None
    if own_id and binary.is_relative_to(frameworks):
        run('install_name_tool', '-id', '@rpath/' + str(binary.relative_to(frameworks)), binary)
    for line in listing:
        dependency = line.strip().split(' (')[0]
        if dependency == own_id:
            continue
        if dependency.startswith(('/opt/', '/usr/local/')):
            raise SystemExit(f'Ungebündelte Abhängigkeit: {binary}: {dependency}')
        if dependency.startswith('@rpath/') and not (frameworks / dependency[7:]).exists():
            raise SystemExit(f'Fehlende Abhängigkeit: {binary}: {dependency}')
    load_commands = subprocess.check_output(['otool', '-l', str(binary)], text=True)
    for line in load_commands.splitlines():
        if line.strip().startswith('path /opt/'):
            run('install_name_tool', '-delete_rpath', line.strip().split()[1], binary)
run('codesign', '--force', '--deep', '--sign', '-', STAGE)
run('codesign', '--verify', '--deep', '--strict', STAGE)
# Only replace the existing bundle once packaging and verification succeeded.
shutil.rmtree(SOURCE)
shutil.copytree(STAGE, SOURCE, symlinks=True)
print('Mac-App gebündelt und Abhängigkeiten geprüft:', SOURCE)
