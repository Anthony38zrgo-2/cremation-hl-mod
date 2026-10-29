"""Validate the mansion light shut-off mechanic in real GoldSrc engine."""
from pathlib import Path
import os
import re
import subprocess
import time

ROOT = Path(__file__).resolve().parents[2]
MOD = ROOT / 'Cremation'
if __name__ == '__main__' and os.environ.get('CREMATION_VALIDATION_DLL_ACTIVE') != '1':
    from validation_dll import run_with_validation_dll
    run_with_validation_dll(MOD, Path(__file__).resolve())
    raise SystemExit

env = dict(os.environ, CREMATION_MANSION_LIGHT_TEST='1')
env['PATH'] = str(Path(os.environ.get('ProgramFiles(x86)', 'C:/Program Files (x86)')) / 'Steam') + os.pathsep + env['PATH']
(MOD / 'startup-diagnostics.txt').write_text('')

with (MOD / 'diagnostics/mansion-lights-server.log').open('wb') as log:
    process = subprocess.Popen([str(ROOT / 'hlds.exe'), '-console', '-game', 'Cremation',
                                '-num_edicts', '2048', '-insecure', '-nomaster', '-port', '27031',
                                '+ip', '127.0.0.1', '+sv_lan', '1', '+maxplayers', '1',
                                '+map', 'cs_mansion'], cwd=ROOT, env=env,
                               stdout=log, stderr=subprocess.STDOUT, stdin=subprocess.PIPE,
                               creationflags=subprocess.CREATE_NO_WINDOW)
    try:
        deadline = time.monotonic() + 35
        lights_off = []
        while time.monotonic() < deadline:
            if process.poll() is not None:
                raise RuntimeError(f'GoldSrc exited prematurely: {process.returncode}')
            phases = (MOD / 'startup-diagnostics.txt').read_text(errors='replace')
            lights_off = re.findall(r'MansionLightOff light=(\S+) style=(\d+) count=(\d+)/11 at=([\d.-]+),([\d.-]+),([\d.-]+)', phases)
            if len(lights_off) >= 11:
                break
            time.sleep(0.5)
        else:
            raise AssertionError(f'Mansion lights test timed out. Found only {len(lights_off)}/11 lights turned off. Trace:\n{phases}')

        print(f'PASS: All 11 mansion lights turned off sequentially in real GoldSrc engine!')
        for name, style, count, x, y, z in lights_off:
            print(f'  - Light #{count}: {name} (style {style}) at ({x}, {y}, {z})')
        assert len(lights_off) == 11
        assert {int(style) for _, style, _, _, _, _ in lights_off} == set(range(32, 43))
        assert 'FATAL ERROR' not in phases and 'Host_Error' not in phases
    finally:
        if process.poll() is None:
            process.terminate()
        process.wait(timeout=5)
