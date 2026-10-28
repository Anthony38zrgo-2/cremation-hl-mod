"""Recover Randy's original model, alpha textures and motion banks from HOTD2 PC."""
from pathlib import Path
import collections, hashlib, json, sys
import numpy as np
from PIL import Image

MOD = Path(__file__).resolve().parents[1]
BASE = MOD / 'assets-source/hotd2-randy'
SOURCE = BASE / 'source'
sys.path.insert(0, str(MOD / 'tools'))
sys.path.insert(0, r'D:\games\House_of_the_dead_2_ISO\David_analysis\hotd2-re-reference\tools')
import extract_hotd2_johnny as shared
from tex_export import Exe, descriptors, texture_files

shared.KIND = 0x19
shared.POL_RESOURCE = 'zstin.bin'
shared.TEXTURE_RESOURCE = 'zstin.bin'

def main():
    exe = Exe(r'D:\games\House_of_the_dead_2_ISO\HOD2.EXE')
    root, count = shared.skeleton(exe, shared.KIND)
    nodes = []
    def visit(node, parent=None):
        nodes.append(dict(index=node.index, model_id=node.model_id, offset=node.offset, flags=node.flags, parent=parent))
        for child in node.children: visit(child, node.index)
    visit(root)
    if count != 16 or len(nodes) != count: raise ValueError('Unexpected Randy skeleton')
    geometry, encoding = shared.raw_or_prs(SOURCE / 'F12999_zstin.bin')
    bank, _ = shared.raw_or_prs(SOURCE / 'F13800_zstin.bin')
    pol_index, ids, models = shared.model_data(exe, geometry, nodes)
    bank_models = len(ids)
    attachments = shared.attachment_data(exe, geometry, models)
    active = {n['model_id'] for n in nodes if n['model_id']} | {a['model_id'] for a in attachments}
    models = [m for m in models if m['id'] in active]
    row = next(row for row in texture_files(exe) if row[1] == 'zstin.bin')
    uses = collections.defaultdict(list)
    for model in models:
        for mi, mat in enumerate(model['materials']):
            if mat['texture'] != 0xffffffff:
                uses[mat['texture']].append(dict(model=model['id'], material=mi,
                    tsp=f"0x{mat['words'][2]:08X}", uses_texture_alpha=(mat['words'][2]&0x180000)!=0x80000))
    texture_dir = BASE / 'textures'; texture_dir.mkdir(parents=True, exist_ok=True)
    textures = []
    for desc in descriptors(exe, row[2], row[3]):
        width, height, data = shared.decode_texture(bank, desc)
        rgba = shared.rgba_bytes(data, desc['fmt'] & 7)
        name = f"Randy_tex_{desc['index']:02d}_id{desc['id']}.png"
        Image.frombytes('RGBA',(width,height),rgba).transpose(Image.Transpose.FLIP_TOP_BOTTOM).save(texture_dir/name)
        alpha=np.frombuffer(rgba,dtype=np.uint8).reshape(-1,4)[:,3]
        textures.append({**desc, 'path':f'textures/{name}', 'alpha_zero':int((alpha==0).sum()),
                         'alpha_partial':int(((alpha>0)&(alpha<255)).sum()), 'material_uses':uses[desc['index']]})
    banks = [('ans.bin','F12632_ans.bin',tuple(range(0x0,0xb))),
             ('szom.bin','F12585_szom.bin',tuple(range(0x11a,0x13d))+tuple(range(0x3a1,0x3ac))),
             ('zom.bin','F12583_zom.bin',(0x3db,0x3da,0x3cc,0x3dc,0x3de,0x3f0))]
    clips=[]
    for name, filename, ids in banks:
        clips += shared.extract_motions(exe,count,ids,SOURCE/filename,name,{})
    result=dict(identity=dict(name='Randy',kind=25,nodes=count), geometry=dict(resource='zstin.bin',file_index=pol_index,
                bank_models=bank_models, active_models=len(models),decoding=encoding),
                source_motion_sets=dict(idle='0x7',run='0xA (ans.bin; original Randy leaper locomotion)',jump='0x3',attack=['0x9','0x8'],death='0x11D'),
                alpha_policy='Preserve alpha for source materials with an alpha-enabled TSP pass.',
                textures=textures,models=models,nodes=nodes,attachments=attachments,clips=clips)
    (BASE/'Randy_extracted.json').write_text(json.dumps(result,separators=(',',':')),encoding='utf8')
    (BASE/'Randy_alpha_audit.json').write_text(json.dumps(textures,indent=2)+'\n',encoding='utf8')
    print(f'Recovered Randy: {len(models)} active meshes, {len(textures)} textures, {len(clips)} original clips; alpha-bearing textures: {sum(bool(t["alpha_zero"] or t["alpha_partial"]) for t in textures)}')

if __name__=='__main__': main()
