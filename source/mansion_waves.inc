// Mansion state lives in native saved entvars, not session-only counters.
// Controller: frags=wave, armorvalue=emitted, health=CT cursor,
// dmgtime=next spawn, teleport_time=next wave, impulse=finished.
#include "mansion_waves.h"
static float mansionHudAt=0,mansionTickAt=0,mansionVerifyAt=0,mansionSpawnErrorAt=0;
#ifdef CREMATION_ENABLE_VALIDATION
static bool mansionTest=false;
static bool mansionJudgmentTest=false;
static void judgmentProbeFrame(float dt);
static bool mansionRouteTest=false;
static bool mansionCZRouteTest=false;
static bool mansionDoorTest=false;
static bool mansionDoorStateReported=false;
static bool mansionDoorTouchTest=false;
static edict_t* mansionDoorTouchPlayer=nullptr;
static int mansionDoorTouchStage=0;
static float mansionDoorTouchSince=0;
static bool mansionInventoryTest=false,mansionInventoryChecked=false;
static float mansionRouteReport=0;
#endif
static float mansionMoveElapsed=0;
static float mansionDoorHudAt=0;
static bool mansionNavWalk(edict_t* e,const Vector& goal,float dt,float speed);
static void mansionNavReset();
static void mansionNavBeginTick();
static void mansionNavFinishTick();
#ifdef CREMATION_ENABLE_VALIDATION
static void mansionNavReport();
static float mansionDoorStableSince=0;
#endif
struct MansionEntityRef{edict_t* entity=nullptr;int serial=0;};
static MansionEntityRef mansionController,mansionDoors[2];
static string_t mansionUpperTargets[4]{};
static int mansionStartWave(){
    if(!globals->mapname)return 0;
    const char* name=text(globals->mapname);
    if(!std::strcmp(name,"cs_mansion"))return 1;
    constexpr char prefix[]="cs_mansion_wave";
    if(std::strncmp(name,prefix,sizeof(prefix)-1))return 0;
    char* end=nullptr;long wave=std::strtol(name+sizeof(prefix)-1,&end,10);
    return end&&!*end&&wave>=1&&wave<=4?int(wave):0;
}
static bool mansionPerimeterCultistRoles[80]{};
static void mansionAssignCultistRoles(){
    for(bool& role:mansionPerimeterCultistRoles)role=false;
    int candidates[40]{},count=0,totalCultists=0;
    for(int index=0;index<mansion::hordeTotal(4);++index)if(mansion::cultistSlot(4,index)>=0){
        ++totalCultists;if(!mansion::entersBackDoor(4,index))candidates[count++]=index;
    }
    for(int i=count-1;i>0;--i){int j=engine.pfnRandomLong(0,i);std::swap(candidates[i],candidates[j]);}
    const int perimeter=std::min(totalCultists/2,count);
    for(int i=0;i<perimeter;++i)mansionPerimeterCultistRoles[candidates[i]]=true;
    char report[128];std::snprintf(report,sizeof report,"perimeter=%d door=%d backdoor=2",perimeter,count-perimeter);
    startupTrace("MansionCultistRoles",report);
}
static bool mansionMap(){return mansionStartWave()!=0;}
static edict_t* mansionFind(const char* name){auto e=engine.pfnFindEntityByString(nullptr,"targetname",name);return e&&engine.pfnIndexOfEdict(e)>0?e:nullptr;}
static bool mansionActor(edict_t* e){return valid(e)&&e->v.targetname&&!std::strncmp(text(e->v.targetname),"mansion_enemy_",14);}
static edict_t* mansionState(){
    auto& c=mansionController;
    if(valid(c.entity)&&c.entity->serialnumber==c.serial)return c.entity;
    c.entity=mansionFind("mansion_wave_state");c.serial=valid(c.entity)?c.entity->serialnumber:0;return c.entity;
}
static int mansionAlive(){int n=0;for(int i=1;i<globals->maxEntities;++i){auto e=engine.pfnPEntityOfEntIndex(i);if(mansionActor(e)&&aliveEnemy(e))++n;}return n;}
static bool mansionHasPlayer(){for(int i=1;i<=globals->maxClients;++i){auto p=engine.pfnPEntityOfEntIndex(i);if(valid(p)&&(p->v.flags&FL_CLIENT)&&p->v.health>0&&p->v.deadflag==DEAD_NO)return true;}return false;}
static Vector mansionDoorOrigin(int door){return door==1?Vector(0,-216,104):Vector(-360,-768,104);}
static edict_t* mansionDoor(int door){
    if(door<1||door>2)return nullptr;
    auto& cached=mansionDoors[door-1];
    if(valid(cached.entity)&&cached.entity->serialnumber==cached.serial)return cached.entity;
    const Vector expected=mansionDoorOrigin(door);
    for(edict_t* e=nullptr;(e=engine.pfnFindEntityByString(e,"classname",const_cast<char*>("func_door_rotating")))!=nullptr;){
        if(engine.pfnIndexOfEdict(e)<=0)continue;
        Vector delta=e->v.origin-expected;
        if(std::fabs(delta.x)<1.f&&std::fabs(delta.y)<1.f&&std::fabs(delta.z)<1.f){cached={e,e->serialnumber};return e;}
    }
    return nullptr;
}
static float mansionDoorHealth(int door){
    auto state=mansionState();if(!valid(state))return 0;
    if(!(state->v.iuser1&1)){
        auto a=mansionDoor(1),b=mansionDoor(2);
        state->v.vuser1.x=valid(a)?1000.f:0.f;
        state->v.vuser1.y=valid(b)?1000.f:0.f;
        state->v.iuser1|=1;
#ifdef CREMATION_ENABLE_VALIDATION
        if(mansionDoorTest&&!mansionDoorStateReported){
            mansionDoorStateReported=true;
            for(int i=1;i<=2;++i){auto d=i==1?a:b;char report[180];
                std::snprintf(report,sizeof report,"door=%d found=%d origin=%.0f,%.0f,%.0f solid=%d effects=%d",i,valid(d)?1:0,valid(d)?d->v.origin.x:0.f,valid(d)?d->v.origin.y:0.f,valid(d)?d->v.origin.z:0.f,valid(d)?d->v.solid:-1,valid(d)?d->v.effects:0);
                startupTrace("MansionDoorState",report);
            }
        }
#endif
    }
    float& saved=door==1?state->v.vuser1.x:state->v.vuser1.y;auto e=mansionDoor(door);
    if(!valid(e)||(e->v.effects&EF_NODRAW)||e->v.solid==SOLID_NOT)saved=0;
    return std::max(0.f,std::min(1000.f,saved));
}
static void mansionDoorFrame(){
    if(!mansionMap())return;auto state=mansionState();if(!valid(state))return;
    float first=mansionDoorHealth(1),second=mansionDoorHealth(2);
    if(!msgMansionDoors||globals->time<mansionDoorHudAt)return;mansionDoorHudAt=globals->time+.5f;
    for(int i=1;i<=globals->maxClients&&i<33;++i){auto p=engine.pfnPEntityOfEntIndex(i);if(!valid(p)||!(p->v.flags&FL_CLIENT))continue;
        engine.pfnMessageBegin(MSG_ONE_UNRELIABLE,msgMansionDoors,nullptr,p);engine.pfnWriteByte(1);
        engine.pfnWriteShort(int(first));engine.pfnWriteShort(int(second));engine.pfnMessageEnd();
    }
}
struct MansionDoorStrike{edict_t* actor=nullptr;int serial=0;float next=0;};
static std::vector<MansionDoorStrike> mansionDoorStrikes;
static bool mansionStrikeDoor(edict_t* actor,int door,float damage=65.f){
    auto panel=mansionDoor(door);auto state=mansionState();
    if(!aliveEnemy(actor)||!valid(panel)||!valid(state)||damage<=0||mansionDoorHealth(door)<=0)return false;
    MansionDoorStrike* strike=nullptr;for(auto& s:mansionDoorStrikes)if(s.actor==actor&&s.serial==actor->serialnumber){strike=&s;break;}
    if(!strike){mansionDoorStrikes.push_back({actor,actor->serialnumber,0});strike=&mansionDoorStrikes.back();}
    if(globals->time<strike->next)return false;
    Vector from=actor->v.origin+Vector(0,0,36),to=center(panel);if(length2d(to-from)>170)return false;
    TraceResult tr{};engine.pfnTraceHull(&from.x,&to.x,0,1,actor,&tr);
    if(tr.pHit!=panel&&tr.flFraction<.999f)return false;
    strike->next=globals->time+.65f;
    float& saved=door==1?state->v.vuser1.x:state->v.vuser1.y;
    saved=std::max(0.f,saved-damage);
#ifdef CREMATION_ENABLE_VALIDATION
    if(mansionDoorTest){char report[128];std::snprintf(report,sizeof report,"door=%d damage=%.0f remaining=%.0f",door,damage,saved);startupTrace("MansionDoorHit",report);}
#endif
    if(saved<=0){
        // Keep the native rotating door. Avoid its damage virtual entirely:
        // a breakable's Killed/Die path is a class-specific vtable path.
        constexpr int DoorNoAutoReturn=32;
        panel->v.spawnflags|=DoorNoAutoReturn;
        bool moving=length(panel->v.avelocity)>1.f;
        Vector turn=panel->v.angles;
        bool alreadyOpen=std::fabs(turn.x)>10.f||std::fabs(turn.y)>10.f||std::fabs(turn.z)>10.f;
        float swingDirection=turn.x*panel->v.avelocity.x+turn.y*panel->v.avelocity.y+turn.z*panel->v.avelocity.z;
        bool closing=moving&&swingDirection<-.01f;
        if((!moving&&!alreadyOpen)||closing)raven.pfnUse(panel,actor);
        else if(!moving)panel->v.nextthink=0;
        panel->v.solid=SOLID_NOT;
    }
    return true;
}
static void mansionDoorRepairPostThink(edict_t* player,int buttons){
    if(!mansionMap()||!valid(player)||(buttons&IN_USE)==0||player->v.health<=0)return;
    int slot=engine.pfnIndexOfEdict(player);if(slot<=0||slot>=33)return;
    static float nextRepair[33]{};if(globals->time<nextRepair[slot])return;nextRepair[slot]=globals->time+.1f;
    int selected=0;float nearest=96.f;
    for(int door=1;door<=2;++door){auto e=mansionDoor(door);if(!valid(e)||mansionDoorHealth(door)<=0||e->v.solid==SOLID_NOT)continue;
        Vector eye=player->v.origin+player->v.view_ofs,to=center(e);float distance=length(to-eye);if(distance>=nearest)continue;
        TraceResult tr{};engine.pfnTraceLine(&eye.x,&to.x,1,player,&tr);if(tr.flFraction<.999f&&tr.pHit!=e)continue;
        selected=door;nearest=distance;
    }
    if(!selected)return;auto e=mansionDoor(selected);auto state=mansionState();if(!valid(e)||!valid(state))return;
    float& saved=selected==1?state->v.vuser1.x:state->v.vuser1.y;
    saved=std::min(1000.f,saved+10.f);
}
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
static bool mansionRouteActive(edict_t* e){return mansionMap()&&mansionActor(e)&&aliveEnemy(e)&&!mansionContact(e);}
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
static bool mansionTargetStarts(edict_t* e,const char* prefix){const char* name=text(e->v.target);return name&&std::strncmp(name,prefix,std::strlen(prefix))==0;}
static void mansionSetUpperPatrol(edict_t* e,int point){
    point%=4;if(!mansionUpperTargets[point]){char target[48];std::snprintf(target,sizeof target,"mansion_upper_patrol_%d",point);mansionUpperTargets[point]=engine.pfnAllocString(target);}
    e->v.target=mansionUpperTargets[point];
}
static Vector mansionUpperPoint(int point){static const Vector points[]={{-624,-176,192.1f},{-352,-256,192.1f},{-64,-256,192.1f},{-352,128,192.1f}};return points[point%4];}
struct MansionCultistState{edict_t* entity=nullptr;int serial=0,point=0;bool perimeter=false,backdoor=false,relocating=false;float nextShift=0;};
static std::vector<MansionCultistState> mansionCultists;
static bool mansionCultist(edict_t* e){return mansionActor(e)&&!std::strcmp(text(e->v.classname),"monster_human_grunt")&&!std::strncmp(text(e->v.targetname),"mansion_enemy_4_",16);}
static int mansionNearestCultistPerimeterPoint(const Vector& origin){int best=0;float nearest=1e30f;
    for(int i=0;i<40;++i){const auto& p=mansion::cultistPerimeter[i];float dx=p[0]-origin.x,dy=p[1]-origin.y,squared=dx*dx+dy*dy;
        if(squared<nearest){nearest=squared;best=i;}}
    return best;
}
static MansionCultistState& mansionCultistState(edict_t* e){
    for(auto& s:mansionCultists)if(s.entity==e&&s.serial==e->serialnumber)return s;
    const char* tail=std::strrchr(text(e->v.targetname),'_');int index=tail?std::atoi(tail+1):0;
    bool perimeter=index>=0&&index<80&&mansionPerimeterCultistRoles[index];
    bool backdoor=!perimeter&&mansion::entersBackDoor(4,index);
    int point=mansionNearestCultistPerimeterPoint(e->v.origin);
    mansionCultists.push_back({e,e->serialnumber,point,perimeter,backdoor,perimeter,globals->time+5});return mansionCultists.back();
}
static bool mansionMoveCultist(edict_t* e,float dt){
    auto& state=mansionCultistState(e);
    if(state.perimeter){
        if(!state.relocating&&globals->time>=state.nextShift){int previous=state.point;state.point=(state.point+1)%40;state.nextShift=globals->time+5.f;state.relocating=true;
            char report[80];std::snprintf(report,sizeof report,"from=%d to=%d",previous,state.point);startupTrace("CultistPerimeterShift",report);}
        if(state.relocating&&mansionNavWalk(e,Vector(mansion::cultistPerimeter[state.point][0],mansion::cultistPerimeter[state.point][1],mansion::cultistPerimeter[state.point][2]),dt,85.f)){
            state.relocating=false;state.nextShift=globals->time+5.f;
        }
        return state.relocating;
    }
    if(!state.backdoor)return false;
    if(mansionDoorHealth(2)<=0){mansionSetUpperPatrol(e,0);return true;}
    Vector entry(-328,-792,48.1f);
    if(length2d(entry-e->v.origin)<136){mansionStrikeDoor(e,2,65.f);return true;}
    mansionNavWalk(e,entry,dt,85.f);return true;
}
static bool mansionRouteMarkerMove(edict_t* e,float dt){
    const char* target=text(e->v.target);bool side=target&&std::strncmp(target,"mansion_side_route_",19)==0;
    bool main=target&&std::strncmp(target,"mansion_route_",14)==0;
    if(!side&&!main)return false;
    if(mansionDoorHealth(side?2:1)<=0){mansionSetUpperPatrol(e,0);return true;}
    auto goal=mansionFind(target);if(!valid(goal))return true;
    if(goal->v.target){auto next=mansionFind(text(goal->v.target));
        if(valid(next)&&length2d(next->v.origin-e->v.origin)<length2d(goal->v.origin-e->v.origin)&&mansionShortcut(e,next->v.origin)){
            e->v.target=goal->v.target;goal=next;
        }
    }
    Vector delta=goal->v.origin-e->v.origin;float reach=goal->v.target?18.f:20.f;
    const char* goalName=text(goal->v.targetname);size_t goalLength=std::strlen(goalName);
    if(goalLength>=3&&!std::strcmp(goalName+goalLength-3,"_01"))reach=48;
    if(std::fabs(goal->v.origin.x-208)<1&&std::fabs(goal->v.origin.y+256)<1)reach=40;
    if(length2d(delta)<reach&&std::fabs(delta.z)<20){
    if(!goal->v.target){e->v.target=engine.pfnAllocString(side?"mansion_wait_door_2":"mansion_wait_door_1");return true;}
        e->v.target=goal->v.target;goal=mansionFind(text(e->v.target));if(!valid(goal))return true;delta=goal->v.origin-e->v.origin;
    }
    float yaw=std::atan2(delta.y,delta.x)*180.f/3.14159265f;
    const char* name=text(e->v.classname);
    float speed=qzIs(e)?35.f:(!std::strcmp(name,"monster_max")?110.f:(!std::strcmp(name,"monster_randy")?110.f:((czIs(e)||!std::strcmp(name,"monster_human_grunt"))?85.f:55.f)));
    float distance=std::min(length2d(delta),speed*dt)*(qzIs(e)&&slowed(e)?balance::SlowFactor:1.f);
    bool moved=qzIs(e)?qzWalk(e,yaw,distance):hookWalkMove(e,yaw,distance,0)!=0;
    if(!moved)for(float turn:{30.f,-30.f,60.f,-60.f,90.f,-90.f}){
        if(qzIs(e)?qzWalk(e,yaw+turn,distance):hookWalkMove(e,yaw+turn,distance,0)!=0){yaw+=turn;moved=true;break;}
    }
    if(moved){e->v.angles.y=yaw;if(judgmentKuarl(e))judgmentAnimate(e,16,153,5.1f,true);else if(czIs(e)){czSequence(e,3);e->v.fuser1=globals->time+.15f;}else if(!qzIs(e))hordeWalkAnimation(e);}
    return true;
}
static void mansionMove(edict_t* e,float dt){
    if(judgmentZeal(e))return;
    bool cultist=mansionCultist(e),perimeterCultist=false;
    if(cultist){const char* tail=std::strrchr(text(e->v.targetname),'_');int index=tail?std::atoi(tail+1):0;
        perimeterCultist=index>=0&&index<80&&mansionPerimeterCultistRoles[index];}
    if(!mansionMap()||!mansionActor(e)||!aliveEnemy(e)||(!perimeterCultist&&mansionContact(e))||pushed(e)||czObjectiveOwns(e))return;
    if(czIs(e)&&!czSmokeCanLeave(e))return;if(czIs(e)&&czMeleeBusy(e))return;
    if(qzIs(e)&&(e->v.impulse!=QZIdle&&e->v.impulse!=QZChase))return;
    if(perimeterCultist){auto& state=mansionCultistState(e);
        if(!mansionContact(e)||state.relocating||globals->time>=state.nextShift)mansionMoveCultist(e,dt);
        return;}
    if(cultist){auto& state=mansionCultistState(e);if(state.backdoor){mansionMoveCultist(e,dt);return;}}
    if(mansionTargetStarts(e,"mansion_upper_patrol_")){
        const char* end=text(e->v.target)+std::strlen("mansion_upper_patrol_");int point=std::atoi(end);
        if(mansionNavWalk(e,mansionUpperPoint(point),dt,qzIs(e)?35.f:(!std::strcmp(text(e->v.classname),"monster_randy")?110.f:55.f)))mansionSetUpperPatrol(e,point+1);
        return;
    }
    int door=mansionTargetStarts(e,"mansion_wait_door_2")?2:(mansionTargetStarts(e,"mansion_wait_door_1")?1:0);
    if(door){
        if(mansionDoorHealth(door)<=0){mansionSetUpperPatrol(e,0);return;}
        Vector entry=door==1?Vector(64,-256,56.1f):Vector(-328,-792,48.1f);
        if(length2d(entry-e->v.origin)<136){mansionStrikeDoor(e,door);return;}
        mansionNavWalk(e,entry,dt,qzIs(e)?35.f:(!std::strcmp(text(e->v.classname),"monster_randy")?110.f:(!std::strcmp(text(e->v.classname),"monster_max")?110.f:55.f)));
        return;
    }
    if(mansionRouteMarkerMove(e,dt))return;
}
static void mansionPrecache(){
    if(!mansionMap())return;
    for(auto path:{"debris/flesh1.wav","debris/flesh2.wav","debris/flesh3.wav"})engine.pfnPrecacheSound(const_cast<char*>(path));
    // Native Spawn performs each monster's full sound/model precache now,
    // before GoldSrc closes its resource list. No prototypes enter the wave.
    for(int type=0;type<6;++type){auto e=engine.pfnCreateNamedEntity(engine.pfnAllocString(mansion::classname(type)));if(!valid(e)){startupTrace("MansionPrecacheCreateFailed",mansion::classname(type));continue;}
        e->v.origin=Vector(1336,392,24);hookSpawn(e);engine.pfnRemoveEntity(e);}
    judgmentPrecache();
}
static bool mansionEmit(edict_t* state){
    int wave=int(state->v.frags),index=int(state->v.armorvalue),type=mansion::kind(wave,index);
    // The boss is a separate encounter after the complete regular horde.
    if(wave==4&&index==mansion::hordeTotal(wave)&&mansionAlive()!=0)return false;
#ifdef CREMATION_ENABLE_VALIDATION
    bool johnny=mansion::isJohnny(wave,index)&&!mansionCZRouteTest;
    const char* actorMarkerFormat=johnny?"mansion_enemy_johnny_%d_%02d":(wave<=2?"mansion_enemy_fixed_%d_%02d":"mansion_enemy_%d_%02d");
    if(mansionCZRouteTest)type=5;else if(johnny)type=0;
#else
    bool johnny=mansion::isJohnny(wave,index);
    const char* actorMarkerFormat=johnny?"mansion_enemy_johnny_%d_%02d":(wave<=2?"mansion_enemy_fixed_%d_%02d":"mansion_enemy_%d_%02d");
    if(johnny)type=0;
#endif
    // Regular enemies preserve round-robin route assignment. The tactical NPC
    // starts at the point whose nearest living player is farthest away.
    int cursor=int(state->v.health)%mansion::SpawnCount;
    if(type==5){
        std::vector<int> available;
        for(int i=0;i<mansion::SpawnCount;++i){char name[40];std::snprintf(name,sizeof name,"mansion_ct_%02d",i);auto candidate=mansionFind(name);if(!valid(candidate))continue;
            Vector hull=candidate->v.origin-Vector(0,0,3.9f);TraceResult tr{};engine.pfnTraceHull(&hull.x,&hull.x,0,1,nullptr,&tr);
            if(!tr.fStartSolid&&!tr.fAllSolid)available.push_back(i);
        }
        if(available.empty())return false;
        float farthest=-1;
        for(int candidate:available){char name[40];std::snprintf(name,sizeof name,"mansion_ct_%02d",candidate);auto spawn=mansionFind(name);if(!valid(spawn))continue;
            float nearest=100000.f;bool foundPlayer=false;
            for(int player=1;player<=globals->maxClients;++player){auto p=engine.pfnPEntityOfEntIndex(player);if(!valid(p)||!(p->v.flags&FL_CLIENT)||p->v.health<=0||p->v.deadflag!=DEAD_NO)continue;
                nearest=std::min(nearest,length2d(p->v.origin-spawn->v.origin));foundPlayer=true;}
            float score=foundPlayer?nearest:float(-candidate);
            if(score>farthest){farthest=score;cursor=candidate;}
        }
        state->v.dmg_save=float(cursor+1);
    }
    int cultistSlot=mansion::cultistSlot(wave,index);
    if(cultistSlot>=0){
        int order[mansion::SpawnCount]{};
        for(int i=0;i<mansion::SpawnCount;++i)order[i]=i;
        for(int i=mansion::SpawnCount-1;i>0;--i){int j=engine.pfnRandomLong(0,i);std::swap(order[i],order[j]);}
        bool found=false;
        for(int candidateIndex:order){
            char candidateName[40];std::snprintf(candidateName,sizeof candidateName,"mansion_ct_%02d",candidateIndex);
            auto candidate=mansionFind(candidateName);if(!valid(candidate))continue;
            Vector candidateAt=candidate->v.origin-Vector(0,0,39.9f),candidateHull=candidateAt+Vector(0,0,36);TraceResult candidateTrace{};
            engine.pfnTraceHull(&candidateHull.x,&candidateHull.x,0,1,nullptr,&candidateTrace);
            if(candidateTrace.fStartSolid||candidateTrace.fAllSolid)continue;
            bool crowded=false;for(int entity=1;entity<globals->maxEntities;++entity){auto other=engine.pfnPEntityOfEntIndex(entity);
                if(aliveEnemy(other)&&length2d(other->v.origin-candidateAt)<72.f&&std::fabs(other->v.origin.z-candidateAt.z)<56.f){crowded=true;break;}}
            if(crowded)continue;
            cursor=candidateIndex;found=true;break;
        }
        if(!found)return false;
    }
    char marker[64];std::snprintf(marker,sizeof marker,"mansion_ct_%02d",cursor);
    auto point=mansionFind(marker);if(!valid(point))return false;
    Vector at=point->v.origin-Vector(0,0,39.9f),hull=at+Vector(0,0,36);TraceResult tr{};
    engine.pfnTraceHull(&hull.x,&hull.x,0,1,nullptr,&tr);if(tr.fStartSolid||tr.fAllSolid)return false;
    if(type==1){Vector top=at+Vector(0,0,72);engine.pfnTraceLine(&at.x,&top.x,1,nullptr,&tr);if(tr.fStartSolid||tr.flFraction<1)return false;}
    if(wave==4&&index==mansion::hordeTotal(wave)){
        startupTrace("JudgmentEncounter","horde_alive=0 begin_boss_phase");
        if(!judgmentSpawnPair(at,point->v.angles,wave,index,cursor))return false;
        state->v.armorvalue+=1;state->v.health=float((cursor+1)%mansion::SpawnCount);
        char bossReport[120];std::snprintf(bossReport,sizeof bossReport,"wave=4 index=%d ct=%d type=judgment actors=2",index,cursor);startupTrace("MansionSpawn",bossReport);
        return true;
    }
    auto e=engine.pfnCreateNamedEntity(engine.pfnAllocString(mansion::classname(type)));if(!valid(e)){if(globals->time>=mansionSpawnErrorAt){startupTrace("MansionCreateFailed",mansion::classname(type));mansionSpawnErrorAt=globals->time+5;}return false;}
    e->v.origin=at;e->v.angles=point->v.angles;
    if(type==4){static const int weapons[]={1,8,3,10,5,12};e->v.weapons=weapons[(index/4+index%4)%6];}
    std::snprintf(marker,sizeof marker,actorMarkerFormat,wave,index);
    e->v.targetname=engine.pfnAllocString(marker);
    hookSpawn(e);
    if(type==5)e->v.weapons=cursor%4;
    if(!aliveEnemy(e)){engine.pfnRemoveEntity(e);startupTrace("MansionError","monster Spawn failed");return false;}
    std::snprintf(marker,sizeof marker,actorMarkerFormat,wave,index);e->v.targetname=engine.pfnAllocString(marker);
    bool perimeterCultist=index>=0&&index<80&&mansionPerimeterCultistRoles[index];
    if(cultistSlot>=0&&perimeterCultist)e->v.target=engine.pfnAllocString("mansion_cultist_perimeter");
    else if(cultistSlot>=0&&mansion::entersBackDoor(wave,index))e->v.target=engine.pfnAllocString("mansion_cultist_backdoor");
    else{std::snprintf(marker,sizeof marker,type==5||mansion::doorForSector(cursor)>=2?"mansion_side_route_%02d_00":"mansion_route_%02d_00",cursor);e->v.target=engine.pfnAllocString(marker);}
    // Quake's standalone idle patrol must not independently move this actor;
    // the shared Mansion approach owns idle movement; native combat stays intact.
    e->v.chain=nullptr;
    engine.pfnSetOrigin(e,&at.x);
    state->v.armorvalue+=1;if(type!=5)state->v.health=float((cursor+1)%mansion::SpawnCount);
    char report[260];std::snprintf(report,sizeof report,"wave=%d index=%d ct=%d type=%s at=%.0f,%.0f,%.0f model=%s",wave,index,cursor,johnny?"monster_johnny":mansion::classname(type),at.x,at.y,at.z,text(e->v.model));startupTrace("MansionSpawn",report);
    if(cultistSlot>=0){char role[128];std::snprintf(role,sizeof role,"index=%d role=%s ct=%d",index,perimeterCultist?"perimeter":(mansion::entersBackDoor(wave,index)?"backdoor":"door"),cursor);startupTrace("MansionCultistSpawn",role);}
    return true;
}
static void mansionHud(edict_t* state,int alive){
    int wave=int(state->v.frags),pending=mansion::total(wave)-int(state->v.armorvalue);
    int hudState=state->v.impulse==2?2:(state->v.impulse?1:0);
    for(int i=1;i<=globals->maxClients;++i){auto p=engine.pfnPEntityOfEntIndex(i);if(!valid(p)||!(p->v.flags&FL_CLIENT))continue;
        judgmentSendHud(p);
        if(msgCremationWaveHUD){engine.pfnMessageBegin(MSG_ONE_UNRELIABLE,msgCremationWaveHUD,nullptr,p);
            engine.pfnWriteByte(1);engine.pfnWriteByte(wave);engine.pfnWriteShort(std::max(0,alive));
            engine.pfnWriteByte(pending>0?1:0);engine.pfnWriteByte(hudState);engine.pfnMessageEnd();}
    }
}
static void mansionActivate(){
    mansionHudAt=mansionTickAt=mansionVerifyAt=mansionMoveElapsed=mansionSpawnErrorAt=mansionDoorHudAt=0;
    mansionCultists.clear();mansionDoorStrikes.clear();
    mansionController={};for(auto& door:mansionDoors)door={};for(auto& target:mansionUpperTargets)target=0;
    mansionNavReset();mansionAssignCultistRoles();
#ifdef CREMATION_ENABLE_VALIDATION
    mansionDoorStableSince=0;
    mansionDoorStateReported=false;
    mansionDoorTouchPlayer=nullptr;mansionDoorTouchStage=0;mansionDoorTouchSince=0;
    mansionRouteReport=0;char env[8]{};GetEnvironmentVariableA("CREMATION_MANSION_VALIDATE",env,sizeof env);mansionTest=!std::strcmp(env,"1");mansionRouteTest=!std::strcmp(env,"2")||!std::strcmp(env,"6");mansionDoorTest=!std::strcmp(env,"6");mansionDoorTouchTest=!std::strcmp(env,"7");
    mansionCZRouteTest=!std::strcmp(env,"4");mansionRouteTest=mansionRouteTest||mansionCZRouteTest;mansionJudgmentTest=!std::strcmp(env,"5");
#endif
    if(mansionMap())startupTrace("MansionActivate","TT=10 CT=40 sectors=N,E,S,W waves=41,41,81,81 CZ=1/wave");
#ifdef CREMATION_ENABLE_VALIDATION
    mansionInventoryTest=!std::strcmp(env,"3");mansionInventoryChecked=false;
#endif
}
#ifdef CREMATION_ENABLE_VALIDATION
static bool mansionDoorMoved(edict_t* door){
    return valid(door)&&(std::fabs(door->v.angles.x)>8.f||std::fabs(door->v.angles.y)>8.f||std::fabs(door->v.angles.z)>8.f||length(door->v.avelocity)>1.f);
}
static void mansionDoorTouchProbe(){
    if(mansionDoorTouchStage==0){
        mansionDoorTouchPlayer=engine.pfnCreateFakeClient("DoorTouchProbe");
        if(!valid(mansionDoorTouchPlayer)){startupTrace("MansionDoorTouchFailed","could not create fake player");mansionDoorTouchStage=99;return;}
        char reject[128]{};if(raven.pfnClientConnect)raven.pfnClientConnect(mansionDoorTouchPlayer,"DoorTouchProbe","127.0.0.1",reject);
        raven.pfnClientPutInServer(mansionDoorTouchPlayer);mansionDoorTouchStage=1;
    }
    if(mansionDoorTouchStage==1||mansionDoorTouchStage==3){
        int doorIndex=mansionDoorTouchStage==1?1:2;auto door=mansionDoor(doorIndex);
        if(!valid(door)||!valid(mansionDoorTouchPlayer)||door->v.targetname||!raven.pfnTouch){
            startupTrace("MansionDoorTouchFailed","native door, no-targetname or touch callback check failed");mansionDoorTouchStage=99;return;
        }
        Vector playerOrigin=doorIndex==1?Vector(0,-336,104):Vector(-328,-840,104);
        engine.pfnSetOrigin(mansionDoorTouchPlayer,&playerOrigin.x);
        raven.pfnTouch(door,mansionDoorTouchPlayer);
        char report[100];std::snprintf(report,sizeof report,"door=%d targetname=%d",doorIndex,door->v.targetname?1:0);startupTrace("MansionDoorTouchIssued",report);
        mansionDoorTouchSince=globals->time;mansionDoorTouchStage=doorIndex==1?2:4;return;
    }
    if(mansionDoorTouchStage==2||mansionDoorTouchStage==4){
        int doorIndex=mansionDoorTouchStage==2?1:2;auto door=mansionDoor(doorIndex);
        if(mansionDoorMoved(door)){
            char report[100];std::snprintf(report,sizeof report,"door=%d angle=%.1f,%.1f,%.1f",doorIndex,door->v.angles.x,door->v.angles.y,door->v.angles.z);startupTrace("MansionDoorTouchOpened",report);
            mansionDoorTouchStage=doorIndex==1?3:5;
            if(mansionDoorTouchStage==5)startupTrace("MansionDoorTouchComplete","both native rotating doors opened through their player touch callback");
        }else if(globals->time-mansionDoorTouchSince>5.f){startupTrace("MansionDoorTouchFailed",doorIndex==1?"front door did not move after player touch":"side door did not move after player touch");mansionDoorTouchStage=99;}
    }
}
#endif
static void mansionFrame(float dt){
    if(!mansionMap())return;auto state=mansionState();if(!valid(state))return;mansionDoorFrame();
#ifdef CREMATION_ENABLE_VALIDATION
    if(mansionDoorTouchTest){mansionDoorTouchProbe();return;}
    if(mansionJudgmentTest){judgmentProbeFrame(dt);return;}
#endif
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
        mansionNavBeginTick();
        for(int i=1;i<globals->maxEntities;++i){auto e=engine.pfnPEntityOfEntIndex(i);if(mansionActor(e))mansionMove(e,std::min(mansionMoveElapsed,.1f));}
        mansionNavFinishTick();
        judgmentFrame(std::min(mansionMoveElapsed,.1f));
        mansionMoveElapsed=0;
    }
    if(globals->time<mansionTickAt)return;mansionTickAt=globals->time+.1f;
#ifdef CREMATION_ENABLE_VALIDATION
    if(mansionRouteTest){
        if(state->v.frags==0){state->v.frags=mansionCZRouteTest?4:3;state->v.armorvalue=0;state->v.health=0;state->v.max_health=0;}
        if(state->v.armorvalue<(mansionCZRouteTest?1:(mansionDoorTest?80:20)))mansionEmit(state);
        for(int i=1;i<globals->maxEntities;++i){auto e=engine.pfnPEntityOfEntIndex(i);if(!mansionActor(e)||!aliveEnemy(e))continue;
            const char* target=text(e->v.target);bool waitFront=target&&!std::strcmp(target,"mansion_wait_door_1")&&length2d(e->v.origin-Vector(64,-256,56.1f))<136;
            bool waitSide=target&&!std::strcmp(target,"mansion_wait_door_2")&&length2d(e->v.origin-Vector(-328,-792,48.1f))<136;
            bool czAtHostages=mansionCZRouteTest&&e->v.iuser1==4&&length(e->v.origin-Vector(-624,-176,192.1f))<24;
            if(mansionDoorTest&&(waitFront||waitSide)){
                if(e->v.iuser4!=1){char report[160];std::snprintf(report,sizeof report,"%s door=%d at=%.1f,%.1f,%.1f",text(e->v.targetname),waitFront?1:2,e->v.origin.x,e->v.origin.y,e->v.origin.z);startupTrace("MansionDoorTestArrival",report);e->v.iuser4=1;}
            }
            else if((!mansionCZRouteTest&&(waitFront||waitSide))||czAtHostages){char report[240];std::snprintf(report,sizeof report,"%s type=%s at=%.1f,%.1f,%.1f target=%s phase=%d",text(e->v.targetname),text(e->v.classname),e->v.origin.x,e->v.origin.y,e->v.origin.z,target?target:"none",e->v.iuser1);startupTrace("MansionRouteArrival",report);state->v.max_health+=1;engine.pfnRemoveEntity(e);}
            else if(globals->time>=mansionRouteReport){char report[250];std::snprintf(report,sizeof report,"%s at=%.1f,%.1f,%.1f target=%s flags=%d move=%d bounds=%.0f,%.0f,%.0f/%.0f,%.0f,%.0f",text(e->v.targetname),e->v.origin.x,e->v.origin.y,e->v.origin.z,text(e->v.target),int(e->v.flags),e->v.movetype,e->v.mins.x,e->v.mins.y,e->v.mins.z,e->v.maxs.x,e->v.maxs.y,e->v.maxs.z);startupTrace("MansionRouteProgress",report);}
        }
        if(globals->time>=mansionRouteReport)mansionRouteReport=globals->time+10;
        if(mansionDoorTest){
            auto first=mansionDoor(1),second=mansionDoor(2);
            auto opened=[](edict_t* door){return valid(door)&&(std::fabs(door->v.angles.x)>10.f||std::fabs(door->v.angles.y)>10.f||std::fabs(door->v.angles.z)>10.f);};
            if(mansionDoorHealth(1)<=0&&mansionDoorHealth(2)<=0&&opened(first)&&opened(second)&&!state->v.impulse){
                state->v.impulse=1;startupTrace("MansionDoorBreakComplete","both native doors reached zero HP, swung open and remained non-solid");
                mansionDoorStableSince=globals->time;
            }
            if(mansionDoorStableSince>0&&globals->time-mansionDoorStableSince>=20.f&&state->v.impulse==1){
                mansionNavReport();state->v.impulse=3;startupTrace("MansionDoorStableComplete","20 seconds of post-break horde navigation");
            }
            return;
        }
        if(state->v.max_health==(mansionCZRouteTest?1:20)&&!state->v.impulse){state->v.impulse=1;startupTrace("MansionRouteComplete",mansionCZRouteTest?"CZ used side stairs and reached upper hostage zone":"20 routes from four fronts reached their assigned door");}
        return;
    }
#endif
#ifdef CREMATION_ENABLE_VALIDATION
    if(!mansionHasPlayer()&&!mansionTest)return;
#else
    if(!mansionHasPlayer())return;
#endif
    if(state->v.impulse)return;
    if(state->v.frags==0){int requestedWave=mansionStartWave();state->v.frags=requestedWave?requestedWave:1;state->v.armorvalue=0;state->v.health=0;state->v.dmg_save=0;
#ifdef CREMATION_ENABLE_VALIDATION
        state->v.dmgtime=globals->time+(mansionTest?.2f:5.f);
#else
        state->v.dmgtime=globals->time+5.f;
#endif
        char report[80];std::snprintf(report,sizeof report,"begin=%d total=%d",int(state->v.frags),mansion::total(int(state->v.frags)));startupTrace("MansionWave",report);}
    int wave=int(state->v.frags),total=mansion::total(wave);
    if(int(state->v.armorvalue)<total){
        if(globals->time>=state->v.dmgtime){bool emitted=mansionEmit(state);
#ifdef CREMATION_ENABLE_VALIDATION
            state->v.dmgtime=globals->time+(emitted?(mansionTest?.20f:.375f):.25f);
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
            if(judgmentIs(e)){auto k=judgmentKuarl(e)?e:judgmentPartner(e);auto z=judgmentZeal(e)?e:judgmentPartner(e);if(valid(k)){judgmentSetHp(k,0);judgmentDropKuarl(k);}if(valid(z))judgmentKillZeal(z);break;}
            if(e->pvPrivateData){auto table=*reinterpret_cast<void***>(e->pvPrivateData);reinterpret_cast<TakeDamageFn>(table[ravenholm_abi::slot::TakeDamage])(e->pvPrivateData,&state->v,&state->v,10000.f,1<<6);}
            break;}
    }
#endif
}
