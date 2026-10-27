"""Temporarily select the validation build for a dedicated HLDS fixture."""
from contextlib import contextmanager
from pathlib import Path
import os
import runpy
import shutil
import subprocess
import tempfile


@contextmanager
def using_validation_dll(mod: Path):
    processes = subprocess.run(
        ['tasklist', '/NH'],
        capture_output=True, text=True, check=True)
    running = [line.strip() for line in processes.stdout.splitlines()
               if line.lower().startswith(('hl.exe', 'hlds.exe'))]
    if running:
        raise RuntimeError('Close Half-Life and other HLDS processes before running isolated validation.')

    installed = mod / 'dlls/cremation.dll'
    validation = mod / 'source/build/validation/cremation.dll'
    if not installed.is_file() or not validation.is_file():
        raise FileNotFoundError('Build both server variants with tools/build_server.ps1 first.')
    with tempfile.NamedTemporaryFile(prefix='cremation-production-', suffix='.dll',
                                     dir=mod / 'source/build', delete=False) as backup_file:
        backup = Path(backup_file.name)
    shutil.copy2(installed, backup)
    try:
        shutil.copy2(validation, installed)
        yield
    finally:
        shutil.copy2(backup, installed)
        backup.unlink(missing_ok=True)


def run_with_validation_dll(mod: Path, script: Path):
    with using_validation_dll(mod):
        os.environ['CREMATION_VALIDATION_DLL_ACTIVE'] = '1'
        try:
            runpy.run_path(str(script), run_name='__main__')
        finally:
            os.environ.pop('CREMATION_VALIDATION_DLL_ACTIVE', None)
