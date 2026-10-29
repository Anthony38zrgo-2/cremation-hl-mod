"""Bounded HLDS check of mansion loadout, supplies, stations and tactical revival."""
from pathlib import Path
import json
import os
import subprocess
import time

MOD = Path(__file__).resolve().parents[1]
ROOT = MOD.parent
if __name__ == '__main__' and os.environ.get('CREMATION_VALIDATION_DLL_ACTIVE') != '1':
    from validation_dll import run_with_validation_dll
    run_with_validation_dll(MOD, Path(__file__).resolve())
    raise SystemExit

env = dict(os.environ, CREMATION_RULES_VALIDATE='1')
env['PATH'] = r'C:\Program Files (x86)\Steam' + os.pathsep + env['PATH']
trace_path = MOD / 'startup-diagnostics.txt'
trace_path.write_text('')
with (MOD / 'diagnostics/mansion-rules-server.log').open('wb') as log:
    process = subprocess.Popen([
        str(ROOT / 'hlds.exe'), '-console', '-game', 'Cremation',
        '-num_edicts', '2048', '-insecure', '-nomaster', '-port', '27029',
        '+ip', '127.0.0.1', '+sv_lan', '1', '+maxplayers', '1', '+map', 'cs_mansion'
    ], cwd=ROOT, env=env, stdout=log, stderr=subprocess.STDOUT,
        stdin=subprocess.PIPE, creationflags=subprocess.CREATE_NO_WINDOW)
    try:
        deadline = time.monotonic() + 35
        while time.monotonic() < deadline:
            if process.poll() is not None:
                raise RuntimeError(f'HLDS exited: {process.returncode}')
            trace = trace_path.read_text(errors='replace') if trace_path.exists() else ''
            if 'MansionRulesComplete' in trace:
                break
            time.sleep(.25)
        else:
            raise AssertionError('Mansion rule checks timed out')
        checks = [line for line in trace.splitlines() if line.startswith('MansionRules ')]
        print('\n'.join(checks))
        (MOD / 'diagnostics/mansion-rules-trace.log').write_text(trace)
        assert len(checks) == 13 and all(' PASS ' in line for line in checks), trace[-8000:]
        (MOD / 'diagnostics/mansion-rules-runtime.json').write_text(
            json.dumps(dict(checks=checks, passed=True), indent=2) + '\n')
    finally:
        if process.poll() is None:
            process.terminate()
        process.wait(timeout=5)
