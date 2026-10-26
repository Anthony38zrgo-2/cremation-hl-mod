"""Verify original donor assets and capture the exterior in the real client."""
from pathlib import Path
import json,os,subprocess,time,sys,collections
from PIL import Image,ImageDraw
from bsp import BSP
from map_brushes import read_map
ROOT=Path(__file__).resolve().parents[2];MOD=ROOT/'Cremation'
props=json.loads((MOD/'assets-source/truth-exterior/props.json').read_text())
assert len(props)==9
for prop in props:assert Path(prop['source']).read_bytes()==(MOD/prop['installed']).read_bytes()
b=BSP(MOD/'maps/cs_mansion.bsp');names={e.get('targetname'):e for e in b.entities}
for i in range(4):
    assert names[f'truth_forest_{i}']['classname']=='func_illusionary'
    rear=names[f'truth_forest_rear_{i}'];assert rear['classname']=='func_illusionary'
    bounds=b.models[int(rear['model'][1:])]
    assert abs(bounds[2]-216)<2 and abs(bounds[5]-728)<2
for p in props:assert 'vostok_cover_truth_'+p['name'] in names
assert (MOD/'truth_textures.wad').read_bytes()==(ROOT/'czero/czde_truth.wad').read_bytes()
def mansion_brushes(path):
    shapes=collections.Counter()
    for e in read_map(path):
        for brush in e['brushes']:
            lo,hi=brush.bounds
            if lo[0]>=-736-.01 and hi[0]<=208+.01 and lo[1]>=-800-.01 and hi[1]<=288+.01:
                key=tuple(sorted(tuple(round(v,5) for v in [*f.normal,f.distance]) for f in brush.faces))
                shapes[key]+=1
    return shapes
assert mansion_brushes(MOD/'backups/before-truth-exterior/cs_mansion_reactor.map')==mansion_brushes(MOD/'assets-source/mansion-reactor/cs_mansion_reactor.map')
assert not b.clear((-2896,-256,279),0) and b.clear((-2896,-256,281),0)
assert not b.clear((-2896,-256,217),0)
for cx,cy in [(1088,1184),(-1792,-1696)]:
    assert b.line_clear((cx-120,cy,80),(cx+120,cy,80)),(cx,cy,'old shed walls remain')
    assert b.clear((cx,cy,100),0)
source=read_map(MOD/'assets-source/mansion-reactor/cs_mansion_reactor.map')
for rect,texture in [((-1184,-1088,480,576),'-0out_grss1'),((960,1088,1216,1280),'woods_plank_org'),((-1920,-1792,-1664,-1600),'woods_plank_org')]:
    matching=[]
    for brush in source[0]['brushes']:
        lo,hi=brush.bounds
        if all(abs(a-b)<.01 for a,b in zip((lo[0],lo[1],hi[0],hi[1]),rect)) and abs(hi[2]-24)<.01:
            matching += [f.texture.split()[0] for f in brush.faces if f.normal[2]<-.9]
    assert matching==[texture],(rect,matching)
print('PASS: Mansion geometry unchanged; open wooden pavilions; original Mansion grass; two separated forest layers.')
if '--static' in sys.argv:raise SystemExit(0)
existing=set(ROOT.glob('*.bmp'))|set(MOD.glob('*.bmp'))
env=dict(os.environ,CREMATION_TRUTH_VALIDATE='1');env['PATH']=r'C:\Program Files (x86)\Steam'+os.pathsep+env['PATH']
(MOD/'startup-diagnostics.txt').write_text('')
with (MOD/'diagnostics/truth-client.log').open('wb') as log:
    p=subprocess.Popen([str(ROOT/'hl.exe'),'-game','Cremation','-num_edicts','2048','-window','-w','1280','-h','800','-console','+exec','cremation.cfg'],cwd=ROOT,env=env,stdout=log,stderr=subprocess.STDOUT,creationflags=subprocess.CREATE_NO_WINDOW)
    try:
        deadline=time.monotonic()+40
        while time.monotonic()<deadline:
            if p.poll() is not None:raise RuntimeError(f'Client exited {p.returncode}')
            trace=(MOD/'startup-diagnostics.txt').read_text(errors='replace')
            if 'TruthTestComplete' in trace:break
            time.sleep(.25)
        else:raise AssertionError('Truth probe timed out: '+trace[-2000:])
        assert 'TruthTest FAIL' not in trace,trace
        print('PASS: nine original Truth props; '+str(trace.count('TruthTest PASS'))+' engine checks; six review captures.')
    finally:
        if p.poll() is None:p.terminate()
        p.wait(timeout=5)
files=sorted((set(ROOT.glob('*.bmp'))|set(MOD.glob('*.bmp')))-existing,key=lambda p:p.stat().st_mtime)
assert len(files)==6,files
sheet=Image.new('RGB',(1920,800));draw=ImageDraw.Draw(sheet)
for i,(path,label) in enumerate(zip(files,('facade','garden','pit','weapons-house','powerups-house','perimeter'))):
    pic=Image.open(path);pic.save(MOD/f'diagnostics/truth-{label}.png')
    x=(i%3)*640;y=(i//3)*400;sheet.paste(pic.resize((640,400)),(x,y));draw.text((x+12,y+12),label,fill='white')
sheet.save(MOD/'diagnostics/truth-exterior.png')
