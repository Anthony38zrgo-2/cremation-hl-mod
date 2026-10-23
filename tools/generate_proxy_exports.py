from pathlib import Path
import pefile, json
root=Path(__file__).resolve().parents[2]
source=root/'Cremation/source'
pe=pefile.PE(str(root/'Half-Life2Classic_Ravenholm/dlls/hl.dll'))
names=[s.name.decode('ascii') for s in pe.DIRECTORY_ENTRY_EXPORT.symbols if s.name]
custom={'GiveFnptrsToDll','GetEntityAPI','GetEntityAPI2'}
# Cremation's own entities. The engine resolves every BSP classname through the
# export table, and save/restore recreates runtime entities the same way.
project=['weapon_ricochet','cremation_disc','weapon_bounce_shotgun','cremation_pellet','cremation_grenade']
lines=['LIBRARY cremation','EXPORTS','    GiveFnptrsToDll=_GiveFnptrsToDll@8','    GetEntityAPI','    GetEntityAPI2']+[f'    {name}' for name in project]
forward=[name for name in names if name not in custom]
code=['// Generated Win32 tail jumps preserve all original calling conventions.',f'static FARPROC forwardTargets[{len(forward)}];','static const char* forwardNames[]={']
code.extend('    "'+name+'",' for name in forward)
code.append('};')
for i,name in enumerate(forward):
    lines.append(f'    forward_{i}')
    code.append(f'extern "C" __declspec(naked) void forward_{i}() {{ __asm jmp dword ptr [forwardTargets+{i*4}] }}')
(source/'forwarders.inc').write_text('\n'.join(code)+'\n')
(source/'export-names.json').write_text(json.dumps({f'forward_{i}':name for i,name in enumerate(forward)},indent=2)+'\n')
(source/'cremation.def').write_text('\n'.join(lines)+'\n')
(source/'build').mkdir(exist_ok=True)
print('Generated forwarding table:',len(names),'original exports + Ricochet.')
