#!/usr/bin/env python3
"""Offline structural / byte-diff validation for Cremation animated DLL patch."""
from __future__ import annotations
import argparse, importlib.util, pathlib, struct, hashlib

# Loads the patch script from this directory, no third-party dependencies.
spec=importlib.util.spec_from_file_location('cremation_patch',pathlib.Path(__file__).with_name('apply_anim_patches.py'))
mod=importlib.util.module_from_spec(spec);spec.loader.exec_module(mod)

def verify(original,patched,profile):
    old=original.read_bytes(); new=patched.read_bytes()
    expected, manifest=mod.patch(old,profile)
    assert new == expected, 'Patched DLL does not match deterministic rebuild'
    a=mod.PE(bytearray(old));b=mod.PE(bytearray(new))
    assert len(old)==len(new)
    assert a.nsec==b.nsec and a.imagebase==b.imagebase
    assert mod.pe_checksum(bytearray(new),b.checksumoff)==mod.u32(new,b.checksumoff), 'Checksum mismatch'
    assert [s['name'] for s in a.sections]==[s['name'] for s in b.sections]
    for x,y in zip(a.sections,b.sections):
        assert x['rawsize']==y['rawsize'] and x['fileoff']==y['fileoff'] and x['rva']==y['rva']
        if x['name']!='.text':assert x==y, f'{x["name"]} changed header'
    # Import/export directories, relocations and all non-.text sections remain untouched.
    assert old[a.opt+96:a.opt+96+16*8]==new[b.opt+96:b.opt+96+16*8]
    for section in a.sections:
        if section['name'] != '.text':
            i=section['fileoff'];j=i+section['rawsize'];assert old[i:j]==new[i:j],section['name']
    print(f'PASS {profile}: same size ({len(new)}), checksum, .reloc, imports/exports, exact reversible bytes; sha256={mod.hexhash(new)}')

if __name__=='__main__':
    ap=argparse.ArgumentParser()
    ap.add_argument('original',type=pathlib.Path)
    ap.add_argument('patched',type=pathlib.Path)
    ap.add_argument('--profile',choices=['production','validation'],required=True)
    args=ap.parse_args();verify(args.original,args.patched,args.profile)
