"""Install original Condition Zero AWP and Desert Eagle assets unchanged."""
from pathlib import Path
import hashlib
import shutil
import struct

mod = Path(__file__).resolve().parents[1]
donor = mod.parent / 'czero'
sounds = {'weapons/awp1.wav', 'weapons/awp_deploy.wav',
          'weapons/deagle-1.wav', 'weapons/deagle-2.wav', 'weapons/zoom.wav'}
for weapon in ('awp', 'deagle'):
    for prefix in ('v', 'p', 'w'):
        source = donor / 'models' / f'{prefix}_{weapon}.mdl'
        target = mod / 'models/cremation' / weapon / source.name
        target.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(source, target)
        assert hashlib.sha256(source.read_bytes()).digest() == hashlib.sha256(target.read_bytes()).digest()
    data = (donor / 'models' / f'v_{weapon}.mdl').read_bytes()
    count, offset = struct.unpack_from('<ii', data, 164)
    for index in range(count):
        base = offset + index * 176
        events, event_offset = struct.unpack_from('<ii', data, base + 48)
        for event_index in range(events):
            event = event_offset + event_index * 76
            if struct.unpack_from('<i', data, event + 4)[0] == 5004:
                sounds.add(data[event + 12:event + 76].split(b'\0')[0].decode('ascii'))
for name in sorted(sounds):
    source = donor / 'sound' / name
    target = mod / 'sound' / name
    target.parent.mkdir(parents=True, exist_ok=True)
    shutil.copy2(source, target)
print(f'Installed 6 original Condition Zero models and {len(sounds)} sounds.')
