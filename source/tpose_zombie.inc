// Tpose: calavera roja del minion con brillo magenta, invertida en T-pose,
// vuelo + bolas de energia como el alien controller. Sin animaciones propias:
// la locomocion queda congelada y el disparo es una bola nativa directa.
static constexpr const char* TposeClass="monster_tpose";
static constexpr const char* TposeModel="models/cremation/minion.mdl";
static constexpr int TposeFrozenSeq=0;
static constexpr float TposeHealth=80.f;
static constexpr float TposeHover=90.f;
static constexpr float TposeSpeed=140.f;
static constexpr float TposeShootCooldown=2.4f;
static constexpr float TposeShootRange=1200.f;
static bool tposeIs(edict_t* e){return valid(e)&&!std::strcmp(text(e->v.classname),TposeClass);}
static void tposePrecache(){
    engine.pfnPrecacheModel(const_cast<char*>(TposeModel));
    // Las bolas del controller se crean en caliente con raven.pfnSpawn; aqui
    // solo se precargan sus assets directos. No se instancian entidades en
    // precarga: el hookSpawn generico sobre clases nativas del donor en esta
    // fase rompia el arranque (0xC0000005 entre WeaponBalance y voces).
    engine.pfnPrecacheModel(const_cast<char*>("sprites/xspark4.spr"));
    engine.pfnPrecacheModel(const_cast<char*>("sprites/xspark1.spr"));
    for(auto s:{"debris/zap4.wav","weapons/electro4.wav"})engine.pfnPrecacheSound(const_cast<char*>(s));
}
static void tposeSpawn(edict_t* e){
    // El Spawn nativo del controller ya corrio: casco, vuelo, faccion y sonidos.
    // Solo se sustituye la visual por el minion invertido con brillo magenta.
    // El minion mide ~72 de alto con origen en los pies: invertido (pitch 180)
    // cuelga de 0 a -72. La hull se centra sobre lo visual (-80..+8) y se
    // extiende (+-32) para que los impactos correspondan al modelo.
    e->v.classname=engine.pfnAllocString(TposeClass);
    engine.pfnSetModel(e,TposeModel);
    Vector lo(-32,-32,-80),hi(32,32,8);engine.pfnSetSize(e,&lo.x,&hi.x);
    e->v.movetype=MOVETYPE_FLY;e->v.solid=SOLID_SLIDEBOX;e->v.flags|=FL_FLY|FL_MONSTER;
    e->v.health=e->v.max_health=TposeHealth;e->v.takedamage=DAMAGE_AIM;
    e->v.view_ofs=Vector(0,0,-36);
    e->v.rendermode=kRenderNormal;e->v.renderfx=kRenderFxGlowShell;
    e->v.rendercolor=Vector(255,0,255);e->v.renderamt=25;
    e->v.sequence=TposeFrozenSeq;e->v.frame=0;e->v.framerate=0;e->v.animtime=globals->time;
    e->v.angles.x=180;e->v.angles.z=0;
    e->v.teleport_time=globals->time+1.f;
    if(e->pvPrivateData)reinterpret_cast<void(__thiscall*)(void*)>(ravenholm_abi::address(ravenModule,ravenholm_abi::ResetSequenceInfoRva))(e->pvPrivateData);
    startupTrace("TposeSpawn","minion invertido magenta; vuelo + bolas controller");
}
static edict_t* tposeEnemy(edict_t* e){
    edict_t* best=nullptr;float closest=TposeShootRange;
    for(int i=1;i<=globals->maxClients;++i){
        auto p=engine.pfnPEntityOfEntIndex(i);
        if(!valid(p)||!(p->v.flags&FL_CLIENT)||p->v.health<=0||p->v.deadflag!=DEAD_NO)continue;
        if(dmcInvisible(p))continue;
        float d=length(p->v.origin-e->v.origin);if(d>closest)continue;
        Vector from=e->v.origin+e->v.view_ofs,to=p->v.origin+p->v.view_ofs;TraceResult tr{};
        engine.pfnTraceLine(&from.x,&to.x,1,e,&tr);
        if(tr.fStartSolid||!(tr.flFraction>=.999f||tr.pHit==p))continue;
        closest=d;best=p;
    }
    return best;
}
static void tposeShoot(edict_t* e,edict_t* player){
    Vector fwd;engine.pfnMakeVectors(&e->v.angles.x);fwd=globals->v_forward;
    // La bola sale del pecho visual (centro del minion invertido, -40).
    Vector src=e->v.origin+Vector(0,0,-40)+normal(Vector(fwd.x,fwd.y,0))*32.f;
    auto ball=engine.pfnCreateNamedEntity(engine.pfnAllocString("controller_energy_ball"));
    if(!valid(ball))return;
    ball->v.origin=src;ball->v.angles=e->v.angles;ball->v.owner=e;
    raven.pfnSpawn(ball);
    if(!valid(ball))return;
    Vector target=center(player);
    Vector dir=normal(target-src);
    float speed=engine.pfnCVarGetFloat?engine.pfnCVarGetFloat("sk_controller_speedball1"):0.f;
    if(!(speed>0))speed=650.f;
    ball->v.velocity=dir*speed;ball->v.owner=e;
    engine.pfnEmitSound(e,CHAN_WEAPON,"debris/zap4.wav",1,ATTN_NORM,0,100);
}
static bool tposeThink(edict_t* e){
    if(!tposeIs(e))return false;
    if(!aliveEnemy(e))return false;
    e->v.nextthink=globals->time+.1f;
    // Visual permanente: minion + magenta + invertido + congelado en T-pose.
    if(std::strcmp(text(e->v.model),TposeModel))engine.pfnSetModel(e,TposeModel);
    e->v.rendermode=kRenderNormal;e->v.renderfx=kRenderFxGlowShell;
    e->v.rendercolor=Vector(255,0,255);e->v.renderamt=25;
    e->v.movetype=MOVETYPE_FLY;e->v.flags|=FL_FLY;
    auto player=tposeEnemy(e);
    float yaw=e->v.angles.y;
    if(valid(player)){Vector d=player->v.origin-e->v.origin;yaw=std::atan2(d.y,d.x)*180.f/3.14159265f;}
    e->v.angles=Vector(180,yaw,0);
    // Sin animaciones: congelado salvo muerte nativa.
    e->v.sequence=TposeFrozenSeq;e->v.frame=0;e->v.framerate=0;e->v.animtime=globals->time;
    if(pushed(e))return true;
    float step=TposeSpeed*.1f*(slowed(e)?balance::SlowFactor:1.f);
    Vector want=e->v.origin;
    if(valid(player)){
        float orbit=globals->time*.6f+float(engine.pfnIndexOfEdict(e))*1.7f;
        Vector anchor=player->v.origin+Vector(std::cos(orbit)*180.f,std::sin(orbit)*180.f,0);
        Vector top=anchor+Vector(0,0,256),bottom=anchor-Vector(0,0,256);TraceResult g{};
        engine.pfnTraceLine(&top.x,&bottom.x,1,e,&g);
        float floorZ=!g.fStartSolid&&g.flFraction<1?g.vecEndPos.z:e->v.origin.z-TposeHover;
        want=Vector(anchor.x,anchor.y,floorZ+TposeHover);
        Vector delta=want-e->v.origin;float dist=length(delta);
        if(dist>8.f){
            Vector dir=normal(delta);float move=std::min(dist,step);
            Vector to=e->v.origin+dir*move;TraceResult tr{};
            engine.pfnTraceMonsterHull(e,&e->v.origin.x,&to.x,1,e,&tr);
            if(!tr.fStartSolid&&!tr.fAllSolid&&tr.flFraction>=.999f)engine.pfnSetOrigin(e,&to.x);
            else for(float turn:{45.f,-45.f,90.f,-90.f,135.f,-135.f}){
                float a=std::atan2(dir.y,dir.x)*180.f/3.14159265f+turn;
                float r=a*3.14159265f/180.f;Vector alt=e->v.origin+Vector(std::cos(r)*move,std::sin(r)*move,dir.z*move*.5f);
                engine.pfnTraceMonsterHull(e,&e->v.origin.x,&alt.x,1,e,&tr);
                if(!tr.fStartSolid&&!tr.fAllSolid&&tr.flFraction>=.999f){engine.pfnSetOrigin(e,&alt.x);break;}
            }
        }
        if(globals->time>=e->v.teleport_time&&length(player->v.origin-e->v.origin)<=TposeShootRange){
            tposeShoot(e,player);
            e->v.teleport_time=globals->time+TposeShootCooldown;
        }
    }else{
        // Sin jugador: flotar en su sitio sin caer.
        e->v.velocity=Vector(0,0,0);
    }
    return true;
}
