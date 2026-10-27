// Health loot is separate from physical gibs. Lifetimes and death-roll marks
// use saved native entvars; no corpse can roll again when damaged or gibbed.
static constexpr int HeartRolled=1<<29;
#ifdef CREMATION_ENABLE_VALIDATION
static void heartProbeFrame();
#endif
static constexpr const char* HeartModel="models/cremation/heart_pickup.mdl";
static constexpr const char* HeartSquare="sprites/cremation/heart_red_square.spr";
static bool heartIs(edict_t* e){return valid(e)&&!std::strcmp(text(e->v.targetname),"cremation_heart");}
static bool heartRollDeath(edict_t* e){
    if(!valid(e)||!(e->v.flags&FL_MONSTER)||!e->pvPrivateData||(e->v.spawnflags&HeartRolled))return false;
    auto table=*reinterpret_cast<void***>(e->pvPrivateData);int faction=reinterpret_cast<int(__thiscall*)(void*)>(table[ravenholm_abi::slot::Classify])(e->pvPrivateData);
    if(faction==0||faction==1||faction==2||faction==3||faction==11)return false;
    e->v.spawnflags|=HeartRolled;
    return engine.pfnRandomLong(1,100)<=10;
}
static void heartPrecache(){
    engine.pfnPrecacheModel(const_cast<char*>(HeartModel));engine.pfnPrecacheModel(const_cast<char*>(HeartSquare));
    engine.pfnPrecacheSound(const_cast<char*>("items/smallmedkit1.wav"));
}
static edict_t* heartSpawn(const Vector& origin){
    auto e=engine.pfnCreateNamedEntity(engine.pfnAllocString("info_target"));if(!valid(e))return nullptr;raven.pfnSpawn(e);
    e->v.targetname=engine.pfnAllocString("cremation_heart");engine.pfnSetModel(e,HeartModel);e->v.body=0;
    e->v.solid=SOLID_NOT;e->v.movetype=MOVETYPE_NONE;e->v.takedamage=DAMAGE_NO;
    e->v.scale=1.2f;e->v.renderfx=kRenderFxGlowShell;e->v.rendercolor=Vector(255,25,20);e->v.renderamt=8;
    e->v.oldorigin=origin;e->v.teleport_time=globals->time;e->v.dmgtime=globals->time+10;e->v.pain_finished=0;
    engine.pfnSetOrigin(e,&origin.x);return e;
}
static void heartRemove(edict_t* e){
    for(int i=1;i<globals->maxEntities;++i){auto a=engine.pfnPEntityOfEntIndex(i);
        if(valid(a)&&a->v.owner==e&&!std::strcmp(text(a->v.targetname),"cremation_heart_square"))engine.pfnRemoveEntity(a);
    }if(valid(e))engine.pfnRemoveEntity(e);
}
static bool heartConsume(edict_t* e,edict_t* p){
    if(!heartIs(e)||!valid(p)||!(p->v.flags&FL_CLIENT)||p->v.deadflag!=DEAD_NO||p->v.health<=0||p->v.health>=100||globals->time>=e->v.dmgtime)return false;
    if(length2d(p->v.origin-e->v.origin)>32||std::fabs(p->v.origin.z-e->v.origin.z)>48)return false;
    Vector eye=p->v.origin+p->v.view_ofs;TraceResult tr{};engine.pfnTraceLine(&eye.x,&e->v.origin.x,1,p,&tr);
    if(tr.fStartSolid||tr.fAllSolid||tr.flFraction<1)return false;
    p->v.health=std::min(100.f,p->v.health+25.f);
    engine.pfnEmitSound(p,CHAN_ITEM,"items/smallmedkit1.wav",1,ATTN_NORM,0,100);heartRemove(e);return true;
}
static void heartFrame(){
    std::vector<edict_t*> hearts,squares;
    for(int i=1;i<globals->maxEntities;++i){auto e=engine.pfnPEntityOfEntIndex(i);if(!valid(e))continue;
        if(heartIs(e))hearts.push_back(e);
        else if(!std::strcmp(text(e->v.targetname),"cremation_heart_square"))squares.push_back(e);
    }
    for(auto e:hearts){
        if(globals->time>=e->v.dmgtime){heartRemove(e);continue;}
        float age=globals->time-e->v.teleport_time;
        Vector at=e->v.oldorigin+Vector(0,0,std::sin(age*3)*5);engine.pfnSetOrigin(e,&at.x);
        e->v.angles=Vector(65,std::fmod(age*60,360.f),0);
        if(globals->time>=e->v.pain_finished){e->v.pain_finished=globals->time+.15f;
            engine.pfnMessageBegin(MSG_PVS,SvcTemp,&at.x,nullptr);engine.pfnWriteByte(TE_DLIGHT);coords(at);
            for(int n:{8,255,20,15,2,0})engine.pfnWriteByte(n);engine.pfnMessageEnd();
        }
        bool seen[6]={};
        for(auto a:squares)if(valid(a)&&a->v.owner==e&&a->v.impulse==e->serialnumber&&a->v.skin>=0&&a->v.skin<6)seen[a->v.skin]=true;
        for(int j=0;j<6;++j)if(!seen[j]){
            auto a=engine.pfnCreateNamedEntity(engine.pfnAllocString("info_target"));if(!valid(a))continue;raven.pfnSpawn(a);
            a->v.targetname=engine.pfnAllocString("cremation_heart_square");a->v.owner=e;a->v.impulse=e->serialnumber;a->v.skin=j;
            a->v.solid=SOLID_NOT;a->v.movetype=MOVETYPE_NONE;a->v.scale=.12f;a->v.rendermode=kRenderTransAdd;a->v.renderamt=220;
            engine.pfnSetModel(a,HeartSquare);squares.push_back(a);
        }
        for(int i=1;i<=globals->maxClients&&heartIs(e);++i){auto p=engine.pfnPEntityOfEntIndex(i);heartConsume(e,p);}
    }
    for(auto a:squares){if(!valid(a))continue;auto e=a->v.owner;
        if(!heartIs(e)||a->v.impulse!=e->serialnumber){engine.pfnRemoveEntity(a);continue;}
        float age=globals->time-e->v.teleport_time,angle=age*2.5f+a->v.skin*1.047198f;
        Vector at=e->v.origin+Vector(std::cos(angle)*20,std::sin(angle)*20,std::sin(age*4+a->v.skin)*13);
        engine.pfnSetOrigin(a,&at.x);
    }
}
