// Mansion state lives in native saved entvars, not session-only counters.
// Controller: frags=wave, armorvalue=emitted, health=CT cursor,
// dmgtime=next spawn, teleport_time=next wave, impulse=finished.
#include "mansion_waves.h"
static float mansionHudAt=0,mansionTickAt=0,mansionVerifyAt=0,mansionSpawnErrorAt=0;
static float mansionAliveAt=-1;
static int mansionAliveCached=0;
static float mansionHordeAliveAt=-1;
static int mansionHordeAliveCached=0;
struct MansionScientistRef{edict_t* entity=nullptr;int serial=0,wave=0;bool rescued=false;};
static MansionScientistRef mansionScientist;
static int mansionScientistWaveSpawned=0;
static bool mansionInvaded=false;
static float mansionScientistAppearAt=0;
static float mansionScientistKnockAt=0,mansionScientistVoiceAt=0,mansionScientistThankUntil=0;
static float mansionScientistEntryAt=0;
static int mansionScientistScenePhase=0;
static bool mansionScientistRunningIn=false;
static float mansionScientistSceneEndAt=0;
static edict_t* mansionScientistCamera=nullptr;
static int mansionScientistCameraSerial=0;
static bool mansionScientistSceneCameraActive=false;
static bool mansionScientistWasFrozen[33]{};
static void reactorScientistStationActivate();
static int mansionScientistVoiceLine=0;
static bool mansionScientistFemale=false;
static float mansionScientistLastHealth=0,mansionScientistPainAt=0;
static int mansionGroundAssigned=0;
static MansionScientistRef mansionRaiders[2];
static float mansionRaidHitAt[2]{};
struct MansionRaidAttack{bool active=false;int sequence=-1;float frame=0;};
static MansionRaidAttack mansionRaidAttacks[2];
static bool mansionEventMove(edict_t* e,float dt);
struct MansionSense{edict_t* actor=nullptr;edict_t* visiblePlayer=nullptr;int serial=0,visiblePlayerSerial=0;bool contact=false;float next=0,decisionInterval=.35f,visiblePlayerDistance=1e30f;};
static std::vector<MansionSense> mansionSense;
struct MansionRouteIntent{edict_t* actor=nullptr;int serial=0;Vector destination{0,0,0};float nextDecision=0,speed=55.f;bool ready=false;string_t target=0;float blocked=0,recoveryAt=0;int sideBias=0;bool lastRunning=false;};
static std::vector<MansionRouteIntent> mansionRouteIntents;
struct MansionTacticalSense{edict_t* actor=nullptr;edict_t* target=nullptr;int serial=0,targetSerial=0;Vector lastSeen{0,0,0};float reactAt=0,rememberUntil=0,nextSense=0;};
static std::vector<MansionTacticalSense> mansionTacticalSenses;
struct MansionPlayerPursuit{edict_t* actor=nullptr;edict_t* player=nullptr;int actorSerial=0,playerSerial=0;};
static std::vector<MansionPlayerPursuit> mansionPlayerPursuits;
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
static bool mansionLightTest=false;
static bool mansionNearRecoveryProbe(edict_t* actor,edict_t* player);
static float mansionRouteReport=0;
#endif
static float mansionMoveElapsed=0;
static float mansionDoorHudAt=0;
static bool mansionNavWalk(edict_t* e,const Vector& goal,float dt,float speed,bool running=false);
static MansionRouteIntent& mansionRouteIntentFor(edict_t* e);
static bool mansionBridgeEdgeClear(edict_t* e,const Vector& a,const Vector& b);
static bool mansionBridgeSpotFree(edict_t* e,const Vector& at);
static bool mansionBridgeNudge(edict_t* e,const Vector& goal);
static void mansionRouteAnimate(edict_t* e,bool running);
static bool mansionInsideFloor(const Vector& at);
static bool mansionPlaceInside(edict_t* e,const Vector& desired);
static void czPistol(edict_t* e,edict_t* target);
static int mansionNearestNav(const Vector& p);
static Vector czNavPoint(int n);
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
static edict_t* mansionFind(const char* name){auto e=engine.pfnFindEntityByString(nullptr,"targetname",name);return e&&engine.pfnIndexOfEdict(e)>0?e:nullptr;}
static edict_t* mansionState(){
    auto& c=mansionController;
    if(valid(c.entity)&&c.entity->serialnumber==c.serial)return c.entity;
    c.entity=mansionFind("mansion_wave_state");c.serial=valid(c.entity)?c.entity->serialnumber:0;return c.entity;
}
static int mansionAlive(){
    if(mansionAliveAt==globals->time)return mansionAliveCached;
    int count=0;for(auto e:getActiveEnemies()){if(mansionActor(e)&&aliveEnemy(e))++count;}
    mansionAliveCached=count;mansionAliveAt=globals->time;return count;
}
static int mansionHordeAlive(){
    if(mansionHordeAliveAt==globals->time)return mansionHordeAliveCached;
    int count=0;for(auto e:getActiveEnemies()){if(mansionActor(e)&&aliveEnemy(e)&&!czIs(e))++count;}
    mansionHordeAliveCached=count;mansionHordeAliveAt=globals->time;return count;
}
static bool mansionHasPlayer(){for(int i=1;i<=globals->maxClients;++i){auto p=engine.pfnPEntityOfEntIndex(i);if(valid(p)&&(p->v.flags&FL_CLIENT)&&p->v.health>0&&p->v.deadflag==DEAD_NO)return true;}return false;}
struct MansionLightDef{int style;const char* name;Vector origin;};
static const MansionLightDef mansionLightList[11]={
    {34,"lampa6", Vector(-528.f, 160.f,176.f)}, // 1: Ground floor garage/storage
    {35,"lampa8", Vector(-528.f,-672.f,176.f)}, // 2: Ground floor west room
    {32,"lampa2", Vector(-128.f,-672.f,176.f)}, // 3: Ground floor south room
    {33,"lampa4", Vector(-128.f, 160.f,176.f)}, // 4: Ground floor north room
    {42,"lampa7", Vector(-488.f,-280.f,176.f)}, // 5: Ground floor back lounge
    {39,"lampa11",Vector(-528.f,-640.f,280.f)}, // 6: Upper floor hostage room
    {41,"lampa9", Vector(-528.f, 160.f,280.f)}, // 7: Upper floor west room
    {40,"lampa10",Vector(-520.f,-232.f,280.f)}, // 8: Upper floor west corridor
    {37,"lampa3", Vector(-128.f, 160.f,280.f)}, // 9: Upper floor north room
    {38,"lampa1", Vector(-128.f,-672.f,280.f)}, // 10: Upper floor south bedroom
    {36,"lampa5", Vector(-136.f,-256.f,280.f)}  // 11: Upper floor central hallway/stairs
};
static float mansionLightNextTurnOff=0;
static int mansionLightsOffCount=0;
static bool mansionLightFlickering=false;

static void mansionLightsReset(){
    mansionLightsOffCount=0;
    mansionLightNextTurnOff=0;
    mansionLightFlickering=false;
    for(const auto& l:mansionLightList)engine.pfnLightStyle(l.style,const_cast<char*>("m"));
}

static void mansionLightsFrame(float dt){
#ifdef CREMATION_ENABLE_VALIDATION
    if(!mansionHasPlayer()&&!mansionLightTest)return;
    auto state=mansionState();
    if(!valid(state)||(!mansionLightTest&&state->v.frags<1))return;
    float interval=mansionLightTest?2.f:60.f;
#else
    if(!mansionHasPlayer())return;
    auto state=mansionState();
    if(!valid(state)||state->v.frags<1)return;
    float interval=60.f;
#endif
    if(mansionLightNextTurnOff==0){
        mansionLightNextTurnOff=globals->time+interval;
        mansionLightFlickering=false;
        return;
    }
    if(mansionLightsOffCount>=11)return;
    const auto& light=mansionLightList[mansionLightsOffCount];
    if(!mansionLightFlickering&&globals->time>=mansionLightNextTurnOff-1.5f){
        mansionLightFlickering=true;
        engine.pfnLightStyle(light.style,const_cast<char*>("mamamamamaa"));
        auto lamp=mansionFind(light.name);
        engine.pfnEmitSound(valid(lamp)?lamp:state,CHAN_ITEM,"buttons/spark6.wav",.8f,ATTN_NORM,0,100);
    }
    if(globals->time>=mansionLightNextTurnOff){
        engine.pfnLightStyle(light.style,const_cast<char*>("a"));
        auto lamp=mansionFind(light.name);
        engine.pfnEmitSound(valid(lamp)?lamp:state,CHAN_ITEM,"buttons/spark5.wav",1.f,ATTN_NORM,0,100);
        engine.pfnMessageBegin(MSG_PVS,SvcTemp,&light.origin.x,nullptr);
        engine.pfnWriteByte(TE_SPARKS);
        coords(light.origin);
        engine.pfnMessageEnd();
        if(valid(lamp)&&raven.pfnUse)raven.pfnUse(lamp,state);
        ++mansionLightsOffCount;
        mansionLightFlickering=false;
        mansionLightNextTurnOff=globals->time+interval;
        char report[120];
        std::snprintf(report,sizeof report,"light=%s style=%d count=%d/11 at=%.0f,%.0f,%.0f",light.name,light.style,mansionLightsOffCount,light.origin.x,light.origin.y,light.origin.z);
        startupTrace("MansionLightOff",report);
    }
}
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
        state->v.vuser1.x=valid(a)?2000.f:0.f;
        state->v.vuser1.y=valid(b)?2000.f:0.f;
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
    return std::max(0.f,std::min(2000.f,saved));
}
static void mansionDoorFrame(){
    if(!mansionMap()||!msgMansionDoors||globals->time<mansionDoorHudAt)return;
    auto state=mansionState();if(!valid(state))return;mansionDoorHudAt=globals->time+.1f;
    float first=mansionDoorHealth(1),second=mansionDoorHealth(2);int zombies=mansionAlive();int wave=std::max(0,std::min(255,int(state->v.frags)));
    for(int i=1;i<=globals->maxClients&&i<33;++i){auto p=engine.pfnPEntityOfEntIndex(i);if(!valid(p)||!(p->v.flags&FL_CLIENT))continue;
        engine.pfnMessageBegin(MSG_ONE_UNRELIABLE,msgMansionDoors,nullptr,p);engine.pfnWriteByte(2);
        engine.pfnWriteShort(int(first));engine.pfnWriteShort(int(second));engine.pfnWriteShort(zombies);engine.pfnWriteByte(wave);engine.pfnMessageEnd();
        auto sc=mansionScientist.entity;bool present=valid(sc)&&sc->serialnumber==mansionScientist.serial;
        engine.pfnMessageBegin(MSG_ONE_UNRELIABLE,msgMansionEvent,nullptr,p);engine.pfnWriteShort(present?int(std::max(0.f,sc->v.health)):0);engine.pfnWriteShort(present?int(sc->v.max_health):0);engine.pfnWriteByte(mansionInvaded);engine.pfnWriteByte(state->v.impulse==2);engine.pfnMessageEnd();
    }
}
struct MansionDoorStrike{edict_t* actor=nullptr;int serial=0;float next=0,roleCheckAt=0;int door=0,slot=-1;Vector outward{0,0,0},position{0,0,0};};
static std::vector<MansionDoorStrike> mansionDoorStrikes;
static void mansionReleaseDoorRole(edict_t* actor){
    mansionDoorStrikes.erase(std::remove_if(mansionDoorStrikes.begin(),mansionDoorStrikes.end(),[&](const MansionDoorStrike& s){return s.actor==actor;}),mansionDoorStrikes.end());
}
static bool mansionDoorAttackClear(edict_t* actor,int door){
    auto panel=mansionDoor(door);if(!aliveEnemy(actor)||!valid(panel))return false;
    Vector from=actor->v.origin+Vector(0,0,36),to=center(panel);if(length2d(to-from)>170.f)return false;
    TraceResult tr{};engine.pfnTraceHull(&from.x,&to.x,1,1,actor,&tr);
    return tr.pHit==panel||tr.flFraction>=.999f;
}
static int mansionDoorRole(edict_t* actor,int door){
    if(mansionDoorHealth(door)<=0)mansionDoorStrikes.erase(std::remove_if(mansionDoorStrikes.begin(),mansionDoorStrikes.end(),[&](const MansionDoorStrike& s){return s.door==door;}),mansionDoorStrikes.end());
    MansionDoorStrike* entry=nullptr;
    for(auto& strike:mansionDoorStrikes)if(strike.actor==actor&&strike.serial==actor->serialnumber){entry=&strike;break;}
    bool initialize=false;
    if(entry&&entry->door!=door){entry->door=door;entry->slot=-1;entry->next=0;entry->roleCheckAt=0;initialize=true;}
    if(!entry){mansionDoorStrikes.push_back({actor,actor->serialnumber,0,0,door,-1});entry=&mansionDoorStrikes.back();initialize=true;}
    auto assignPosition=[&](){auto panel=mansionDoor(door);Vector outward=valid(panel)?actor->v.origin-panel->v.origin:Vector(1,0,0);float length=std::max(1.f,length2d(outward));entry->outward=Vector(outward.x/length,outward.y/length,0);entry->position=actor->v.origin;};
    if(initialize)assignPosition();
    if(entry->roleCheckAt>globals->time)return entry->slot;
    entry->roleCheckAt=globals->time+.25f;
    bool clear=mansionDoorAttackClear(actor,door),used[3]{};int queueRole=3;
    for(const auto& strike:mansionDoorStrikes)if(strike.door==door&&strike.actor!=actor&&strike.slot>=0){if(strike.slot<3)used[strike.slot]=true;else queueRole=std::max(queueRole,strike.slot+1);}
    if(entry->slot<3&&!clear){entry->slot=queueRole;}
    if(clear&&entry->slot>=3){for(int slot=0;slot<3;++slot)if(!used[slot]){entry->slot=slot;assignPosition();break;}}
    if(entry->slot<0){if(clear){for(int slot=0;slot<3;++slot)if(!used[slot]){entry->slot=slot;assignPosition();break;}}
        if(entry->slot<0)entry->slot=queueRole;}
    return entry->slot;
}
static bool mansionStrikeDoor(edict_t* actor,int door,float damage=65.f){
    auto panel=mansionDoor(door);auto state=mansionState();
    if(!aliveEnemy(actor)||!valid(panel)||!valid(state)||damage<=0||mansionDoorHealth(door)<=0||mansionDoorRole(actor,door)>=3)return false;
    MansionDoorStrike* strike=nullptr;for(auto& s:mansionDoorStrikes)if(s.actor==actor&&s.serial==actor->serialnumber){strike=&s;break;}
    if(!strike){mansionDoorStrikes.push_back({actor,actor->serialnumber,0});strike=&mansionDoorStrikes.back();}
    if(globals->time<strike->next)return false;
    Vector from=actor->v.origin+Vector(0,0,36),to=center(panel);if(length2d(to-from)>170)return false;
    // Other attackers in the doorway must not body-block a valid strike.
    TraceResult tr{};engine.pfnTraceHull(&from.x,&to.x,1,1,actor,&tr);
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
static bool mansionSiegeMove(edict_t* actor,int door,const Vector& entry,float dt,float speed){
    auto panel=mansionDoor(door);if(!valid(panel))return true;
#ifdef CREMATION_ENABLE_VALIDATION
    if(mansionRouteTest&&!mansionDoorTest){mansionNavWalk(actor,entry,dt,speed);return true;}
#endif
    if(length2d(entry-actor->v.origin)>220.f){mansionNavWalk(actor,entry,dt,speed);return true;}
    int role=mansionDoorRole(actor,door);Vector outward=Vector(1,0,0),reserved=actor->v.origin;
    for(const auto& strike:mansionDoorStrikes)if(strike.actor==actor&&strike.serial==actor->serialnumber){outward=strike.outward;reserved=strike.position;break;}
    Vector tangent(-outward.y,outward.x,0);
    Vector goal=reserved;
    if(role<3){if(role==1)goal=goal+tangent*34.f;else if(role==2)goal=goal-tangent*34.f;
        mansionStrikeDoor(actor,door,65.f);
    }else{
        // Fix 2: la cola no es fila india sobre el eje del puente. Escalon
        // lateral + 68u entre puestos para hulls de 32u sin solaparse.
        goal=panel->v.origin+outward*(100.f+float(role-3)*68.f);goal.z=entry.z;
        goal=goal+tangent*((role%2)?36.f:-36.f);
        // Si otro en cola ya reservo muy cerca, abre aun mas el abanico.
        for(const auto& s:mansionDoorStrikes)if(s.actor!=actor&&s.slot>=3&&s.door==door){
            if(length2d(s.position-goal)<56.f){goal=goal+tangent*((role%2)?-36.f:36.f);break;}
        }
    }
    if(length2d(goal-actor->v.origin)>18.f||std::fabs(goal.z-actor->v.origin.z)>20.f)mansionNavWalk(actor,goal,dt,speed);
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
    saved=std::min(2000.f,saved+10.f);
}
static bool mansionContact(edict_t* e){
    int index=engine.pfnIndexOfEdict(e);if(index<=0)return false;
    if(size_t(index)>=mansionSense.size())mansionSense.resize(index+1);
    auto& sense=mansionSense[index];
    if(sense.actor==e&&sense.serial==e->serialnumber&&globals->time<sense.next)return sense.contact;
    sense.actor=e;sense.serial=e->serialnumber;
    float nearestPlayer=1e30f;for(int i=1;i<=globals->maxClients;++i){auto p=engine.pfnPEntityOfEntIndex(i);
        if(valid(p)&&(p->v.flags&FL_CLIENT)&&p->v.health>0&&p->v.deadflag==DEAD_NO)nearestPlayer=std::min(nearestPlayer,length(p->v.origin-e->v.origin));}
    float interval=nearestPlayer<320.f?.05f:(nearestPlayer<MansionZombieNoticeRange?.15f:.35f);
    if(sense.next==0)sense.next=globals->time+float(index%7)*.025f;
    if(globals->time<sense.next)return sense.contact;
    sense.decisionInterval=interval;
    sense.next=globals->time+interval;
    sense.visiblePlayer=nullptr;sense.visiblePlayerSerial=0;sense.visiblePlayerDistance=1e30f;
    if(czIs(e)){sense.contact=czObjectiveOwns(e);if(sense.contact)mansionReleaseDoorRole(e);return sense.contact;}
    if(qzIs(e)){sense.contact=qzEnemy(e)!=nullptr;if(sense.contact)mansionReleaseDoorRole(e);return sense.contact;}
    bool contact=false;
    for(int i=1;i<globals->maxEntities;++i){auto p=engine.pfnPEntityOfEntIndex(i);
        if(!valid(p)||p->v.health<=0||p->v.deadflag!=DEAD_NO)continue;
        if(!(p->v.flags&FL_CLIENT)&&!necroMinion(p)&&!bounce::isSentry(p))continue;
        float targetDistance=length(p->v.origin-e->v.origin);
        if(targetDistance>((p->v.flags&FL_CLIENT)?MansionZombieNoticeRange:2048.f))continue;
        Vector from=e->v.origin+e->v.view_ofs,to=p->v.origin+p->v.view_ofs;TraceResult tr{};
        engine.pfnTraceLine(&from.x,&to.x,1,e,&tr);
        if(tr.flFraction==1||tr.pHit==p){contact=true;
            if(p->v.flags&FL_CLIENT){float distance=length(p->v.origin-e->v.origin);if(distance<sense.visiblePlayerDistance){sense.visiblePlayer=p;sense.visiblePlayerSerial=p->serialnumber;sense.visiblePlayerDistance=distance;}}
        }
    }sense.contact=contact;return contact;
}
static edict_t* mansionVisiblePlayer(edict_t* e,float maximumDistance){
    int index=engine.pfnIndexOfEdict(e);if(index<=0||size_t(index)>=mansionSense.size())return nullptr;
    const auto& sense=mansionSense[index];auto player=sense.visiblePlayer;
    if(sense.actor!=e||sense.serial!=e->serialnumber||!valid(player)||player->serialnumber!=sense.visiblePlayerSerial||
       !(player->v.flags&FL_CLIENT)||player->v.health<=0||player->v.deadflag!=DEAD_NO||sense.visiblePlayerDistance>maximumDistance)return nullptr;
    return player;
}
static bool mansionOutsideCentral(const Vector& origin){
    return origin.x<-1184.f||origin.x>480.f||origin.y<-1088.f||origin.y>576.f;
}
static bool mansionOnBridgeRoute(edict_t* e){
    const char* target=text(e->v.target);
    if(!target)return false;
    size_t len=std::strlen(target);
    if(len<3)return false;
    if(!std::strncmp(target,"mansion_route_",14)||!std::strncmp(target,"mansion_side_route_",19)){
        const char* suffix=target+len-3;
        if(!std::strcmp(suffix,"_00")||!std::strcmp(suffix,"_01")||!std::strcmp(suffix,"_02"))return true;
    }
    return false;
}
static float mansionChaseSpeed(edict_t* e){
    const char* name=text(e->v.classname);
    if(!std::strcmp(name,"monster_randy")||!std::strcmp(name,"monster_fastzombie"))return 150.f;
    if(!std::strcmp(name,"monster_poisionzombie"))return 110.f;
    return 125.f;
}
static float mansionLocomotionSpeed(edict_t* e,bool& running){
    if(qzIs(e)){running=false;return 35.f;}
    if(auto state=davidState(e)){
        if(davidCrawling(*state)){running=false;return 16.f;}
        running=state->running;
        return running?mansionChaseSpeed(e):55.f;
    }
    // Fix 4: histeresis run/walk para genericos. Sin memoria el flag parpadea
    // cada tick en el umbral y la secuencia se reinicia a frame 0.
    auto& intent=mansionRouteIntentFor(e);
    bool hasRun=false;
    auto model=static_cast<unsigned char*>(engine.pfnGetModelPtr(e));running=false;
    if(model){int count=*reinterpret_cast<int*>(model+164),offset=*reinterpret_cast<int*>(model+168);
        for(int i=0;i<count;++i){auto seq=model+offset+i*176;int activity=*reinterpret_cast<int*>(seq+40);
            if(activity==4||(activity==3&&!std::strncmp(reinterpret_cast<const char*>(seq),"run",3))){hasRun=true;break;}}}
    // Randy siempre corre: sin histeresis ni chequeo de modelo.
    if(randyClass(e)){running=true;intent.lastRunning=true;return mansionChaseSpeed(e);}
    if(!hasRun){intent.lastRunning=false;running=false;}
    else{
        float nearest=1e30f;
        for(int i=1;i<=globals->maxClients;++i){auto p=engine.pfnPEntityOfEntIndex(i);if(valid(p)&&(p->v.flags&FL_CLIENT)&&p->v.health>0&&p->v.deadflag==DEAD_NO)nearest=std::min(nearest,length2d(p->v.origin-e->v.origin));}
        // Corre hasta 60 para parar, anda hasta 90 para arrancar.
        bool want=intent.lastRunning?(nearest>=60.f):(nearest>=90.f);
        // Sin jugador cercano, conserva el ultimo estado en vez de forzar run.
        if(nearest>900.f)want=intent.lastRunning;
        running=want;intent.lastRunning=want;
    }
    return running?mansionChaseSpeed(e):(!std::strcmp(text(e->v.classname),"monster_poisionzombie")?52.f:55.f);
}
static bool mansionChaseVisiblePlayer(edict_t* e,edict_t* player,float dt){
    if(!valid(e)||!valid(player)||tposeIs(e)||zombieBusy(e))return false;Vector delta=player->v.origin-e->v.origin;float distance=length2d(delta);
    if(distance<=80.f)return true;
    auto state=davidState(e);
    // Fix 5: no adelantar movedAt. Solo marca progreso si el WalkMove movio.
    if(state){
        state->target=player;
        state->targetSerial=player->serialnumber;
    }
    bool shouldRun=true;
    if(state)shouldRun=state->running;
    else if(!std::strcmp(text(e->v.classname),"monster_poisionzombie"))shouldRun=false;
    float speed=shouldRun?mansionChaseSpeed(e):(state&&davidCrawling(*state)?16.f:(std::strstr(text(e->v.classname),"poision")?52.f:55.f));
    int playerNode=mansionNearestNav(player->v.origin);Vector navGoal=playerNode>=0?czNavPoint(playerNode):player->v.origin;
    Vector before=e->v.origin;
    // Use the shared mansion route until close, then steer directly toward the
    // visible player. This bypasses door attack queue roles during pursuit.
    if(distance>192.f&&length2d(navGoal-e->v.origin)>64.f){
        mansionNavWalk(e,navGoal,dt,speed,shouldRun);
        bool moved=length2d(e->v.origin-before)>.1f;
        if(moved){e->v.angles.y=std::atan2(delta.y,delta.x)*180.f/3.14159265f;mansionReleaseDoorRole(e);if(state)state->movedAt=globals->time;}
        return moved;
    }
    Vector direction=normal(Vector(delta.x,delta.y,0));Vector goal=player->v.origin-direction*64.f;delta=goal-e->v.origin;
    float yaw=std::atan2(delta.y,delta.x)*180.f/3.14159265f;
    float step=std::min(length2d(delta),speed*std::min(dt,.1f));bool moved=hookWalkMove(e,yaw,step,0)!=0;
    if(!moved)for(float turn:{20.f,-20.f,40.f,-40.f,70.f,-70.f,100.f,-100.f,135.f,-135.f})if(hookWalkMove(e,yaw+turn,step,0)){yaw+=turn;moved=true;break;}
    if(moved){
        e->v.angles.y=yaw;
        if(shouldRun)hordeRunAnimation(e);else hordeWalkAnimation(e);
        mansionReleaseDoorRole(e);
        if(state)state->movedAt=globals->time;
        return true;
    }
    if(playerNode>=0&&length2d(navGoal-e->v.origin)>18.f){
        mansionNavWalk(e,navGoal,dt,speed,shouldRun);
        moved=length2d(e->v.origin-before)>.1f;
        if(moved){e->v.angles.y=std::atan2(delta.y,delta.x)*180.f/3.14159265f;mansionReleaseDoorRole(e);if(state)state->movedAt=globals->time;}
        return moved;
    }
    return false;
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
static bool mansionBridgePursuitClear(edict_t* e,edict_t* player){
    Vector goal=player->v.origin+Vector(0,0,player->v.mins.z),delta=goal-e->v.origin;
    if(std::fabs(delta.z)>18.f)return false;
    Vector from=e->v.origin+Vector(0,0,36.1f),to=goal+Vector(0,0,36.1f);TraceResult tr{};
    engine.pfnTraceHull(&from.x,&to.x,1,1,e,&tr);
    if(tr.fStartSolid||tr.fAllSolid)return false;
    // Fix 3: un monstruo vivo en medio no es pared. La separacion lateral de
    // mansionRouteStep lo rodea; solo el mundo bloquea la persecucion directa.
    if(tr.flFraction<.995f&&!(valid(tr.pHit)&&tr.pHit!=e&&aliveEnemy(tr.pHit)))return false;
    int steps=std::max(1,int(std::ceil(length2d(delta)/32.f)));
    for(int i=1;i<=steps;++i){Vector at=e->v.origin+delta*(float(i)/steps),top=at+Vector(0,0,18),bottom=at-Vector(0,0,24);
        engine.pfnTraceLine(&top.x,&bottom.x,1,e,&tr);
        if(tr.fStartSolid)return false;
        // Si la linea pega primero en un vivo, no significa falta de suelo.
        if(valid(tr.pHit)&&tr.pHit!=e&&aliveEnemy(tr.pHit))continue;
        if(tr.flFraction>=1.f||tr.vecPlaneNormal.z<.65f)return false;}
    return true;
}
static bool mansionTargetStarts(edict_t* e,const char* prefix){const char* name=text(e->v.target);return name&&std::strncmp(name,prefix,std::strlen(prefix))==0;}
static float mansionRouteSpeed(edict_t* e){const char* name=text(e->v.classname);return qzIs(e)?35.f:((czIs(e)||!std::strcmp(name,"monster_human_grunt"))?85.f:(randyClass(e)?mansionChaseSpeed(e):55.f));}
static MansionRouteIntent& mansionRouteIntentFor(edict_t* e){
    static MansionRouteIntent dummy{};
    int idx=engine.pfnIndexOfEdict(e);
    if(idx<=0)return dummy;
    if(size_t(idx)>=mansionRouteIntents.size())mansionRouteIntents.resize(idx+1);
    auto& it=mansionRouteIntents[idx];
    if(it.actor!=e||it.serial!=e->serialnumber)it=MansionRouteIntent{e,e->serialnumber};
    return it;
}
static bool mansionBridgeEdgeClear(edict_t* e,const Vector& a,const Vector& b){
    float sweep=std::max(a.z,b.z)+36.2f;Vector from(a.x,a.y,sweep),to(b.x,b.y,sweep);TraceResult tr{};
    engine.pfnTraceHull(&from.x,&to.x,1,1,e,&tr);
    if(tr.fStartSolid||tr.fAllSolid)return false;
    if(tr.flFraction>=.995f)return true;
    if(valid(tr.pHit)&&tr.pHit!=e&&aliveEnemy(tr.pHit))return true;
    return false;
}
static bool mansionBridgeSpotFree(edict_t* e,const Vector& at){
    Vector probe=at+Vector(0,0,36.2f);TraceResult tr{};
    engine.pfnTraceHull(&probe.x,&probe.x,1,1,e,&tr);
    if(tr.fStartSolid||tr.fAllSolid)return false;
    Vector lo=at+e->v.mins,hi=at+e->v.maxs;
    for(int i=1;i<=globals->maxClients;++i){auto o=engine.pfnPEntityOfEntIndex(i);
        if(!valid(o)||o==e||o->v.solid==SOLID_NOT||o->v.health<=0||o->v.deadflag!=DEAD_NO)continue;
        Vector oLo=o->v.origin+o->v.mins,oHi=o->v.origin+o->v.maxs;
        if(lo.x<oHi.x-2&&hi.x>oLo.x+2&&lo.y<oHi.y-2&&hi.y>oLo.y+2&&lo.z<oHi.z-2&&hi.z>oLo.z+2)return false;}
    for(auto o:getActiveEnemies()){
        if(o==e||o->v.solid==SOLID_NOT||!aliveEnemy(o))continue;
        Vector oLo=o->v.origin+o->v.mins,oHi=o->v.origin+o->v.maxs;
        if(lo.x<oHi.x-2&&hi.x>oLo.x+2&&lo.y<oHi.y-2&&hi.y>oLo.y+2&&lo.z<oHi.z-2&&hi.z>oLo.z+2)return false;}
    return true;
}
static bool mansionBridgeNudge(edict_t* e,const Vector& goal){
    Vector delta=goal-e->v.origin;float yaw=std::atan2(delta.y,delta.x)*180.f/3.14159265f;
    float rad=yaw*3.14159265f/180.f;Vector fwd(std::cos(rad),std::sin(rad),0),side(-fwd.y,fwd.x,0);
    Vector cands[]={e->v.origin+fwd*32.f,e->v.origin+side*28.f,e->v.origin-side*28.f,e->v.origin+fwd*20.f+side*28.f,e->v.origin+fwd*20.f-side*28.f,e->v.origin+side*48.f,e->v.origin-side*48.f};
    for(auto& at:cands){
        if(length2d(at-e->v.origin)<10.f)continue;
        if(!mansionBridgeEdgeClear(e,e->v.origin,at))continue;
        if(!mansionBridgeSpotFree(e,at))continue;
        Vector old=e->v.origin;
        engine.pfnSetOrigin(e,&at.x);engine.pfnDropToFloor(e);e->v.velocity=Vector(0,0,0);
        char rep[160];std::snprintf(rep,sizeof rep,"class=%s from=%.0f,%.0f,%.0f to=%.0f,%.0f,%.0f",text(e->v.classname),old.x,old.y,old.z,e->v.origin.x,e->v.origin.y,e->v.origin.z);
        startupTrace("MansionBridgeNudge",rep);
        return true;
    }
    return false;
}
static void mansionRouteAnimate(edict_t* e,bool running){
    if(judgmentKuarl(e)){judgmentAnimate(e,16,153,5.1f,true);return;}
    if(czIs(e)){
        // Fix 4: histeresis CZ. No alternar walk/run/idle cada tick; walk<->run
        // preserva frame en czSequence y el idle espera .6s via fuser1.
        int want=running?4:3;
        if(e->v.sequence!=want)czSequence(e,want);
        e->v.fuser1=globals->time+.6f;
        return;
    }
    if(qzIs(e))return;
    if(running)hordeRunAnimation(e);else hordeWalkAnimation(e);
}
static void mansionRouteStep(edict_t* e,const Vector& goal,float dt,float speed,bool running=false){
    if(!valid(e)||!aliveEnemy(e))return;
    auto& intent=mansionRouteIntentFor(e);
    Vector delta=goal-e->v.origin;float yaw=std::atan2(delta.y,delta.x)*180.f/3.14159265f;
    float distance=std::min(length2d(delta),speed*dt)*(qzIs(e)&&slowed(e)?balance::SlowFactor:1.f);
    if(distance<=0.f){intent.blocked=0;return;}
    // Fix 2: separacion. Si hay un vivo justo delante, sesga al lado libre.
    float bias=intent.sideBias>=0?1.f:-1.f;
    {
        float best=1e30f;int side=0;
        for(auto o:getActiveEnemies()){
            if(o==e||!aliveEnemy(o)||o->v.solid==SOLID_NOT)continue;
            Vector d=o->v.origin-e->v.origin;float dist2d=length2d(d);
            if(dist2d>72.f||std::fabs(d.z)>56.f)continue;
            float ang=std::atan2(d.y,d.x)*180.f/3.14159265f;float rel=ang-yaw;
            while(rel>180)rel-=360;while(rel<-180)rel+=360;
            if(std::fabs(rel)<50.f&&dist2d<best){best=dist2d;side=(rel>=0?-1:1);}
        }
        if(side!=0)bias=float(side);
    }
    bool moved=hookWalkMove(e,yaw,distance,0)!=0;
    float firstYaw=yaw;
    if(!moved){
        // Fix 1: abanico completo con sesgo inicial, no solo +-30/60/90.
        float turns[]={30.f*bias,-30.f*bias,60.f*bias,-60.f*bias,90.f*bias,-90.f*bias,135.f,-135.f,180.f};
        for(float t:turns){if(hookWalkMove(e,yaw+t,distance,0)){yaw+=t;moved=true;break;}}
    }
    if(moved){
        // Fix 5: movedAt solo con progreso real, no antes del WalkMove.
        intent.blocked=0;intent.sideBias=(yaw!=firstYaw)?(yaw>firstYaw?1:-1):intent.sideBias;
        if(auto st=davidState(e))st->movedAt=globals->time;
        e->v.angles.y=yaw;mansionRouteAnimate(e,running);
        return;
    }
    // Fix 1: sin progreso no vibrar: acumula, prueba nudge validado y redecide.
    intent.blocked+=dt;
    intent.sideBias=int(-bias);
    if(intent.blocked>=1.25f&&globals->time>=intent.recoveryAt){
        // Embebido en un brush: el nudge con arista falla en solido; busca
        // punto libre en anillo sin exigir arista desde dentro.
        if(!mansionBridgeSpotFree(e,e->v.origin)){
            Vector self=e->v.origin;
            for(float r:{32.f,48.f,64.f,96.f}){
                bool freed=false;
                for(float a:{0.f,90.f,180.f,270.f,45.f,135.f,225.f,315.f}){
                    float rad=a*3.14159265f/180.f;
                    Vector at=self+Vector(std::cos(rad)*r,std::sin(rad)*r,0);
                    if(!mansionBridgeSpotFree(e,at)||!mansionInsideFloor(at))continue;
                    Vector old=e->v.origin;
                    engine.pfnSetOrigin(e,&at.x);engine.pfnDropToFloor(e);e->v.velocity=Vector(0,0,0);
                    intent.blocked=0;intent.recoveryAt=globals->time+2.f;
                    if(auto st=davidState(e))st->movedAt=globals->time;
                    char rep[160];std::snprintf(rep,sizeof rep,"class=%s from=%.0f,%.0f,%.0f to=%.0f,%.0f,%.0f",text(e->v.classname),old.x,old.y,old.z,e->v.origin.x,e->v.origin.y,e->v.origin.z);
                    startupTrace("MansionBridgeUnwedge",rep);
                    freed=true;break;
                }
                if(freed)break;
            }
            if(intent.blocked==0){e->v.angles.y=yaw;mansionRouteAnimate(e,running);return;}
        }
        if(mansionBridgeNudge(e,goal)){intent.blocked=0;intent.recoveryAt=globals->time+2.f;
            if(auto st=davidState(e))st->movedAt=globals->time;
            e->v.angles.y=yaw;mansionRouteAnimate(e,running);return;}
        intent.recoveryAt=globals->time+.5f;
    }
    if(intent.blocked>=2.5f){
        intent.ready=false;intent.nextDecision=0;intent.blocked=0;intent.recoveryAt=globals->time+1.f;
        char rep[128];std::snprintf(rep,sizeof rep,"class=%s target=%s blocked=2.5s nudge_failed",text(e->v.classname),text(e->v.target)?text(e->v.target):"none");
        startupTrace("MansionBridgeStall",rep);
    }
}
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
    mansionSiegeMove(e,2,entry,dt,85.f);return true;
}
static bool mansionRouteMarkerMove(edict_t* e,float dt){
    const char* target=text(e->v.target);bool side=target&&std::strncmp(target,"mansion_side_route_",19)==0;
    bool main=target&&std::strncmp(target,"mansion_route_",14)==0;
    if(!side&&!main)return false;
    // Waypoint 00 coincides with the CT spawn. Never navigate back to it
    // after a chase or interruption; it only identifies the route's start.
    size_t targetLength=std::strlen(target);if(targetLength>=3&&!std::strcmp(target+targetLength-3,"_00")){
        auto start=mansionFind(target);if(valid(start)&&start->v.target){e->v.target=start->v.target;target=text(e->v.target);}
    }
    // Trial: horde zombies keep their assigned sector route, but follow each
    // waypoint through the CSZ graph planner. Cultists retain their authored
    // direct movement so this comparison changes zombie pathing alone.
    // Actors on initial bridge approach waypoints (_00, _01, _02) strictly
    // use authored direct steps so they never stall outside the CSZ graph.
    if(!mansionCultist(e)&&!mansionOnBridgeRoute(e)){
        if(mansionDoorHealth(side?2:1)<=0){mansionSetUpperPatrol(e,0);return true;}
        auto goal=mansionFind(target);if(!valid(goal))return true;
        int startNode=mansionNearestNav(e->v.origin),goalNode=mansionNearestNav(goal->v.origin);
        if(startNode>=0&&goalNode>=0){
            Vector startGap=czNavPoint(startNode)-e->v.origin,endGap=czNavPoint(goalNode)-goal->v.origin;
            // The CSZ graph covers the mansion interior well, but not every
            // authored perimeter route. Keep those outer segments on their
            // original waypoints instead of snapping actors to distant nodes.
            if(length2d(startGap)<192.f&&std::fabs(startGap.z)<48.f&&length2d(endGap)<128.f&&std::fabs(endGap.z)<40.f){
                Vector delta=goal->v.origin-e->v.origin;float reach=goal->v.target?18.f:20.f;
                const char* goalName=text(goal->v.targetname);size_t goalLength=std::strlen(goalName);
                if(goalLength>=3&&(!std::strcmp(goalName+goalLength-3,"_01")||!std::strcmp(goalName+goalLength-3,"_02")))reach=48;
                if(std::fabs(goal->v.origin.x-208)<1&&std::fabs(goal->v.origin.y+256)<1)reach=40;
                if(length2d(delta)<reach&&std::fabs(delta.z)<20){
                    if(!goal->v.target){e->v.target=engine.pfnAllocString(side?"mansion_wait_door_2":"mansion_wait_door_1");return true;}
                    e->v.target=goal->v.target;goal=mansionFind(text(e->v.target));if(!valid(goal))return true;
                }
                return mansionNavWalk(e,goal->v.origin,dt,mansionRouteSpeed(e));
            }
        }
    }
    int entityIndex=engine.pfnIndexOfEdict(e);if(entityIndex<=0)return true;
    if(size_t(entityIndex)>=mansionRouteIntents.size())mansionRouteIntents.resize(entityIndex+1);
    auto& intent=mansionRouteIntents[entityIndex];if(intent.actor!=e||intent.serial!=e->serialnumber)intent=MansionRouteIntent{e,e->serialnumber};
    if(intent.ready&&intent.target==e->v.target&&globals->time<intent.nextDecision){mansionRouteStep(e,intent.destination,dt,intent.speed);return true;}
    // Complete the spawn-hub funnel and bridge before switching to the
    // interior graph after a door is destroyed.
    if(mansionDoorHealth(side?2:1)<=0&&!(mansionOutsideCentral(e->v.origin)&&mansionOnBridgeRoute(e))){mansionSetUpperPatrol(e,0);return true;}
    auto goal=mansionFind(target);if(!valid(goal))return true;
    if(goal->v.target){auto next=mansionFind(text(goal->v.target));
        if(valid(next)&&length2d(next->v.origin-e->v.origin)<length2d(goal->v.origin-e->v.origin)&&mansionShortcut(e,next->v.origin)){
            e->v.target=goal->v.target;goal=next;
        }
    }
    Vector delta=goal->v.origin-e->v.origin;float reach=goal->v.target?18.f:20.f;
    const char* goalName=text(goal->v.targetname);size_t goalLength=std::strlen(goalName);
    if(goalLength>=3&&(!std::strcmp(goalName+goalLength-3,"_01")||!std::strcmp(goalName+goalLength-3,"_02")))reach=48;
    if(std::fabs(goal->v.origin.x-208)<1&&std::fabs(goal->v.origin.y+256)<1)reach=40;
    if(length2d(delta)<reach&&std::fabs(delta.z)<20){
    if(!goal->v.target){e->v.target=engine.pfnAllocString(side?"mansion_wait_door_2":"mansion_wait_door_1");return true;}
        e->v.target=goal->v.target;goal=mansionFind(text(e->v.target));if(!valid(goal))return true;delta=goal->v.origin-e->v.origin;
    }
    intent.destination=goal->v.origin;intent.speed=mansionRouteSpeed(e);intent.ready=true;intent.target=e->v.target;
    float interval=.35f;int senseIndex=engine.pfnIndexOfEdict(e);if(senseIndex>0&&size_t(senseIndex)<mansionSense.size())interval=mansionSense[senseIndex].decisionInterval;
    intent.nextDecision=globals->time+interval;
    mansionRouteStep(e,intent.destination,dt,intent.speed);
    return true;
}
static bool mansionTacticalZombie(edict_t* e){
    return valid(e)&&(davidClass(e)||randyClass(e)||qzIs(e)||!std::strcmp(text(e->v.classname),"monster_poisionzombie"));
}
static bool mansionRaidMelee(edict_t* e,edict_t* target,int index,float dt);
static edict_t* mansionPriorityPlayer(edict_t* e){
    if(!mansionTacticalZombie(e)||czIs(e))return nullptr;
    int index=engine.pfnIndexOfEdict(e);if(index<=0)return nullptr;
    if(size_t(index)>=mansionPlayerPursuits.size())mansionPlayerPursuits.resize(index+1);
    auto& s=mansionPlayerPursuits[index];if(s.actor!=e||s.actorSerial!=e->serialnumber){s=MansionPlayerPursuit{};s.actor=e;s.actorSerial=e->serialnumber;}
    if(valid(s.player)&&s.player->serialnumber==s.playerSerial&&(s.player->v.flags&FL_CLIENT)&&s.player->v.health>0&&s.player->v.deadflag==DEAD_NO)return s.player;
    s.player=nullptr;s.playerSerial=0;float closest=MansionZombieNoticeRange;
    for(int i=1;i<=globals->maxClients;++i){auto p=engine.pfnPEntityOfEntIndex(i);if(!johnnyPlayerAlive(p)||dmcInvisible(p)||!johnnyCanSee(e,p))continue;float distance=length(p->v.origin-e->v.origin);if(distance<closest){closest=distance;s.player=p;s.playerSerial=p->serialnumber;}}
    if(s.player){for(int i=0;i<2;++i)if(mansionRaiders[i].entity==e&&mansionRaiders[i].serial==e->serialnumber){
        if(mansionRaidAttacks[i].active&&davidState(e))davidActivity(e->pvPrivateData,nullptr,1);mansionRaidAttacks[i].active=false;}
        startupTrace("MansionPlayerPriority","zombie detected player; scientist objective interrupted");}
    return s.player;
}
static bool mansionPursuePlayer(edict_t* e,edict_t* player,float dt){
    mansionReleaseDoorRole(e);
    if(auto state=davidState(e)){state->target=player;state->targetSerial=player->serialnumber;}
    Vector delta=player->v.origin-e->v.origin;float distance=length2d(delta);
    // Fix 4: histeresis 60/90 en vez de umbral unico 70. Sin esto run/walk
    // parpadea y cada cambio reinicia la secuencia a frame 0.
    bool running;
    if(auto state=davidState(e)){
        bool was=state->running;
        running=was?(distance>=60.f):(distance>=90.f);
        running=running&&!davidCrawling(*state);
        state->running=running;
    }else{
        auto& intent=mansionRouteIntentFor(e);
        // Randy siempre corre, sin histeresis.
        if(randyClass(e)){running=true;intent.lastRunning=true;}
        else{
            bool want=intent.lastRunning?(distance>=60.f):(distance>=90.f);
            running=want;intent.lastRunning=want;
        }
    }
    // The CSZ graph ends at the mansion perimeter. On exterior bridges follow
    // the player directly when visible, or the authored bridge waypoints.
    if(mansionOutsideCentral(e->v.origin)&&mansionOnBridgeRoute(e)&&distance>64.f){
        if(!zombieBusy(e)&&johnnyCanSee(e,player)&&mansionBridgePursuitClear(e,player)){
            Vector before=e->v.origin;
            mansionRouteStep(e,player->v.origin,dt,running?mansionChaseSpeed(e):55.f,running);
            // movedAt ya se actualiza dentro de mansionRouteStep solo si hubo progreso.
            (void)before;
        }else if(!zombieBusy(e))mansionRouteMarkerMove(e,dt);
        return true;
    }
    if(czIs(e)){
        if(distance<=64.f&&std::fabs(delta.z)<80.f){e->v.angles.y=std::atan2(delta.y,delta.x)*180.f/3.14159265f;if(!czMeleeBusy(e))czFire(e,player);}
        else if(!czMeleeBusy(e)){
            // Fix 4: CZ tambien con histeresis (no forzar run 4 siempre).
            mansionRouteAnimate(e,running);
            mansionNavWalk(e,player->v.origin,dt,running?140.f:85.f,running);
        }return true;
    }
    for(int i=0;i<2;++i)if(mansionRaiders[i].entity==e&&mansionRaiders[i].serial==e->serialnumber){
        if(distance<=64.f&&std::fabs(delta.z)<80.f){mansionRaidMelee(e,player,i,dt);return true;}
        mansionRaidAttacks[i].active=false;
    }
    if(zombieBusy(e))return true;
    if(distance<=60.f&&std::fabs(delta.z)<80.f){e->v.angles.y=std::atan2(delta.y,delta.x)*180.f/3.14159265f;return true;}
    Vector before=e->v.origin;
    float speed=running?mansionChaseSpeed(e):55.f;if(auto state=davidState(e))if(davidCrawling(*state))speed=16.f;
    mansionNavWalk(e,player->v.origin,dt,speed,running);
    if(length2d(e->v.origin-before)>.1f){if(auto state=davidState(e))state->movedAt=globals->time;}
    return true;
}
static bool mansionTacticalMove(edict_t* e,float dt){
    if(tposeIs(e))return false;
    if(!aliveEnemy(e)||pushed(e)||czIs(e)||judgmentIs(e)||mansionCultist(e))return false;
    if(!mansionTacticalZombie(e))return false;
    int index=engine.pfnIndexOfEdict(e);if(index<=0)return false;
    if(size_t(index)>=mansionTacticalSenses.size())mansionTacticalSenses.resize(index+1);
    auto& s=mansionTacticalSenses[index];if(s.actor!=e||s.serial!=e->serialnumber){s=MansionTacticalSense{};s.actor=e;s.serial=e->serialnumber;}
    if(globals->time>=s.nextSense){
        s.nextSense=globals->time+.15f;auto target=czEnemy(e);
        if(valid(target)){
            if(s.target!=target||s.targetSerial!=target->serialnumber){s.reactAt=globals->time+.4f;s.target=target;s.targetSerial=target->serialnumber;}
            s.lastSeen=target->v.origin;s.rememberUntil=globals->time+2.f;
        }
    }
    if(!valid(s.target)||s.target->serialnumber!=s.targetSerial||s.target->v.health<=0||globals->time>s.rememberUntil){s.target=nullptr;return false;}
    if(globals->time<s.reactAt)return true;
    if(zombieBusy(e))return true;
    Vector delta=s.lastSeen-e->v.origin;float distance=length2d(delta);
    if(auto state=davidState(e)){state->target=s.target;state->targetSerial=s.targetSerial;}
    if(distance<=72.f){e->v.angles.y=std::atan2(delta.y,delta.x)*180.f/3.14159265f;return true;}
    Vector tacBefore=e->v.origin;
    bool running=false;float speed=mansionLocomotionSpeed(e,running);
    mansionReleaseDoorRole(e);mansionNavWalk(e,s.lastSeen,dt,speed,running);
    if(length2d(e->v.origin-tacBefore)>.1f){if(auto state=davidState(e))state->movedAt=globals->time;}
    return true;
}
static void mansionMove(edict_t* e,float dt){
    if(tposeIs(e))return;
    if(auto state=davidState(e))if(state->dying)return;
    if(mansionMap()&&mansionActor(e)&&aliveEnemy(e)&&!pushed(e))if(auto player=mansionPriorityPlayer(e)){mansionPursuePlayer(e,player,dt);return;}
    if(mansionEventMove(e,dt))return;
    if(judgmentZeal(e))return;
    bool cultist=mansionCultist(e),perimeterCultist=false;
    if(cultist){const char* tail=std::strrchr(text(e->v.targetname),'_');int index=tail?std::atoi(tail+1):0;
        perimeterCultist=index>=0&&index<80&&mansionPerimeterCultistRoles[index];}
    if(!mansionMap()||!mansionActor(e)||!aliveEnemy(e)||pushed(e)||czObjectiveOwns(e))return;
    if(mansionTacticalMove(e,dt))return;
    bool onBridge=mansionOnBridgeRoute(e)||mansionOutsideCentral(e->v.origin);
    bool contact=!mansionTacticalZombie(e)&&!onBridge&&!perimeterCultist&&mansionContact(e);
    if(zombieBusy(e))return;
    if(!mansionTacticalZombie(e)&&!onBridge&&!cultist&&!perimeterCultist&&!czIs(e)&&!qzIs(e)&&!judgmentIs(e)&&!tposeIs(e)){
        if(auto player=mansionVisiblePlayer(e,MansionZombieNoticeRange)){
            if(mansionChaseVisiblePlayer(e,player,dt))return;
        }
    }
    if(contact&&!mansionTargetStarts(e,"mansion_wait_door_")&&!mansionTargetStarts(e,"mansion_route_")&&!mansionTargetStarts(e,"mansion_side_route_"))return;
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
        mansionSiegeMove(e,door,entry,dt,qzIs(e)?35.f:(!std::strcmp(text(e->v.classname),"monster_randy")?110.f:(!std::strcmp(text(e->v.classname),"monster_max")?110.f:55.f)));
        return;
    }
    if(mansionRouteMarkerMove(e,dt))return;
}
static void mansionPrecache(){
    if(!mansionMap())return;
    for(auto path:{"debris/flesh1.wav","debris/flesh2.wav","debris/flesh3.wav","buttons/spark5.wav","buttons/spark6.wav"})engine.pfnPrecacheSound(const_cast<char*>(path));
    for(auto path:{"models/cremation/scientists/valve_hd/models/scientist.mdl","models/cremation/scientists/bshift_hd/models/scientist.mdl","models/cremation/scientists/female_valve/scientist.mdl","models/cremation/scientists/female_gearbox/scientist.mdl"})engine.pfnPrecacheModel(const_cast<char*>(path));
    // GoldSrc retains these pointers for client resource transfers.
    static char valveSequences[10][96]{},bshiftSequences[11][96]{};
    for(int i=1;i<=10;++i){auto& path=valveSequences[i-1];std::snprintf(path,sizeof path,"models/cremation/scientists/valve_hd/models/scientist%02d.mdl",i);engine.pfnPrecacheGeneric(path);}
    for(int i=1;i<=11;++i){auto& path=bshiftSequences[i-1];std::snprintf(path,sizeof path,"models/cremation/scientists/bshift_hd/models/scientist%02d.mdl",i);engine.pfnPrecacheGeneric(path);}
    // Native Spawn performs each monster's full sound/model precache now,
    // before GoldSrc closes its resource list. No prototypes enter the wave.
    for(int type=0;type<7;++type){auto e=engine.pfnCreateNamedEntity(engine.pfnAllocString(mansion::classname(type)));if(!valid(e)){startupTrace("MansionPrecacheCreateFailed",mansion::classname(type));continue;}
        e->v.origin=Vector(1336,392,24);hookSpawn(e);engine.pfnRemoveEntity(e);}
    auto scientist=engine.pfnCreateNamedEntity(engine.pfnAllocString("monster_scientist"));if(valid(scientist)){hookSpawn(scientist);engine.pfnRemoveEntity(scientist);}
    for(auto path:{"debris/wood1.wav","cremation/scientist_rescue/male_help.wav","cremation/scientist_rescue/male_fear.wav","cremation/scientist_rescue/male_thanks.wav","cremation/scientist_rescue/male_pain.wav","cremation/scientist_rescue/female_help.wav","cremation/scientist_rescue/female_fear.wav","cremation/scientist_rescue/female_thanks.wav","cremation/scientist_rescue/female_pain.wav"})engine.pfnPrecacheSound(const_cast<char*>(path));
    judgmentPrecache();
}
static void mansionScientistMessage(const char* message){
    if(!engine.pfnClientPrintf)return;
    for(int i=1;i<=globals->maxClients;++i){auto player=engine.pfnPEntityOfEntIndex(i);if(valid(player)&&(player->v.flags&FL_CLIENT))engine.pfnClientPrintf(player,print_chat,message);}
}
static void mansionScientistNarration(int phase){
    if(!msgMansionNarration)return;
    for(int i=1;i<=globals->maxClients&&i<33;++i){auto player=engine.pfnPEntityOfEntIndex(i);if(!valid(player)||!(player->v.flags&FL_CLIENT))continue;
        engine.pfnMessageBegin(MSG_ONE,msgMansionNarration,nullptr,player);engine.pfnWriteByte(phase);engine.pfnMessageEnd();}
}
static void mansionScientistSceneCamera(bool active){
    if(active){
        if(!valid(mansionScientistCamera)||mansionScientistCamera->serialnumber!=mansionScientistCameraSerial){
            mansionScientistCamera=engine.pfnCreateNamedEntity(engine.pfnAllocString("info_target"));if(!valid(mansionScientistCamera))return;raven.pfnSpawn(mansionScientistCamera);
            mansionScientistCamera->v.targetname=engine.pfnAllocString("mansion_scientist_scene_camera");mansionScientistCamera->v.solid=SOLID_NOT;mansionScientistCamera->v.movetype=MOVETYPE_NOCLIP;
            mansionScientistCameraSerial=mansionScientistCamera->serialnumber;
        }
        // Low view from inside the room towards the scientist's final mark.
        mansionScientistCamera->v.origin=Vector(-480,-304,220);mansionScientistCamera->v.angles=Vector(0,137,0);engine.pfnSetOrigin(mansionScientistCamera,&mansionScientistCamera->v.origin.x);
        for(int i=1;i<=globals->maxClients&&i<33;++i){auto player=engine.pfnPEntityOfEntIndex(i);if(!valid(player)||!(player->v.flags&FL_CLIENT)||player->v.health<=0)continue;
            // GoldSrc excludes entities with no model from client snapshots.
            // The native trigger_camera also assigns the player's model before use.
            if(player->v.model&&*text(player->v.model)){engine.pfnSetModel(mansionScientistCamera,text(player->v.model));mansionScientistCamera->v.effects&=~EF_NODRAW;mansionScientistCamera->v.rendermode=kRenderTransTexture;mansionScientistCamera->v.renderamt=0;}
            mansionScientistWasFrozen[i]=(player->v.flags&FL_FROZEN)!=0;player->v.flags|=FL_FROZEN;if(engine.pfnSetView)engine.pfnSetView(player,mansionScientistCamera);}
        mansionScientistSceneCameraActive=true;mansionScientistNarration(2);startupTrace("MansionSceneCamera","view set; network model assigned");
    }else{
        if(!mansionScientistSceneCameraActive){if(mansionScientistScenePhase)mansionScientistNarration(0);return;}mansionScientistSceneCameraActive=false;
        for(int i=1;i<=globals->maxClients&&i<33;++i){auto player=engine.pfnPEntityOfEntIndex(i);if(!valid(player)||!(player->v.flags&FL_CLIENT))continue;
            if(engine.pfnSetView)engine.pfnSetView(player,player);if(!mansionScientistWasFrozen[i])player->v.flags&=~FL_FROZEN;mansionScientistWasFrozen[i]=false;}
        mansionScientistNarration(0);
    }
}
static int mansionScientistRunSequence(edict_t* e){
    auto data=static_cast<unsigned char*>(engine.pfnGetModelPtr(e));if(!data)return 5;int count=*reinterpret_cast<int*>(data+164),offset=*reinterpret_cast<int*>(data+168);
    int walk=-1;for(int i=0;i<count;++i){auto label=reinterpret_cast<const char*>(data+offset+i*176);if(std::strstr(label,"run"))return i;if(walk<0&&std::strstr(label,"walk"))walk=i;}
    return walk>=0?walk:5;
}
static bool mansionScientistSpawn(int wave){
    if(valid(mansionScientist.entity)&&mansionScientist.entity->serialnumber==mansionScientist.serial)return true;
    static const char* models[]={"models/cremation/scientists/valve_hd/models/scientist.mdl","models/cremation/scientists/bshift_hd/models/scientist.mdl","models/cremation/scientists/female_valve/scientist.mdl","models/cremation/scientists/female_gearbox/scientist.mdl"};
    static const int skins[]={2,2,3,3};
    int choice=engine.pfnRandomLong(2,3);
#ifdef CREMATION_ENABLE_VALIDATION
    char forced[8]{};GetEnvironmentVariableA("CREMATION_SCIENTIST_VARIANT",forced,sizeof forced);if(forced[0]=='0'||forced[0]=='2')choice=forced[0]-'0';
#endif
    auto scientist=engine.pfnCreateNamedEntity(engine.pfnAllocString("monster_scientist"));
    if(!valid(scientist)){startupTrace("MansionScientistSpawnFailed","monster_scientist allocation failed");return false;}
    scientist->v.origin=Vector(64,-256,56.1f);scientist->v.targetname=engine.pfnAllocString("mansion_rescue_scientist");
    hookSpawn(scientist);
    if(!aliveEnemy(scientist)||!scientist->pvPrivateData){if(valid(scientist))engine.pfnRemoveEntity(scientist);startupTrace("MansionScientistSpawnFailed","native scientist Spawn failed");return false;}
    if(raven.pfnThink)raven.pfnThink(scientist);
    scientist->v.flags&=~FL_GODMODE;scientist->v.takedamage=DAMAGE_AIM;
    scientist->v.health*=10.f;scientist->v.max_health=scientist->v.health;
    engine.pfnEmitSound(scientist,CHAN_BODY,"debris/wood1.wav",1.f,ATTN_NONE,0,100);
    engine.pfnSetModel(scientist,const_cast<char*>(models[choice]));scientist->v.skin=engine.pfnRandomLong(0,skins[choice]-1);
    Vector scientistMins(-16,-16,0),scientistMaxs(16,16,72);engine.pfnSetSize(scientist,&scientistMins.x,&scientistMaxs.x);
    scientist->v.origin=Vector(64,-256,56.1f);scientist->v.angles.y=180.f;scientist->v.movetype=MOVETYPE_NONE;scientist->v.velocity=Vector(0,0,0);scientist->v.sequence=13;scientist->v.frame=0;scientist->v.animtime=globals->time;scientist->v.nextthink=globals->time+.2f;
    engine.pfnSetOrigin(scientist,&scientist->v.origin.x);
    mansionScientist={scientist,scientist->serialnumber,wave,false};
    mansionScientistFemale=choice>=2;mansionScientistLastHealth=scientist->v.health;mansionScientistPainAt=0;
    mansionScientistKnockAt=globals->time+2.f;mansionScientistVoiceAt=globals->time+.4f;mansionScientistVoiceLine=0;mansionScientistThankUntil=0;mansionScientistEntryAt=0;
    char detail[128];std::snprintf(detail,sizeof detail,"wave=%d variant=%d skin=%d front_door=64,-256,56",wave,choice,int(scientist->v.skin));startupTrace("MansionScientistSpawn",detail);
    return true;
}
static void mansionScientistEnsure(int wave){
    if(wave<1||wave>4||mansionScientistWaveSpawned==wave)return;
    if(mansionScientistAppearAt==0){mansionScientistAppearAt=globals->time+2.f;return;}if(globals->time<mansionScientistAppearAt)return;
    if(mansionScientistSpawn(wave))mansionScientistWaveSpawned=wave;
}
static void mansionScientistRescue(MansionScientistRef& ref){
    auto scientist=ref.entity;if(!valid(scientist)||ref.rescued)return;
    ref.rescued=true;mansionScientistRunningIn=false;
    scientist->v.velocity=Vector(0,0,0);scientist->v.movetype=MOVETYPE_NONE;scientist->v.sequence=13;scientist->v.frame=0;scientist->v.animtime=globals->time;
    reactorScientistStationActivate();startupTrace("MansionScientistRescued","upstairs central room; third floating weapon station activated");
}
static bool mansionScientistThink(edict_t* e){
    auto& ref=mansionScientist;
    if(!valid(e)||e!=ref.entity||e->serialnumber!=ref.serial||!e->v.targetname||std::strcmp(text(e->v.targetname),"mansion_rescue_scientist"))return false;
    if(!aliveEnemy(e))return false;
    e->v.nextthink=globals->time+.25f;return true;
}
static void mansionEventAnimate(edict_t* e,float dt){
    auto data=static_cast<unsigned char*>(engine.pfnGetModelPtr(e));if(!data)return;
    int count=*reinterpret_cast<int*>(data+164),offset=*reinterpret_cast<int*>(data+168);if(e->v.sequence<0||e->v.sequence>=count)return;
    auto seq=data+offset+e->v.sequence*176;float fps=*reinterpret_cast<float*>(seq+32);int frames=*reinterpret_cast<int*>(seq+56);
    if(frames>1)e->v.frame=std::fmod(e->v.frame+255.f*fps*dt/float(frames-1),256.f);e->v.animtime=globals->time;
}
static void mansionScientistFrame(float dt){
    auto& ref=mansionScientist;auto e=ref.entity;
    if(!valid(e)||e->serialnumber!=ref.serial||!aliveEnemy(e))return;
    mansionEventAnimate(e,dt);
    if(mansionScientistSceneEndAt>0&&globals->time>=mansionScientistSceneEndAt){if(mansionScientistScenePhase!=3){mansionScientistScenePhase=3;mansionScientistNarration(3);mansionScientistSceneEndAt=globals->time+.5f;}else{mansionScientistSceneEndAt=0;mansionScientistSceneCamera(false);}}
    if(ref.rescued)return;
    if(mansionScientistRunningIn){
        Vector destination(-624,-176,192.1f);bool arrived=mansionNavWalk(e,destination,dt,180.f,true);
        if(arrived){mansionScientistRescue(ref);mansionScientistMessage("Cientifico rescatado");startupTrace("MansionScientistScene","reached upstairs central-room mark through doorway");}
        else{e->v.movetype=MOVETYPE_STEP;e->v.sequence=mansionScientistRunSequence(e);}
        return;
    }
    e->v.velocity=Vector(0,0,0);
    if(mansionScientistThankUntil>0){if(globals->time>=mansionScientistThankUntil)mansionScientistRescue(ref);return;}
    auto door=mansionDoor(1);
    bool open=valid(door)&&(std::fabs(door->v.angles.x)>30.f||std::fabs(door->v.angles.y)>30.f||std::fabs(door->v.angles.z)>30.f||door->v.solid==SOLID_NOT);
    if(open||mansionScientistScenePhase==1||mansionScientistScenePhase==2){
        if(mansionScientistScenePhase==0){mansionScientistScenePhase=1;mansionScientistEntryAt=globals->time+.5f;mansionScientistNarration(1);return;}
        if(globals->time<mansionScientistEntryAt)return;
        if(mansionScientistScenePhase==1){mansionScientistScenePhase=2;mansionScientistSceneCamera(true);mansionScientistSceneEndAt=globals->time+2.5f;mansionScientistEntryAt=globals->time+.5f;return;}
        Vector behindWall(-352,-320,192.1f);engine.pfnSetOrigin(e,&behindWall.x);e->v.movetype=MOVETYPE_STEP;e->v.velocity=Vector(0,0,0);e->v.angles.y=180;e->v.sequence=mansionScientistRunSequence(e);e->v.frame=0;e->v.animtime=globals->time;
        mansionScientistRunningIn=true;startupTrace("MansionScientistScene","scientist appeared behind upstairs wall and started running through doorway");return;
    }
    if(mansionScientistScenePhase!=0)return;
    mansionScientistEntryAt=0;
    if(globals->time>=mansionScientistKnockAt){engine.pfnEmitSound(e,CHAN_BODY,"debris/wood1.wav",1.f,ATTN_NORM,0,100);mansionScientistKnockAt=globals->time+engine.pfnRandomFloat(2.f,3.5f);}
    if(globals->time>=mansionScientistVoiceAt){bool fear=(mansionScientistVoiceLine++%2)!=0;const char* line=mansionScientistFemale?(fear?"cremation/scientist_rescue/female_fear.wav":"cremation/scientist_rescue/female_help.wav"):(fear?"cremation/scientist_rescue/male_fear.wav":"cremation/scientist_rescue/male_help.wav");
        engine.pfnEmitSound(e,CHAN_VOICE,line,1.f,ATTN_NORM,0,100);mansionScientistVoiceAt=globals->time+engine.pfnRandomFloat(3.5f,5.f);}

}
static bool mansionInsideFloor(const Vector& at){
    Vector top=at+Vector(0,0,18),bottom=at-Vector(0,0,24);TraceResult tr{};
    engine.pfnTraceLine(&top.x,&bottom.x,1,nullptr,&tr);
    if(tr.fStartSolid||tr.flFraction>=1.f||tr.vecPlaneNormal.z<.65f)return false;
    return true;
}
static bool mansionPlaceInside(edict_t* e,const Vector& desired){
    // Todo punto interior debe pertenecer al grafo de rutas y estar libre.
    // Prueba el punto pedido, anillo 32/64 y el nodo nav mas cercano.
    Vector cands[10];
    cands[0]=desired;
    cands[1]=desired+Vector(32,0,0);cands[2]=desired+Vector(-32,0,0);
    cands[3]=desired+Vector(0,32,0);cands[4]=desired+Vector(0,-32,0);
    cands[5]=desired+Vector(64,0,0);cands[6]=desired+Vector(-64,0,0);
    cands[7]=desired+Vector(0,64,0);cands[8]=desired+Vector(0,-64,0);
    int nearNode=mansionNearestNav(desired);
    cands[9]=(nearNode>=0)?czNavPoint(nearNode):desired;
    for(auto& at:cands){
        int node=mansionNearestNav(at);
        if(node<0)continue;
        Vector np=czNavPoint(node);
        if(length2d(np-at)>96.f||std::fabs(np.z-at.z)>20.f)continue;
        {
            Vector probe=at+Vector(0,0,36.2f);TraceResult tr{};
            engine.pfnTraceHull(&probe.x,&probe.x,1,1,e,&tr);
            if(tr.fStartSolid||tr.fAllSolid)continue;
        }
        if(!mansionBridgeSpotFree(e,at))continue;
        if(!mansionInsideFloor(at))continue;
        Vector old=e->v.origin;
        engine.pfnSetOrigin(e,&at.x);engine.pfnDropToFloor(e);
        e->v.velocity=Vector(0,0,0);
        {
            Vector probe=e->v.origin+Vector(0,0,36.2f);TraceResult tr{};
            engine.pfnTraceHull(&probe.x,&probe.x,1,1,e,&tr);
            if(tr.fStartSolid||tr.fAllSolid){engine.pfnSetOrigin(e,&old.x);continue;}
        }
        if(!mansionInsideFloor(e->v.origin)){engine.pfnSetOrigin(e,&old.x);continue;}
        char rep[192];std::snprintf(rep,sizeof rep,"class=%s from=%.0f,%.0f,%.0f to=%.0f,%.0f,%.0f",
            text(e->v.classname),old.x,old.y,old.z,e->v.origin.x,e->v.origin.y,e->v.origin.z);
        startupTrace("MansionInsidePlace",rep);
        return true;
    }
    char rep[160];std::snprintf(rep,sizeof rep,"class=%s desired=%.0f,%.0f,%.0f no_free_spot",
        text(e->v.classname),desired.x,desired.y,desired.z);
    startupTrace("MansionInsideBlocked",rep);
    return false;
}
static void mansionEventFrame(){
    auto state=mansionState();if(!valid(state))return;
    auto sc=mansionScientist.entity;
    if(valid(sc)&&sc->serialnumber==mansionScientist.serial){
        if(sc->v.health<mansionScientistLastHealth&&globals->time>=mansionScientistPainAt){
            engine.pfnEmitSound(sc,CHAN_VOICE,mansionScientistFemale?"cremation/scientist_rescue/female_pain.wav":"cremation/scientist_rescue/male_pain.wav",1.f,ATTN_NORM,0,100);
            mansionScientistPainAt=globals->time+1.5f;startupTrace("MansionScientistPain",mansionScientistFemale?"female":"male");
        }mansionScientistLastHealth=sc->v.health;
    }
    if(mansionScientist.entity&&(!valid(sc)||sc->serialnumber!=mansionScientist.serial||sc->v.health<=0||sc->v.deadflag!=DEAD_NO)){
        if(state->v.impulse!=2){state->v.impulse=2;mansionScientistSceneEndAt=0;mansionScientistSceneCamera(false);mansionScientistMessage("GAME OVER - Cientifico muerto. Reinicia o sal desde el menu.");startupTrace("MansionGameOver","scientist died");
            for(auto e:getActiveEnemies())if(mansionActor(e)){e->v.velocity=Vector(0,0,0);e->v.flags|=FL_FROZEN;}}
        return;
    }
    if(!mansionInvaded&&(mansionDoorHealth(1)<=0||mansionDoorHealth(2)<=0)){
        mansionInvaded=true;for(const auto& light:mansionLightList)engine.pfnLightStyle(light.style,const_cast<char*>("a"));
        startupTrace("MansionInvasion","first broken door");
    }
    if(!mansionInvaded)return;
    int count=0;for(auto& r:mansionRaiders)if(r.entity)++count;
    for(auto e:getActiveEnemies()){
        if(count>=2&&mansionGroundAssigned>=3)break;if(!mansionActor(e)||!aliveEnemy(e)||czIs(e)||judgmentIs(e)||tposeIs(e)||mansionCultist(e))continue;
        if(auto dying=davidState(e))if(dying->dying)continue;
        bool assigned=mansionTargetStarts(e,"mansion_invasion_idle");for(auto& r:mansionRaiders)if(r.entity==e)assigned=true;if(assigned)continue;
        if(count>=2){Vector at(-128.f-float(mansionGroundAssigned)*128.f,-512,56.1f);
            // Planta baja: el punto fijo debe estar en el grafo y libre.
            // Sin esto el zombie nace dentro de la geometria y se queda quieto.
            if(mansionPlaceInside(e,at)){e->v.target=engine.pfnAllocString("mansion_invasion_idle");++mansionGroundAssigned;}
            continue;}
        // Choose models whose native melee animation events can be driven for
        // the scientist objective. Other zombie types keep their own attacks.
        if(!davidState(e)&&!randyClass(e))continue;
        Vector at=count==0?Vector(-352,-640,192.1f):Vector(-352,128,192.1f);
        // Raiders de planta alta: mismo requisito de grafo + punto libre.
        if(!mansionPlaceInside(e,at))continue;
        e->v.target=0;
        mansionRaiders[count]={e,e->serialnumber};++count;
    }
}
static bool mansionRaidMelee(edict_t* e,edict_t* target,int index,float dt){
    auto& attack=mansionRaidAttacks[index];
    if(auto state=davidState(e))if(state->dying){attack.active=false;return false;}
    if(!aliveEnemy(e)||!valid(target)||target->v.health<=0||target->v.deadflag!=DEAD_NO||!e->pvPrivateData){attack.active=false;return false;}
    auto model=static_cast<unsigned char*>(engine.pfnGetModelPtr(e));if(!model)return false;
    int count=*reinterpret_cast<int*>(model+164),offset=*reinterpret_cast<int*>(model+168);
    if(!attack.active){
        if(length(target->v.origin-e->v.origin)>64.f||globals->time<mansionRaidHitAt[index]||zombieBusy(e))return false;
        e->v.angles.y=std::atan2(target->v.origin.y-e->v.origin.y,target->v.origin.x-e->v.origin.x)*180.f/3.14159265f;
        auto table=*reinterpret_cast<void***>(e->pvPrivateData);
        reinterpret_cast<void(__thiscall*)(void*,int)>(table[ravenholm_abi::slot::SetActivity])(e->pvPrivateData,30);
        int seq=e->v.sequence;if(seq<0||seq>=count)return false;
        auto desc=model+offset+seq*176;int activity=*reinterpret_cast<int*>(desc+40),events=*reinterpret_cast<int*>(desc+48);
#ifdef CREMATION_ENABLE_VALIDATION
        {char detail[100];std::snprintf(detail,sizeof detail,"start=%d seq=%d act=%d events=%d",index,seq,activity,events);startupTrace("MansionRaidMelee",detail);}
#endif
        if((activity!=30&&activity!=31)||events<=0)return false;
        attack.active=true;attack.sequence=seq;attack.frame=0;e->v.frame=0;e->v.animtime=globals->time;
    }
    if(e->v.sequence!=attack.sequence){attack.active=false;return false;}
    auto desc=model+offset+attack.sequence*176;float fps=*reinterpret_cast<float*>(desc+32);
    int frames=*reinterpret_cast<int*>(desc+56),events=*reinterpret_cast<int*>(desc+48),eventOffset=*reinterpret_cast<int*>(desc+52);
    if(frames<=1||events<0||events>32){attack.active=false;return false;}
    float before=attack.frame;attack.frame=std::min(float(frames-1),before+dt*fps*(slowed(e)?balance::SlowFactor:1.f));
    e->v.frame=attack.frame*255.f/float(frames-1);e->v.animtime=globals->time;
    for(int i=0;i<events;++i){auto item=model+eventOffset+i*76;int frame=*reinterpret_cast<int*>(item),id=*reinterpret_cast<int*>(item+4);
        if(id<1||id>3||float(frame)<=before||float(frame)>attack.frame)continue;
        MonsterEvent_t event{};event.event=id;event.options=reinterpret_cast<char*>(item+12);
        auto table=*reinterpret_cast<void***>(e->pvPrivateData);
#ifdef CREMATION_ENABLE_VALIDATION
        engine.pfnMakeVectors(&e->v.angles.x);Vector start=e->v.origin+Vector(0,0,e->v.size.z*.5f),end=start+globals->v_forward*70.f;TraceResult probe{};
        engine.pfnTraceHull(&start.x,&end.x,0,3,e,&probe);
        {char row[160];std::snprintf(row,sizeof row,"hit=%s solid=%d targetbounds=%.0f,%.0f attackerheight=%.0f",valid(probe.pHit)?text(probe.pHit->v.classname):"world",target->v.solid,target->v.mins.z,target->v.maxs.z,e->v.size.z);startupTrace("MansionRaidTrace",row);}
#endif
        reinterpret_cast<void(__thiscall*)(void*,MonsterEvent_t*)>(table[ravenholm_abi::slot::HandleAnimEvent])(e->pvPrivateData,&event);
#ifdef CREMATION_ENABLE_VALIDATION
        {char detail[100];std::snprintf(detail,sizeof detail,"event=%d target_hp=%.1f distance=%.1f",id,target->v.health,length(target->v.origin-e->v.origin));startupTrace("MansionRaidMelee",detail);}
#endif
    }
    if(attack.frame>=frames-1){attack.active=false;mansionRaidHitAt[index]=globals->time+.25f;}
    return true;
}
static bool mansionScientistRoom(const Vector& p){return p.x>-664.f&&p.x<-432.f&&p.y>-480.f&&p.y<-144.f&&p.z>=180.f&&p.z<300.f;}
static edict_t* mansionCZRoomPlayer(edict_t* e){
    if(!mansionScientistRoom(e->v.origin))return nullptr;
    for(int i=1;i<=globals->maxClients;++i){auto p=engine.pfnPEntityOfEntIndex(i);if(johnnyPlayerAlive(p)&&!dmcInvisible(p)&&mansionScientistRoom(p->v.origin))return p;}
    return nullptr;
}
static void mansionCZEquipPistol(edict_t* e,bool pistol){
    if((e->v.iuser1==5)==pistol)return;e->v.iuser1=pistol?5:0;
    engine.pfnSetModel(e,pistol?"models/cremation/czero/zombie_glock.mdl":CZModel);
    Vector lo(-16,-16,0),hi(16,16,72);engine.pfnSetSize(e,&lo.x,&hi.x);
    if(pistol)e->v.button=20;czSequence(e,pistol?19:75);czGunFrame(e);
    startupTrace("MansionCZWeapon",pistol?"pistol: player in scientist room":"knife: resume scientist objective");
}
static bool mansionEventMove(edict_t* e,float dt){
    if(tposeIs(e))return false;
    if(!mansionActor(e)||!aliveEnemy(e))return false;
    bool czInside=czIs(e)&&e->v.origin.x<0&&e->v.origin.x>-768&&e->v.origin.y>-768&&e->v.origin.y<224;
    if(!mansionInvaded&&!czInside)return false;
    if(auto state=davidState(e))if(state->dying)return false;
    // Keep every exterior actor, including the CS Zero NPC, on its funnel
    // and bridge until it reaches the interior navigation area.
    if(mansionInvaded&&mansionOutsideCentral(e->v.origin)&&mansionOnBridgeRoute(e))return false;
    auto sc=mansionScientist.entity;
    if(czIs(e)){
        if(!aliveEnemy(sc))return true;
        if(auto player=mansionCZRoomPlayer(e)){mansionCZEquipPistol(e,true);czPistol(e,player);return true;}
        mansionCZEquipPistol(e,false);
        Vector delta=sc->v.origin-e->v.origin;float distance=length2d(delta);
        if(distance<=64.f&&std::fabs(delta.z)<80.f){e->v.angles.y=std::atan2(delta.y,delta.x)*180.f/3.14159265f;if(!czMeleeBusy(e))czFire(e,sc);}
        else if(!czMeleeBusy(e)){bool czRun=distance>90.f;mansionRouteAnimate(e,czRun);mansionNavWalk(e,sc->v.origin,dt,czRun?140.f:85.f,czRun);}
        return true;
    }
    for(int i=0;i<2;++i)if(mansionRaiders[i].entity==e&&mansionRaiders[i].serial==e->serialnumber){
        if(!aliveEnemy(sc))return true;
        if(mansionRaidMelee(e,sc,i,dt))return true;
        mansionEventAnimate(e,dt);
        Vector goal=(mansionScientist.rescued?sc->v.origin:Vector(-624,-176,192.1f))+Vector(0,i?48.f:-48.f,0);
        if(!mansionScientist.rescued||length(sc->v.origin-e->v.origin)>60.f){
            Vector evBefore=e->v.origin;
            bool running=false;float speed=mansionLocomotionSpeed(e,running);mansionNavWalk(e,goal,dt,speed,running);
            if(length2d(e->v.origin-evBefore)>.1f){if(auto state=davidState(e))state->movedAt=globals->time;}
        }
        return true;
    }
    if(mansionTacticalMove(e,dt))return true;
    if(e->v.origin.x<0&&e->v.origin.x>-768&&e->v.origin.y>-768&&e->v.origin.y<256){return true;}
    // Exterior horde continues its siege against a remaining intact door.
    int door=mansionDoorHealth(1)>0?1:(mansionDoorHealth(2)>0?2:0);
    if(!door)return true;
    const char* destination=door==1?"mansion_wait_door_1":"mansion_wait_door_2";
    if(!mansionTargetStarts(e,destination))e->v.target=engine.pfnAllocString(destination);
    return false;
}
static bool mansionEventThink(edict_t* e){
    if(tposeIs(e))return false;
    if(!mansionMap()||!mansionActor(e)||!aliveEnemy(e))return false;
    if(auto state=davidState(e))if(state->dying)return false;
    auto state=mansionState();if(valid(state)&&state->v.impulse==2){e->v.nextthink=globals->time+.2f;return true;}
    if(mansionPriorityPlayer(e)){for(auto& r:mansionRaiders)if(r.entity==e&&r.serial==e->serialnumber){e->v.nextthink=globals->time+.2f;return true;}return false;}
    if(!mansionInvaded)return false;
    if(czIs(e))return false; // Keep the knife animation and visual updating.
    for(auto& r:mansionRaiders)if(r.entity==e&&r.serial==e->serialnumber){e->v.nextthink=globals->time+.2f;return true;}
    if(e->v.origin.x<0&&e->v.origin.x>-768&&e->v.origin.y>-768&&e->v.origin.y<256){
        if(valid(czEnemy(e)))return false;
        if(!mansionTargetStarts(e,"mansion_invasion_idle_ready")){e->v.target=engine.pfnAllocString("mansion_invasion_idle_ready");e->v.sequence=0;e->v.frame=0;e->v.animtime=globals->time;}e->v.nextthink=globals->time+.2f;return true;
    }return false;
}

static bool mansionEmit(edict_t* state){
    int wave=int(state->v.frags),index=int(state->v.armorvalue),type=mansion::kind(wave,index);
    // The boss is a separate encounter after the complete regular horde.
    if(wave==4&&index==mansion::hordeTotal(wave)&&mansionAlive()!=0)return false;
#ifdef CREMATION_ENABLE_VALIDATION
    if(!mansionTest&&type!=5&&!(wave==4&&index==mansion::hordeTotal(wave))&&mansionHordeAlive()>=10)return false;
#else
    if(type!=5&&!(wave==4&&index==mansion::hordeTotal(wave))&&mansionHordeAlive()>=10)return false;
#endif
#ifdef CREMATION_ENABLE_VALIDATION
    bool johnny=type!=5&&type!=6&&mansion::isJohnny(wave,index-(wave<=3?1:0))&&!mansionCZRouteTest;
    const char* actorMarkerFormat=johnny?"mansion_enemy_johnny_%d_%02d":(wave<=2?"mansion_enemy_fixed_%d_%02d":"mansion_enemy_%d_%02d");
    if(mansionCZRouteTest)type=5;else if(johnny)type=0;
#else
    bool johnny=type!=5&&type!=6&&mansion::isJohnny(wave,index-(wave<=3?1:0));
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
    if(type==5)e->v.weapons=-1; // No C4 objective.
    if(!aliveEnemy(e)){engine.pfnRemoveEntity(e);startupTrace("MansionError","monster Spawn failed");return false;}
    invalidateActiveEnemies();
    std::snprintf(marker,sizeof marker,actorMarkerFormat,wave,index);e->v.targetname=engine.pfnAllocString(marker);
    bool perimeterCultist=index>=0&&index<80&&mansionPerimeterCultistRoles[index];
    if(cultistSlot>=0&&perimeterCultist)e->v.target=engine.pfnAllocString("mansion_cultist_perimeter");
    else if(cultistSlot>=0&&mansion::entersBackDoor(wave,index))e->v.target=engine.pfnAllocString("mansion_cultist_backdoor");
    else{std::snprintf(marker,sizeof marker,type==5||mansion::doorForSector(cursor)>=2?"mansion_side_route_%02d_01":"mansion_route_%02d_01",cursor);e->v.target=engine.pfnAllocString(marker);}
    // Quake's standalone idle patrol must not independently move this actor;
    // the shared Mansion approach owns idle movement; native combat stays intact.
    e->v.chain=nullptr;
    engine.pfnSetOrigin(e,&at.x);
    state->v.armorvalue+=1;if(type!=5)state->v.health=float((cursor+1)%mansion::SpawnCount);
    mansionAliveAt=-1;mansionHordeAliveAt=-1;
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
static bool mansionGameOverFrame(){
    if(!mansionMap())return false;auto state=mansionState();if(!valid(state)||state->v.impulse!=2)return false;
    mansionDoorFrame();if(globals->time>=mansionHudAt){mansionHud(state,mansionAlive());mansionHudAt=globals->time+.5f;}return true;
}
static void mansionActivate(){
    mansionHudAt=mansionTickAt=mansionVerifyAt=mansionMoveElapsed=mansionSpawnErrorAt=mansionDoorHudAt=0;
    mansionAliveAt=-1;mansionAliveCached=0;mansionHordeAliveAt=-1;mansionHordeAliveCached=0;
    mansionCultists.clear();mansionDoorStrikes.clear();mansionSense.assign(globals->maxEntities,MansionSense{});mansionRouteIntents.assign(globals->maxEntities,MansionRouteIntent{});
    mansionController={};for(auto& door:mansionDoors)door={};for(auto& target:mansionUpperTargets)target=0;
    mansionScientist={};mansionScientistWaveSpawned=0;mansionInvaded=false;mansionScientistAppearAt=0;mansionScientistKnockAt=0;mansionScientistVoiceAt=0;mansionScientistThankUntil=0;mansionScientistEntryAt=0;mansionScientistScenePhase=0;mansionScientistRunningIn=false;mansionScientistSceneEndAt=0;mansionScientistVoiceLine=0;mansionScientistFemale=false;mansionScientistLastHealth=0;mansionScientistPainAt=0;mansionGroundAssigned=0;mansionScientistSceneCamera(false);mansionTacticalSenses.clear();mansionPlayerPursuits.clear();for(auto& r:mansionRaiders)r={};for(auto& t:mansionRaidHitAt)t=0;for(auto& a:mansionRaidAttacks)a={};
    mansionNavReset();mansionAssignCultistRoles();
    mansionLightsReset();
#ifdef CREMATION_ENABLE_VALIDATION
    mansionDoorStableSince=0;
    mansionDoorStateReported=false;
    mansionDoorTouchPlayer=nullptr;mansionDoorTouchStage=0;mansionDoorTouchSince=0;
    mansionRouteReport=0;char env[8]{};GetEnvironmentVariableA("CREMATION_MANSION_VALIDATE",env,sizeof env);mansionTest=!std::strcmp(env,"1");mansionRouteTest=!std::strcmp(env,"2")||!std::strcmp(env,"6");mansionDoorTest=!std::strcmp(env,"6");mansionDoorTouchTest=!std::strcmp(env,"7");
    mansionCZRouteTest=!std::strcmp(env,"4");mansionRouteTest=mansionRouteTest||mansionCZRouteTest;mansionJudgmentTest=!std::strcmp(env,"5");
    char lightEnv[8]{};GetEnvironmentVariableA("CREMATION_MANSION_LIGHT_TEST",lightEnv,sizeof lightEnv);mansionLightTest=!std::strcmp(lightEnv,"1");
#endif
    if(mansionMap())startupTrace("MansionActivate","TT=10 CT=40 sectors=N,E,S,W waves=40,40,80,81 CZ=scientist objective");
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
#ifdef CREMATION_ENABLE_VALIDATION
static void mansionRescueProbe(float dt){
    static int stage=0;static bool same=false,paths=false;static float hp=0,started=0,beforeAttack=0;auto state=mansionState();
    if(stage==3)return;
    if(stage==0){stage=1;state->v.frags=1;
        if(!mansionScientistSpawn(1)){startupTrace("MansionRescueProbe","FAIL spawn");stage=3;return;}
        auto sc=mansionScientist.entity;int serial=sc->serialnumber;hp=sc->v.health;sc->v.health-=7.f;
        auto door=mansionDoor(1);Vector initial=sc->v.origin;mansionScientistFrame(.05f);
        bool waits=length(sc->v.origin-initial)<.1f&&!mansionScientist.rescued&&sc->v.sequence==13;
        Vector oldAngles=door->v.angles;door->v.angles.y=45.f;mansionScientistFrame(.05f);
        waits=waits&&length(sc->v.origin-initial)<.1f&&mansionScientistEntryAt>globals->time;
        mansionScientistEntryAt=globals->time;mansionScientistFrame(.05f);door->v.angles=oldAngles;
        bool thanked=!mansionScientist.rescued&&sc->v.origin.x<0&&sc->v.origin.z<100&&mansionScientistThankUntil>globals->time;
        mansionScientistThankUntil=globals->time;mansionScientistFrame(.05f);
        Vector upstairs=sc->v.origin;mansionScientistFrame(.05f);
        same=waits&&thanked&&mansionScientist.entity==sc&&sc->serialnumber==serial&&sc->v.health==hp-7.f&&sc->v.origin.z>180&&length(sc->v.origin-upstairs)<.1f&&sc->v.sequence==13&&sc->v.movetype==MOVETYPE_NONE;
        for(int i=0;i<7;++i)mansionEmit(state);
        mansionDoorHealth(1);state->v.vuser1.x=0;mansionEventFrame();mansionEventFrame();
        sc->v.takedamage=DAMAGE_NO;started=globals->time;
    }
    auto sc=mansionScientist.entity;inMansionMove=true;mansionNavBeginTick();
    int arrived=0;for(auto& r:mansionRaiders)if(valid(r.entity)){mansionEventMove(r.entity,std::min(.1f,dt));if(length(r.entity->v.origin-sc->v.origin)<80.f)++arrived;}
    mansionNavFinishTick();inMansionMove=false;
    if(stage==1){
        if(arrived<2&&globals->time-started<35.f)return;
        paths=arrived==2;stage=2;started=globals->time;beforeAttack=sc->v.health;sc->v.takedamage=DAMAGE_AIM;
    }
    bool nativeHit=sc->v.health<beforeAttack;
    if(!nativeHit&&globals->time-started<10.f)return;
    for(auto& r:mansionRaiders){char at[120];std::snprintf(at,sizeof at,"raider at %.0f,%.0f,%.0f",r.entity->v.origin.x,r.entity->v.origin.y,r.entity->v.origin.z);startupTrace("MansionRescueRoute",at);}
    stage=3;sc->v.takedamage=DAMAGE_AIM;
    auto table=*reinterpret_cast<void***>(sc->pvPrivateData);reinterpret_cast<TakeDamageFn>(table[ravenholm_abi::slot::TakeDamage])(sc->pvPrivateData,&mansionRaiders[0].entity->v,&mansionRaiders[0].entity->v,10000.f,1<<2);
    mansionEventFrame();int raiders=0;for(auto& r:mansionRaiders)if(valid(r.entity))++raiders;
    char detail[220];std::snprintf(detail,sizeof detail,"%s native_private=%d health10x=%.0f same_entity=%d raiders=%d ground=%d defeat=%d paths=%d native_hit=%d",same&&raiders==2&&paths&&nativeHit&&state->v.impulse==2?"PASS":"FAIL",sc->pvPrivateData?1:0,hp,int(same),raiders,mansionGroundAssigned,int(state->v.impulse==2),paths?2:arrived,int(nativeHit));startupTrace("MansionRescueProbe",detail);
}

#include "mansion_tactical_validation.inc"
#endif
static void mansionFrame(float dt){
    if(!mansionMap())return;auto state=mansionState();if(!valid(state))return;mansionEventFrame();mansionDoorFrame();if(!mansionInvaded&&state->v.impulse!=2)mansionLightsFrame(dt);
#ifdef CREMATION_ENABLE_VALIDATION
    char tacticsProbe[8]{};GetEnvironmentVariableA("CREMATION_MANSION_TACTICS",tacticsProbe,sizeof tacticsProbe);if(tacticsProbe[0]=='1'){mansionTacticalProbe(std::min(.1f,dt));return;}
    char rescueProbe[8]{};GetEnvironmentVariableA("CREMATION_RESCUE_VALIDATE",rescueProbe,sizeof rescueProbe);if(rescueProbe[0]=='1'){mansionRescueProbe(dt);return;}
    if(mansionDoorTouchTest){mansionDoorTouchProbe();return;}
    if(mansionJudgmentTest){judgmentProbeFrame(dt);return;}
#endif
    if(state->v.impulse==2){if(globals->time>=mansionHudAt){mansionHud(state,mansionAlive());mansionHudAt=globals->time+.5f;}return;}
#ifdef CREMATION_ENABLE_VALIDATION
    if(mansionInventoryTest){
        if(!mansionInventoryChecked){mansionInventoryChecked=true;auto player=engine.pfnCreateFakeClient("InventoryProbe");
            if(valid(player)){char reject[128]{};if(raven.pfnClientConnect)raven.pfnClientConnect(player,"InventoryProbe","127.0.0.1",reject);
                raven.pfnClientPutInServer(player);startupInventoryGive(player);
                weaponRegistryProbe(player);
                char report[100];std::snprintf(report,sizeof report,"origin=%.0f,%.0f,%.0f weapons=%d",player->v.origin.x,player->v.origin.y,player->v.origin.z,player->v.weapons);startupTrace("InventoryProbeSpawn",report);
                auto drop=engine.pfnCreateNamedEntity(engine.pfnAllocString("weapon_rpg"));if(valid(drop)){drop->v.origin=player->v.origin+Vector(400,0,0);hookSpawn(drop);drop->v.owner=player;drop->v.effects|=EF_NODRAW;}
                mansionVerifyAt=globals->time+.5f;
            }
        }
        if(mansionVerifyAt>0&&globals->time>=mansionVerifyAt){int owned=0,ground=0;
            for(int i=1;i<globals->maxEntities;++i){auto e=engine.pfnPEntityOfEntIndex(i);if(!valid(e)||std::strncmp(text(e->v.classname),"weapon_",7))continue;if(valid(e->v.aiment)&&(e->v.aiment->v.flags&FL_CLIENT))++owned;else if(!mansionGrenadePickup(e))++ground;}
            char report[100];std::snprintf(report,sizeof report,"owned=%d ground=%d",owned,ground);startupTrace("InventoryProbeComplete",report);mansionVerifyAt=0;
        }return;
    }
#endif
    mansionCZRespawnFrame();
    int alive=mansionAlive();
    if(globals->time>=mansionHudAt){mansionHud(state,alive);mansionHudAt=globals->time+.5f;}
    mansionMoveElapsed+=dt;
    if(mansionMoveElapsed>=.05f){
        inMansionMove=true;
        mansionNavBeginTick();
        for(int i=1;i<globals->maxEntities;++i){auto e=engine.pfnPEntityOfEntIndex(i);if(mansionActor(e))mansionMove(e,std::min(mansionMoveElapsed,.1f));}
        mansionScientistFrame(std::min(mansionMoveElapsed,.1f));
        mansionNavFinishTick();
        judgmentFrame(std::min(mansionMoveElapsed,.1f));
        inMansionMove=false;
        mansionMoveElapsed=0;
    }
    if(globals->time<mansionTickAt)return;mansionTickAt=globals->time+.1f;
#ifdef CREMATION_ENABLE_VALIDATION
    if(mansionRouteTest){
        if(state->v.frags==0){state->v.frags=mansionCZRouteTest?4:3;state->v.armorvalue=0;state->v.health=0;state->v.max_health=0;}
        if(state->v.armorvalue<(mansionCZRouteTest?1:(mansionDoorTest?80:20)))mansionEmit(state);
        for(int i=1;i<globals->maxEntities;++i){auto e=engine.pfnPEntityOfEntIndex(i);if(!mansionActor(e)||!aliveEnemy(e)||tposeIs(e))continue;
            const char* target=text(e->v.target);bool waitFront=target&&!std::strcmp(target,"mansion_wait_door_1")&&length2d(e->v.origin-Vector(64,-256,56.1f))<140;
            bool waitSide=target&&!std::strcmp(target,"mansion_wait_door_2")&&length2d(e->v.origin-Vector(-328,-792,48.1f))<140;
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
    int wave=int(state->v.frags),total=mansion::total(wave),index=int(state->v.armorvalue);
    mansionScientistEnsure(wave);
    if(index<total){
        int type=mansion::kind(wave,index);
        bool isBombPlanter=(type==5);
#ifdef CREMATION_ENABLE_VALIDATION
        bool canSpawn=isBombPlanter||mansionTest||(mansionHordeAlive()<10);
#else
        bool canSpawn=isBombPlanter||(mansionHordeAlive()<10);
#endif
        if(globals->time>=state->v.dmgtime){
            if(canSpawn){
                bool emitted=mansionEmit(state);
#ifdef CREMATION_ENABLE_VALIDATION
                state->v.dmgtime=globals->time+(emitted?(mansionTest?.20f:.75f):.25f);
#else
                state->v.dmgtime=globals->time+(emitted?.75f:.25f);
#endif
            }else{
                state->v.dmgtime=globals->time+.25f;
            }
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
