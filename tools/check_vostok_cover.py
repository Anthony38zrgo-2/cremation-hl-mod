"""Check original resources, native animation and cover collision; capture four views."""
from pathlib import Path
import hashlib,json,os,subprocess,time
from PIL import Image,ImageDraw
ROOT=Path(__file__).resolve().parents[2];MOD=ROOT/'Cremation'
if __name__=='__main__' and os.environ.get('CREMATION_VALIDATION_DLL_ACTIVE')!='1':
    from validation_dll import run_with_validation_dll
    run_with_validation_dll(MOD,Path(__file__).resolve());raise SystemExit
for prop in json.loads((MOD/'assets-source/vostok-cover/installed.json').read_text()):
    assert Path(prop['source']).read_bytes()==(MOD/prop['installed']).read_bytes()
existing=set(ROOT.glob('*.bmp'))|set(MOD.glob('*.bmp'))
env=dict(os.environ,CREMATION_VOSTOK_VALIDATE='1');env['PATH']=r'C:\Program Files (x86)\Steam'+os.pathsep+env['PATH']
(MOD/'startup-diagnostics.txt').write_text('')
with (MOD/'diagnostics/vostok-client.log').open('wb') as log:
    p=subprocess.Popen([str(ROOT/'hl.exe'),'-game','Cremation','-num_edicts','2048','-window','-w','1280','-h','800','-console','+exec','cremation.cfg'],cwd=ROOT,env=env,stdout=log,stderr=subprocess.STDOUT,creationflags=subprocess.CREATE_NO_WINDOW)
    try:
        deadline=time.monotonic()+35
        while time.monotonic()<deadline:
            if p.poll() is not None:raise RuntimeError(f'Client exited {p.returncode}')
            trace=(MOD/'startup-diagnostics.txt').read_text(errors='replace')
            if 'VostokTestComplete' in trace:break
            time.sleep(.25)
        else:raise AssertionError('Vostok probe timed out: '+trace[-2000:])
        assert 'VostokTest FAIL' not in trace,trace
        print('PASS: original models unchanged; '+str(trace.count('VostokTest PASS'))+' native animation/collision checks.')
    finally:
        if p.poll() is None:p.terminate()
        p.wait(timeout=5)
files=sorted((set(ROOT.glob('*.bmp'))|set(MOD.glob('*.bmp')))-existing,key=lambda p:p.stat().st_mtime)
assert len(files)==4,files
sheet=Image.new('RGB',(1280,800));draw=ImageDraw.Draw(sheet)
for i,(path,label) in enumerate(zip(files,('jeep','tree-spawn','tree-north','tree-east'))):
    pic=Image.open(path);pic.save(MOD/f'diagnostics/vostok-{label}.png')
    x=(i%2)*640;y=(i//2)*400;sheet.paste(pic.resize((640,400)),(x,y));draw.text((x+12,y+12),label,fill='white')
sheet.save(MOD/'diagnostics/vostok-cover.png')
