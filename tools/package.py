"""Package only reviewed install files; never include the parent workspace."""
from pathlib import Path
import hashlib
import json
import shutil
import zipfile

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT/'dist'
OUT.mkdir(exist_ok=True)
tests = json.loads((ROOT/'build/tests/results.json').read_text())
verification = json.loads((ROOT/'build/verification.json').read_text())
assert verification['passed'] and len(tests) == 4 and all(t['passed'] for t in tests)
shutil.copyfile(ROOT/'bin/CELESTE.8xp', OUT/'CELESTE.8xp')
for name in ['INSTALL.txt', 'LICENSE', 'THIRD_PARTY.md']:
    shutil.copyfile(ROOT/name, OUT/name)
(OUT/'TESTS.txt').write_text('\n'.join(t['output'] for t in tests)+
    '\nNative eZ80 build and binary/physics checks: PASS.\nCalculator/emulator gameplay testing: PENDING.\n')
names = ['CELESTE.8xp', 'clibs.8xg', 'INSTALL.txt', 'LICENSE', 'THIRD_PARTY.md', 'TESTS.txt']
assert all((OUT/n).is_file() for n in names)
(OUT/'SHA256SUMS.txt').write_text(''.join(hashlib.sha256((OUT/n).read_bytes()).hexdigest()+'  '+n+'\n' for n in names))
archive = OUT/'CEleste-Speedrun-install.zip'
with zipfile.ZipFile(archive, 'w', zipfile.ZIP_DEFLATED) as z:
    for name in names+['SHA256SUMS.txt']:
        z.write(OUT/name, name)
with zipfile.ZipFile(archive) as z:
    assert z.testzip() is None
    for row in z.read('SHA256SUMS.txt').decode().splitlines():
        digest, name = row.split('  ')
        assert hashlib.sha256(z.read(name)).hexdigest() == digest
print('PASS install ZIP CRC and SHA256 manifest:', archive.name)
