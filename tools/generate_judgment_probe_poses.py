"""Independent ray targets from the authored poses and compiled hitbox centers."""
from pathlib import Path
import struct
import numpy as np
from scipy.spatial.transform import Rotation

MOD = Path(__file__).resolve().parents[1]
mdl = (MOD/'models/cremation/hotd2/judgment/zeal.mdl').read_bytes()
count, offset = struct.unpack_from('<ii', mdl, 156)
boxes = {}
for i in range(count):
    bone, _, *bounds = struct.unpack_from('<ii6f', mdl, offset+i*32)
    boxes[bone] = (np.asarray(bounds[:3])+np.asarray(bounds[3:]))*.5
rows = []
for sequence, name in [(4, 'charge'), (9, 'hover')]:
    lines = (MOD/f'assets-source/hotd2-judgment/goldsrc/zeal/{name}.smd').read_text().splitlines()
    parents = {}
    for line in lines[lines.index('nodes')+1:]:
        if line == 'end': break
        values = line.split(); parents[int(values[0])] = int(values[-1])
    frames = []; pose = None
    for line in lines[lines.index('skeleton')+1:]:
        if line == 'end': break
        if line.startswith('time '):
            pose = {}; frames.append(pose)
        elif line:
            values = line.split(); pose[int(values[0])] = np.asarray(list(map(float, values[1:])))
    for index in (0,16,33,50):
        matrices = {}
        for bone, values in frames[index].items():
            local = np.eye(4); local[:3,:3] = Rotation.from_euler('xyz',values[3:]).as_matrix(); local[:3,3] = values[:3]
            matrices[bone] = local if parents[bone]<0 else matrices[parents[bone]]@local
        points = [(matrices[bone]@np.append(boxes[bone],1))[:3] for bone in (1,2,19,22)]
        coords = ','.join('{'+','.join(f'{v:.7f}f' for v in point)+'}' for point in points)
        rows.append('{'+f'{sequence},{index*256/(len(frames)-1):.7f}f,'+'{'+coords+'}}')
output = '// Generated from SMD transforms; GET_BONE_POSITION does not return reliable angles.\n'
output += 'struct JudgmentProbePose{int sequence;float frame;float points[4][3];};\nstatic const JudgmentProbePose judgmentProbePoses[]={\n'
output += ',\n'.join(rows)+'\n};\n'
(MOD/'source/judgment_probe_poses.h').write_text(output)
print('Generated 128 independent body/head/wing ray targets across poses and yaw angles.')
