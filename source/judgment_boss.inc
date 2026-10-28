// Judgment is emitted as one wave slot and consists of two linked, independently
// hittable GoldSrc monsters. Both bodies carry the same shared health pool.
#include "studio.h"
static constexpr const char* KuarlClass="monster_kuarl";
static constexpr const char* ZealClass="monster_zeal";
static constexpr const char* KuarlModel="models/cremation/hotd2/judgment/kuarl.mdl";
static constexpr const char* ZealModel="models/cremation/hotd2/judgment/zeal.mdl";
static constexpr float JudgmentHealth=1500.f,JudgmentFastHealth=1050.f,JudgmentSoloHealth=450.f;
static constexpr float KuarlHeight=280.f,KuarlMeleeRange=220.f;
static bool mansionRouteActive(edict_t* e);
static bool judgmentIs(edict_t* e){return valid(e)&&e->v.classname&&(!std::strcmp(text(e->v.classname),KuarlClass)||!std::strcmp(text(e->v.classname),ZealClass));}
static bool judgmentKuarl(edict_t* e){return judgmentIs(e)&&!std::strcmp(text(e->v.classname),KuarlClass);}
static bool judgmentZeal(edict_t* e){return judgmentIs(e)&&!std::strcmp(text(e->v.classname),ZealClass);}
static void judgmentThink(edict_t* e,float dt);
static void judgmentFrame(float dt);
static bool judgmentSpawnPair(const Vector& at,const Vector& angles,int wave,int index,int cursor);
static void judgmentDropKuarl(edict_t* kuarl);
static void judgmentKillZeal(edict_t* zeal);

struct JudgmentTable{void** base;std::array<void*,128> slots;void* trace;};
static std::vector<std::unique_ptr<JudgmentTable>> judgmentTables;
static void* judgmentBaseTrace=nullptr;
static edict_t* judgmentObject(void* object){auto vars=reinterpret_cast<entvars_t**>(object)[1];return vars?vars->pContainingEntity:nullptr;}
static edict_t* judgmentPartner(edict_t* e){auto p=e?e->v.owner:nullptr;return valid(p)&&p->serialnumber==e->v.iuser4&&judgmentIs(p)?p:nullptr;}
static void judgmentSetHp(edict_t* e,float hp){if(!valid(e))return;e->v.fuser4=hp;if(e->v.deadflag==DEAD_NO)e->v.health=hp;auto p=judgmentPartner(e);if(valid(p)){p->v.fuser4=hp;if(p->v.deadflag==DEAD_NO)p->v.health=hp;}}
static void judgmentAbsBox(edict_t* e){
    // The Gargantua hardcodes an 80x80x214 box; the Controller's small hull
    // excludes Zeal's animated wings. Broad-phase bounds must enclose the MDL.
    Vector lo=e->v.mins,hi=e->v.maxs;auto hdr=static_cast<studiohdr_t*>(engine.pfnGetModelPtr(e));
    if(hdr&&e->v.sequence>=0&&e->v.sequence<hdr->numseq){auto sequences=reinterpret_cast<mstudioseqdesc_t*>(reinterpret_cast<unsigned char*>(hdr)+hdr->seqindex);auto& seq=sequences[e->v.sequence];
        Vector forward,right,up;engine.pfnAngleVectors(&e->v.angles.x,&forward.x,&right.x,&up.x);
        for(int mask=0;mask<8;++mask){Vector p((mask&1)?seq.bbmax[0]:seq.bbmin[0],(mask&2)?seq.bbmax[1]:seq.bbmin[1],(mask&4)?seq.bbmax[2]:seq.bbmin[2]);p=forward*p.x-right*p.y+up*p.z;
            for(int axis=0;axis<3;++axis){lo[axis]=std::min(lo[axis],p[axis]);hi[axis]=std::max(hi[axis],p[axis]);}}
    }
    e->v.absmin=e->v.origin+lo-Vector(1,1,1);e->v.absmax=e->v.origin+hi+Vector(1,1,1);
}
static void judgmentAnimate(edict_t* e,int sequence,float frames,float duration,bool loop=false,bool restart=false){
    if(!valid(e))return;
    if(restart||e->v.sequence!=sequence){e->v.sequence=sequence;e->v.frame=0;e->v.animtime=globals->time;e->v.fuser1=globals->time;}
    float phase=duration>0?(globals->time-e->v.fuser1)/duration:0;
    if(loop)phase=phase-std::floor(phase);else phase=std::min(1.f,std::max(0.f,phase));
    e->v.frame=phase*255.f;e->v.framerate=1.f;e->v.animtime=globals->time;
    engine.pfnSetOrigin(e,&e->v.origin.x);
}
static void judgmentDropKuarl(edict_t* kuarl){
    if(!judgmentKuarl(kuarl)||kuarl->v.iuser1>=2)return;
    kuarl->v.iuser1=2;kuarl->v.health=0;kuarl->v.deadflag=DEAD_DYING;kuarl->v.takedamage=DAMAGE_NO;kuarl->v.solid=SOLID_NOT;
    kuarl->v.enemy=nullptr;kuarl->v.fuser2=globals->time+2.55f;kuarl->v.iuser2=7;kuarl->v.fuser1=globals->time;
    kuarl->v.sequence=7;kuarl->v.frame=0;kuarl->v.animtime=globals->time;kuarl->v.framerate=1;kuarl->v.nextthink=0;
    if(auto zeal=judgmentPartner(kuarl)){zeal->v.iuser1=1;zeal->v.fuser3=0;zeal->v.fuser2=globals->time+1.f;zeal->v.iuser2=9;zeal->v.fuser1=globals->time;zeal->v.sequence=9;zeal->v.frame=0;zeal->v.animtime=globals->time;}
    startupTrace("JudgmentPhase","shared HP reached 450; Kuarl falls and Zeal becomes solo boss");
}
static void judgmentKillZeal(edict_t* zeal){
    if(!judgmentZeal(zeal)||zeal->v.iuser1>=2)return;
    zeal->v.iuser1=2;zeal->v.health=0;zeal->v.deadflag=DEAD_DYING;zeal->v.takedamage=DAMAGE_NO;zeal->v.solid=SOLID_NOT;
    zeal->v.enemy=nullptr;zeal->v.iuser2=2;zeal->v.fuser1=globals->time;zeal->v.fuser2=globals->time+3.65f;
    zeal->v.sequence=2;zeal->v.frame=0;zeal->v.animtime=globals->time;zeal->v.framerate=1;zeal->v.nextthink=0;
    startupTrace("JudgmentPhase","Zeal defeated; boss pair cleared");
}
static int __fastcall judgmentDamage(void* object,void*,entvars_t* inflictor,entvars_t* attacker,float amount,int bits){
    auto e=judgmentObject(object);if(!judgmentIs(e)||e->v.takedamage==DAMAGE_NO||e->v.deadflag!=DEAD_NO||amount<=0)return 0;
    if(enemyCombatBlocked(object,inflictor,attacker))return 0;
    float hp=e->v.fuser4;
    // Per successful hit: armored Kuarl barely reacts to bullets; the exposed
    // weak point takes the original game's player-count-scaled damage.
    int players=0;for(int i=1;i<=globals->maxClients;++i){auto p=engine.pfnPEntityOfEntIndex(i);if(valid(p)&&(p->v.flags&FL_CLIENT)&&p->v.health>0&&p->v.deadflag==DEAD_NO)++players;}
    float applied=judgmentKuarl(e)?1.f:(players>1?25.f:30.f);
    hp=std::max(0.f,hp-applied);judgmentSetHp(e,hp);
    auto kuarl=judgmentKuarl(e)?e:judgmentPartner(e);
    auto zeal=judgmentZeal(e)?e:judgmentPartner(e);
    if(judgmentZeal(e)&&valid(kuarl)&&kuarl->v.iuser1==0){kuarl->v.pain_finished=globals->time+.45f;kuarl->v.iuser2=8;kuarl->v.iuser3=0;kuarl->v.fuser1=globals->time;}
    if(hp<=JudgmentSoloHealth&&valid(kuarl)&&kuarl->v.iuser1==0)judgmentDropKuarl(kuarl);
    if(hp<=0.f&&valid(zeal)&&zeal->v.iuser1<2)judgmentKillZeal(zeal);
    return 1;
}
static void __fastcall judgmentKilled(void* object,void*,entvars_t*,int){auto e=judgmentObject(object);if(judgmentKuarl(e))judgmentDropKuarl(e);else if(judgmentZeal(e))judgmentKillZeal(e);}
static void __fastcall judgmentTraceAttack(void* object,void*,entvars_t* attacker,float amount,Vector direction,TraceResult* tr,int bits){
    if(enemyCombatBlocked(object,attacker,attacker)||!tr||amount<=0||!judgmentBaseTrace)return;
    // Use the Controller's ordinary CBaseMonster TraceAttack for both bodies.
    // Gargantua's TraceAttack discards bullets before TakeDamage is reached.
    reinterpret_cast<void(__thiscall*)(void*,entvars_t*,float,Vector,TraceResult*,int)>(judgmentBaseTrace)(object,attacker,amount,direction,tr,bits);
}
static void judgmentInstall(edict_t* e){
    if(!valid(e)||!e->pvPrivateData)return;auto base=*reinterpret_cast<void***>(e->pvPrivateData);
    for(auto& t:judgmentTables){if(t->base==base||t->slots.data()==base){if(judgmentZeal(e))judgmentBaseTrace=t->trace;*reinterpret_cast<void***>(e->pvPrivateData)=t->slots.data();return;}}
    auto t=std::make_unique<JudgmentTable>();t->base=base;t->trace=base[ravenholm_abi::slot::TraceAttack];if(judgmentZeal(e))judgmentBaseTrace=t->trace;std::copy_n(base,128,t->slots.begin());
    t->slots[ravenholm_abi::slot::TraceAttack]=reinterpret_cast<void*>(&judgmentTraceAttack);
    t->slots[ravenholm_abi::slot::TakeDamage]=reinterpret_cast<void*>(&judgmentDamage);t->slots[ravenholm_abi::slot::Killed]=reinterpret_cast<void*>(&judgmentKilled);
    *reinterpret_cast<void***>(e->pvPrivateData)=t->slots.data();judgmentTables.push_back(std::move(t));
}
static float judgmentCurrentHealth(){
    for(int i=1;i<globals->maxEntities;++i){auto e=engine.pfnPEntityOfEntIndex(i);if(judgmentIs(e)&&aliveEnemy(e))return e->v.fuser4;}
    return 0;
}
static void judgmentSendHud(edict_t* player){
    if(!msgJudgmentHUD||!valid(player))return;float hp=judgmentCurrentHealth();
    engine.pfnMessageBegin(MSG_ONE,msgJudgmentHUD,nullptr,player);engine.pfnWriteByte(1);engine.pfnWriteByte(hp>0?1:0);
    engine.pfnWriteShort(int(std::ceil(hp)));engine.pfnWriteShort(int(JudgmentHealth));engine.pfnMessageEnd();
}
static void judgmentPrecache(){
    for(auto path:{KuarlModel,ZealModel})engine.pfnPrecacheModel(const_cast<char*>(path));
    for(auto path:{"zombie/claw_strike1.wav","zombie/claw_strike2.wav","zombie/claw_hit1.wav","zombie/claw_hit2.wav"})engine.pfnPrecacheSound(const_cast<char*>(path));
    // Native base classes perform their own model and sound registration here.
    for(auto cls:{"monster_gargantua","monster_alien_controller"}){auto e=engine.pfnCreateNamedEntity(engine.pfnAllocString(cls));if(!valid(e))continue;e->v.origin=Vector(1336,392,24);hookSpawn(e);engine.pfnRemoveEntity(e);}
}
static edict_t* judgmentPlayer(edict_t* e){edict_t* best=nullptr;float nearest=1.e9f;for(int i=1;i<=globals->maxClients;++i){auto p=engine.pfnPEntityOfEntIndex(i);if(!valid(p)||!(p->v.flags&FL_CLIENT)||p->v.health<=0||p->v.deadflag!=DEAD_NO)continue;float d=length(p->v.origin-e->v.origin);if(d<nearest){nearest=d;best=p;}}return best;}
static void judgmentFace(edict_t* e,const Vector& target){Vector d=target-e->v.origin;e->v.angles.y=std::atan2(d.y,d.x)*180.f/3.14159265f;}
static bool judgmentFlightTrace(edict_t* e,const Vector& from,const Vector& to,TraceResult& tr){
    tr=TraceResult{};engine.pfnTraceMonsterHull(e,&from.x,&to.x,1,e,&tr);
    return !tr.fStartSolid&&!tr.fAllSolid&&tr.flFraction>=.999f;
}
static bool judgmentFlightOpen(edict_t* e,const Vector& at){TraceResult tr{};return judgmentFlightTrace(e,at,at,tr);}
static bool judgmentFlightRecover(edict_t* e){
    if(judgmentFlightOpen(e,e->v.origin))return true;
    // Old saves and initial placements may already overlap a brush. Recover
    // to the nearest free flight volume once; normal travel is always swept.
    for(float distance:{8.f,16.f,32.f,64.f,96.f})for(Vector axis:{Vector(0,0,1),Vector(1,0,0),Vector(-1,0,0),Vector(0,1,0),Vector(0,-1,0),Vector(0,0,-1)}){
        Vector at=e->v.origin+axis*distance;if(judgmentFlightOpen(e,at)){engine.pfnSetOrigin(e,&at.x);e->v.vuser1=Vector(0,0,0);startupTrace("JudgmentFlight","recovered overlapping flight volume");return true;}}
    return false;
}
static Vector judgmentFlightGoal(edict_t* e,const Vector& wanted){
    if(judgmentFlightOpen(e,wanted))return wanted;
    Vector best=e->v.origin;float score=1.e9f;
    for(float radius:{64.f,128.f})for(float z:{0.f,80.f,-80.f})for(int angle=0;angle<8;++angle){
        float a=angle*3.14159265f*.25f;Vector candidate=wanted+Vector(std::cos(a)*radius,std::sin(a)*radius,z);TraceResult tr{};
        if(!judgmentFlightOpen(e,candidate)||!judgmentFlightTrace(e,e->v.origin,candidate,tr))continue;
        float cost=length(candidate-wanted)+length(candidate-e->v.origin)*.1f;if(cost<score){score=cost;best=candidate;}}
    return best;
}
static bool judgmentFlightDetour(edict_t* e,const Vector& goal){
    TraceResult tr{};
    // Three checked segments: rise, cross above the obstacle, then descend.
    // Keep the two intermediate points until reached so wall avoidance cannot
    // oscillate between left/right directions without making progress.
    for(float lift:{64.f,128.f,192.f,256.f,384.f,512.f}){Vector first=e->v.origin+Vector(0,0,lift),second=goal+Vector(0,0,lift);
        if(judgmentFlightTrace(e,e->v.origin,first,tr)&&judgmentFlightTrace(e,first,second,tr)&&judgmentFlightTrace(e,second,goal,tr)){
            e->v.vuser2=first;e->v.vuser3=second;e->v.fuser3=1;e->v.pain_finished=globals->time+12.f;return true;}}
    return false;
}
static void judgmentFly(edict_t* e,const Vector& target,float speed,float dt){
    if(!judgmentFlightRecover(e))return;Vector goal=judgmentFlightGoal(e,target);
    if(e->v.fuser3>0){if(globals->time>=e->v.pain_finished)e->v.fuser3=0;
        else{Vector waypoint=e->v.fuser3==1?e->v.vuser2:e->v.vuser3;if(length(waypoint-e->v.origin)<12.f)e->v.fuser3=e->v.fuser3==1?2.f:0.f;
            if(e->v.fuser3>0)goal=e->v.fuser3==1?e->v.vuser2:e->v.vuser3;}}
    Vector delta=goal-e->v.origin;float d=length(delta);if(d<1.f)return;
    Vector desired=normal(delta),direction=desired;float ahead=std::min(d,std::max(48.f,std::min(112.f,speed*.4f)));TraceResult tr{};
    if(!judgmentFlightTrace(e,e->v.origin,e->v.origin+desired*ahead,tr)){
        if(e->v.fuser3==0&&judgmentFlightDetour(e,goal)){goal=e->v.vuser2;delta=goal-e->v.origin;d=length(delta);desired=direction=normal(delta);}
        else{
        float best=-1.e9f;Vector side=normal(Vector(-desired.y,desired.x,0));if(length(side)<.1f)side=Vector(1,0,0);
        for(float turn:{0.f,.65f,-.65f,1.3f,-1.3f,2.f,-2.f,3.14f})for(float lift:{0.f,.7f,-.7f}){
            Vector candidate=normal(desired*std::cos(turn)+side*std::sin(turn)+Vector(0,0,lift));TraceResult probe{};
            judgmentFlightTrace(e,e->v.origin,e->v.origin+candidate*ahead,probe);if(probe.fStartSolid||probe.fAllSolid||probe.flFraction<.4f)continue;
            float score=probe.flFraction*3.f+dot(candidate,desired)+dot(candidate,e->v.vuser1)*.45f;
            if(score>best){best=score;direction=candidate;}}
        if(best<-1.e8f)return;
        }
    }
    Vector to=e->v.origin+direction*std::min(d,speed*dt);judgmentFlightTrace(e,e->v.origin,to,tr);
    if(!tr.fStartSolid&&!tr.fAllSolid&&tr.flFraction>0){float fraction=tr.flFraction>=1.f?1.f:std::max(0.f,tr.flFraction-.02f);Vector at=e->v.origin+(to-e->v.origin)*fraction;
        if(judgmentFlightOpen(e,at)){engine.pfnSetOrigin(e,&at.x);e->v.vuser1=direction;}}
    judgmentFace(e,goal);
}
static void judgmentPlayerHit(edict_t* attacker,edict_t* player,float damageAmount){
    if(!valid(attacker)||!valid(player)||!player->pvPrivateData)return;Vector from=attacker->v.origin+attacker->v.view_ofs,to=player->v.origin+player->v.view_ofs;TraceResult tr{};engine.pfnTraceLine(&from.x,&to.x,0,attacker,&tr);
    if(tr.flFraction<1.f&&tr.pHit!=player)return;auto table=*reinterpret_cast<void***>(player->pvPrivateData);
    reinterpret_cast<TakeDamageFn>(table[ravenholm_abi::slot::TakeDamage])(player->pvPrivateData,&attacker->v,&attacker->v,damageAmount,1<<2);
    sound(attacker,"zombie/claw_hit1.wav");
}
static void judgmentSoloThink(edict_t* zeal,float dt){
    if(!judgmentZeal(zeal))return;
    if(zeal->v.iuser1>=2){float phase=std::min(1.f,std::max(0.f,(globals->time-zeal->v.fuser1)/3.65f));zeal->v.frame=phase*255.f;if(phase>=1.f)zeal->v.deadflag=DEAD_DEAD;return;}
    auto player=judgmentPlayer(zeal);if(!valid(player))return;float t=globals->time;
    int pattern=int(std::floor(t/2.2f))%4;float speed=(int(std::floor(t/3.4f))%2)?270.f:155.f;
    Vector forward=normal(player->v.origin-zeal->v.origin),side(-forward.y,forward.x,0),aim=player->v.origin;
    if(pattern==1)aim=aim+side*110.f;else if(pattern==2)aim=aim-side*110.f;else if(pattern==3)aim=aim+side*(std::sin(t*3.f)*85.f);
    Vector goal=aim+Vector(0,0,36.f+std::sin(t*4.f)*20.f);judgmentFly(zeal,goal,speed,dt);
    judgmentAnimate(zeal,4,68,2.27f,true);
    if(length(zeal->v.origin-player->v.origin)<68.f&&t>=zeal->v.fuser2){judgmentPlayerHit(zeal,player,22.f);zeal->v.fuser2=t+1.f;}
}
static bool judgmentSpawnPair(const Vector& at,const Vector& angles,int wave,int index,int cursor){
    auto kuarl=engine.pfnCreateNamedEntity(engine.pfnAllocString("monster_gargantua"));if(!valid(kuarl))return false;
    kuarl->v.origin=at;kuarl->v.angles=angles;int kresult=hookSpawn(kuarl);if(!valid(kuarl)||!kuarl->pvPrivateData||kresult<0){if(valid(kuarl))engine.pfnRemoveEntity(kuarl);return false;}
    kuarl->v.classname=engine.pfnAllocString(KuarlClass);engine.pfnSetModel(kuarl,KuarlModel);
    Vector kmins(-60,-60,0),kmaxs(60,60,KuarlHeight);engine.pfnSetSize(kuarl,&kmins.x,&kmaxs.x);kuarl->v.movetype=MOVETYPE_STEP;kuarl->v.solid=SOLID_SLIDEBOX;kuarl->v.flags&=~(FL_FLY|FL_SWIM|FL_ONGROUND);kuarl->v.flags|=FL_MONSTER;kuarl->v.velocity=Vector(0,0,0);
    kuarl->v.health=kuarl->v.max_health=JudgmentHealth;kuarl->v.takedamage=DAMAGE_AIM;kuarl->v.deadflag=DEAD_NO;kuarl->v.view_ofs=Vector(0,0,230);kuarl->v.nextthink=0;kuarl->v.framerate=1;kuarl->v.iuser1=0;kuarl->v.iuser2=10;kuarl->v.iuser3=0;kuarl->v.fuser1=globals->time;kuarl->v.fuser2=globals->time+1.f;kuarl->v.fuser4=JudgmentHealth;kuarl->v.enemy=nullptr;
    auto zeal=engine.pfnCreateNamedEntity(engine.pfnAllocString("monster_alien_controller"));if(!valid(zeal)){engine.pfnRemoveEntity(kuarl);return false;}
    Vector zAt=at+Vector(0,0,KuarlHeight+20);zeal->v.origin=zAt;zeal->v.angles=angles;int zresult=hookSpawn(zeal);if(!valid(zeal)||!zeal->pvPrivateData||zresult<0){if(valid(zeal))engine.pfnRemoveEntity(zeal);engine.pfnRemoveEntity(kuarl);return false;}
    zeal->v.classname=engine.pfnAllocString(ZealClass);engine.pfnSetModel(zeal,ZealModel);
    Vector zmins(-48,-48,-8),zmaxs(48,48,68);engine.pfnSetSize(zeal,&zmins.x,&zmaxs.x);zeal->v.movetype=MOVETYPE_FLY;zeal->v.solid=SOLID_SLIDEBOX;zeal->v.flags&=~FL_ONGROUND;zeal->v.flags|=FL_MONSTER|FL_FLY;zeal->v.velocity=Vector(0,0,0);
    zeal->v.health=zeal->v.max_health=JudgmentHealth;zeal->v.takedamage=DAMAGE_AIM;zeal->v.deadflag=DEAD_NO;zeal->v.view_ofs=Vector(0,0,0);zeal->v.nextthink=0;zeal->v.framerate=1;zeal->v.iuser1=0;zeal->v.iuser2=9;zeal->v.iuser3=0;zeal->v.fuser1=globals->time;zeal->v.fuser2=globals->time+1.f;zeal->v.fuser3=0;zeal->v.fuser4=JudgmentHealth;zeal->v.enemy=nullptr;
    kuarl->v.owner=zeal;kuarl->v.iuser4=zeal->serialnumber;zeal->v.owner=kuarl;zeal->v.iuser4=kuarl->serialnumber;
    char marker[64];std::snprintf(marker,sizeof marker,"mansion_enemy_judgment_kuarl_%d_%02d",wave,index);kuarl->v.targetname=engine.pfnAllocString(marker);
    std::snprintf(marker,sizeof marker,"mansion_enemy_judgment_zeal_%d_%02d",wave,index);zeal->v.targetname=engine.pfnAllocString(marker);
    std::snprintf(marker,sizeof marker,"mansion_route_%02d_00",cursor);kuarl->v.target=engine.pfnAllocString(marker);zeal->v.target=0;
    kuarl->v.health=kuarl->v.max_health=zeal->v.health=zeal->v.max_health=JudgmentHealth;judgmentSetHp(kuarl,JudgmentHealth);
    judgmentInstall(kuarl);judgmentInstall(zeal);enemyCombatInstall(kuarl);enemyCombatInstall(zeal);
    engine.pfnSetOrigin(kuarl,&at.x);engine.pfnSetOrigin(zeal,&zAt.x);
    engine.pfnDropToFloor(kuarl);
    char report[180];std::snprintf(report,sizeof report,"wave=%d index=%d shared_hp=1500 kuarl=%d zeal=%d models=custom",wave,index,engine.pfnIndexOfEdict(kuarl),engine.pfnIndexOfEdict(zeal));startupTrace("JudgmentSpawn",report);return true;
}
static void judgmentThink(edict_t* e,float dt){
    if(!judgmentIs(e))return;
    if(e->v.iuser1>=2){float duration=judgmentKuarl(e)?2.55f:3.65f;float phase=std::min(1.f,std::max(0.f,(globals->time-e->v.fuser1)/duration));e->v.frame=phase*255.f;if(phase>=1.f)e->v.deadflag=DEAD_DEAD;e->v.nextthink=0;
        if(judgmentKuarl(e)){auto zeal=judgmentPartner(e);if(valid(zeal)&&zeal->v.iuser1==1)judgmentSoloThink(zeal,dt);}return;}
    if(!judgmentKuarl(e))return;auto zeal=judgmentPartner(e);auto player=judgmentPlayer(e);if(!valid(zeal)||!valid(player))return;
    const bool solo=zeal->v.iuser1==1;float hp=e->v.fuser4;float phaseBoost=hp<=JudgmentFastHealth?1.45f:1.f;Vector toPlayer=player->v.origin-e->v.origin;float dist=length2d(toPlayer);judgmentFace(e,player->v.origin);
    if(e->v.iuser1==2)return;
    if(mansionRouteActive(e)){judgmentAnimate(e,16,153,5.1f,true);}
    else if(globals->time<e->v.pain_finished){judgmentAnimate(e,8,53,1.76f,false);e->v.iuser3=0;}
    else if(dist>KuarlMeleeRange){e->v.iuser3=0;float speed=55.f*phaseBoost;float step=std::min(dist-KuarlMeleeRange+8.f,speed*dt);if(step>0&&!hookWalkMove(e,e->v.angles.y,step,0))for(float turn:{30.f,-30.f,60.f,-60.f,90.f,-90.f})if(hookWalkMove(e,e->v.angles.y+turn,step,0)){e->v.angles.y+=turn;break;}judgmentAnimate(e,16,153,5.1f,true);}
    else if(e->v.iuser3==0&&globals->time>=e->v.fuser2){e->v.iuser2=engine.pfnRandomLong(0,3);e->v.iuser3=1;e->v.fuser1=globals->time;e->v.fuser2=globals->time+1.45f;int seq=int(e->v.iuser2);float counts[]={109,87,111,91};judgmentAnimate(e,seq,counts[seq],counts[seq]/30.f,false,true);sound(e,"zombie/claw_strike1.wav");}
    else if(e->v.iuser3>0){int seq=int(e->v.iuser2);float counts[]={109,87,111,91};judgmentAnimate(e,seq,counts[seq],counts[seq]/30.f,false);
        if(e->v.iuser3==1&&globals->time>=e->v.fuser1+.62f){if(dist<KuarlMeleeRange+40.f)judgmentPlayerHit(e,player,28.f);e->v.iuser3=2;}
        if(globals->time>=e->v.fuser2){e->v.iuser3=0;e->v.fuser2=globals->time+.65f;judgmentAnimate(e,10,60,2.f,true,true);}}
    else judgmentAnimate(e,10,60,2.f,true);

    // Zeal occupies Kuarl's far side, with short alternating sweeps and an
    // orbiting bob. Once alone, the four lateral/diagonal rush families drive
    // independent passes at the player rather than remaining a target dummy.
    float t=globals->time;float lateral=std::sin(t*(hp<=JudgmentFastHealth?2.7f:1.35f))* (solo?120.f:55.f);
    Vector forward=normal(Vector(toPlayer.x,toPlayer.y,0));Vector side(-forward.y,forward.x,0);Vector goal;
    if(!solo){float orbit=t*(hp<=JudgmentFastHealth?.95f:.65f);goal=e->v.origin-forward*45.f+forward*(std::cos(orbit)*130.f)+side*(std::sin(orbit)*130.f)+Vector(0,0,KuarlHeight*.90f+std::sin(t*1.8f)*24.f);}
    else {int pattern=int(std::floor(t/2.2f))%4;float speed=(int(std::floor(t/3.4f))%2)?270.f:155.f;Vector aim=player->v.origin;
        if(pattern==1)aim=aim+side*110.f;else if(pattern==2)aim=aim-side*110.f;else if(pattern==3)aim=aim+side*(std::sin(t*3.f)*85.f);
        goal=aim+Vector(0,0,36.f+std::sin(t*4.f)*20.f);judgmentFly(zeal,goal,speed*phaseBoost,dt);
        if(length(zeal->v.origin-player->v.origin)<68.f&&t>=zeal->v.fuser2){judgmentPlayerHit(zeal,player,22.f);zeal->v.fuser2=t+1.f;}
    }
    if(!solo){judgmentFly(zeal,goal,((int(std::floor(t/3.6f))%2)?215.f:105.f)*phaseBoost,dt);
        if(zeal->v.iuser3>0){judgmentAnimate(zeal,4,68,2.27f,false);if(zeal->v.iuser3==1&&t>=zeal->v.fuser1+1.f){if(length(zeal->v.origin-player->v.origin)<70.f)judgmentPlayerHit(zeal,player,18.f);zeal->v.iuser3=2;}
            if(t>=zeal->v.fuser1+2.27f){zeal->v.iuser3=0;zeal->v.fuser2=t+.7f;judgmentAnimate(zeal,9,68,2.27f,true,true);}}
        else {judgmentAnimate(zeal,9,68,2.27f,true);if(length(zeal->v.origin-player->v.origin)<65.f&&t>=zeal->v.fuser2){zeal->v.iuser3=1;zeal->v.fuser1=t;judgmentAnimate(zeal,4,68,2.27f,false,true);sound(zeal,"zombie/claw_strike2.wav");}}
    }
}
static void judgmentFrame(float dt){
    for(int i=1;i<globals->maxEntities;++i){auto e=engine.pfnPEntityOfEntIndex(i);if(judgmentKuarl(e)){judgmentThink(e,std::min(dt,.1f));auto p=judgmentPartner(e);if(valid(p)&&p->v.iuser1>=2)judgmentThink(p,dt);}}
}
