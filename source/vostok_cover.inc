// Native entvars keep the original animation state across saves. Invisible
// func_wall brushes supply collision independently of swaying studio meshes.
static bool vostokCover(edict_t* e) {
    return valid(e)&&e->v.targetname&&!std::strncmp(text(e->v.targetname),"vostok_cover_",13);
}
static bool vostokCoverSpawn(edict_t* e) {
    if(!vostokCover(e))return false;
    const char* path=text(e->v.model);
    engine.pfnPrecacheModel(const_cast<char*>(path));engine.pfnSetModel(e,path);
    // Donor headers use zero global bounds. Retain the animated canopy in the
    // PVS even when its trunk origin falls outside the current view.
    auto mdl=static_cast<unsigned char*>(engine.pfnGetModelPtr(e));
    if(mdl){int index=*reinterpret_cast<int*>(mdl+168);
        engine.pfnSetSize(e,reinterpret_cast<float*>(mdl+index+96),reinterpret_cast<float*>(mdl+index+108));}
    e->v.movetype=MOVETYPE_NONE;e->v.solid=SOLID_NOT;e->v.takedamage=DAMAGE_NO;
    e->v.sequence=0;e->v.frame=0;e->v.animtime=globals->time;
    if(e->v.framerate<=0)e->v.framerate=1;
    e->v.nextthink=globals->time+.05f;
    return true;
}
static bool vostokCoverThink(edict_t* e) {
    if(!vostokCover(e))return false;
    auto mdl=static_cast<unsigned char*>(engine.pfnGetModelPtr(e));
    if(mdl){
        int index=*reinterpret_cast<int*>(mdl+168);
        float fps=*reinterpret_cast<float*>(mdl+index+32);
        int frames=*reinterpret_cast<int*>(mdl+index+56);
        if(frames>1)e->v.frame=std::fmod(e->v.frame+
            (globals->time-e->v.animtime)*fps*256.f/(frames-1)*e->v.framerate,256.f);
    }
    e->v.animtime=globals->time;e->v.nextthink=globals->time+.05f;
    return true;
}
