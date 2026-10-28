"""Run native tests without uploading source to a hosted compiler."""
from pathlib import Path
import json
import os
import shutil
import subprocess

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT/'build/tests'
OUT.mkdir(parents=True, exist_ok=True)
compiler = os.environ.get('CXX') or shutil.which('c++') or shutil.which('clang++') or shutil.which('g++')
if not compiler:
    raise SystemExit('A native C++ compiler is required.')
for name in ['run_records.h', 'run_controls.h', 'speedrun.cpp', 'speedrun.h']:
    shutil.copyfile(ROOT/'src'/name, OUT/name)
for name in ['classic.h', 'emu.h', 'fileioc.h']:
    (OUT/name).write_text('#pragma once\n')
# Compile the actual runtime prefix, including init, pause rendering and update.
# Remaining drawing helpers and room data are replaced with explicit SDK fakes.
source = (ROOT/'src/emu.cpp').read_text()
(OUT/'runtime.inc').write_text(source[:source.index('void save_game()')])
for name in ['graphx.h', 'keypadc.h', 'fontlibc.h', 'debug.h', 'tice.h', 'profiler.h', 'practice.h', 'font.inc']:
    (OUT/name).write_text('')
(OUT/'gfx').mkdir(exist_ok=True)
(OUT/'gfx/gfx.h').write_text('')
results = []
for name in ['run_records_test', 'save_adapter_test', 'run_controls_test', 'runtime_pause_test']:
    shutil.copyfile(ROOT/'tests'/f'{name}.cpp', OUT/f'{name}.cpp')
    executable = OUT/(name+('.exe' if os.name == 'nt' else ''))
    subprocess.run([compiler, '-std=c++14', '-O2', '-Wall', '-Wextra', '-Werror',
                    '-I'+str(OUT), str(OUT/f'{name}.cpp'), '-o', str(executable)], check=True)
    result = subprocess.run([str(executable)], check=True, text=True, capture_output=True)
    print(result.stdout, end='')
    results.append({'test': name, 'passed': True, 'output': result.stdout})
(OUT/'results.json').write_text(json.dumps(results, indent=2))
