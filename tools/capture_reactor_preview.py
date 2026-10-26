"""Capture four real-engine screenshots in an isolated preview game process."""
from pathlib import Path
import os
import subprocess
import time
from PIL import Image

ROOT=Path(__file__).resolve().parents[2]
MOD=ROOT/'Cremation'
existing=set(ROOT.glob('*.bmp'))|set(MOD.glob('*.bmp'))
env=dict(os.environ,CREMATION_REACTOR_PREVIEW='1')
env['PATH']=r'C:\Program Files (x86)\Steam'+os.pathsep+env['PATH']
(MOD/'startup-diagnostics.txt').write_text('')
with (MOD/'diagnostics/reactor-preview-client.log').open('wb') as log:
    p=subprocess.Popen([str(ROOT/'hl.exe'),'-game','Cremation','-num_edicts','2048','-window','-w','1280','-h','800','-condebug','-console','+developer','1','+exec','cremation.cfg'],cwd=ROOT,env=env,stdout=log,stderr=subprocess.STDOUT,creationflags=subprocess.CREATE_NO_WINDOW)
    try:
        deadline=time.monotonic()+30
        while time.monotonic()<deadline:
            if p.poll() is not None:raise RuntimeError(f'Client exited: {p.returncode}')
            trace=(MOD/'startup-diagnostics.txt').read_text(errors='replace')
            if 'ReactorPreviewComplete' in trace:
                time.sleep(.5)
                break
            time.sleep(.25)
        else:raise AssertionError('Screenshot preview timed out')
    finally:
        if p.poll() is None:p.terminate()
        p.wait(timeout=5)
files=sorted((set(ROOT.glob('*.bmp'))|set(MOD.glob('*.bmp')))-existing,key=lambda path:path.stat().st_mtime)
assert len(files)==4,files
for path,name in zip(files,('overview','rewards','powerups','bottom')):
    target=MOD/f'diagnostics/reactor-preview-{name}.png'
    Image.open(path).save(target)
    print(target)
