"""Validate that compiled hand strikes lead with the correct hands at their damage event."""
def verify_strikes(mdl, report):
    rows=[]
    for index,source,event,contact,hands in [(3,0x3EA,1,21,(5,)),(4,0x3E9,2,27,(8,)),(5,0x3C9,3,20,(5,8))]:
        source_seq=report['sequences'][index]
        assert source_seq['source_id']==source and source_seq['event_frame']==contact
        seq=mdl.i(168)+index*176
        assert mdl.i(seq+40)==(31 if event==3 else 30)
        assert mdl.i(seq+48)==1
        entry=mdl.i(seq+52)
        assert mdl.i(entry)==contact and mdl.i(entry+4)==event
        _,pos=mdl.pose(index,contact)
        lead=[float(pos[hand,0]-pos[2,0]) for hand in hands]
        assert min(lead)>10, (source_seq['name'], 'hands must lead ahead of the head',lead)
        rows.append(dict(sequence=source_seq['name'],source_motion=hex(source),event=event,contact_frame=contact,hand_lead_ahead_of_head=lead))
    return rows
