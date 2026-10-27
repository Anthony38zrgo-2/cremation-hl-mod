"""Exercise real native death scheduling in a separate, hidden HLDS process."""
from pathlib import Path
import os
import subprocess
import time
import uuid
from validation_dll import using_validation_dll

ROOT = Path(__file__).resolve().parents[2]
MOD = ROOT / 'Cremation'
log = MOD / 'validation-hotd2-deaths-engine.txt'
run = uuid.uuid4().hex
marker = 'Hotd2Death BEGIN ' + run
env = dict(os.environ, CREMATION_DAVID_VALIDATE='2', CREMATION_DEATH_RUN=run)
with using_validation_dll(MOD):
    with log.open('wb') as output:
        process = subprocess.Popen(
            [str(ROOT / 'hlds.exe'), '-console', '-game', 'Cremation', '-insecure',
             '-nomaster', '-port', '27049', '-num_edicts', '4096', '+ip', '127.0.0.1',
             '+sv_lan', '1', '+maxplayers', '1', '+map', 'rapidcore'],
            cwd=ROOT, env=env, stdout=output, stderr=subprocess.STDOUT,
            creationflags=subprocess.CREATE_NO_WINDOW)
        try:
            deadline = time.monotonic() + 15
            while time.monotonic() < deadline and process.poll() is None:
                time.sleep(.25)
                text = (MOD / 'startup-diagnostics.txt').read_text(errors='replace')
                own = text.partition(marker)[2]
                if 'Hotd2Death COMPLETE' in own or 'Hotd2Death INCOMPLETE' in own:
                    break
            text = (MOD / 'startup-diagnostics.txt').read_text(errors='replace')
            text = text.partition(marker)[2]
            (MOD / 'validation-hotd2-deaths-diagnostics.txt').write_text(text)
            print('\n'.join(line for line in text.splitlines() if 'Hotd2Death' in line))
            assert process.poll() is None, f'HLDS exited: {process.returncode}; see {log}'
            assert 'Hotd2Death COMPLETE' in text and 'Hotd2Death FAIL' not in text
            print('PASS: six native death scenarios retain one continuous clip, finish grounded and reject duplicate/conflicting death restarts.')
        finally:
            if process.poll() is None:
                process.terminate()
            process.wait(timeout=5)
