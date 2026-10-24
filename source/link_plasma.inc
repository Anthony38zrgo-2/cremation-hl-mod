// Animated additive plasma: irregular green body, white/yellow core and diffuse halo.
static constexpr const char* PlasmaBody="sprites/cremation/link_plasma_body.spr";
static constexpr const char* PlasmaCore="sprites/cremation/link_plasma_core.spr";
static constexpr const char* PlasmaHalo="sprites/cremation/link_plasma_halo.spr";
static void plasmaStyle(edict_t* e,const char* path,float scale,int brightness) {
    engine.pfnSetModel(e,path);e->v.effects&=~EF_NODRAW;
    e->v.movetype=MOVETYPE_NONE;e->v.solid=SOLID_NOT;e->v.takedamage=0;
    e->v.rendermode=kRenderTransAdd;e->v.renderfx=kRenderFxNone;
    e->v.renderamt=float(brightness);e->v.rendercolor=Vector(255,255,255);
    e->v.scale=scale;e->v.frame=0;e->v.framerate=0;e->v.animtime=globals->time;e->v.nextthink=0;
}
static edict_t* plasmaLayer(edict_t* owner,const Vector& origin,const char* path,float scale,int brightness) {
    auto e=engine.pfnCreateNamedEntity(engine.pfnAllocString("info_target"));if(!e)return nullptr;
    raven.pfnSpawn(e);e->v.targetname=engine.pfnAllocString("cremation_link_plasma_fx");
    e->v.owner=owner;plasmaStyle(e,path,scale,brightness);
    Vector mins(-18,-18,-18),maxs(18,18,18);engine.pfnSetSize(e,&mins.x,&maxs.x);engine.pfnSetOrigin(e,&origin.x);
    return e;
}
static void plasmaUpdate(NailProjectile& nail) {
    float age=globals->time-nail.born;
    nail.entity->v.frame=std::fmod(age*16.f,16.f);nail.entity->v.animtime=globals->time;
    nail.entity->v.scale=.25f*(1.f+.04f*std::sin(age*19.f));
    for(int i=0;i<2;++i) {
        auto e=nail.visuals[i];
        if(!valid(e)||e->serialnumber!=nail.visualSerials[i])continue;
        engine.pfnSetOrigin(e,&nail.entity->v.origin.x);
        e->v.frame=std::fmod(age*(i?12.f:16.f),16.f);e->v.animtime=globals->time;
        e->v.scale=(i?.4f:.22f)*(1.f+.06f*std::sin(age*13.f));
    }
    if(globals->time>=nail.nextLight) {
        nail.nextLight=globals->time+.08f;Vector at=nail.entity->v.origin;
        engine.pfnMessageBegin(MSG_PVS,SvcTemp,&at.x,nullptr);
        engine.pfnWriteByte(TE_DLIGHT);coords(at);engine.pfnWriteByte(10);
        engine.pfnWriteByte(35);engine.pfnWriteByte(255);engine.pfnWriteByte(45);
        engine.pfnWriteByte(1);engine.pfnWriteByte(0);engine.pfnMessageEnd();
    }
}
