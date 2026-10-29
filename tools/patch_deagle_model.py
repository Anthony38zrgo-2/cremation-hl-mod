"""Replace missing Counter-Strike animation event sounds in the Deagle viewmodel."""
from pathlib import Path
import struct

model = Path(__file__).resolve().parents[1] / "models/cremation/deagle/v_deagle.mdl"
data = bytearray(model.read_bytes())
sequence_count, sequence_offset = struct.unpack_from("<ii", data, 164)
patches = {
    "reload": {6: "weapons/357_reload1.wav", 55: "weapons/357_cock1.wav"},
    "draw": {8: "weapons/357_cock1.wav"},
}
changed = 0
for index in range(sequence_count):
    base = sequence_offset + index * 176
    name = data[base:base + 32].split(b"\0", 1)[0].decode("ascii", "replace")
    event_count, event_offset = struct.unpack_from("<ii", data, base + 48)
    for event_index in range(event_count):
        event = event_offset + event_index * 76
        frame, event_id = struct.unpack_from("<ii", data, event)
        replacement = patches.get(name, {}).get(frame)
        if event_id != 5004 or not replacement:
            continue
        encoded = replacement.encode("ascii")
        data[event + 12:event + 76] = encoded + bytes(64 - len(encoded))
        changed += 1
if changed != 3:
    raise SystemExit(f"Expected to patch 3 sound events, found {changed}: {model}")
model.write_bytes(data)
print(f"Patched {changed} Deagle viewmodel sound events: {model}")
