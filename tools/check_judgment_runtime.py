"""Exercise Judgment's ground/air movement, shared damage and phase changes in HLDS."""
from pathlib import Path
import os
import re
import subprocess
import time

MOD = Path(__file__).resolve().parents[1]
ROOT = MOD.parent

if __name__ == '__main__' and os.environ.get('CREMATION_VALIDATION_DLL_ACTIVE') != '1':
    from validation_dll import run_with_validation_dll
    run_with_validation_dll(MOD, Path(__file__).resolve())
    raise SystemExit

trace = MOD / 'startup-diagnostics.txt'
trace.write_text('')
env = dict(os.environ, CREMATION_MANSION_VALIDATE='5')
with (MOD / 'diagnostics/judgment-server.log').open('wb') as log:
    process = subprocess.Popen([str(ROOT/'hlds.exe'), '-console', '-game', 'Cremation',
                                '-num_edicts', '2048', '-insecure', '-nomaster', '-port', '27029',
                                '+ip', '127.0.0.1', '+sv_lan', '1', '+maxplayers', '1',
                                '+map', 'cs_mansion_wave4'], cwd=ROOT, env=env,
                               stdout=log, stderr=subprocess.STDOUT, stdin=subprocess.PIPE,
                               creationflags=subprocess.CREATE_NO_WINDOW)
    try:
        deadline = time.monotonic() + 35
        while time.monotonic() < deadline:
            if process.poll() is not None:
                raise RuntimeError(f'GoldSrc exited: {process.returncode}')
            phases = trace.read_text(errors='replace')
            if 'JudgmentProbeComplete' in phases:
                break
            time.sleep(.25)
        else:
            raise AssertionError('Judgment runtime fixture timed out\n' + phases[-6000:])
        (MOD/'diagnostics/judgment-runtime-trace.log').write_text(phases)
        checks = re.findall(r'JudgmentProbe(PASS|FAIL) (.*)', phases)
        print('\n'.join(status + ': ' + detail for status, detail in checks))
        print('\n'.join(re.findall(r'JudgmentProbeMovement .*', phases)))
        assert checks and all(status == 'PASS' for status, _ in checks), phases[-6000:]
        assert 'JudgmentProbeComplete failures=0' in phases
        log.flush()
        engine = (MOD/'diagnostics/judgment-server.log').read_text(errors='replace')
        assert 'not precached' not in engine and 'FATAL ERROR' not in engine, engine
        print('PASS: real GoldSrc movement, grounded animated feet, flying Zeal, 1500 HP and shared damage/phase/death.')
    finally:
        if process.poll() is None:
            process.terminate()
        process.wait(timeout=5)
