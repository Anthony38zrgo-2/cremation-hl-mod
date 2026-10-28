"""Read back the compiled Randy model and compare its native animation contract."""
from pathlib import Path
import json, struct
import numpy as np
from PIL import Image
from verify_david_assets import Studio, source_pose

MOD=Path(__file__).resolve().parents[1]
BASE=MOD/'assets-source/hotd2-randy'

def main():
    mdl=Studio(MOD/'models/cremation/hotd2/randy/randy.mdl')
    native=Studio(MOD/'models/zombie_fast.mdl')
    report=json.loads((BASE/'conversion-report.json').read_text())
    assert mdl.i(140)==16 and mdl.i(156)==15 and mdl.i(164)==native.i(164)==9
    errors=[]
    for seq,entry in enumerate(report['sequences']):
        at=mdl.i(168)+seq*176; donor=native.i(168)+seq*176
        assert mdl.data[at:at+32].split(b'\0')[0]==native.data[donor:donor+32].split(b'\0')[0]
        assert mdl.i(at+40)==native.i(donor+40) and (mdl.i(at+36)&1)==(native.i(donor+36)&1)
        fps=struct.unpack_from('<f',mdl.data,at+32)[0]; donor_fps=struct.unpack_from('<f',native.data,donor+32)[0]
        duration=(mdl.i(at+56)-1)/fps
        expected=52/60 if entry['name']=='run1' else (native.i(donor+56)-1)/donor_fps
        assert abs(duration-expected)<.00001,(entry['name'],duration,expected)
        event_ids=[mdl.i(mdl.i(at+52)+i*76+4) for i in range(mdl.i(at+48))]
        donor_events=[native.i(native.i(donor+52)+i*76+4) for i in range(native.i(donor+48))]
        assert event_ids==donor_events,(entry['name'],event_ids,donor_events)
        for frame in sorted({0,(entry['frames']-1)//2,entry['frames']-1}):
            r,p=mdl.pose(seq,frame);rs,ps=source_pose(BASE/'goldsrc'/f'{entry["name"]}.smd',frame)
            error=float(np.linalg.norm(p-ps,axis=1).max());assert error<.1,(entry['name'],frame,error)
            assert float(np.max(np.abs(r-rs)))<.003
            errors.append(error)
    out=BASE/'compiled-preview';out.mkdir(exist_ok=True);textures=[];masked=[]
    for i in range(mdl.i(180)):
        at=mdl.i(184)+i*80;name=mdl.data[at:at+64].split(b'\0')[0].decode()
        flags,w,h,off=struct.unpack_from('<4i',mdl.data,at+64)
        image=Image.frombytes('P',(w,h),mdl.data[off:off+w*h]);image.putpalette(mdl.data[off+w*h:off+w*h+768])
        if name.startswith('{'):
            assert flags&64;image.info['transparency']=255;masked.append(name)
        path=out/f'tex_{i:02d}.png';image.save(path);textures.append(str(path))
    poses=[]
    for name,seq,frame in [('Randy',0,0),('Carrera',1,5),('Salto',6,12)]:
        pose=mdl.geometry(seq,frame);pose['name']=name;poses.append(pose)
    (BASE/'compiled-preview.json').write_text(json.dumps(dict(textures=textures,poses=poses),separators=(',',':')))
    result=dict(bones=16,hitboxes=15,sequences=9,compiled_triangles=len(poses[0]['faces']),
                max_position_error=max(errors),pose_samples=len(errors),masked_textures=masked,
                native_event_and_activity_contract=True)
    (BASE/'compiled-validation.json').write_text(json.dumps(result,indent=2)+'\n')
    print(f'PASS: {len(errors)} source/compiled poses; 15 hitboxes; native fast-zombie activities/events and Randy original run timing preserved.')

if __name__=='__main__':main()
