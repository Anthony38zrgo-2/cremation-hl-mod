import zipfile
import hashlib
import os
import shutil
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
CREMATION = ROOT / 'Cremation'

files_map = {
    # Mod Cremation Active DLLs
    'Mod_Cremation/dlls/cremation.dll': CREMATION / 'dlls/cremation.dll',
    'Mod_Cremation/dlls/hl_ravenholm.dll': CREMATION / 'dlls/hl_ravenholm.dll',
    'Mod_Cremation/dlls/hl.dll': CREMATION / 'dlls/hl.dll',
    'Mod_Cremation/cl_dlls/client.dll': CREMATION / 'cl_dlls/client.dll',
    'Mod_Cremation/cl_dlls/client_ravenholm.dll': CREMATION / 'cl_dlls/client_ravenholm.dll',
    
    # Validation variant
    'Mod_Cremation/build_variants/cremation_validation.dll': CREMATION / 'source/build/validation/cremation.dll',
    
    # Engine GoldSrc
    'Engine_GoldSrc/hw.dll': ROOT / 'hw.dll',
    'Engine_GoldSrc/sw.dll': ROOT / 'sw.dll',
    'Engine_GoldSrc/swds.dll': ROOT / 'swds.dll',
    'Engine_GoldSrc/FileSystem_Stdio.dll': ROOT / 'FileSystem_Stdio.dll',
    'Engine_GoldSrc/tier0.dll': ROOT / 'tier0.dll',
    'Engine_GoldSrc/vstdlib.dll': ROOT / 'vstdlib.dll',
    'Engine_GoldSrc/vgui.dll': ROOT / 'vgui.dll',
    'Engine_GoldSrc/vgui2.dll': ROOT / 'vgui2.dll',
    'Engine_GoldSrc/SDL2.dll': ROOT / 'SDL2.dll',
    'Engine_GoldSrc/steam_api.dll': ROOT / 'steam_api.dll',
    'Engine_GoldSrc/Mss32.dll': ROOT / 'Mss32.dll',
    'Engine_GoldSrc/a3dapi.dll': ROOT / 'a3dapi.dll',
    'Engine_GoldSrc/Core.dll': ROOT / 'Core.dll',
    'Engine_GoldSrc/DemoPlayer.dll': ROOT / 'DemoPlayer.dll',
    'Engine_GoldSrc/proxy.dll': ROOT / 'proxy.dll',
    
    # Valve Base Half-Life 1 DLLs
    'Base_Valve_HL1/cl_dlls/GameUI.dll': ROOT / 'valve/cl_dlls/GameUI.dll',
    'Base_Valve_HL1/cl_dlls/particleman.dll': ROOT / 'valve/cl_dlls/particleman.dll',
    'Base_Valve_HL1/cl_dlls/client.dll': ROOT / 'valve/cl_dlls/client.dll',
    'Base_Valve_HL1/dlls/Director.dll': ROOT / 'valve/dlls/Director.dll',
    'Base_Valve_HL1/dlls/hl.dll': ROOT / 'valve/dlls/hl.dll'
}

readme_lines = [
    '========================================================================',
    '       CREMATION MOD & GOLDSRC ENGINE - REPOSITORIO DE DLLs',
    '========================================================================',
    '',
    'Este archivo comprimido contiene todas las librerias dinamicas (DLLs)',
    'utilizadas para la ejecucion, intercepcion por proxy e inspeccion',
    'tecnica del mod Cremation y del motor Half-Life 1 (GoldSrc).',
    '',
    '------------------------------------------------------------------------',
    '1. MOD CREMATION (Librerias activas de juego y proxy)',
    '------------------------------------------------------------------------',
    ' - Mod_Cremation/dlls/cremation.dll',
    '     Proxy Maestro de Servidor (GameDLL). Intercepta la API de GoldSrc,',
    '     administra oleadas de mansion, IA de zombies (HOTD2 David/Steve/Johnny,',
    '     Randy, Max, Quake Zombie, CZero), combate, armas personalizadas',
    '     y delega llamadas nativas a hl_ravenholm.dll.',
    ' - Mod_Cremation/dlls/hl_ravenholm.dll',
    '     DLL nativa servidora donante (Ravenholm/HL base). Provee la logica',
    '     base del juego a la cual cremation.dll hace forward / hooking.',
    ' - Mod_Cremation/dlls/hl.dll',
    '     DLL servidora Half-Life estandar empaquetada con el mod.',
    ' - Mod_Cremation/cl_dlls/client.dll',
    '     Proxy Maestro de Cliente. Maneja HUD tactico, radar, sistema de clima',
    '     (niebla roja volumetrica GL y lluvia de sangre), barras de vida de jefes,',
    '     audio dinamico y reenvia a client_ravenholm.dll.',
    ' - Mod_Cremation/cl_dlls/client_ravenholm.dll',
    '     DLL nativa cliente donante (Ravenholm base).',
    ' - Mod_Cremation/build_variants/cremation_validation.dll',
    '     Compilacion de servidor con instrumentacion de pruebas de runtime activa.',
    '',
    '------------------------------------------------------------------------',
    '2. MOTOR GOLDSRC / HALF-LIFE 1 (Runtime del motor)',
    '------------------------------------------------------------------------',
    ' - Engine_GoldSrc/hw.dll               : Nucleo del motor grafico Hardware / OpenGL.',
    ' - Engine_GoldSrc/sw.dll               : Nucleo del motor en modo Software render.',
    ' - Engine_GoldSrc/swds.dll             : Motor del servidor dedicado (hlds.exe).',
    ' - Engine_GoldSrc/FileSystem_Stdio.dll : Sistema de archivos virtual (VFS) de GoldSrc.',
    ' - Engine_GoldSrc/SDL2.dll             : Capa de ventana, eventos, raton y teclado.',
    ' - Engine_GoldSrc/vgui.dll / vgui2.dll : Motor de interfaz gráfica de Valve (VGUI 1/2).',
    ' - Engine_GoldSrc/tier0.dll/vstdlib.dll: Librerias base y utilidades de Valve.',
    ' - Engine_GoldSrc/Mss32.dll / a3dapi.dll: Motores de audio Miles Sound y Aureal 3D.',
    ' - Engine_GoldSrc/steam_api.dll        : Capa de integracion con el cliente Steam.',
    ' - Engine_GoldSrc/Core.dll / DemoPlayer.dll / proxy.dll : Soporte de red y demos.',
    '',
    '------------------------------------------------------------------------',
    '3. BASE VALVE HALF-LIFE 1 (DLLs originales del juego base)',
    '------------------------------------------------------------------------',
    ' - Base_Valve_HL1/cl_dlls/client.dll      : DLL de cliente original de Valve.',
    ' - Base_Valve_HL1/cl_dlls/GameUI.dll      : Menu e interfaz de usuario de Half-Life.',
    ' - Base_Valve_HL1/cl_dlls/particleman.dll : Gestor de particulas nativo.',
    ' - Base_Valve_HL1/dlls/hl.dll             : DLL de servidor original de Half-Life.',
    ' - Base_Valve_HL1/dlls/Director.dll       : Modo espectador y director de camaras.',
    '',
    '========================================================================',
    'TABLA DE ARCHIVOS, TAMANO Y HASHES SHA-256 PARA AUDITORIA:',
    '========================================================================'
]

zip_path = CREMATION / 'cremation_dlls.zip'
with zipfile.ZipFile(zip_path, 'w', compression=zipfile.ZIP_DEFLATED, compresslevel=6) as zf:
    for arcname, srcpath in files_map.items():
        if not srcpath.exists():
            print(f'WARNING: {srcpath} does not exist!')
            continue
        data = srcpath.read_bytes()
        sha = hashlib.sha256(data).hexdigest()
        size = len(data)
        readme_lines.append(f'{arcname:<45} | {size:>10} bytes | SHA256: {sha}')
        zf.writestr(arcname, data)
        print(f'Added: {arcname} ({size:,} bytes)')
    
    readme_text = '\r\n'.join(readme_lines) + '\r\n'
    zf.writestr('README_DLLS.txt', readme_text)

root_copy = ROOT / 'cremation_dlls.zip'
shutil.copy2(zip_path, root_copy)

print(f'\nSuccess! ZIP created at:')
print(f'  {zip_path} ({os.path.getsize(zip_path):,} bytes)')
print(f'  {root_copy} ({os.path.getsize(root_copy):,} bytes)')
