"""Project the image-generator panorama to a GoldSrc cubemap (no repainting)."""
from pathlib import Path
import hashlib
import json
import numpy as np
from PIL import Image
from bsp import BSP

MOD = Path(__file__).resolve().parents[1]
SOURCE = MOD/'assets-source/skybox-mountains/city-mountains-panorama.png'
SKY = 'cremation_mountains'


def install(update_map=True):
    source = np.asarray(Image.open(SOURCE).convert('RGB'), dtype=np.float32)
    h, w = source.shape[:2]
    assert w == 2*h, (w, h)
    # Cube borders alone cannot catch an internal equirectangular wrap seam
    # or a pinched zenith. Validate the image-editor's repaired source too.
    wrap_delta = np.abs(source[:,0]-source[:,-1])
    zenith_range = float(np.ptp(source[0],axis=0).max())
    assert wrap_delta.max() <= 16 and wrap_delta.mean() <= 2, 'Panorama wrap is visibly discontinuous'
    assert zenith_range <= 6, 'Panorama zenith contains a pinching discontinuity'
    # This installed GoldSrc renderer rejects sky TGAs above 256x256 and
    # silently falls back to desert after logging LoadTGA: texture too large.
    size = 256
    s, t = np.meshgrid(np.linspace(-1, 1, size), np.linspace(1, -1, size))
    one = np.ones_like(s)
    # Quake/GoldSrc sky projection: st_to_vec + skytexorder.
    # https://github.com/id-Software/Quake/blob/master/WinQuake/gl_warp.c
    faces = {'rt': (one, -s, t), 'lf': (-one, s, t),
             'bk': (s, one, t), 'ft': (-s, -one, t),
             'up': (-t, -s, one), 'dn': (t, -s, -one)}
    output = MOD/'gfx/env'
    output.mkdir(parents=True, exist_ok=True)
    images = {}
    for face, (x, y, z) in faces.items():
        norm = np.sqrt(x*x+y*y+z*z)
        u = (np.arctan2(y, x)/(2*np.pi)+.5)*w-.5
        v = np.clip((.5-np.arcsin(z/norm)/np.pi)*h-.5, 0, h-1)
        x0 = np.floor(u).astype(int)
        y0 = np.floor(v).astype(int)
        fx = (u-x0)[..., None]
        fy = (v-y0)[..., None]
        a = source[y0, x0 % w]*(1-fx)+source[y0, (x0+1) % w]*fx
        b = source[np.minimum(y0+1, h-1), x0 % w]*(1-fx)+source[np.minimum(y0+1, h-1), (x0+1) % w]*fx
        pixels = np.rint(a*(1-fy)+b*fy).clip(0,255).astype(np.uint8)
        images[face] = pixels
        Image.fromarray(pixels).save(output/f'{SKY}{face}.tga')
    # Every adjoining cube edge samples precisely the same spherical ray.
    sides = {face: [pixels[0], pixels[-1], pixels[:,0], pixels[:,-1]] for face,pixels in images.items()}
    edge_errors = []
    for face, edges in sides.items():
        for edge in edges:
            error = min(float(np.abs(edge.astype(float)-candidate.astype(float)).max())
                        for other, candidates in sides.items() if other != face
                        for candidate in candidates + [c[::-1] for c in candidates])
            edge_errors.append(error)
    assert max(edge_errors) <= 1, edge_errors
    if update_map:
        path = MOD/'maps/cs_mansion.bsp'
        bsp = BSP(path)
        bsp.entities[0]['skyname'] = SKY
        bsp.write(path, bsp.entities)
        check = BSP(path)
        assert all(check.lump(i) == bsp.lump(i) for i in range(1,15))
        placement = MOD/'mansion-placement.json'
        data = json.loads(placement.read_text())
        data['sky'] = SKY
        placement.write_text(json.dumps(data,indent=2)+'\n')
    report = dict(sky=SKY, source=str(SOURCE), panorama_size=[w,h], face_size=size,
                  max_cube_edge_difference=max(edge_errors),
                  source_wrap_max_difference=float(wrap_delta.max()),
                  source_wrap_mean_difference=float(wrap_delta.mean()),
                  source_zenith_range=zenith_range,
                  revision='seam-repaired dark crimson mountains',
                  generator='built-in image_gen', original_city_preserved=True,
                  files={face:hashlib.sha256((output/f'{SKY}{face}.tga').read_bytes()).hexdigest() for face in faces},
                  interactive_validation=False)
    (MOD/'mountain-sky-assets.json').write_text(json.dumps(report,indent=2)+'\n')
    print(f'Installed {SKY}: six {size}x{size} TGA faces, maximum edge difference {max(edge_errors):.0f}/255.')


if __name__ == '__main__':
    install()
