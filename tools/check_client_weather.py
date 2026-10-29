"""Verify client weather: reddish fog and red rain in real GoldSrc engine."""
from pathlib import Path
import os
import subprocess
import time

ROOT = Path(__file__).resolve().parents[2] # Half-Life root
MOD = Path(__file__).resolve().parents[1] # Cremation root
diag_file = MOD / 'diagnostics/weather-diagnostics.txt'

if diag_file.exists():
    diag_file.unlink()

env = dict(os.environ)
env['PATH'] = r'C:\Program Files (x86)\Steam' + os.pathsep + env.get('PATH', '')

print('Starting GoldSrc client on cs_mansion...')
cmd = [
    str(ROOT / 'hl.exe'),
    '-game', 'Cremation',
    '-num_edicts', '2048',
    '-window', '-w', '800', '-h', '600',
    '-condebug', '-console',
    '+developer', '1',
    '+map', 'cs_mansion'
]

with (MOD / 'diagnostics/weather-test-client.log').open('wb') as log:
    proc = subprocess.Popen(
        cmd,
        cwd=ROOT,
        env=env,
        stdout=log,
        stderr=subprocess.STDOUT,
        creationflags=subprocess.CREATE_NO_WINDOW
    )
    try:
        deadline = time.monotonic() + 15
        found = False
        while time.monotonic() < deadline:
            if proc.poll() is not None:
                break
            if diag_file.exists():
                text = diag_file.read_text(errors='replace').strip()
                if text and 'fog=1' in text and 'rain=1' in text:
                    print(f'Weather Diagnostics Verified: {text}')
                    found = True
                    break
            time.sleep(0.5)

        assert found, f'Weather diagnostics file not generated or invalid (log: {MOD / "diagnostics/weather-test-client.log"})'
        print('PASS: Client successfully initialized on cs_mansion with reddish fog (fog=1) and red rain (rain=1)!')
    finally:
        if proc.poll() is None:
            proc.terminate()
            try:
                proc.wait(timeout=3)
            except Exception:
                proc.kill()
