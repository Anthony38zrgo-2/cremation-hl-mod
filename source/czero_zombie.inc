// Compatible monster adaptation, not the CS player/bot DLL. Native HGrunt
// allocation, studio TraceAttack and saves; isolated damage/death and knife pursuit AI.
static constexpr const char* CZClass="monster_czero_zombie";
static bool czObjectiveThink(edict_t* e,float dt);
static bool czObjectiveOwns(edict_t* e);
static bool czBombActive();
static void czBombFrame();
#ifdef CREMATION_ENABLE_VALIDATION
static void czProbeFrame();
static void czAnimationProbeFrame();
#endif
static constexpr const char* CZModel="models/cremation/czero/zombie_knife.mdl";
static constexpr const char* CZGun="models/cremation/czero/p_knife.mdl";
static bool czIs(edict_t* e){return valid(e)&&!std::strcmp(text(e->v.classname),CZClass);}
static void mansionCZDeath(edict_t* e);
static int __fastcall czClassify(void*,void*){return 7;}
static edict_t* czObject(void* object){for(int i=1;i<globals->maxEntities;++i){auto e=engine.pfnPEntityOfEntIndex(i);if(czIs(e)&&e->pvPrivateData==object)return e;}return nullptr;}
static void __fastcall czKilled(void* object,void*,entvars_t* attacker,int gib){
    auto e=czObject(object);if(!valid(e))return;
    bool first=e->v.deadflag==DEAD_NO;e->v.health=std::min(0.f,e->v.health);
    if(first&&mansionMap()&&mansionActor(e)&&e->v.iuser1==5){e->v.iuser1=0;engine.pfnSetModel(e,CZModel);Vector lo(-16,-16,0),hi(16,16,72);engine.pfnSetSize(e,&lo.x,&hi.x);}
    if(first){e->v.deadflag=DEAD_DYING;e->v.enemy=nullptr;e->v.solid=SOLID_NOT;e->v.sequence=101;e->v.frame=0;e->v.animtime=globals->time;e->v.framerate=1;e->v.nextthink=globals->time+.05f;}
    if(first)mansionCZDeath(e);
    if(first&&attacker&&(attacker->flags&FL_CLIENT))attacker->frags+=1;
    if(gib==2||(gib!=1&&e->v.health<-35)){
        if(engine.pfnCVarGetFloat("violence_hgibs")!=0)humanGibSpawn(center(e),e->v.velocity);
        e->v.effects|=EF_NODRAW;e->v.flags|=FL_KILLME;e->v.takedamage=DAMAGE_NO;
    }
}
static int __fastcall czDamage(void* object,void*,entvars_t* inflictor,entvars_t* attacker,float amount,int bits){
    if(enemyCombatBlocked(object,inflictor,attacker))return 0;
    auto e=czObject(object);if(!valid(e)||e->v.takedamage==DAMAGE_NO||amount<=0)return 0;
    if(e->v.deadflag!=DEAD_NO){if(amount>=40&&!(bits&(1<<12)))czKilled(object,nullptr,attacker,2);return 0;}
    // Do not inherit Ravenholm HGrunt's scripted 50-HP survival clamp.
    e->v.health-=amount;
    if(e->v.health<=0){auto table=*reinterpret_cast<void***>(object);reinterpret_cast<void(__thiscall*)(void*,entvars_t*,int)>(table[ravenholm_abi::slot::Killed])(object,attacker,(bits&(1<<13))?2:((bits&(1<<12))?1:0));}
    return 1;
}
struct CZTable{void** base;std::array<void*,128> slots;};
static std::vector<std::unique_ptr<CZTable>> czTables;
static void czInstall(edict_t* e){
    auto table=*reinterpret_cast<void***>(e->pvPrivateData);
    for(auto& t:czTables){if(table==t->slots.data())return;if(table==t->base){*reinterpret_cast<void***>(e->pvPrivateData)=t->slots.data();return;}}
    auto t=std::make_unique<CZTable>();t->base=table;std::copy_n(table,128,t->slots.begin());
    // Isolated damage/death remove donor scripted immortality while preserving
    // native studio TraceAttack/hitgroups and all shared weapon-effect hooks.
    t->slots[ravenholm_abi::slot::TakeDamage]=reinterpret_cast<void*>(&czDamage);t->slots[ravenholm_abi::slot::Killed]=reinterpret_cast<void*>(&czKilled);
    t->slots[ravenholm_abi::slot::Classify]=reinterpret_cast<void*>(&czClassify);
    *reinterpret_cast<void***>(e->pvPrivateData)=t->slots.data();czTables.push_back(std::move(t));
    humanGibInstall(e);weaponBloodInstall(e);
}
static void czSequence(edict_t* e,int sequence){
    e->v.gaitsequence=0;if(e->v.sequence==sequence)return;bool wasLoco=(e->v.sequence==3||e->v.sequence==4);bool newLoco=(sequence==3||sequence==4);float preserved=(wasLoco&&newLoco)?e->v.frame:0.f;e->v.sequence=sequence;e->v.frame=preserved;e->v.animtime=globals->time;
}
static bool czMeleeBusy(edict_t* e){return e->v.sequence==76&&(globals->time<e->v.dmgtime||e->v.frame<250.f);}
static void czPrecache(){
    czFadeMessage=engine.pfnRegUserMsg("ScreenFade",10);
    for(auto p:{CZModel,CZGun,CZSmokeModel,CZSmokeSprite,"models/cremation/czero/zombie_glock.mdl","models/cremation/czero/p_glock18.mdl","models/cremation/czero/w_c4.mdl","models/cremation/czero/w_flashbang.mdl"})engine.pfnPrecacheModel(const_cast<char*>(p));
    for(auto p:{"cremation/czero/knife_slash1.wav","cremation/czero/knife_hit1.wav","cremation/czero/sg_explode.wav","cremation/czero/flashbang-1.wav","cremation/czero/c4_beep1.wav","cremation/czero/c4_explode1.wav","cremation/czero/c4_plant.wav","cremation/czero/c4_disarm.wav","cremation/czero/c4_disarmed.wav","cremation/czero/glock18-2.wav"})engine.pfnPrecacheSound(const_cast<char*>(p));
}
static void czSpawn(edict_t* e){
    e->v.classname=engine.pfnAllocString(CZClass);czInstall(e);engine.pfnSetModel(e,CZModel);
    Vector lo(-16,-16,0),hi(16,16,72);engine.pfnSetSize(e,&lo.x,&hi.x);
    e->v.movetype=MOVETYPE_STEP;e->v.solid=SOLID_SLIDEBOX;e->v.flags|=FL_MONSTER;
    e->v.health=e->v.max_health=100;e->v.takedamage=DAMAGE_AIM;e->v.deadflag=DEAD_NO;e->v.view_ofs=Vector(0,0,62);
    e->v.iuser1=e->v.iuser2=e->v.iuser3=e->v.iuser4=0;
    e->v.weapons=-1;e->v.frags=0;e->v.impulse=0;e->v.button=0;e->v.dmgtime=0;e->v.pain_finished=0;
    e->v.teleport_time=globals->time+.5f;e->v.radsuit_finished=globals->time;e->v.enemy=nullptr;
    e->v.framerate=1;e->v.sequence=-1;czSequence(e,75);e->v.nextthink=globals->time+.05f;
}
static edict_t* czEnemy(edict_t* e){
    edict_t* best=nullptr;float closest=MansionZombieNoticeRange;
    for(int i=1;i<globals->maxEntities;++i){auto p=engine.pfnPEntityOfEntIndex(i);
        if(!valid(p)||dmcInvisible(p)||p->v.health<=0||p->v.deadflag!=DEAD_NO||(!(p->v.flags&FL_CLIENT)&&!necroMinion(p)&&!bounce::isSentry(p)))continue;
        float dist=length(center(p)-(e->v.origin+e->v.view_ofs));if(dist>closest)continue;
        Vector eye=e->v.origin+e->v.view_ofs,to=center(p);TraceResult tr{};engine.pfnTraceLine(&eye.x,&to.x,0,e,&tr);
        if((tr.flFraction==1||tr.pHit==p)&&(dist<80||!czSmokeBlocks(eye,to))){best=p;closest=dist;}
    }return best;
}
static void czGunFrame(edict_t* e){
    edict_t* gun=nullptr;
    for(int i=1;i<globals->maxEntities;++i){auto a=engine.pfnPEntityOfEntIndex(i);if(valid(a)&&a->v.owner==e&&!std::strcmp(text(a->v.targetname),"czombie_knife_visual")){gun=a;break;}}
    if(!aliveEnemy(e)){if(valid(gun))engine.pfnRemoveEntity(gun);return;}
    if(!valid(gun)){gun=engine.pfnCreateNamedEntity(engine.pfnAllocString("info_target"));if(!valid(gun))return;raven.pfnSpawn(gun);
        gun->v.targetname=engine.pfnAllocString("czombie_knife_visual");gun->v.owner=e;gun->v.aiment=e;gun->v.impulse=e->serialnumber;
        gun->v.movetype=MOVETYPE_FOLLOW;gun->v.solid=SOLID_NOT;engine.pfnSetModel(gun,CZGun);}
    const char* desired=e->v.iuser1==5?"models/cremation/czero/p_glock18.mdl":CZGun;
    if(std::strcmp(text(gun->v.model),desired))engine.pfnSetModel(gun,desired);
    // Inherit freeze/conversion visuals and never overwrite the NPC's effects.
    gun->v.rendermode=e->v.rendermode;gun->v.renderfx=e->v.renderfx;gun->v.rendercolor=e->v.rendercolor;gun->v.renderamt=e->v.renderamt;
    gun->v.effects=(e->v.effects&EF_NODRAW)|((e->v.impulse==2||e->v.iuser1==4)?EF_NODRAW:0);
}
static void czFire(edict_t* e,edict_t* target){
    if(length2d(target->v.origin-e->v.origin)>64||globals->time<e->v.dmgtime)return;
    Vector from=e->v.origin+Vector(0,0,40),to=center(target);
    TraceResult tr{};engine.pfnTraceHull(&from.x,&to.x,0,1,e,&tr);
    if(tr.pHit==target&&target->pvPrivateData&&target->v.takedamage){auto table=*reinterpret_cast<void***>(target->pvPrivateData);
        reinterpret_cast<TakeDamageFn>(table[ravenholm_abi::slot::TakeDamage])(target->pvPrivateData,&e->v,&e->v,20.f,1<<7);
        engine.pfnEmitSound(e,CHAN_WEAPON,"cremation/czero/knife_hit1.wav",1,ATTN_NORM,0,100);
    }else engine.pfnEmitSound(e,CHAN_WEAPON,"cremation/czero/knife_slash1.wav",1,ATTN_NORM,0,100);
    e->v.dmgtime=globals->time+.65f/(slowed(e)?balance::SlowFactor:1.f);czSequence(e,76);e->v.frame=0;
}
static bool czThink(edict_t* e){
    if(!czIs(e))return false;czGunFrame(e);
    float dt=std::max(0.f,std::min(.15f,globals->time-e->v.radsuit_finished));e->v.radsuit_finished=globals->time;
    if(!aliveEnemy(e)){
        if(e->v.sequence<101||e->v.sequence>110){czSequence(e,101);e->v.pain_finished=globals->time+1.5f;}
        e->v.frame=std::min(255.f,e->v.frame+dt*210);if(e->v.frame>=255){e->v.deadflag=DEAD_DEAD;e->v.framerate=0;e->v.animtime=globals->time;e->v.nextthink=0;}
        else e->v.nextthink=globals->time+.1f;return true;
    }
    float rate=slowed(e)?balance::SlowFactor:1.f;e->v.framerate=rate;
    auto h=static_cast<unsigned char*>(engine.pfnGetModelPtr(e));int so=*reinterpret_cast<int*>(h+168)+e->v.sequence*176;
    float fps=*reinterpret_cast<float*>(h+so+32);int frames=*reinterpret_cast<int*>(h+so+56);
    float nextFrame=e->v.frame+dt*rate*fps*256/std::max(1,frames-1);
    int sequenceFlags=*reinterpret_cast<int*>(h+so+36);
    e->v.frame=(sequenceFlags&1)?std::fmod(nextFrame,256.f):std::min(255.f,nextFrame);e->v.animtime=globals->time;
    if(pushed(e)||(e->v.flags&FL_FROZEN)||(!(e->v.flags&FL_ONGROUND)&&e->v.iuser1!=7&&e->v.iuser1!=8)){
        if(e->v.iuser1==4)e->v.teleport_time+=dt;
        if(e->v.flags&FL_FROZEN)e->v.velocity=Vector(0,0,0);
        e->v.nextthink=globals->time+.05f;return true;
    }
    // Mansion owns this NPC's movement and scientist objective; do not enter
    // the bomb-planter state machine or replace its target with the player.
    if(mansionMap()&&mansionActor(e)){e->v.nextthink=globals->time+.05f;return true;}
    if(czObjectiveThink(e,dt)){e->v.nextthink=globals->time+.05f;return true;}
    if(e->v.impulse==2){e->v.impulse=0;czSequence(e,75);}
    if(czMeleeBusy(e)){e->v.nextthink=globals->time+.05f;return true;}
    auto target=czEnemy(e);
    if(target!=e->v.enemy){e->v.enemy=target;e->v.teleport_time=globals->time+.4f;e->v.button=0;}
    if(valid(target)){
        Vector delta=target->v.origin-e->v.origin;e->v.angles.y=std::atan2(delta.y,delta.x)*180/3.14159265f;
        float distance=length2d(delta);
        if(globals->time>=e->v.teleport_time&&distance<=64)czFire(e,target);
        else if((!e->v.target||distance<160)&&distance>48){
            float yaw=e->v.angles.y;Vector direction=normal(Vector(delta.x,delta.y,0));Vector next=e->v.origin+direction*(140*dt);
            Vector top=next+Vector(0,0,16),bottom=next-Vector(0,0,22);TraceResult ground{};engine.pfnTraceLine(&top.x,&bottom.x,1,e,&ground);
            if(ground.flFraction<1&&ground.vecPlaneNormal.z>.65f&&engine.pfnWalkMove(e,yaw,140*dt*rate,0)){czSequence(e,4);e->v.fuser1=globals->time+.15f;}
        }
        if(!czMeleeBusy(e)&&globals->time>e->v.fuser1&&globals->time>e->v.dmgtime)czSequence(e,75);
    }else if(!czMeleeBusy(e)&&globals->time>e->v.fuser1&&globals->time>e->v.dmgtime+.3f)czSequence(e,75);
    e->v.nextthink=globals->time+.05f;return true;
}
static void czVisualCleanup(){
    for(int i=1;i<globals->maxEntities;++i){auto e=engine.pfnPEntityOfEntIndex(i);
        if(valid(e)&&!std::strcmp(text(e->v.targetname),"czombie_knife_visual")&&
           (!czIs(e->v.owner)||!aliveEnemy(e->v.owner)||e->v.impulse!=e->v.owner->serialnumber))engine.pfnRemoveEntity(e);
    }
}
