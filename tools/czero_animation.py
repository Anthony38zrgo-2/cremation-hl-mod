"""Bake CS player gait + weapon pose into standalone monster sequences."""
import struct

def channels(data,bones,sequence,blend=0):
    count=struct.unpack_from('<i',data,sequence+56)[0]
    anim=struct.unpack_from('<i',data,sequence+124)[0]+blend*bones*12
    result=[]
    for bone in range(bones):
        record=anim+bone*12;row=[]
        for offset in struct.unpack_from('<6H',data,record):
            values=[]
            if offset:
                cursor=record+offset
                while len(values)<count:
                    valid,total=struct.unpack_from('<BB',data,cursor);assert 0<valid<=total
                    run=list(struct.unpack_from('<'+str(valid)+'h',data,cursor+2))
                    values.extend(run+[run[-1]]*(total-valid));cursor+=(valid+1)*2
                values=values[:count]
            else:values=[0]*count
            row.append(values)
        result.append(row)
    return result

def bake_gaits(data,torso_sequence=75):
    original_size=len(data)
    bones,bo=struct.unpack_from('<2i',data,140);seq=struct.unpack_from('<i',data,168)[0]
    names=[data[bo+i*112:bo+i*112+32].split(b'\0')[0] for i in range(bones)]
    parents=[struct.unpack_from('<i',data,bo+i*112+32)[0] for i in range(bones)]
    spine=names.index(b'Bip01 Spine');upper=[]
    for bone in range(bones):
        current=bone
        while current>=0 and current!=spine:current=parents[current]
        upper.append(current==spine)
    pose=channels(data,bones,seq+torso_sequence*176)
    reports=[]
    for index in (3,4):
        desc=seq+index*176;gait=channels(data,bones,desc);frames=len(gait[0][0]);assert frames<256
        block=bytearray(bones*12)
        for bone in range(bones):
            for channel in range(6):
                values=[pose[bone][channel][0]]*frames if upper[bone] else gait[bone][channel]
                if not any(values):continue
                offset=len(block)-bone*12;assert offset<65536
                struct.pack_into('<H',block,bone*12+channel*2,offset)
                # One guard sample also makes last-frame interpolation safe.
                samples=values+[values[0]]
                block.extend(struct.pack('<BB',len(samples),len(samples)));block.extend(struct.pack('<'+str(len(samples))+'h',*samples))
        animation=len(data);data.extend(block)
        struct.pack_into('<2i',data,desc+120,1,animation)
        reports.append(dict(sequence=index,frames=frames,upper_bones=sum(upper),lower_bones=bones-sum(upper)))
    # GoldSrc separates/caches studio animation data before its texture section.
    # Keep the new tracks there, rather than after the embedded texture pixels.
    extra=data[original_size:];del data[original_size:]
    insertion=struct.unpack_from('<i',data,184)[0];delta=len(extra)
    data[insertion:insertion]=extra
    for offset in (144,152,160,168,176,184,188,200,208,216,224,232,240):
        value=struct.unpack_from('<i',data,offset)[0]
        if value>=insertion:struct.pack_into('<i',data,offset,value+delta)
    textures,texture_table=struct.unpack_from('<2i',data,180)
    for index in range(textures):
        offset=texture_table+index*80+76;value=struct.unpack_from('<i',data,offset)[0]
        if value>=insertion:struct.pack_into('<i',data,offset,value+delta)
    for index in (3,4):
        offset=seq+index*176+124;old=struct.unpack_from('<i',data,offset)[0]
        struct.pack_into('<i',data,offset,insertion+old-original_size)
    # One complete slash per gameplay cooldown; no repeated throw/slash loops.
    desc=seq+76*176;frames=struct.unpack_from('<i',data,desc+56)[0]
    struct.pack_into('<f',data,desc+32,(frames-1)/.65)
    for index in (57,76):
        desc=seq+index*176;flags=struct.unpack_from('<i',data,desc+36)[0];struct.pack_into('<i',data,desc+36,flags&~1)
    struct.pack_into('<i',data,72,len(data))
    return reports
