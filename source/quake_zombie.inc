// Independent Quake zombie. Reference behavior: id Software zombie.qc (1996/2022).
// Native CZombie supplies engine allocation/save ABI only; its Think and damage
// never control this entity. Per-class cloned tables leave every native NPC alone.
// State uses saved entvars: impulse, button, dmgtime, pain_finished, teleport_time,
// radsuit_finished, dmg_take, dmg_save and enemy. No unsaved gameplay side tables.
static constexpr const char* QZClass="monster_quake_zombie";
static constexpr const char* QZGibClass="quake_zombie_gib";
static constexpr const char* QZModel="models/cremation/quake/zombie.mdl";
static constexpr const char* QZChunk="models/cremation/quake/zom_gib.mdl";
static constexpr float QZScale=2.f,QZHeight=104.f,QZEye=80.f;
static bool qzDiagnostic=false;
enum QZMode {QZIdle,QZChase,QZAttack,QZPain,QZDown,QZRise,QZDead};
static const int qzFrames[]={16,20,19,13,12,6,28,18,13,30,14,12,11};
static const char* qzSounds[]={"cremation/quake/zombie/z_idle.wav","cremation/quake/zombie/z_idle1.wav",
    "cremation/quake/zombie/z_shot1.wav","cremation/quake/zombie/z_gib.wav","cremation/quake/zombie/z_pain.wav",
    "cremation/quake/zombie/z_pain1.wav","cremation/quake/zombie/z_fall.wav","cremation/quake/zombie/z_miss.wav",
    "cremation/quake/zombie/z_hit.wav","cremation/quake/zombie/idle_w2.wav"};
static bool qzIs(edict_t* e){return valid(e)&&!std::strcmp(text(e->v.classname),QZClass);}
static bool qzGib(edict_t* e){return valid(e)&&!std::strcmp(text(e->v.classname),QZGibClass);}
static void qzSound(edict_t* e,int i){engine.pfnEmitSound(e,i==6?CHAN_BODY:CHAN_VOICE,qzSounds[i],1,ATTN_NORM,0,100);}
static void qzPrecache(){
    for(auto p:{QZModel,QZChunk,"models/cremation/quake/h_zombie.mdl","models/cremation/quake/gib_1.mdl",
                "models/cremation/quake/gib_2.mdl","models/cremation/quake/gib_3.mdl"})engine.pfnPrecacheModel(const_cast<char*>(p));
    for(auto p:qzSounds)engine.pfnPrecacheSound(const_cast<char*>(p));
}
static void qzSequence(edict_t* e,int seq,int mode){
    e->v.sequence=seq;e->v.frame=0;e->v.button=0;e->v.animtime=globals->time;
    e->v.framerate=1;e->v.impulse=mode;e->v.dmgtime=globals->time;
}
static bool qzTarget(edict_t* e){
    if(!valid(e)||e->v.health<=0||e->v.deadflag!=DEAD_NO)return false;
    if(e->v.flags&FL_CLIENT)return true;
    return !std::strcmp(text(e->v.classname),"monster_minion")||!std::strcmp(text(e->v.classname),"cremation_sentry");
}
static bool qzVisible(edict_t* e,edict_t* target){
    TraceResult tr{};Vector a=e->v.origin+Vector(0,0,QZEye),b=target->v.origin+target->v.view_ofs;
    if(!(target->v.flags&FL_CLIENT))b=center(target);
    engine.pfnTraceLine(&a.x,&b.x,1,e,&tr);return !tr.fStartSolid&&(tr.flFraction==1||tr.pHit==target);
}
static edict_t* qzEnemy(edict_t* e){
    auto target=e->v.enemy;
    if(qzTarget(target)&&target->serialnumber==int(e->v.dmg_save)&&length(target->v.origin-e->v.origin)<=MansionZombieNoticeRange)return target;
    e->v.enemy=nullptr;float closest=MansionZombieNoticeRange;
    for(int i=1;i<globals->maxEntities;++i){auto t=engine.pfnPEntityOfEntIndex(i);
        if(!qzTarget(t))continue;float d=length(t->v.origin-e->v.origin);
        if(d<closest&&qzVisible(e,t)){closest=d;e->v.enemy=t;e->v.dmg_save=float(t->serialnumber);}}
    return e->v.enemy;
}
static void qzFace(edict_t* e,const Vector& target){Vector d=target-e->v.origin;e->v.angles.y=std::atan2(d.y,d.x)*180.f/3.14159265f;}
static edict_t* qzPiece(edict_t* owner,const char* model,const Vector& at,const Vector& velocity,bool missile){
    auto e=engine.pfnCreateNamedEntity(engine.pfnAllocString(QZGibClass));if(!valid(e))return nullptr;
    // info_target is just an allocated base object; no native Spawn/Think needed.
    e->v.classname=engine.pfnAllocString(QZGibClass);engine.pfnSetModel(e,model);
    Vector zero(0,0,0);engine.pfnSetSize(e,&zero.x,&zero.x);engine.pfnSetOrigin(e,&at.x);
    e->v.movetype=MOVETYPE_BOUNCE;e->v.solid=missile?SOLID_BBOX:SOLID_NOT;e->v.owner=owner;e->v.dmg=missile?10.f:0.f;
    e->v.dmg_save=valid(owner)?float(owner->serialnumber):0;e->v.velocity=velocity;e->v.gravity=1;
    e->v.avelocity=Vector(3000,1000,2000);e->v.dmgtime=globals->time+(missile?2.5f:12.f);
    e->v.nextthink=e->v.dmgtime;return e;
}
static void qzThrow(edict_t* e,edict_t* target){
    if(!qzTarget(target))return;
    float yaw=e->v.angles.y*3.14159265f/180;Vector forward(std::cos(yaw),std::sin(yaw),0),right(std::sin(yaw),-std::cos(yaw),0);
    Vector offset=e->v.sequence==3?Vector(-10,-22,30):(e->v.sequence==10?Vector(-10,-24,29):Vector(-12,-19,29));
    Vector at=e->v.origin+(forward*offset.x+right*offset.y+Vector(0,0,offset.z))*QZScale;
    if(engine.pfnGetAttachment){Vector angles;engine.pfnGetAttachment(e,0,&at.x,&angles.x);}
    TraceResult tr{};Vector from=e->v.origin+Vector(0,0,QZEye);engine.pfnTraceLine(&from.x,&at.x,0,e,&tr);
    if(tr.fStartSolid||tr.flFraction<1)return;
    // With a tall launch point, fixed +200 Z sails over nearby humans. Keep
    // Quake's 600 horizontal speed and solve gravity for the target's height.
    Vector delta=center(target)-at;float travel=std::max(.08f,length2d(delta)/600.f);
    Vector velocity=normal(Vector(delta.x,delta.y,0))*600;
    float gravity=engine.pfnCVarGetFloat?engine.pfnCVarGetFloat("sv_gravity"):800.f;if(gravity<=0)gravity=800.f;
    velocity.z=std::max(-800.f,std::min(800.f,delta.z/travel+.5f*gravity*travel));
    if(qzDiagnostic){char detail[256];std::snprintf(detail,sizeof detail,"from=%.0f,%.0f,%.0f to=%.0f,%.0f,%.0f velocity=%.0f,%.0f,%.0f",at.x,at.y,at.z,center(target).x,center(target).y,center(target).z,velocity.x,velocity.y,velocity.z);startupTrace("QuakeZombieProbe",detail);}
    if(qzPiece(e,QZChunk,at,velocity,true)){qzSound(e,2);startupTrace("QuakeZombie","throw");}
}
static void qzDie(edict_t* e,entvars_t* attacker,bool gib=true){
    if(e->v.impulse==QZDead)return;
    Vector impulse=e->v.velocity;qzSequence(e,12,QZDead);e->v.health=0;e->v.deadflag=DEAD_DEAD;
    e->v.takedamage=DAMAGE_NO;e->v.solid=SOLID_NOT;e->v.velocity=Vector(0,0,0);
    if(gib){e->v.effects|=EF_NODRAW;qzSound(e,3);humanGibSpawn(e->v.origin+Vector(0,0,QZHeight*.5f),impulse);}
    if(attacker&&(attacker->flags&FL_CLIENT))attacker->frags+=1;
    e->v.nextthink=globals->time+.1f;startupTrace("QuakeZombie",gib?"shared anatomical gibs":"non-gib energy death");
}
static edict_t* qzObject(void* object){auto vars=reinterpret_cast<entvars_t**>(object)[1];return vars?vars->pContainingEntity:nullptr;}
static int __fastcall qzClassify(void*,void*){return 7;}
static int __fastcall qzBloodColor(void*,void*){return 247;}
static void __fastcall qzKilled(void* object,void*,entvars_t* attacker,int gib){auto e=qzObject(object);if(qzIs(e))qzDie(e,attacker,gib!=1);}
static int __fastcall qzDamage(void* object,void*,entvars_t* inflictor,entvars_t* attacker,float amount,int bits){
    if(enemyCombatBlocked(object,inflictor,attacker))return 0;
    auto e=qzObject(object);if(!qzIs(e)||e->v.impulse==QZDead||amount<=0)return 0;
    // Cremation energy/thermal attacks must accumulate like other monsters.
    // The Quake single-frame regeneration rule remains for ordinary bullets.
    bool plasma=inflictor&&inflictor->classname&&!std::strcmp(text(inflictor->classname),"cremation_nail");
    if(plasma||(bits&((1<<10)|(1<<8)|(1<<3)))){
        e->v.armorvalue=std::min(60.f,e->v.armorvalue+amount);e->v.health=60.f-e->v.armorvalue;
        if(e->v.health<=0)qzDie(e,attacker,(bits&(1<<12))==0);
        return 1;
    }
    // Quake resets health after sublethal hits, but damage in ONE server frame
    // combines (including shotgun pellets and simultaneous projectiles).
    if(e->v.radsuit_finished!=globals->time)e->v.dmg_take=0;
    e->v.radsuit_finished=globals->time;e->v.dmg_take+=amount;
    if(e->v.dmg_take>=60.f-e->v.armorvalue){qzDie(e,attacker,(bits&(1<<12))==0);return 1;}e->v.health=60.f-e->v.armorvalue;
    if(attacker){auto a=attacker->pContainingEntity;if(qzTarget(a)){e->v.enemy=a;e->v.dmg_save=float(a->serialnumber);}}
    if(amount<9||e->v.impulse==QZDown||e->v.impulse==QZRise)return 1;
    if(amount>=25||(e->v.impulse!=QZPain&&e->v.pain_finished>globals->time)){
        qzSequence(e,9,QZDown);qzSound(e,4);startupTrace("QuakeZombie","knockdown");return 1;}
    if(e->v.impulse==QZPain){e->v.pain_finished=globals->time+3;return 1;}
    int seqs[]={4,6,7,8};int pick=engine.pfnRandomLong(0,3);qzSequence(e,seqs[pick],QZPain);qzSound(e,pick?5:4);return 1;
}
struct QZTable {void** original;std::array<void*,128> slots;};
static std::vector<std::unique_ptr<QZTable>> qzTables;
static void qzInstall(edict_t* e){
    if(!e->pvPrivateData)return;auto table=*reinterpret_cast<void***>(e->pvPrivateData);
    for(auto& t:qzTables){if(table==t->slots.data())return;if(table==t->original){*reinterpret_cast<void***>(e->pvPrivateData)=t->slots.data();return;}}
    auto t=std::make_unique<QZTable>();t->original=table;std::copy_n(table,t->slots.size(),t->slots.begin());
    t->slots[ravenholm_abi::slot::Classify]=reinterpret_cast<void*>(&qzClassify);t->slots[ravenholm_abi::slot::TakeDamage]=reinterpret_cast<void*>(&qzDamage);t->slots[ravenholm_abi::slot::Killed]=reinterpret_cast<void*>(&qzKilled);
    t->slots[14]=reinterpret_cast<void*>(&qzBloodColor);
    *reinterpret_cast<void***>(e->pvPrivateData)=t->slots.data();qzTables.push_back(std::move(t));
}
static void qzSpawn(edict_t* e){
    e->v.classname=engine.pfnAllocString(QZClass);qzInstall(e);engine.pfnSetModel(e,QZModel);
    Vector mins(-16,-16,0),maxs(16,16,QZHeight);engine.pfnSetSize(e,&mins.x,&maxs.x);
    e->v.movetype=MOVETYPE_STEP;e->v.solid=SOLID_SLIDEBOX;e->v.flags|=FL_MONSTER;
    e->v.health=e->v.max_health=60;e->v.armorvalue=0;e->v.takedamage=DAMAGE_AIM;e->v.deadflag=DEAD_NO;
    e->v.view_ofs=Vector(0,0,QZEye);e->v.enemy=nullptr;e->v.dmg_save=0;e->v.dmg_take=0;
    e->v.teleport_time=globals->time+1;e->v.pain_finished=0;e->v.radsuit_finished=-1;
    qzSequence(e,0,QZIdle);e->v.nextthink=globals->time+.1f;
    weaponBloodInstall(e);
    startupTrace("QuakeZombie","spawn health=60 model=models/cremation/quake/zombie.mdl");
}
static bool qzTouch(edict_t* e,edict_t* other){
    if(!qzGib(e))return false;
    if(qzDiagnostic&&e->v.dmg>0){char detail[180];std::snprintf(detail,sizeof detail,"touch=%s dmg=%.0f at=%.0f,%.0f,%.0f",valid(other)?text(other->v.classname):"world",e->v.dmg,e->v.origin.x,e->v.origin.y,e->v.origin.z);startupTrace("QuakeZombieProbe",detail);}
    if(other==e->v.owner&&valid(other)&&other->serialnumber==int(e->v.dmg_save))return true;
    if(e->v.dmg>0&&valid(other)&&other->v.takedamage&&other->pvPrivateData){
        // Keep the existing undead/cultist alliance. Player allies remain targets.
        auto table=*reinterpret_cast<void***>(other->pvPrivateData);
        int faction=reinterpret_cast<int(__thiscall*)(void*)>(table[ravenholm_abi::slot::Classify])(other->pvPrivateData);
        if(faction!=7){auto owner=e->v.owner;entvars_t* attacker=valid(owner)&&owner->serialnumber==int(e->v.dmg_save)?&owner->v:&e->v;
            reinterpret_cast<TakeDamageFn>(table[ravenholm_abi::slot::TakeDamage])(other->pvPrivateData,&e->v,attacker,e->v.dmg,0);qzSound(e,8);}
        e->v.flags|=FL_KILLME;return true;
    }
    if(e->v.dmg>0){qzSound(e,7);e->v.dmg=0;e->v.solid=SOLID_NOT;e->v.velocity=Vector(0,0,0);e->v.avelocity=Vector(0,0,0);}
    return true;
}
static bool qzClearTall(edict_t* e,const Vector& at){
    // GoldSrc's world uses fixed humanoid hulls; explicitly check the extra
    // height instead of letting the scaled model walk through low ceilings.
    for(auto offset:{Vector(0,0,0),Vector(-16,-16,0),Vector(-16,16,0),Vector(16,-16,0),Vector(16,16,0)}){
        Vector a=at+offset+Vector(0,0,18),b=at+offset+Vector(0,0,QZHeight);TraceResult tr{};
        engine.pfnTraceLine(&a.x,&b.x,0,e,&tr);if(tr.fStartSolid||tr.fAllSolid||tr.flFraction<1)return false;
    }return true;
}
static bool qzWalk(edict_t* e,float yaw,float distance){
    float a=yaw*3.14159265f/180;Vector to=e->v.origin+Vector(std::cos(a)*distance,std::sin(a)*distance,0);
    if(!qzClearTall(e,to))return false;
    return engine.pfnWalkMove(e,yaw,distance,0)!=0;
}
static bool qzThink(edict_t* e){
    if(qzGib(e)){e->v.flags|=FL_KILLME;return true;}
    if(!qzIs(e))return false;
    e->v.nextthink=globals->time+.1f;
    if(e->v.impulse==QZDead){
        if(e->v.effects&EF_NODRAW){e->v.flags|=FL_KILLME;return true;}
        e->v.button=std::min(10,e->v.button+1);e->v.frame=e->v.button*25.5f;e->v.animtime=globals->time;
        e->v.framerate=e->v.button<10?1.f:0.f;if(e->v.button==10)e->v.nextthink=0;return true;
    }
    const int mode=e->v.impulse;auto enemy=qzEnemy(e);
    if(mode==QZDown&&e->v.button==10){
        if(globals->time<e->v.dmgtime+5)return true;
        e->v.solid=SOLID_SLIDEBOX;
        // WalkMove(0) tests the current hull against world and live entities.
        if(!qzWalk(e,0,0)){e->v.solid=SOLID_NOT;e->v.dmgtime=globals->time;return true;}
        e->v.impulse=QZRise;qzSound(e,0);startupTrace("QuakeZombie","rise");
    }
    if(mode==QZIdle||mode==QZChase){
        if(enemy){
            qzFace(e,enemy->v.origin);
            if(globals->time>=e->v.teleport_time&&length(enemy->v.origin-e->v.origin)<1000&&qzVisible(e,enemy)&&engine.pfnRandomFloat(0,1)<.4f){
                int attacks[]={3,10,11};qzSequence(e,attacks[engine.pfnRandomLong(0,2)],QZAttack);return true;}
            if(e->v.sequence!=2)qzSequence(e,2,QZChase);
            // Quake run frames advance 1..8 units per 0.1 s, never teleport.
            static const float distances[]={1,1,0,1,2,3,4,4,2,0,0,0,2,4,6,7,3,8};
            if(!pushed(e)){float distance=distances[e->v.button%18]*(slowed(e)?balance::SlowFactor:1.f);
                bool moved=qzWalk(e,e->v.angles.y,distance);
                if(!moved)for(float turn:{45.f,-45.f,90.f,-90.f,135.f,-135.f})if(qzWalk(e,e->v.angles.y+turn,distance))break;}
        }else{
            bool mansionRoute=e->v.targetname&&!std::strncmp(text(e->v.targetname),"mansion_enemy_",14)&&e->v.target;
            if(mansionRoute){if(e->v.sequence!=2)qzSequence(e,2,QZChase);}
            else if(e->v.sequence!=1)qzSequence(e,1,QZIdle);
            auto goal=e->v.chain;
            if(!valid(goal))for(int i=1;i<globals->maxEntities;++i){auto g=engine.pfnPEntityOfEntIndex(i);
                if(valid(g)&&g->v.netname&&e->v.targetname&&!std::strcmp(text(g->v.netname),text(e->v.targetname))){goal=g;e->v.chain=g;break;}}
            if(valid(goal)){
                if(length2d(goal->v.origin-e->v.origin)<6){auto next=engine.pfnFindEntityByString(nullptr,"targetname",text(goal->v.target));if(valid(next))goal=e->v.chain=next;}
                qzFace(e,goal->v.origin);static const float steps[]={0,2,3,2,1,0,0,0,0,0,2,2,1,0,0,0,0,0,0};
                if(!pushed(e))qzWalk(e,e->v.angles.y,steps[e->v.button%19]*(slowed(e)?balance::SlowFactor:1.f));
            }
        }
    }else if(mode==QZAttack&&enemy)qzFace(e,enemy->v.origin);
    int frames=qzFrames[std::max(0,std::min(12,e->v.sequence))];
    ++e->v.button;
    if(e->v.impulse==QZDown&&e->v.button==9){e->v.solid=SOLID_NOT;qzSound(e,6);}
    if(e->v.impulse==QZDown&&e->v.button==10)e->v.dmgtime=globals->time;
    if(e->v.button>=frames){
        if(e->v.impulse==QZAttack){qzThrow(e,enemy);e->v.teleport_time=globals->time+.3f;}
        if(e->v.impulse==QZPain||e->v.impulse==QZRise||e->v.impulse==QZAttack)qzSequence(e,enemy?2:0,enemy?QZChase:QZIdle);
        else e->v.button=0;
    }
    e->v.frame=255.f*float(e->v.button)/float(std::max(1,frames-1));e->v.animtime=globals->time;
    // Studio interpolation runs between server ticks. Freeze only the held
    // ground pose, rather than freezing every pose at ten visible steps/second.
    e->v.framerate=(e->v.impulse==QZDown&&e->v.button==10)?0.f:1.f;
    return true;
}


