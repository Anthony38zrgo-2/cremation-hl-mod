"""Check hostile-NPC alliance/damage policy in a separate hidden HLDS process."""
from pathlib import Path
import os
import subprocess
import time
import uuid
from validation_dll import using_validation_dll

ROOT = Path(__file__).resolve().parents[2]
MOD = ROOT / 'Cremation'
run = uuid.uuid4().hex
marker = 'EnemyCombatTest BEGIN ' + run
env = dict(os.environ, CREMATION_ENEMY_COMBAT_VALIDATE='1', CREMATION_ENEMY_COMBAT_RUN=run)
with using_validation_dll(MOD):
    with (MOD / 'validation-enemy-combat-engine.txt').open('wb') as output:
        process = subprocess.Popen(
            [str(ROOT / 'hlds.exe'), '-console', '-game', 'Cremation', '-insecure',
             '-nomaster', '-port', '27050', '-num_edicts', '4096', '+ip', '127.0.0.1',
             '+sv_lan', '1', '+maxplayers', '1', '+map', 'rapidcore'],
            cwd=ROOT, env=env, stdout=output, stderr=subprocess.STDOUT,
            creationflags=subprocess.CREATE_NO_WINDOW)
        try:
            deadline = time.monotonic() + 15
            while time.monotonic() < deadline and process.poll() is None:
                time.sleep(.25)
                text = (MOD / 'startup-diagnostics.txt').read_text(errors='replace').partition(marker)[2]
                if 'EnemyCombatTest COMPLETE' in text or 'EnemyCombatTest INCOMPLETE' in text:
                    break
            text = (MOD / 'startup-diagnostics.txt').read_text(errors='replace').partition(marker)[2]
            (MOD / 'validation-enemy-combat-diagnostics.txt').write_text(text)
            print('\n'.join(line for line in text.splitlines() if 'EnemyCombatTest' in line))
            assert process.poll() is None, f'HLDS exited: {process.returncode}'
            assert 'EnemyCombatTest COMPLETE' in text and 'EnemyCombatTest FAIL' not in text
            assert 'EnemyCombat ERROR' not in text and 'David ERROR' not in text
            print('PASS: enemy infighting/friendly damage blocked; player and player allies retain combat.')
        finally:
            if process.poll() is None:
                process.terminate()
            process.wait(timeout=5)
