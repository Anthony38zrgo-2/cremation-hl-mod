"""Bounded actual GoldSrc projectile, damage, fall and recovery acceptance probe."""
from pathlib import Path
import os, subprocess

ROOT = Path(__file__).resolve().parents[2]
if __name__=='__main__' and os.environ.get('CREMATION_VALIDATION_DLL_ACTIVE')!='1':
    from validation_dll import run_with_validation_dll
    run_with_validation_dll(ROOT/'Cremation',Path(__file__).resolve());raise SystemExit
env = dict(os.environ, CREMATION_QZ_VALIDATE='1')
log = ROOT / 'Cremation/validation-quake-zombie-engine.txt'
with log.open('wb') as output:
    p = subprocess.Popen([str(ROOT/'hlds.exe'), '-console', '-game', 'Cremation',
        '-insecure', '-nomaster', '-port', '27029', '+ip', '127.0.0.1',
        '+sv_lan', '1', '+maxplayers', '1', '+map', 'rapidcore'], cwd=ROOT,
        env=env, stdout=output, stderr=subprocess.STDOUT,
        creationflags=subprocess.CREATE_NO_WINDOW)
    try:
        try:
            p.wait(timeout=20)
            raise AssertionError(f'GoldSrc exited unexpectedly: {p.returncode}')
        except subprocess.TimeoutExpired:
            diagnostics = (ROOT/'Cremation/startup-diagnostics.txt').read_text()
            lines = [line for line in diagnostics.splitlines() if 'QuakeZombieTest' in line]
            print('\n'.join(lines))
            assert len(lines) == 9 and all('Test PASS' in line for line in lines), diagnostics
            print('PASS: 20-second real GoldSrc projectile/damage/knockdown/recovery/gib acceptance.')
    finally:
        if p.poll() is None: p.terminate()
        p.wait(timeout=5)
