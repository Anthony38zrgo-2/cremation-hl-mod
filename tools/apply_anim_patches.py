#!/usr/bin/env python3
"""Cremation GoldSrc x86 PE binary animation patch; offline, exact-signature gated.

Usage: python apply_anim_patches.py ORIGINAL_DLL OUTPUT_DLL [--profile production|validation]

Never writes to the input. Aborts if the original signatures do not match.
"""
from __future__ import annotations
import argparse, hashlib, json, pathlib, struct

SETTER_SIG = bytes.fromhex('89 82 a8 01 00 00 c7 82 b0 01 00 00 00 00 00 00')
FALLBACK_SIG = bytes.fromhex(
    'c7 87 a8 01 00 00 00 00 00 00 '
    'c7 87 b0 01 00 00 00 00 00 00 '
    '8b 00 '
    '89 87 b4 01 00 00'
)
ORIG_SHA256 = {
    'production': '8335a06a4e3dc91cd5360e5be5fb48678f8cf72f45b81274dcf6b5bd5c9b97f3',
    'validation': 'f4b085ae715b12eda94b5651ad5eed4dadb56d7caa93573c9e7df8cc95f834e6'
}
EXPECTED_RVAS = {
    'production': {'setter':0x3d5ae, 'fallback':0x2182d, 'czero_4c':0x11b3e, 'czero_14':0x13dd8},
    'validation': {'setter':0x4e7ae, 'fallback':0x2c2fd, 'czero_4c':0x12a0e, 'czero_14':0x14af8}
}

def u16(buf,i): return struct.unpack_from('<H',buf,i)[0]
def u32(buf,i): return struct.unpack_from('<I',buf,i)[0]
def set_u32(buf,i,v): struct.pack_into('<I',buf,i,v)
def hexhash(b): return hashlib.sha256(b).hexdigest()

class PE:
    def __init__(self, b):
        self.b=b
        if b[:2] != b'MZ':raise ValueError('Not MZ')
        self.pe=u32(b,0x3c)
        if b[self.pe:self.pe+4]!=b'PE\0\0':raise ValueError('Not PE')
        if u16(b,self.pe+4)!=0x14c:raise ValueError('Expected x86 PE (Machine I386)')
        self.nsec=u16(b,self.pe+6)
        self.opt=self.pe+24
        if u16(b,self.opt)!=0x10b:raise ValueError('Expected PE32')
        self.imagebase=u32(b,self.opt+28)
        self.checksumoff=self.opt+64
        self.sectoff=self.opt+u16(b,self.pe+20)
        self.sections=[]
        for i in range(self.nsec):
            sh=self.sectoff+40*i
            self.sections.append({'name':bytes(b[sh:sh+8]).split(b'\0')[0].decode('ascii'),
                'sh':sh,'vsize':u32(b,sh+8),'rva':u32(b,sh+12),'rawsize':u32(b,sh+16),'fileoff':u32(b,sh+20),
                'characteristics':u32(b,sh+36)})
    def offset(self,rva):
        for s in self.sections:
            if s['rva']<=rva<s['rva']+s['rawsize']:
                return s['fileoff']+rva-s['rva']
        raise ValueError(f'Unmapped RVA: {rva:#x}')

def rel32(jump_rva, dest_rva):
    delta=dest_rva-(jump_rva+5)
    if not (-2**31<=delta<2**31):raise ValueError('out of range')
    return b'\xe9'+struct.pack('<i',delta)

def rel8(i, dest):
    delta=dest-(i+2)
    if not -128<=delta<=127:raise ValueError('short jump out of range')
    return delta & 255

def build_trampoline(cave_rva, original_next):
    # Original preamble already populated edx=pev, ecx=gpGlobals, eax=requested sequence.
    # Preserve all registers and the stack. Only suppress idempotent idle/walk requests
    # (state 0 or 1, same sequence and same stored state). Attacks/events still reset.
    z=bytearray()
    z+=bytes.fromhex('83 7c 24 0c 01')  # cmp dword [esp+0xc],1
    ja_pos=len(z);z+=b'\x77\x00'    # ja original setter
    z+=bytes.fromhex('39 82 a8 01 00 00')  # cmp [edx+0x1a8],eax
    jne1_pos=len(z);z+=b'\x75\x00'
    z+=b'\x50'                      # push eax; preserve sequence and flags except stack
    z+=bytes.fromhex('8b 44 24 10') # mov eax,[esp+0x10] (= requested state)
    z+=bytes.fromhex('39 82 04 02 00 00') # cmp [edx+0x204],eax
    z+=b'\x58'                      # pop eax; flags unchanged
    jne2_pos=len(z);z+=b'\x75\x00'
    z+=b'\xc3'                      # no-op: return, no state / frame / counter reset
    original_pos=len(z)
    z+=bytes.fromhex('89 82 a8 01 00 00') # overwritten original instruction
    jmp_rva=cave_rva+len(z)
    z+=rel32(jmp_rva,original_next)
    for site in [ja_pos,jne1_pos,jne2_pos]:z[site+1]=rel8(cave_rva+site,cave_rva+original_pos)
    return bytes(z)

def build_fallback_trampoline(cave_rva, original_next):
    # Called after the model was found valid and seq was clamped to idle 0.
    # Retain valid phase/time on transient invalid activity, but sanitize
    # NaN/negative/out-of-range frames and uninitialized animation time.
    z=bytearray()
    z+=bytes.fromhex('8b 8f b0 01 00 00')  # mov ecx,[edi+frame] (bit pattern)
    z+=bytes.fromhex('f7 c1 00 00 00 80')  # test ecx,signbit
    neg_pos=len(z);z+=b'\x75\x00'        # jnz reset
    z+=bytes.fromhex('81 f9 00 00 7f 43')  # cmp ecx, bits(255.0f)
    range_pos=len(z);z+=b'\x77\x00'      # ja reset: positive NaN or >255
    z+=bytes.fromhex('83 bf b4 01 00 00 00') # cmp [edi+animtime],0
    uninit_pos=len(z);z+=b'\x74\x00'      # je reset
    z+=rel32(cave_rva+len(z),original_next) # valid: preserve phase/time
    reset_pos=len(z)
    z+=bytes.fromhex('c7 87 b0 01 00 00 00 00 00 00') # original frame=0
    z+=bytes.fromhex('8b 00 89 87 b4 01 00 00') # original animtime=gpGlobals->time
    z+=rel32(cave_rva+len(z),original_next)
    for pos in [neg_pos,range_pos,uninit_pos]:
        z[pos+1]=rel8(cave_rva+pos,cave_rva+reset_pos)
    return bytes(z)

def pe_checksum(b, checkoff):
    # Portable PE CheckSum calculation; omit the 4-byte CheckSum field.
    total=0
    for i in range(0,len(b),2):
        if checkoff<=i<checkoff+4:continue
        word=b[i] + ((b[i+1]<<8) if i+1<len(b) else 0)
        total+=word
        total=(total & 0xffff)+(total>>16)
    total=(total & 0xffff)+(total>>16)
    return (total+len(b))&0xffffffff

def patch(source,profile):
    old=bytes(source)
    sha=hexhash(old)
    if sha!=ORIG_SHA256[profile]:
        raise ValueError(f'SHA256 mismatch for {profile}: {sha}. Aborting rather than guessing.')
    b=bytearray(old);p=PE(b)
    if p.imagebase!=0x10000000:raise ValueError('Unexpected ImageBase')
    sec=next(s for s in p.sections if s['name']=='.text')
    if sec['characteristics'] & 0x20000000 == 0:raise ValueError('No executable .text')
    orig_setter=EXPECTED_RVAS[profile]['setter']
    orig_fallback=EXPECTED_RVAS[profile]['fallback']
    off1=p.offset(orig_setter);off2=p.offset(orig_fallback)
    assert old[off1:off1+len(SETTER_SIG)]==SETTER_SIG, 'Setter signature not found'
    assert old[off2:off2+len(FALLBACK_SIG)]==FALLBACK_SIG, 'Fallback signature not found'
    assert old.count(SETTER_SIG)==1, 'Setter signature ambiguous'
    assert old.count(FALLBACK_SIG)==1, 'Fallback signature ambiguous'
    # Inject within zero-filled raw .text tail, align to 16 bytes.
    cave_rel=(sec['vsize']+15)&~15
    cave_rva=sec['rva']+cave_rel
    trampoline=build_trampoline(cave_rva,orig_setter+6)
    cave2_rel=(cave_rel+len(trampoline)+15)&~15
    cave2_rva=sec['rva']+cave2_rel
    fbtramp=build_fallback_trampoline(cave2_rva,orig_fallback+28)
    if cave2_rel+len(fbtramp)>sec['rawsize']:raise ValueError('No room in .text raw tail')
    cave_off=sec['fileoff']+cave_rel
    cave2_off=sec['fileoff']+cave2_rel
    if any(b[cave_off:cave2_off+len(fbtramp)]):raise ValueError('Nonzero cave!')
    b[cave_off:cave_off+len(trampoline)]=trampoline
    b[cave2_off:cave2_off+len(fbtramp)]=fbtramp
    set_u32(b,sec['sh']+8,cave2_rel+len(fbtramp))
    # Hook EXACT instruction (6 bytes), preserving preceding register setup.
    b[off1:off1+6]=rel32(orig_setter,cave_rva)+b'\x90'
    # Fast zombie + Randy: preserve valid phase/time during fallback to idle.
    # Only reset phase/time when data is invalid or uninitialized.
    b[off2+10:off2+28]=rel32(orig_fallback+10,cave2_rva)+b'\x90'*13
    # CZero actions already reset frame unconditionally later. These two prior
    # frame=0 stores are provably duplicate writes: between them and the later
    # unconditional store, there is no function call and no frame read.
    czero_redundancies=[('czero_4c',0x12),('czero_14',0x15)]
    for key,lag in czero_redundancies:
        rva=EXPECTED_RVAS[profile][key]
        off=p.offset(rva)
        if old[off:off+10] != bytes.fromhex('c7 86 b0 01 00 00 00 00 00 00'):
            raise ValueError(f'Unexpected CZero early-frame write: {key}')
        # The second store to frame remains, so new attacks still restart.
        later=p.offset(rva+lag)
        if old[later:later+10] != bytes.fromhex('c7 86 b0 01 00 00 00 00 00 00'):
            raise ValueError(f'No matching later CZero frame reset for {key}')
        b[off:off+10] = b'\x90'*10
    set_u32(b,p.checksumoff,0)
    set_u32(b,p.checksumoff,pe_checksum(b,p.checksumoff))
    q=PE(b)
    assert q.offset(cave_rva)==cave_off
    assert len(b)==len(old)
    assert b[off1:off1+6]==rel32(orig_setter,cave_rva)+b'\x90'
    assert b[off2:off2+10]==FALLBACK_SIG[:10]
    assert b[off2+10:off2+28]==rel32(orig_fallback+10,cave2_rva)+b'\x90'*13
    assert b[cave2_off:cave2_off+len(fbtramp)]==fbtramp
    for key,_ in czero_redundancies:
        assert b[p.offset(EXPECTED_RVAS[profile][key]):p.offset(EXPECTED_RVAS[profile][key])+10]==b'\x90'*10
    assert b[cave_off:cave_off+len(trampoline)]==trampoline
    # All changed offsets must be explained by named patches, section VS and checksum.
    allow=set(range(cave_off,cave_off+len(trampoline)))|set(range(cave2_off,cave2_off+len(fbtramp)))|set(range(off1,off1+6))|set(range(off2+10,off2+28))|set(range(sec['sh']+8,sec['sh']+12))|set(range(p.checksumoff,p.checksumoff+4))
    for key,_ in czero_redundancies:
        o=p.offset(EXPECTED_RVAS[profile][key]);allow|=set(range(o,o+10))
    changed={i for i,(x,y) in enumerate(zip(old,b)) if x!=y}
    assert changed<=allow, f'Unexpected mutation at {list(changed-allow)[:10]}'
    return bytes(b),{
        'profile':profile,'input_sha256':sha,'output_sha256':hexhash(b),
        'file_size':len(b),'image_base':hex(p.imagebase),'text_original_vsize':sec['vsize'],
        'text_patched_vsize':cave2_rel+len(fbtramp),
        'patches':[
            {'name':'QuakeZombie_idempotent_noncombat_state_guard', 'hook_rva':hex(orig_setter),
             'trampoline_rva':hex(cave_rva),'trampoline_length':len(trampoline),
             'behavior':'skip reset only when state 0/1, sequence same and stored state same'},
            {'name':'FastZombie_Randy_invalid_sequence_fallback_continuity',
             'fallback_rva':hex(orig_fallback),'hook_rva':hex(orig_fallback+10),'trampoline_rva':hex(cave2_rva),'trampoline_length':len(fbtramp),
             'behavior':'fallback to sequence 0 remains; preserve valid frame and animtime, sanitize negative/NaN/>255 frame or zero animtime'},
            {'name':'CZero_duplicate_frame_zero_elimination','early_reset_rvas':[hex(EXPECTED_RVAS[profile][k]) for k,_ in czero_redundancies],
             'behavior':'remove duplicate first frame=0 on two actions, retain unconditional later frame=0 to restart each attack'}],
        'num_changed_bytes':len(changed),'PE_checksum':hex(u32(b,p.checksumoff))
    }

def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('input',type=pathlib.Path)
    ap.add_argument('output',type=pathlib.Path)
    ap.add_argument('--profile',required=True,choices=['production','validation'])
    args=ap.parse_args()
    out,report=patch(args.input.read_bytes(),args.profile)
    args.output.parent.mkdir(parents=True,exist_ok=True)
    args.output.write_bytes(out)
    (args.output.with_suffix(args.output.suffix+'.manifest.json')).write_text(json.dumps(report,indent=2)+'\n',encoding='utf8')
    print(json.dumps(report,indent=2))

if __name__=='__main__':main()
