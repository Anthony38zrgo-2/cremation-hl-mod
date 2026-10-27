// Mansion state lives in native saved entvars, not session-only counters.
// Controller: frags=wave, armorvalue=emitted, health=CT cursor,
// dmgtime=next spawn, teleport_time=next wave, impulse=finished.
#include "mansion_waves.h"
static float mansionHudAt=0,mansionTickAt=0,mansionVerifyAt=0;
#ifdef CREMATION_ENABLE_VALIDATION
static bool mansionTest=false;
static bool mansionRouteTest=false;
static bool mansionCZRouteTest=false;
static bool mansionInventoryTest=false,mansionInventoryChecked=false;
static float mansionRouteReport=0;
#endif
static float mansionMoveElapsed=0;
static bool mansionMap(){return globals->mapname&&!std::strcmp(text(globals->mapname),"cs_mansion");}
static edict_t* mansionFind(const char* name){auto e=engine.pfnFindEntityByString(nullptr,"targetname",name);return e&&engine.pfnIndexOfEdict(e)>0?e:nullptr;}
static bool mansionActor(edict_t* e){return valid(e)&&e->v.targetname&&!std::strncmp(text(e->v.targetname),"mansion_enemy_",14);}
static edict_t* mansionState(){return mansionFind("mansion_wave_state");}
static int mansionAlive(){int n=0;for(int i=1;i<globals->maxEntities;++i){auto e=engine.pfnPEntityOfEntIndex(i);if(mansionActor(e)&&aliveEnemy(e))++n;}return n;}
static bool mansionHasPlayer(){for(int i=1;i<=globals->maxClients;++i){auto p=engine.pfnPEntityOfEntIndex(i);if(valid(p)&&(p->v.flags&FL_CLIENT)&&p->v.health>0&&p->v.deadflag==DEAD_NO)return true;}return false;}
static bool mansionContact(edict_t* e){
    if(czIs(e))return czObjectiveOwns(e);
    if(qzIs(e))return qzEnemy(e)!=nullptr;
    for(int i=1;i<globals->maxEntities;++i){auto p=engine.pfnPEntityOfEntIndex(i);
        if(!valid(p)||p->v.health<=0||p->v.deadflag!=DEAD_NO)continue;
        if(!(p->v.flags&FL_CLIENT)&&!necroMinion(p)&&!bounce::isSentry(p))continue;
        if(length(p->v.origin-e->v.origin)>2048)continue;
        Vector from=e->v.origin+e->v.view_ofs,to=p->v.origin+p->v.view_ofs;TraceResult tr{};
        engine.pfnTraceLine(&from.x,&to.x,1,e,&tr);
        if(tr.flFraction==1||tr.pHit==p)return true;
    }return false;
}
static bool mansionRouteActive(edict_t* e){return mansionMap()&&mansionActor(e)&&aliveEnemy(e)&&e->v.target&&!mansionContact(e);}
static bool mansionShortcut(edict_t* e,const Vector& destination){
    Vector delta=destination-e->v.origin;float distance=length2d(delta);
    if(distance>96||std::fabs(delta.z)>18)return false;
    float floor=std::max(destination.z,e->v.origin.z);
    Vector from(e->v.origin.x,e->v.origin.y,floor+36.1f),to(destination.x,destination.y,floor+36.1f);TraceResult tr{};
    engine.pfnTraceHull(&from.x,&to.x,1,1,e,&tr);if(tr.fStartSolid||tr.fAllSolid||tr.flFraction<1)return false;
    int steps=std::max(1,int(std::ceil(distance/8)));
    for(int i=0;i<=steps;++i){Vector at=e->v.origin+delta*(float(i)/steps);Vector top(at.x,at.y,floor+18),bottom(at.x,at.y,floor-20);
        engine.pfnTraceLine(&top.x,&bottom.x,1,e,&tr);if(tr.fStartSolid||tr.flFraction==1||tr.vecPlaneNormal.z<.65f)return false;}
    return true;
}
static void mansionMove(edict_t* e,float dt){
    if(!mansionRouteActive(e)||pushed(e)||czObjectiveOwns(e))return;
    if(czIs(e)&&!czSmokeCanLeave(e))return;
    if(czIs(e)&&czMeleeBusy(e))return;
    if(qzIs(e)&&(e->v.impulse!=QZIdle&&e->v.impulse!=QZChase))return;
    auto goal=mansionFind(text(e->v.target));if(!valid(goal))return;
    // Local avoidance can carry an actor past a bend. Advance to the following
    // marker only when it is closer and its swept hull/floor are still valid;
    // otherwise actors on the same route can face each other at an old bend.
    if(goal->v.target){auto next=mansionFind(text(goal->v.target));
        if(valid(next)&&length2d(next->v.origin-e->v.origin)<length2d(goal->v.origin-e->v.origin)&&mansionShortcut(e,next->v.origin)){
            e->v.target=goal->v.target;goal=next;
        }
    }
    Vector delta=goal->v.origin-e->v.origin;
    float reach=goal->v.target?18.f:20.f;
    const char* goalName=text(goal->v.targetname);size_t goalLength=std::strlen(goalName);
    if(goalLength>=3&&!std::strcmp(goalName+goalLength-3,"_01"))reach=48;
    // Routes meet beside the porch; allow room to turn without opposing traffic.
    if(std::fabs(goal->v.origin.x-208)<1&&std::fabs(goal->v.origin.y+256)<1)reach=40;
    if(length2d(delta)<reach&&std::fabs(delta.z)<20){
        if(!goal->v.target){e->v.target=0;return;}
        e->v.target=goal->v.target;goal=mansionFind(text(e->v.target));if(!valid(goal))return;delta=goal->v.origin-e->v.origin;
    }
    float yaw=std::atan2(delta.y,delta.x)*180.f/3.14159265f;
    const char* name=text(e->v.classname);
    float speed=qzIs(e)?35.f:(!std::strcmp(name,"monster_fastzombie")?110.f:((czIs(e)||!std::strcmp(name,"monster_human_grunt"))?85.f:55.f));
    float distance=std::min(length2d(delta),speed*dt)*(qzIs(e)&&slowed(e)?balance::SlowFactor:1.f);
    bool moved=qzIs(e)?qzWalk(e,yaw,distance):hookWalkMove(e,yaw,distance,0)!=0;
    // Collision-only local avoidance; no SetOrigin or cinematic repositioning.
    if(!moved)for(float turn:{30.f,-30.f,60.f,-60.f,90.f,-90.f}){
        if(qzIs(e)?qzWalk(e,yaw+turn,distance):hookWalkMove(e,yaw+turn,distance,0)!=0){yaw+=turn;moved=true;break;}
    }
    if(moved){e->v.angles.y=yaw;if(czIs(e)){czSequence(e,3);e->v.fuser1=globals->time+.15f;}else if(!qzIs(e))hordeWalkAnimation(e);}
}
static void mansionPrecache(){
    if(!mansionMap())return;
    for(auto path:{"debris/flesh1.wav","debris/flesh2.wav","debris/flesh3.wav"})engine.pfnPrecacheSound(const_cast<char*>(path));
    // Native Spawn performs each monster's full sound/model precache now,
    // before GoldSrc closes its resource list. No prototypes enter the wave.
    for(int type=0;type<6;++type){auto e=engine.pfnCreateNamedEntity(engine.pfnAllocString(mansion::classname(type)));if(!valid(e))continue;
        e->v.origin=Vector(1336,392,24);hookSpawn(e);engine.pfnRemoveEntity(e);}
}
static bool mansionEmit(edict_t* state){
    int wave=int(state->v.frags),index=int(state->v.armorvalue),type=mansion::kind(wave,index);
#ifdef CREMATION_ENABLE_VALIDATION
    bool johnny=mansion::isJohnny(wave,index)&&!mansionCZRouteTest;
    const char* actorMarkerFormat=johnny?"mansion_enemy_johnny_%d_%02d":(wave<=2?"mansion_enemy_fixed_%d_%02d":"mansion_enemy_%d_%02d");
    if(mansionCZRouteTest)type=5;else if(johnny)type=0;
#else
    bool johnny=mansion::isJohnny(wave,index);
    const char* actorMarkerFormat=johnny?"mansion_enemy_johnny_%d_%02d":(wave<=2?"mansion_enemy_fixed_%d_%02d":"mansion_enemy_%d_%02d");
    if(johnny)type=0;
#endif
    // Try only the next CT spawn. If occupied, wait for it to clear, preserving
    // exact round-robin order instead of stacking enemies or skipping points.
    int cursor=int(state->v.health)%mansion::SpawnCount;
    if(type==5){
        std::vector<int> available;
        for(int i=0;i<mansion::SpawnCount;++i){char name[40];std::snprintf(name,sizeof name,"mansion_ct_%02d",i);auto candidate=mansionFind(name);if(!valid(candidate))continue;
            Vector hull=candidate->v.origin-Vector(0,0,3.9f);TraceResult tr{};engine.pfnTraceHull(&hull.x,&hull.x,0,1,nullptr,&tr);
            if(!tr.fStartSolid&&!tr.fAllSolid)available.push_back(i);
        }
        if(available.empty())return false;
        cursor=available[engine.pfnRandomLong(0,int(available.size())-1)];state->v.dmg_save=float(cursor+1);
    }
    char marker[64];std::snprintf(marker,sizeof marker,"mansion_ct_%02d",cursor);
    auto point=mansionFind(marker);if(!valid(point))return false;
    Vector at=point->v.origin-Vector(0,0,39.9f),hull=at+Vector(0,0,36);TraceResult tr{};
    engine.pfnTraceHull(&hull.x,&hull.x,0,1,nullptr,&tr);if(tr.fStartSolid||tr.fAllSolid)return false;
    if(type==1){Vector top=at+Vector(0,0,QZHeight);engine.pfnTraceLine(&at.x,&top.x,1,nullptr,&tr);if(tr.fStartSolid||tr.flFraction<1)return false;}
    auto e=engine.pfnCreateNamedEntity(engine.pfnAllocString(mansion::classname(type)));if(!valid(e))return false;
    e->v.origin=at;e->v.angles=point->v.angles;
    if(type==4){static const int weapons[]={1,8,3,10,5,12};e->v.weapons=weapons[(index/4+index%4)%6];}
    std::snprintf(marker,sizeof marker,actorMarkerFormat,wave,index);
    e->v.targetname=engine.pfnAllocString(marker);
    hookSpawn(e);
    if(type==5)e->v.weapons=cursor%4;
    if(!aliveEnemy(e)){engine.pfnRemoveEntity(e);startupTrace("MansionError","monster Spawn failed");return false;}
    std::snprintf(marker,sizeof marker,actorMarkerFormat,wave,index);e->v.targetname=engine.pfnAllocString(marker);
    std::snprintf(marker,sizeof marker,"mansion_route_%02d_00",cursor);e->v.target=engine.pfnAllocString(marker);
    // Quake's standalone idle patrol must not independently move this actor;
    // the shared Mansion approach owns idle movement; native combat stays intact.
    e->v.chain=nullptr;
    engine.pfnSetOrigin(e,&at.x);
    state->v.armorvalue+=1;if(type!=5)state->v.health=float((cursor+1)%mansion::SpawnCount);
    char report[180];std::snprintf(report,sizeof report,"wave=%d index=%d ct=%d type=%s at=%.0f,%.0f,%.0f",wave,index,cursor,johnny?"monster_johnny":mansion::classname(type),at.x,at.y,at.z);startupTrace("MansionSpawn",report);
    return true;
}
static void mansionHud(edict_t* state,int alive){
    char message[180];int wave=int(state->v.frags),pending=mansion::total(wave)-int(state->v.armorvalue);
    if(state->v.impulse==2)std::snprintf(message,sizeof message,"GAME OVER | LA BOMBA HA EXPLOTADO");
    else if(state->v.impulse)std::snprintf(message,sizeof message,"OLEADA 4/4 COMPLETADA | ENEMIGOS VIVOS: 0");
    else if(!wave)std::snprintf(message,sizeof message,"OLEADA 1/4 | ENEMIGOS VIVOS: 0");
    else std::snprintf(message,sizeof message,"OLEADA %d/4 | ENEMIGOS VIVOS: %d%s",wave,alive,pending>0?" | REFUERZOS EN CAMINO":"");
    for(int i=1;i<=globals->maxClients;++i){auto p=engine.pfnPEntityOfEntIndex(i);if(!valid(p)||!(p->v.flags&FL_CLIENT))continue;
        engine.pfnMessageBegin(MSG_ONE_UNRELIABLE,SvcTemp,nullptr,p);engine.pfnWriteByte(TE_TEXTMESSAGE);engine.pfnWriteByte(4);
        engine.pfnWriteShort(-8192);engine.pfnWriteShort(int(.86f*8192));engine.pfnWriteByte(0);
        for(int c:{235,210,150,255,235,210,150,255})engine.pfnWriteByte(c);
        engine.pfnWriteShort(0);engine.pfnWriteShort(0);engine.pfnWriteShort(384);engine.pfnWriteString(message);engine.pfnMessageEnd();
    }
}
static void mansionActivate(){
    mansionHudAt=mansionTickAt=mansionVerifyAt=mansionMoveElapsed=0;
#ifdef CREMATION_ENABLE_VALIDATION
    mansionRouteReport=0;char env[8]{};GetEnvironmentVariableA("CREMATION_MANSION_VALIDATE",env,sizeof env);mansionTest=!std::strcmp(env,"1");mansionRouteTest=!std::strcmp(env,"2");
    mansionCZRouteTest=!std::strcmp(env,"4");mansionRouteTest=mansionRouteTest||mansionCZRouteTest;
#endif
    if(mansionMap())startupTrace("MansionActivate","TT=10 CT=40 sectors=N,E,S,W waves=41,41,81,81 CZ=1/wave");
#ifdef CREMATION_ENABLE_VALIDATION
    mansionInventoryTest=!std::strcmp(env,"3");mansionInventoryChecked=false;
#endif
}
static void mansionFrame(float dt){
    if(!mansionMap())return;auto state=mansionState();if(!valid(state))return;
    if(state->v.impulse==2){if(globals->time>=mansionHudAt){mansionHud(state,mansionAlive());mansionHudAt=globals->time+.5f;}return;}
#ifdef CREMATION_ENABLE_VALIDATION
    if(mansionInventoryTest){
        if(!mansionInventoryChecked){mansionInventoryChecked=true;auto player=engine.pfnCreateFakeClient("InventoryProbe");
            if(valid(player)){char reject[128]{};if(raven.pfnClientConnect)raven.pfnClientConnect(player,"InventoryProbe","127.0.0.1",reject);
                raven.pfnClientPutInServer(player);startupInventoryGive(player);
                char report[100];std::snprintf(report,sizeof report,"origin=%.0f,%.0f,%.0f weapons=%d",player->v.origin.x,player->v.origin.y,player->v.origin.z,player->v.weapons);startupTrace("InventoryProbeSpawn",report);
                auto drop=engine.pfnCreateNamedEntity(engine.pfnAllocString("weapon_rpg"));if(valid(drop)){drop->v.origin=player->v.origin+Vector(400,0,0);hookSpawn(drop);drop->v.owner=player;drop->v.effects|=EF_NODRAW;}
                mansionVerifyAt=globals->time+.5f;
            }
        }
        if(mansionVerifyAt>0&&globals->time>=mansionVerifyAt){int owned=0,ground=0;
            for(int i=1;i<globals->maxEntities;++i){auto e=engine.pfnPEntityOfEntIndex(i);if(!valid(e)||std::strncmp(text(e->v.classname),"weapon_",7))continue;if(valid(e->v.aiment)&&(e->v.aiment->v.flags&FL_CLIENT))++owned;else ++ground;}
            char report[100];std::snprintf(report,sizeof report,"owned=%d ground=%d",owned,ground);startupTrace("InventoryProbeComplete",report);mansionVerifyAt=0;
        }return;
    }
#endif
    int alive=mansionAlive();
    if(globals->time>=mansionHudAt){mansionHud(state,alive);mansionHudAt=globals->time+.5f;}
    mansionMoveElapsed+=dt;
    if(mansionMoveElapsed>=.05f){
        for(int i=1;i<globals->maxEntities;++i){auto e=engine.pfnPEntityOfEntIndex(i);if(mansionActor(e))mansionMove(e,std::min(mansionMoveElapsed,.1f));}
        mansionMoveElapsed=0;
    }
    if(globals->time<mansionTickAt)return;mansionTickAt=globals->time+.1f;
#ifdef CREMATION_ENABLE_VALIDATION
    if(mansionRouteTest){
        if(state->v.frags==0){state->v.frags=4;state->v.armorvalue=0;state->v.health=0;state->v.max_health=0;}
        if(state->v.armorvalue<(mansionCZRouteTest?1:20))mansionEmit(state);
        for(int i=1;i<globals->maxEntities;++i){auto e=engine.pfnPEntityOfEntIndex(i);if(!mansionActor(e)||!aliveEnemy(e))continue;
            if(!e->v.target){char report[180];std::snprintf(report,sizeof report,"%s type=%s at=%.1f,%.1f,%.1f",text(e->v.targetname),text(e->v.classname),e->v.origin.x,e->v.origin.y,e->v.origin.z);startupTrace("MansionRouteArrival",report);state->v.max_health+=1;engine.pfnRemoveEntity(e);}
            else if(globals->time>=mansionRouteReport){char report[250];std::snprintf(report,sizeof report,"%s at=%.1f,%.1f,%.1f target=%s flags=%d move=%d bounds=%.0f,%.0f,%.0f/%.0f,%.0f,%.0f",text(e->v.targetname),e->v.origin.x,e->v.origin.y,e->v.origin.z,text(e->v.target),int(e->v.flags),e->v.movetype,e->v.mins.x,e->v.mins.y,e->v.mins.z,e->v.maxs.x,e->v.maxs.y,e->v.maxs.z);startupTrace("MansionRouteProgress",report);}
        }
        if(globals->time>=mansionRouteReport)mansionRouteReport=globals->time+10;
        if(state->v.max_health==(mansionCZRouteTest?1:20)&&!state->v.impulse){state->v.impulse=1;startupTrace("MansionRouteComplete",mansionCZRouteTest?"CZ route reached main door":"20 routes from four fronts reached main door");}
        return;
    }
#endif
#ifdef CREMATION_ENABLE_VALIDATION
    if(!mansionHasPlayer()&&!mansionTest)return;
#else
    if(!mansionHasPlayer())return;
#endif
    if(state->v.impulse)return;
    if(state->v.frags==0){state->v.frags=1;state->v.armorvalue=0;state->v.health=0;state->v.dmg_save=0;
#ifdef CREMATION_ENABLE_VALIDATION
        state->v.dmgtime=globals->time+(mansionTest?.2f:5.f);
#else
        state->v.dmgtime=globals->time+5.f;
#endif
        startupTrace("MansionWave","begin=1 total=41");}
    int wave=int(state->v.frags),total=mansion::total(wave);
    if(int(state->v.armorvalue)<total){
        if(globals->time>=state->v.dmgtime){bool emitted=mansionEmit(state);
#ifdef CREMATION_ENABLE_VALIDATION
            state->v.dmgtime=globals->time+(emitted?(mansionTest?.05f:.375f):.25f);
#else
            state->v.dmgtime=globals->time+(emitted?.375f:.25f);
#endif
        }
    }else if(alive==0&&!czBombActive()){
        if(wave==4){state->v.impulse=1;startupTrace("MansionComplete","four waves cleared");}
        else if(state->v.teleport_time==0){
#ifdef CREMATION_ENABLE_VALIDATION
            state->v.teleport_time=globals->time+(mansionTest?.5f:8.f);
#else
            state->v.teleport_time=globals->time+8.f;
#endif
        }
        else if(globals->time>=state->v.teleport_time){state->v.frags+=1;state->v.armorvalue=0;state->v.dmg_save=0;state->v.teleport_time=0;state->v.dmgtime=globals->time;
            char report[64];std::snprintf(report,sizeof report,"begin=%d total=%d",wave+1,mansion::total(wave+1));startupTrace("MansionWave",report);}
    }
    // Explicit test-process mode only: native damage kills one actor at a time
    // to exercise death counting, all four compositions and final completion.
#ifdef CREMATION_ENABLE_VALIDATION
    if(mansionTest&&globals->time>=mansionVerifyAt){mansionVerifyAt=globals->time+.1f;
        for(int i=1;i<globals->maxEntities;++i){auto e=engine.pfnPEntityOfEntIndex(i);if(!mansionActor(e)||!aliveEnemy(e))continue;
            if(e->pvPrivateData){auto table=*reinterpret_cast<void***>(e->pvPrivateData);reinterpret_cast<TakeDamageFn>(table[ravenholm_abi::slot::TakeDamage])(e->pvPrivateData,&state->v,&state->v,10000.f,1<<6);}
            break;}
    }
#endif
}
