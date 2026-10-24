"""Retarget original Half-Life MP5 motion onto the existing Unreal Hive Hand mesh."""
from pathlib import Path
import struct as st
import math

MOD=Path(__file__).resolve().parents[1]
SOURCE=MOD/'assets-source/additional_weapons/unreal_hive_hand/v_hgun.mdl'
DONOR=MOD/'assets-source/additional_weapons/unreal_hive_hand/v_mp5_motion.mdl'
OUT=SOURCE.with_name('v_hgun_mp5_anims.mdl')

def integer(b,o):return st.unpack_from('<i',b,o)[0]
def pose(b,seq,bone,channel,frame):
    bi=integer(b,144)+bone*112
    value=st.unpack_from('<f',b,bi+64+channel*4)[0]
    scale=st.unpack_from('<f',b,bi+88+channel*4)[0]
    desc=integer(b,168)+seq*176
    anim=integer(b,desc+124)+bone*12
    offset=st.unpack_from('<H',b,anim+channel*2)[0]
    if not offset:return value
    at=anim+offset
    while True:
        valid,total=st.unpack_from('<2B',b,at)
        if not total:raise ValueError('Invalid animation span')
        if frame<total:
            sample=min(frame+1,valid)
            return value+st.unpack_from('<h',b,at+sample*2)[0]*scale
        frame-=total;at+=2*(valid+1)

def rotation(angles):
    x,y,z=angles;cx,sx=math.cos(x),math.sin(x);cy,sy=math.cos(y),math.sin(y);cz,sz=math.cos(z),math.sin(z)
    return [[cz*cy,cz*sy*sx-sz*cx,cz*sy*cx+sz*sx],
            [sz*cy,sz*sy*sx+cz*cx,sz*sy*cx-cz*sx],[-sy,cy*sx,cy*cx]]
def multiply(a,b):return [[sum(a[i][k]*b[k][j] for k in range(3)) for j in range(3)] for i in range(3)]
def transpose(a):return [list(row) for row in zip(*a)]
def angles(r):
    y=math.asin(max(-1,min(1,-r[2][0])))
    if abs(math.cos(y))>.00001:return [math.atan2(r[2][1],r[2][2]),y,math.atan2(r[1][0],r[0][0])]
    return [math.atan2(-r[1][2],r[1][1]),y,0]

def build_assets():
    hive=SOURCE.read_bytes();donor=DONOR.read_bytes()
    count=integer(hive,140);sequences=integer(donor,164)
    base=[[pose(hive,0,b,c,0) for c in range(6)] for b in range(count)]
    # MP5 Bone01 supplies global movement; Bone04 drives the gun/hand
    # assembly (its descendants contain the MP5 geometry). Keep Hive muzzle
    # bones in their own reference pose rather than importing MP5 fingers.
    mapping={0:0,1:3}
    donor_parent=rotation([pose(donor,0,0,c,0) for c in range(3,6)])
    hive_parent=rotation(base[0][3:])
    basis=multiply(transpose(hive_parent),donor_parent)
    samples=[]
    for seq in range(sequences):
        desc=integer(donor,168)+seq*176;frames=integer(donor,desc+56)
        animation=[]
        for b in range(count):
            channels=[]
            for c in range(6):
                values=[]
                for f in range(frames):
                    delta=0
                    if b in mapping:
                        source=mapping[b]
                        delta=pose(donor,seq,source,c,f)-pose(donor,0,source,c,0)
                        if c>=3:
                            # Transform rotation through the rigs' different
                            # reference axes. Copying MP5 Z directly creates
                            # yaw on Hive Hand instead of upward recoil.
                            current=rotation([pose(donor,seq,source,k,f) for k in range(3,6)])
                            rest=rotation([pose(donor,0,source,k,0) for k in range(3,6)])
                            motion=multiply(current,transpose(rest))
                            if b==1:motion=multiply(multiply(basis,motion),transpose(basis))
                            target=angles(multiply(motion,rotation(base[b][3:])))
                            delta=(target[c-3]-base[b][c]+math.pi)%(2*math.pi)-math.pi
                    values.append(delta)
                channels.append(values)
            animation.append(channels)
        samples.append(animation)
    scales=[[max(max(abs(v) for v in seq[b][c]) for seq in samples)/30000 or .00001 for c in range(6)] for b in range(count)]
    out=bytearray(hive[:integer(hive,188)])
    def append(data):
        while len(out)%4:out.append(0)
        offset=len(out);out.extend(data);return offset
    for b in range(count):
        bone=integer(hive,144)+b*112
        st.pack_into('<6f',out,bone+64,*base[b]);st.pack_into('<6f',out,bone+88,*scales[b])
    descriptors=bytearray()
    for seq,animation in enumerate(samples):
        source=integer(donor,168)+seq*176
        descriptor=bytearray(hive[integer(hive,168):integer(hive,168)+176])
        descriptor[:40]=donor[source:source+40]
        frames=integer(donor,source+56)
        st.pack_into('<2i',descriptor,48,0,0) # no donor sound/attachment events
        st.pack_into('<i',descriptor,56,frames)
        st.pack_into('<2i',descriptor,68,0,0)
        st.pack_into('<3f',descriptor,76,0,0,0)
        st.pack_into('<i',descriptor,120,1)
        st.pack_into('<i',descriptor,156,0)
        track=bytearray(count*12)
        for b in range(count):
            for c in range(6):
                values=[round(v/scales[b][c]) for v in animation[b][c]]
                if not any(values):continue
                relative=len(track)-b*12
                if relative>65535:raise ValueError('Animation offset overflow')
                st.pack_into('<H',track,b*12+c*2,relative)
                for start in range(0,frames,255):
                    part=values[start:start+255]
                    track.extend(st.pack('<2B',len(part),len(part)))
                    track.extend(st.pack('<'+'h'*len(part),*part))
        st.pack_into('<i',descriptor,124,append(track))
        descriptors.extend(descriptor)
    st.pack_into('<2i',out,164,sequences,append(descriptors))
    # Keep all model structures before the texture pixel tail GoldSrc discards.
    texture_index=integer(hive,184)
    st.pack_into('<i',out,188,len(out))
    for n in range(integer(hive,180)):
        tex=texture_index+n*80;w,h,old=st.unpack_from('<3i',hive,tex+68)
        new=append(hive[old:old+w*h+768]);st.pack_into('<i',out,tex+76,new)
    st.pack_into('<i',out,72,len(out));OUT.write_bytes(out)

if __name__=='__main__':build_assets()
