// Cremation server extension. Ravenholm keeps ownership of monsters, inventory,
// player state, saves and the shotgun; this module adds the disc weapon and the
// bouncing shotgun, and forwards every inherited export unchanged.
#include "extdll.h"
#include "plane.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#undef min
#undef max
#include "entity_handle.h"
#include "ravenholm_abi.h"
#include <algorithm>
#include <array>
#include <vector>
#include <string>
#include <cstring>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <cstdio>
#include <memory>
#include <unordered_map>
#include "ricochet_balance.h"
#include "bounce_shotgun.h"
#include "entity_state.h"
#include "raven_ammo.h"
#include "weapon_config.h"
#include "hecu_alliance.h"

// GoldSrc's file-based export reader assumes that export names live in .rdata.
// Reserve space there before linking; finalization never moves names elsewhere.
#pragma section(".rdata",read)
extern "C" __declspec(allocate(".rdata")) const unsigned char cremation_export_reserve[32768] = "CREMATION_EXPORT_RESERVE_BEGIN";

static enginefuncs_t engine{}, hookedEngine{};
static globalvars_t* globals=nullptr;
static DLL_FUNCTIONS raven{};
static HMODULE selfModule=nullptr, ravenModule=nullptr;
#include "forwarders.inc"
static constexpr int SvcTemp=23, SvcWeaponAnim=35, GlockId=2, Mp5Id=4;
static constexpr int Attack=1, Attack2=1<<11, Reload=1<<13;
static constexpr const char* NailViewModel="models/cremation/v_nailgun.mdl";
static constexpr const char* NailPlayerModel="models/cremation/p_nailgun.mdl";
static constexpr const char* PipeWrenchViewModel="models/cremation/v_pipe_wrench.mdl";
static constexpr const char* PipeWrenchPlayerModel="models/cremation/p_pipe_wrench.mdl";
static constexpr const char* RicochetRedModel="models/cremation/v_ricochet_red.mdl";
static int trailSprite=0, flashSprite=0;
static int msgTacticalHUD=0,msgJudgmentHUD=0,msgMansionDoors=0,msgCremationWaveHUD=0;
static int msgLinkMuzzle=0,msgLinkShot=0,msgFlakMuzzle=0;
static int msgWeaponList=0,msgCurWeapon=0,msgItemPickup=0,msgAmmoX=0,msgSetFov=0,msgAmmoPickup=0;
static bool singleGrenadePickup=false;
static int currentMsg=0,currentByte=0,curWeaponId=0;
static bool carrierCurWeapon=false;
static edict_t* messagePlayer=nullptr;
static bool ownAnimation=false;
static int shotgunAmmoIndex=-1,nailAmmoIndex=-1, ammoOffset=-1;
static int grenadeAmmoIndex=-1;
static float lastFrame=0;
static float lastDroppedItemCleanup=0;
static float firstPlayerPostThinkTime=0;
struct HordeProbe {edict_t* entity; int serial; Vector origin; Vector previous; float previousTime; bool jumped=false; float walked=0; Vector patrol{0,0,0}; bool fallback=false,returning=false,combat=false; float initialHealth=0;};
static std::vector<HordeProbe> hordeProbes;
static float hordeReportAt=0;
static int hordeReportsRemaining=0;
static float hordeStartedAt=0;
static constexpr float MansionZombieNoticeRange=640.f;
static void startupTrace(const char* stage,const char* detail="") {
    if(FILE* file=std::fopen("Cremation/startup-diagnostics.txt","a")) {
        std::fprintf(file,"%s %s\n",stage,detail);std::fclose(file);
    }
}

static const char* text(string_t s) { return engine.pfnSzFromIndex(s); }
static bool valid(edict_t* e) { return e && !e->free && !(e->v.flags&FL_KILLME); }
static bool zombieVisionClass(const char* name){
    return name&&(std::strstr(name,"zombie")||!std::strcmp(name,"monster_johnny")||!std::strcmp(name,"monster_max")||!std::strcmp(name,"monster_ken")||!std::strcmp(name,"monster_randy")||!std::strcmp(name,"monster_minion"));
}
static bool zombieHecuVision(edict_t* e){
    if(!valid(e)||!e->pvPrivateData)return false;
    // The compiled SDK layout supplies this field offset. Guard against donor
    // DLL class-layout drift by only changing the native CZombie default (.5).
    auto monster=reinterpret_cast<CBaseMonster*>(e->pvPrivateData);
    if(!std::isfinite(monster->m_flFieldOfView)||std::fabs(monster->m_flFieldOfView-.5f)>.001f)return false;
    monster->m_flFieldOfView=.2f;
    return true;
}
static bool aliveEnemy(edict_t* e) {
    return valid(e) && (e->v.flags&FL_MONSTER) && e->v.deadflag==DEAD_NO && e->v.health>0;
}
static std::vector<edict_t*> g_activeEnemies;
static float g_activeEnemiesTime = -1.f;
static void invalidateActiveEnemies() {
    g_activeEnemiesTime = -1.f;
}
static const std::vector<edict_t*>& getActiveEnemies() {
    if (g_activeEnemiesTime != globals->time) {
        g_activeEnemiesTime = globals->time;
        g_activeEnemies.clear();
        for (int i = globals->maxClients + 1; i < globals->maxEntities; ++i) {
            auto e = engine.pfnPEntityOfEntIndex(i);
            if (aliveEnemy(e)) {
                g_activeEnemies.push_back(e);
            }
        }
    }
    return g_activeEnemies;
}
static float length(const Vector& a) { return std::sqrt(a.x*a.x+a.y*a.y+a.z*a.z); }
static float length2d(const Vector& a) { return std::sqrt(a.x*a.x+a.y*a.y); }
static Vector normal(const Vector& a) { float n=length(a); return n>.001f ? a*(1.f/n):Vector(0,0,0); }
static float dot(const Vector&a,const Vector&b) { return a.x*b.x+a.y*b.y+a.z*b.z; }
static Vector center(edict_t* e) { return (e->v.absmin+e->v.absmax)*.5f; }
#include "enemy_combat_policy.inc"
static bool ricochet(edict_t* e) {
    return valid(e) && e->v.health>0 && e->v.viewmodel && (!std::strcmp(text(e->v.viewmodel),"models/v_9mmhandgun.mdl") || !std::strcmp(text(e->v.viewmodel),RicochetRedModel));
}
static bool nailgun(edict_t* e) {
    if(!valid(e)||e->v.health<=0||!e->v.viewmodel)return false;
    const char* model=text(e->v.viewmodel);
    return !std::strcmp(model,"models/v_9mmAR.mdl")||!std::strcmp(model,NailViewModel);
}
// Temporary availability switch: keep implementations/assets for reactivation.
static constexpr bool ShotgunsPlayable=false;
static bool mansionGrenadePickup(edict_t* e){return valid(e)&&e->v.targetname&&!std::strcmp(text(e->v.classname),"weapon_handgrenade")&&!std::strncmp(text(e->v.targetname),"mansion_grenade_",16);}
static bool mansionTouchGrenade(edict_t* item,edict_t* player);
static bool disabledShotgunClass(const char* n){return !ShotgunsPlayable&&n&&(!std::strcmp(n,"weapon_shotgun")||!std::strcmp(n,"weapon_winchester1887"));}
static bool pipeWrench(edict_t* e) {
    if(!valid(e)||e->v.health<=0||!e->v.viewmodel)return false;
    const char* model=text(e->v.viewmodel);
    return !std::strcmp(model,"models/v_crowbar.mdl")||!std::strcmp(model,PipeWrenchViewModel);
}
static int pipeWrenchSequence(int sequence) {
    static const int map[]={0,3,4,5,6,8,7,10,9};
    return sequence>=0&&sequence<int(sizeof(map)/sizeof(map[0]))?map[sequence]:0;
}
static void coords(const Vector& p) { engine.pfnWriteCoord(p.x);engine.pfnWriteCoord(p.y);engine.pfnWriteCoord(p.z); }
static void sound(edict_t* e,const char* name) { engine.pfnEmitSound(e,CHAN_WEAPON,name,1,ATTN_NORM,0,100); }
#include "blood_cultist_voices.h"
#include "blood_zombie_voices.inc"
static void hookEmitSound(edict_t* entity,int channel,const char* sample,float volume,float attenuation,int flags,int pitch) {
    if(cultistVoiceSound(entity,channel,sample,flags))return;
    if(zombieVoiceSound(entity,sample,flags))return;
    const char* replacement=sample;
    if(pipeWrench(entity)) {
        if(!std::strcmp(sample,"weapons/cbar_hit1.wav"))replacement="weapons/pwrench_hit1.wav";
        else if(!std::strcmp(sample,"weapons/cbar_hit2.wav"))replacement="weapons/pwrench_hit2.wav";
        else if(!std::strcmp(sample,"weapons/cbar_hitbod1.wav"))replacement="weapons/pwrench_hitbod1.wav";
        else if(!std::strcmp(sample,"weapons/cbar_hitbod2.wav"))replacement="weapons/pwrench_hitbod2.wav";
        else if(!std::strcmp(sample,"weapons/cbar_hitbod3.wav"))replacement="weapons/pwrench_hitbod3.wav";
        else if(!std::strcmp(sample,"weapons/cbar_miss1.wav"))replacement="weapons/pwrench_miss1.wav";
    }
    engine.pfnEmitSound(entity,channel,replacement,volume,attenuation,flags,pitch);
}

struct Effect {
    edict_t* entity=nullptr;
    int serial=0;
    float slowUntil=0,pushUntil=0,baseFrame=1;
    int mode=0,fx=0,movement=MOVETYPE_STEP;
    float alpha=0;
    Vector color{0,0,0};
};
static std::vector<Effect> effects;
struct PlayerState { int buttons=0; float nextFire=0,animUntil=0,nailNextFire=0; bool wasActive=false,wasFlak=false,wasNailgun=false,redMode=false; int charges=balance::MaxCharges; float rechargeAt=0; int sentCharges=-1; };
static std::array<PlayerState,33> players{};
struct Projectile {
    edict_t* entity=nullptr; int serial=0;
    edict_t* owner=nullptr; int ownerSerial=0;
    Vector velocity{0,0,0}; float born=0,nextSteer=0; int bounces=0;
    bool explosive=false;
    float nextParticles=0;
    std::vector<std::pair<edict_t*,int>> slowedTargets;
};
static std::vector<Projectile> discs;
struct NailProjectile {
    edict_t* entity=nullptr; int serial=0;
    edict_t* owner=nullptr; int ownerSerial=0;
    Vector velocity{0,0,0}; float born=0,damage=9.f;
    std::array<edict_t*,2> visuals{};std::array<int,2> visualSerials{};float nextLight=0;
};
static std::vector<NailProjectile> nails;

static Effect* getEffect(edict_t* e,bool create=false) {
    if (!valid(e)) return nullptr;
    int i=engine.pfnIndexOfEdict(e);
    if (i<0 || i>=int(effects.size())) return nullptr;
    auto& s=effects[i];
    if (s.entity!=e || s.serial!=e->serialnumber) {
        if (!create) return nullptr;
        s=Effect{};s.entity=e;s.serial=e->serialnumber;
    }
    return &s;
}
static bool slowed(edict_t* e) { auto s=getEffect(e);return s && s->slowUntil>globals->time && aliveEnemy(e); }
static bool pushed(edict_t* e) { auto s=getEffect(e);return s && s->pushUntil>globals->time && aliveEnemy(e); }
static void blue(edict_t* e) {
    e->v.renderfx=kRenderFxGlowShell;
    e->v.rendermode=kRenderTransTexture;
    e->v.renderamt=175;
    e->v.rendercolor=Vector(55,135,255);
}
static void restoreSlow(Effect& s) {
    auto e=s.entity;
    if(valid(e) && e->serialnumber==s.serial) {
        e->v.rendermode=s.mode;e->v.renderfx=s.fx;e->v.renderamt=s.alpha;
        e->v.rendercolor=s.color;e->v.framerate=s.baseFrame;
    }
    s.slowUntil=0;
}
static void expireEffects() {
    for(auto& s:effects) {
        auto e=s.entity;if(!e)continue;
        if(!valid(e)||e->serialnumber!=s.serial) {s=Effect{};continue;}
        if(s.slowUntil && (s.slowUntil<=globals->time || !aliveEnemy(e)))restoreSlow(s);
        if(s.pushUntil && (s.pushUntil<=globals->time || !aliveEnemy(e))) {
            if(e->v.movetype==MOVETYPE_TOSS)e->v.movetype=s.movement;
            s.pushUntil=0;
        }
    }
}
static void freeze(edict_t* e) {
    if(!aliveEnemy(e))return;
    auto s=getEffect(e,true);if(!s)return;
    if(s->slowUntil<=globals->time) {
        if(s->slowUntil)restoreSlow(*s);
        s->mode=e->v.rendermode;s->fx=e->v.renderfx;s->alpha=e->v.renderamt;
        s->color=e->v.rendercolor;s->baseFrame=e->v.framerate;
    }
    s->slowUntil=globals->time+balance::SlowDuration;
    e->v.framerate=s->baseFrame*balance::SlowFactor;
    blue(e);sound(e,"ricochet/freeze.wav");
}

// CBaseEntity::TakeDamage is virtual slot 11 in Valve's Win32 HLSDK ABI.
// No Ravenholm private fields, class sizes or subclass layouts are accessed.
using TakeDamageFn=int(__thiscall*)(void*,entvars_t*,entvars_t*,float,int);
static void damage(edict_t* e,edict_t* projectile,edict_t* owner,float amount) {
    if(!e->pvPrivateData || !e->v.takedamage || amount<=0)return;
    auto table=*reinterpret_cast<void***>(e->pvPrivateData);
    auto fn=reinterpret_cast<TakeDamageFn>(table[ravenholm_abi::slot::TakeDamage]);
    fn(e->pvPrivateData,&projectile->v,&owner->v,amount,1<<6); // DMG_BLAST
}
// Shared small impulse for the bounce shotgun grenade. Same bookkeeping as the
// disc wave (temporary TOSS physics, AI displacement suppression, restore on
// land/death/expiry) with caller-supplied magnitudes.
static void applyPush(edict_t* e,const Vector& dir,float horizontal,float vertical,float duration) {
    if(!aliveEnemy(e))return;
    auto s=getEffect(e,true);if(!s)return;
    if(s->pushUntil<=globals->time)s->movement=e->v.movetype;
    Vector flat=normal(Vector(dir.x,dir.y,0));
    Vector velocity=e->v.velocity+flat*horizontal;
    float n=length2d(velocity);
    if(n>balance::MaxHorizontalPush) {velocity.x*=balance::MaxHorizontalPush/n;velocity.y*=balance::MaxHorizontalPush/n;}
    velocity.z=std::min(260.f,std::max(velocity.z,vertical));
    e->v.flags&=~FL_ONGROUND;e->v.groundentity=nullptr;
    e->v.movetype=MOVETYPE_TOSS;e->v.velocity=velocity;
    s->pushUntil=globals->time+duration;
}
static void knock(edict_t* e,const Vector& dir,float strength) {
    applyPush(e,dir,balance::BlastPush*strength,balance::BlastLift*strength,balance::PushDuration);
}
static bool canSee(const Vector& from,edict_t* target,edict_t* ignore) {
    TraceResult tr{};Vector to=center(target);
    engine.pfnTraceLine(from,to,0,ignore,&tr);
    return !tr.fStartSolid && (tr.flFraction>=.999f || tr.pHit==target);
}
static void burst(const Projectile& d,const Vector& at) {
    engine.pfnMessageBegin(MSG_PVS,SvcTemp,at,nullptr);
    engine.pfnWriteByte(TE_BEAMCYLINDER);coords(at);coords(at+Vector(0,0,balance::BlastRadius*2));
    engine.pfnWriteShort(trailSprite);engine.pfnWriteByte(0);engine.pfnWriteByte(0);
    engine.pfnWriteByte(3);engine.pfnWriteByte(24);engine.pfnWriteByte(0);
    engine.pfnWriteByte(255);engine.pfnWriteByte(160);engine.pfnWriteByte(55);
    engine.pfnWriteByte(210);engine.pfnWriteByte(0);engine.pfnMessageEnd();
    engine.pfnMessageBegin(MSG_PVS,SvcTemp,at,nullptr);
    engine.pfnWriteByte(TE_SPRITE);coords(at);engine.pfnWriteShort(flashSprite);
    engine.pfnWriteByte(9);engine.pfnWriteByte(200);engine.pfnMessageEnd();
    sound(d.entity,"ricochet/blast.wav");
    if(!valid(d.owner) || d.owner->serialnumber!=d.ownerSerial)return;
    for(int i=globals->maxClients+1;i<globals->maxEntities;++i) {
        auto e=engine.pfnPEntityOfEntIndex(i);if(!aliveEnemy(e))continue;
        Vector offset=center(e)-at;float distance=length(offset);
        if(distance>balance::BlastRadius || !canSee(at,e,d.owner))continue;
        float strength=std::max(.25f,1.f-distance/balance::BlastRadius);
        damage(e,d.entity,d.owner,balance::BlastDamage*strength);
        knock(e,length(offset)>1?offset:d.velocity,strength);
    }
}
static void removeDisc(Projectile& d) {
    if(valid(d.entity) && d.entity->serialnumber==d.serial)engine.pfnRemoveEntity(d.entity);
    d.entity=nullptr;
}
static void steer(Projectile& d,float dt) {
    // RPG-style continuous guidance toward the point under the owner's aim.
    // There is no laser sprite/entity and no automatic enemy selection.
    Vector aim=d.owner->v.v_angle+d.owner->v.punchangle;
    float yaw=aim.y*3.14159265359f/180,pitch=aim.x*3.14159265359f/180;
    Vector direction(std::cos(pitch)*std::cos(yaw),std::cos(pitch)*std::sin(yaw),-std::sin(pitch));
    Vector eye=d.owner->v.origin+d.owner->v.view_ofs,to=eye+direction*8192;
    TraceResult tr{};engine.pfnTraceLine(&eye.x,&to.x,0,d.owner,&tr);
    Vector target=tr.flFraction<1.f?Vector(tr.vecEndPos):to;
    Vector desired=normal(target-d.entity->v.origin);
    Vector current=normal(d.velocity);
    float cosine=std::max(-1.f,std::min(1.f,dot(current,desired)));
    float angle=std::acos(cosine),turn=std::min(angle,balance::GuidanceTurnRate*dt);
    Vector axis(current.y*desired.z-current.z*desired.y,current.z*desired.x-current.x*desired.z,current.x*desired.y-current.y*desired.x);
    if(length(axis)<.001f)axis=std::fabs(current.z)<.9f?Vector(current.y,-current.x,0):Vector(0,current.z,-current.y);
    axis=normal(axis);
    Vector cross(axis.y*current.z-axis.z*current.y,axis.z*current.x-axis.x*current.z,axis.x*current.y-axis.y*current.x);
    Vector next=current*std::cos(turn)+cross*std::sin(turn)+axis*(dot(axis,current)*(1-std::cos(turn)));
    if(length(next)>.5f)d.velocity=normal(next)*balance::DiscSpeed;
}
// Sweep the complete segment against enemy bounds independently of the world
// trace: a primary disc can cross overlapping enemies without teleporting past
// walls, changing its direction or spending a surface reflection.
static bool crossesEnemy(const Vector& from,const Vector& to,edict_t* target) {
    Vector delta=to-from;float enter=0,leave=1;
    for(int axis=0;axis<3;++axis) {
        float start=from[axis],speed=delta[axis];
        float lo=target->v.absmin[axis],hi=target->v.absmax[axis];
        if(std::fabs(speed)<.00001f) {if(start<lo || start>hi)return false;continue;}
        float a=(lo-start)/speed,b=(hi-start)/speed;
        if(a>b)std::swap(a,b);
        enter=std::max(enter,a);leave=std::min(leave,b);
        if(enter>leave)return false;
    }
    return true;
}
static void discParticles(Projectile& d) {
    if(globals->time<d.nextParticles)return;
    d.nextParticles=globals->time+.1f;
    Vector at=d.entity->v.origin,back=normal(d.velocity)*-3.f;
    for(int n=0;n<5;++n) {
        float phase=globals->time*19.f+n*2.39996f+d.serial;
        Vector offset(std::cos(phase)*6.f,std::sin(phase)*6.f,std::sin(phase*1.7f)*5.f);
        Vector start=at+offset,end=start+back+offset*.35f;
        engine.pfnMessageBegin(MSG_PVS,SvcTemp,&at.x,nullptr);
        engine.pfnWriteByte(TE_BEAMPOINTS);coords(start);coords(end);
        engine.pfnWriteShort(trailSprite);
        engine.pfnWriteByte(0);engine.pfnWriteByte(0); // frame / frame rate
        engine.pfnWriteByte(2);engine.pfnWriteByte(3);engine.pfnWriteByte(2); // life / width / noise
        engine.pfnWriteByte(d.explosive?255:60);engine.pfnWriteByte(d.explosive?150:145);engine.pfnWriteByte(d.explosive?40:255);
        engine.pfnWriteByte(220);engine.pfnWriteByte(0);engine.pfnMessageEnd();
    }
}
static void advanceDiscs(float dt) {
    std::vector<edict_t*> targets;
    if(!discs.empty())for(int i=1;i<globals->maxEntities;++i) {
        auto e=engine.pfnPEntityOfEntIndex(i);if(aliveEnemy(e))targets.push_back(e);
    }
    for(auto& d:discs) {
        if(!valid(d.entity) || d.entity->serialnumber!=d.serial) {d.entity=nullptr;continue;}
        if(!valid(d.owner) || d.owner->serialnumber!=d.ownerSerial || d.owner->v.health<=0 || globals->time-d.born>balance::DiscLifetime) {removeDisc(d);continue;}
        if(d.explosive)steer(d,dt);
        float remaining=dt;
        // Bound bounce iterations; trace the entire traveled segment to avoid tunneling.
        for(int sub=0;sub<8 && remaining>0 && d.entity;++sub) {
            Vector from=d.entity->v.origin,to=from+d.velocity*remaining;
            TraceResult tr{};engine.pfnTraceLine(from,to,d.explosive?0:1,d.owner,&tr);
            if(!d.explosive && !tr.fStartSolid && !tr.fAllSolid) {
                Vector stop=tr.flFraction<1?Vector(tr.vecEndPos):to;
                for(auto target:targets)if(aliveEnemy(target) && crossesEnemy(from,stop,target)) {
                    auto identity=std::make_pair(target,target->serialnumber);
                    if(std::find(d.slowedTargets.begin(),d.slowedTargets.end(),identity)==d.slowedTargets.end()) {
                        freeze(target);d.slowedTargets.push_back(identity);
                    }
                }
            }
            if(tr.fStartSolid || tr.fAllSolid) {if(d.explosive)burst(d,from);removeDisc(d);break;}
            if(tr.flFraction>=1) {engine.pfnSetOrigin(d.entity,to);break;}
            Vector at=tr.vecEndPos+tr.vecPlaneNormal*2.f;
            engine.pfnSetOrigin(d.entity,at);
            // Secondary guides but detonates on its first contact. Only the
            // primary freezes targets and survives successive reflections.
            if(d.explosive) {burst(d,at);removeDisc(d);break;}
            if(++d.bounces>5) {removeDisc(d);break;}
            d.velocity=d.velocity-tr.vecPlaneNormal*(2.f*dot(d.velocity,tr.vecPlaneNormal));
            sound(d.entity,"ricochet/bounce.wav");
            remaining*=1.f-tr.flFraction;
        }
        if(d.entity) {discParticles(d);d.entity->v.angles.y+=dt*720;d.entity->v.velocity=Vector(0,0,0);}
    }
    discs.erase(std::remove_if(discs.begin(),discs.end(),[](const Projectile& d){return !d.entity;}),discs.end());
}
static void animation(edict_t* player,int sequence) {
    player->v.weaponanim=sequence;
    ownAnimation=true;
    engine.pfnMessageBegin(MSG_ONE,SvcWeaponAnim,nullptr,player);
    engine.pfnWriteByte(sequence);engine.pfnWriteByte(0);engine.pfnMessageEnd();
    ownAnimation=false;
}
static int sharedAmmo(edict_t* p,int ammoIndex,int spend) {
    if(!p||!p->pvPrivateData||ammoOffset<0||ammoIndex<0||ammoIndex>=32)return 0;
    int* ammo=reinterpret_cast<int*>(static_cast<char*>(p->pvPrivateData)+ammoOffset)+ammoIndex;
    if(*ammo<0||*ammo>250)return 0;
    if(spend>0) {if(*ammo<spend)return -1;*ammo-=spend;}
    return *ammo;
}
#include "link_plasma.inc"
static bool launchNail(edict_t* player,float nailDamage,bool secondary) {
    Vector aim=player->v.v_angle+player->v.punchangle;
    float yaw=aim.y*3.14159265359f/180.f,pitch=aim.x*3.14159265359f/180.f;
    Vector direction(std::cos(pitch)*std::cos(yaw),std::cos(pitch)*std::sin(yaw),-std::sin(pitch));
    Vector right(std::sin(yaw),-std::cos(yaw),0);
    Vector up=normal(Vector(right.y*direction.z-right.z*direction.y,right.z*direction.x-right.x*direction.z,right.x*direction.y-right.y*direction.x));
    Vector eye=player->v.origin+player->v.view_ofs,origin=eye+direction*48.f+right*16.f-up*5.4f;
    TraceResult muzzle{};engine.pfnTraceLine(eye,origin,0,player,&muzzle);
    if(muzzle.flFraction<1.f)origin=muzzle.vecEndPos-direction*2.f;
    auto e=engine.pfnCreateNamedEntity(engine.pfnAllocString("info_target"));
    if(!e)return false;
    raven.pfnSpawn(e);
    e->v.classname=engine.pfnAllocString("cremation_nail");
    e->v.movetype=MOVETYPE_NONE;e->v.solid=SOLID_NOT;e->v.takedamage=0;
    e->v.owner=player;e->v.angles=Vector(aim.x,aim.y,0);e->v.scale=1.f;
    plasmaStyle(e,PlasmaBody,.25f,240);
    Vector mins(-1,-1,-1),maxs(1,1,1);engine.pfnSetSize(e,mins,maxs);engine.pfnSetOrigin(e,origin);
    nails.push_back({e,e->serialnumber,player,player->serialnumber,direction*750.f,globals->time,nailDamage});
    auto& plasma=nails.back();
    plasma.visuals[0]=plasmaLayer(player,origin,PlasmaCore,.22f,255);
    plasma.visuals[1]=plasmaLayer(player,origin,PlasmaHalo,.4f,150);
    for(int i=0;i<2;++i)if(plasma.visuals[i])plasma.visualSerials[i]=plasma.visuals[i]->serialnumber;
    plasmaUpdate(plasma);
    if(msgLinkShot){engine.pfnMessageBegin(MSG_ONE,msgLinkShot,nullptr,player);engine.pfnWriteByte(engine.pfnIndexOfEdict(player));engine.pfnWriteShort(engine.pfnIndexOfEdict(e));for(auto visual:plasma.visuals)engine.pfnWriteShort(valid(visual)?engine.pfnIndexOfEdict(visual):0);coords(direction*750.f);engine.pfnMessageEnd();}

    engine.pfnMessageBegin(MSG_BROADCAST,SvcTemp,nullptr,nullptr);
    engine.pfnWriteByte(TE_BEAMFOLLOW);engine.pfnWriteShort(engine.pfnIndexOfEdict(e));engine.pfnWriteShort(trailSprite);
    engine.pfnWriteByte(1);engine.pfnWriteByte(2);
    engine.pfnWriteByte(secondary?255:35);engine.pfnWriteByte(secondary?35:255);engine.pfnWriteByte(secondary?35:45);
    engine.pfnWriteByte(85);engine.pfnMessageEnd();
    return true;
}
static void removeNail(NailProjectile& nail) {
    for(int i=0;i<2;++i) {
        auto e=nail.visuals[i];
        if(valid(e)&&e->serialnumber==nail.visualSerials[i])engine.pfnRemoveEntity(e);
        nail.visuals[i]=nullptr;
    }
    if(valid(nail.entity)&&nail.entity->serialnumber==nail.serial)engine.pfnRemoveEntity(nail.entity);
    nail.entity=nullptr;
}
static void advanceNails(float dt) {
    for(auto& nail:nails) {
        if(!valid(nail.entity)||nail.entity->serialnumber!=nail.serial) {removeNail(nail);continue;}
        if(!valid(nail.owner)||nail.owner->serialnumber!=nail.ownerSerial||nail.owner->v.health<=0||globals->time-nail.born>6.f) {removeNail(nail);continue;}
        Vector from=nail.entity->v.origin,to=from+nail.velocity*dt;
        TraceResult tr{};engine.pfnTraceLine(from,to,0,nail.owner,&tr);
        if(tr.fStartSolid||tr.fAllSolid) {removeNail(nail);continue;}
        if(tr.flFraction>=1.f) {engine.pfnSetOrigin(nail.entity,to);plasmaUpdate(nail);continue;}
        engine.pfnSetOrigin(nail.entity,tr.vecEndPos);
        auto victim=tr.pHit;
        if(valid(victim)&&victim->v.takedamage>0&&victim->pvPrivateData) {
            auto table=*reinterpret_cast<void***>(victim->pvPrivateData);
            auto takeDamage=reinterpret_cast<TakeDamageFn>(table[ravenholm_abi::slot::TakeDamage]);
            takeDamage(victim->pvPrivateData,&nail.entity->v,&nail.owner->v,nail.damage,0); // DMC nails use DMG_GENERIC.
        }
        removeNail(nail);
    }
    nails.erase(std::remove_if(nails.begin(),nails.end(),[](const NailProjectile& n){return !n.entity;}),nails.end());
}
static bool fireNailgun(edict_t* p,bool secondary) {
    // Continuous secondary is owned by linkPostThink, never by nail projectiles.
    if(secondary)return false;
    const int cost=1;
    if(sharedAmmo(p,nailAmmoIndex,0)<cost)return false;
    if(!launchNail(p,9.f*1.75f,false))return false;
    int remaining=sharedAmmo(p,nailAmmoIndex,cost);
    if(msgAmmoX&&nailAmmoIndex>=0&&remaining>=0) {
        engine.pfnMessageBegin(MSG_ONE,msgAmmoX,nullptr,p);
        engine.pfnWriteByte(nailAmmoIndex);engine.pfnWriteByte(remaining);engine.pfnMessageEnd();
    }
    sound(p,"weapons/ut2004/link_fire.wav");
    animation(p,5);
    p->v.punchangle.x-=.2f;
    return true;
}
#include "humanoid_gibs.inc"
static bool heartRollDeath(edict_t* e);
static edict_t* heartSpawn(const Vector& origin);
static float davidMeleeScale(entvars_t* attacker);
#include "dmc_powerups.inc"
#include "weapon_blood.inc"
#include "link_gun.inc"
#include "ut2004_rpg.inc"
#include "necromancer.inc"
static int bounceSpawn(edict_t* e) { return raven.pfnSpawn ? raven.pfnSpawn(e) : 0; }
static void bounceAnimation(edict_t* p,int sequence) {
 animation(p,sequence);
 if(msgFlakMuzzle&&bounce::isBouncePlayer(p)&&(sequence==1||sequence==2)){engine.pfnMessageBegin(MSG_ONE,msgFlakMuzzle,nullptr,p);engine.pfnWriteByte(sequence);engine.pfnMessageEnd();}
}
static void bouncePush(edict_t* e,const Vector& dir,float horizontal,float vertical,float duration) {
    applyPush(e,dir,horizontal,vertical,duration);
}
static int sharedShells(edict_t* p,int spend) {
    if(!p || !p->pvPrivateData || ammoOffset<0 || shotgunAmmoIndex<0 || shotgunAmmoIndex>=32)return 0;
    int* ammo=reinterpret_cast<int*>(static_cast<char*>(p->pvPrivateData)+ammoOffset)+shotgunAmmoIndex;
    if(*ammo<0 || *ammo>125)return 0;
    if(spend>0) {if(*ammo<spend)return -1;*ammo-=spend;}
    return *ammo;
}
#include "shockrifle.inc"
#include "quake_zombie.inc"
#ifdef CREMATION_ENABLE_VALIDATION
#include "quake_zombie_validation.inc"
#endif
static bool inMansionMove = false;
static int mansionStartWave(){
    if(!globals||!globals->mapname)return 0;
    const char* name=text(globals->mapname);
    if(!name)return 0;
    if(!std::strcmp(name,"cs_mansion"))return 1;
    constexpr char prefix[]="cs_mansion_wave";
    if(std::strncmp(name,prefix,sizeof(prefix)-1))return 0;
    char* end=nullptr;long wave=std::strtol(name+sizeof(prefix)-1,&end,10);
    return end&&!*end&&wave>=1&&wave<=4?int(wave):0;
}
static bool mansionMap(){return mansionStartWave()!=0;}
static bool mansionActor(edict_t* e){return valid(e)&&e->v.targetname&&!std::strncmp(text(e->v.targetname),"mansion_enemy_",14);}
#include "czero_smoke.inc"
#include "czero_zombie.inc"
#include "david_zombie.inc"
#include "heart_pickups.inc"
static bool disabledShotgunActive(edict_t* p){return !ShotgunsPlayable&&bounce::isTfcPlayer(p);}
static void attachBounce() {
    bounce::Host host{};
    host.engine=&engine;host.globals=globals;
    host.shells=sharedShells;
    host.spawn=bounceSpawn;host.sound=sound;host.animation=bounceAnimation;host.push=bouncePush;
    bounce::attach(host);
}
static void launch(edict_t* player,float spread,bool explosive) {
    Vector aim=player->v.v_angle+player->v.punchangle;aim.y+=spread;
    float yaw=aim.y*3.14159265359f/180,pitch=aim.x*3.14159265359f/180;
    Vector direction(std::cos(pitch)*std::cos(yaw),std::cos(pitch)*std::sin(yaw),-std::sin(pitch));
    Vector eye=player->v.origin+player->v.view_ofs;
    Vector origin=eye+direction*16;
    // Keep the muzzle out of walls; the first frame's trace will resolve contact.
    TraceResult tr{};engine.pfnTraceLine(eye,origin,0,player,&tr);
    origin=tr.flFraction<1 ? tr.vecEndPos-direction*2 : origin;
    auto e=engine.pfnCreateNamedEntity(engine.pfnAllocString("info_target"));if(!e)return;
    raven.pfnSpawn(e);
    e->v.classname=engine.pfnAllocString("cremation_disc");
    e->v.movetype=MOVETYPE_NONE;e->v.solid=SOLID_NOT;e->v.takedamage=0;
    e->v.owner=player;e->v.angles=Vector(0,aim.y,0);e->v.scale=1;
    e->v.rendermode=kRenderNormal;e->v.renderfx=kRenderFxGlowShell;e->v.renderamt=8;
    e->v.rendercolor=explosive?Vector(255,150,40):Vector(60,145,255);
    engine.pfnSetModel(e,explosive?"models/ricochet/disc_hard.mdl":"models/ricochet/disc.mdl");
    Vector mins(-4,-4,-4),maxs(4,4,4);engine.pfnSetSize(e,mins,maxs);engine.pfnSetOrigin(e,origin);
    discs.push_back({e,e->serialnumber,player,player->serialnumber,direction*balance::DiscSpeed,globals->time,0,0,explosive});
    engine.pfnMessageBegin(MSG_BROADCAST,SvcTemp,nullptr,nullptr);
    engine.pfnWriteByte(TE_BEAMFOLLOW);engine.pfnWriteShort(engine.pfnIndexOfEdict(e));engine.pfnWriteShort(trailSprite);
    engine.pfnWriteByte(explosive?9:10);engine.pfnWriteByte(explosive?10:12);
    engine.pfnWriteByte(explosive?255:60);engine.pfnWriteByte(explosive?150:145);engine.pfnWriteByte(explosive?40:255);
    engine.pfnWriteByte(190);engine.pfnMessageEnd();
}
static void recharge(PlayerState& s) {
    while(s.charges<balance::MaxCharges && globals->time>=s.rechargeAt) {
        ++s.charges;s.rechargeAt+=balance::RechargeTime;
    }
}
static bool fire(edict_t* p,bool secondary) {
    int index=engine.pfnIndexOfEdict(p);
    if(index<=0 || index>=33)return false;
    auto& state=players[index];recharge(state);
    if(state.charges<=0)return false;
    int needed=secondary?3:1;
    if(discs.size()+needed>balance::MaxProjectiles)return false;
    if(secondary) {launch(p,0,true);launch(p,-balance::TripleSpread,true);launch(p,balance::TripleSpread,true);}
    else launch(p,0,false);
    state.redMode=secondary;
    p->v.viewmodel=engine.pfnAllocString(secondary?RicochetRedModel:"models/v_9mmhandgun.mdl");
    --state.charges;state.rechargeAt=globals->time+balance::RechargeTime;
    animation(p,5);
    sound(p,secondary?"ricochet/altfire.wav":"ricochet/throw.wav");
    p->v.punchangle.x-=secondary?2.f:1.f;
    return true;
}
static bool zombieBusy(edict_t* e);
static bool zombieSpreadActor(edict_t* e){
    if(!valid(e)||!e->v.classname)return false;
    const char* name=text(e->v.classname);
    return std::strstr(name,"zombie")||!std::strcmp(name,"monster_johnny")||!std::strcmp(name,"monster_max")||!std::strcmp(name,"monster_ken")||!std::strcmp(name,"monster_randy")||!std::strcmp(name,"monster_minion")||davidModel(e);
}
static float zombieSpreadYaw(edict_t* e,float yaw){
    if(!valid(e))return yaw;
    if(valid(e->v.enemy)&&length2d(e->v.enemy->v.origin-e->v.origin)<112.f)return yaw;
    Vector away(0,0,0);
    for(auto other:getActiveEnemies()){
        if(other==e||!zombieSpreadActor(other)||std::fabs(other->v.origin.z-e->v.origin.z)>64.f)continue;
        Vector delta=e->v.origin-other->v.origin;float distance=length2d(delta);
        if(distance<1.f||distance>=176.f)continue;
        away=away+Vector(delta.x/distance,delta.y/distance,0)*((176.f-distance)/176.f);
    }
    float strength=length2d(away);if(strength<.08f)return yaw;
    Vector forward(std::cos(yaw*3.14159265f/180.f),std::sin(yaw*3.14159265f/180.f),0);
    Vector heading=normal(forward+normal(away)*std::min(.8f,strength*.7f));
    return std::atan2(heading.y,heading.x)*180.f/3.14159265f;
}
static int hookWalkMove(edict_t* e,float yaw,float distance,int mode) {
    if(pushed(e))distance=0;
    else if(slowed(e))distance*=balance::SlowFactor;
    if(mansionMap()&&mansionActor(e)&&!inMansionMove)return 1;
    if(zombieSpreadActor(e)&&!zombieBusy(e))yaw=zombieSpreadYaw(e,yaw);
    return engine.pfnWalkMove(e,yaw,distance,mode);
}
static void hookMoveToOrigin(edict_t* e,const float* goal,float distance,int mode) {
    if(pushed(e))distance=0;
    else if(slowed(e))distance*=balance::SlowFactor;
    engine.pfnMoveToOrigin(e,goal,distance,mode);
}
static bool invalidFastSequenceLogged=false;
static constexpr const char* RandyModel="models/cremation/hotd2/randy/randy.mdl";
static constexpr const char* RandyCharred="models/cremation/hotd2/randy/randy_charred.mdl";
static bool randyClass(edict_t* e){return valid(e)&&e->v.classname&&(!std::strcmp(text(e->v.classname),"monster_randy")||!std::strcmp(text(e->v.classname),"monster_fastzombie"));}
static void randySetModel(edict_t* e,const char* path){Vector lo=e->v.mins,hi=e->v.maxs;engine.pfnSetModel(e,path);engine.pfnSetSize(e,&lo.x,&hi.x);}
static void* hookModelPtr(edict_t* e) {
    void* model=engine.pfnGetModelPtr(e);
    if(model && randyClass(e)) {
        int count=*reinterpret_cast<int*>(static_cast<unsigned char*>(model)+164);
        if(e->v.sequence<0 || e->v.sequence>=count) {
            // Ravenholm can request activities absent from its fast-zombie model.
            // Its event dispatcher does not reject negative sequence indices.
            if(!invalidFastSequenceLogged){startupTrace("AnimationFallback","fast zombie missing activity: using idle");invalidFastSequenceLogged=true;}
            e->v.sequence=0;e->v.frame=0;e->v.animtime=globals->time;
        }
    }
    return model;
}
// Passive patrol relinquishes control permanently after contact. No cinematic
// state, relationship override or fabricated enemy pointer is installed.
static bool hordeAlert(HordeProbe& probe) {
    if(probe.combat)return true;
    if(probe.initialHealth>0 && probe.entity->v.health<probe.initialHealth)return true;
    for(int client=1;client<=globals->maxClients;++client) {
        auto player=engine.pfnPEntityOfEntIndex(client);
        if(!valid(player)||!(player->v.flags&FL_CLIENT)||player->v.health<=0||player->v.deadflag!=DEAD_NO)continue;
        float distance=length(player->v.origin-probe.entity->v.origin);
        if(distance<384)return true;
        if(distance>MansionZombieNoticeRange)continue;
        Vector eye=probe.entity->v.origin+Vector(0,0,48);
        Vector target=player->v.origin+player->v.view_ofs;
        TraceResult tr{};engine.pfnTraceLine(&eye.x,&target.x,1,probe.entity,&tr);
        if(tr.flFraction>=1 || tr.pHit==player)return true;
    }
    return false;
}
static bool zombieBusy(edict_t* e){
    if(!valid(e)||!aliveEnemy(e))return false;
    if(auto s=davidState(e)){
        if(s->dying||s->johnnyGuarding||s->johnnyThrowing)return true;
        if(davidBusy(e)&&e->v.frame<250.f)return true;
    }
    if(czIs(e))return czMeleeBusy(e);
    if(qzIs(e))return e->v.impulse==QZAttack||e->v.impulse==QZPain||e->v.impulse==QZDown||e->v.impulse==QZRise||e->v.impulse==QZDead;
    if(!engine.pfnGetModelPtr)return false;
    auto data=static_cast<unsigned char*>(engine.pfnGetModelPtr(e));if(!data)return false;
    int count=*reinterpret_cast<int*>(data+164),offset=*reinterpret_cast<int*>(data+168);
    int seq=e->v.sequence;
    if(seq<0||seq>=count)return false;
    int flags=*reinterpret_cast<int*>(data+offset+seq*176+36);
    int act=*reinterpret_cast<int*>(data+offset+seq*176+40);
    if(act==28||act==29||act==30||act==31||act==26||act==27||(act>=70&&act<=76)||act==100||(act>=36&&act<=39)||act==66){
        if(e->v.frame<250.f)return true;
    }
    if(!(flags&1)&&e->v.frame<250.f)return true;
    return false;
}
static void safeSetLocomotionSequence(edict_t* e, int sequence, float preservedFrame = 0.0f, float baseFramerate = 1.0f) {
    if (!valid(e)) return;
    float rate = slowed(e) ? (baseFramerate * balance::SlowFactor) : baseFramerate;
    if (e->v.sequence != sequence) {
        e->v.sequence = sequence;
        e->v.frame = preservedFrame;
        e->v.animtime = globals->time;
        if (e->pvPrivateData) {
            reinterpret_cast<void(__thiscall*)(void*)>(ravenholm_abi::address(ravenModule, ravenholm_abi::ResetSequenceInfoRva))(e->pvPrivateData);
        }
    }
    e->v.framerate = rate;
}
static void safeSetSequence(edict_t* e, int sequence, float baseFramerate = 1.0f) {
    safeSetLocomotionSequence(e, sequence, 0.0f, baseFramerate);
}
static void hordeWalkAnimation(edict_t* e) {
    if(zombieBusy(e))return;
    if(auto state=davidState(e)){if(aliveEnemy(e))davidActivity(e->pvPrivateData,nullptr,state->running?4:3);return;}
    if(!engine.pfnGetModelPtr)return;
    auto data=static_cast<unsigned char*>(engine.pfnGetModelPtr(e));if(!data)return;
    int count=*reinterpret_cast<int*>(data+164),offset=*reinterpret_cast<int*>(data+168);
    for(int i=0;i<count;++i)if(*reinterpret_cast<int*>(data+offset+i*176+40)==3) {
        int oldAct=(e->v.sequence>=0&&e->v.sequence<count)?*reinterpret_cast<int*>(data+offset+e->v.sequence*176+40):-1;
        float preserved=(oldAct==3||oldAct==4)?e->v.frame:0.f;
        safeSetLocomotionSequence(e,i,preserved,1.0f);
        return;
    }
}
static void hordeRunAnimation(edict_t* e) {
    if(zombieBusy(e))return;
    if(auto state=davidState(e)){if(aliveEnemy(e))davidActivity(e->pvPrivateData,nullptr,state->running?4:3);return;}
    if(!engine.pfnGetModelPtr)return;
    auto data=static_cast<unsigned char*>(engine.pfnGetModelPtr(e));if(!data)return;
    int count=*reinterpret_cast<int*>(data+164),offset=*reinterpret_cast<int*>(data+168);
    int run=-1,locomotion=-1;
    for(int i=0;i<count;++i){int activity=*reinterpret_cast<int*>(data+offset+i*176+40);
        if(activity==4){run=i;break;}
        if(activity==3&&locomotion<0)locomotion=i;
    }
    // Several original GoldSrc zombie models mark their authored run cycle as
    // ACT_WALK (3), and some have no ACT_RUN entry at all. Keep those models'
    // original locomotion cycle instead of leaving a stale sequence selected.
    if(run<0)run=locomotion;
    if(run>=0){
        int oldAct=(e->v.sequence>=0&&e->v.sequence<count)?*reinterpret_cast<int*>(data+offset+e->v.sequence*176+40):-1;
        float preserved=(oldAct==3||oldAct==4)?e->v.frame:0.f;
        safeSetLocomotionSequence(e,run,preserved,1.0f);
    }
}
static void mansionFrame(float dt);
static void mansionRadarFrame();
static void mansionRadarPrecache();
static void mansionActivate();
static void mansionPrecache();
static void startupInventoryPrecache();
static void startupInventoryGive(edict_t* player);
static void reactorPrecache();
static void reactorActivate();
static void reactorFrame();
static void mansionCZRespawnFrame();
#ifdef CREMATION_ENABLE_VALIDATION
static void mansionRulesProbeFrame();
#endif
static void tacticalHudFrame();
static void mansionDoorRepairPostThink(edict_t* player,int buttons);
static bool judgmentIs(edict_t* e);
static void judgmentThink(edict_t* e,float dt);
#include "vostok_cover.inc"
#ifdef CREMATION_ENABLE_VALIDATION
static void vostokProbeFrame();
static void truthProbeFrame();
static void czTacticalProbeFrame();
static void tacticalSaveProbeFrame();
static void tacticalHudProbeFrame();
static void davidProbeFrame();
static void enemyCombatProbeFrame();
static void dmcPreviewFrame();
#endif
static void hookThink(edict_t* e) {
    enemyCombatClearTargets(e);
    if(judgmentIs(e)){judgmentThink(e,.05f);e->v.nextthink=0;return;}
    if(vostokCoverThink(e))return;
    if(czThink(e))return;
    if(qzThink(e))return;
    if(bounce::isSentry(e)) {e->v.nextthink=globals->time+.1f;return;}
    bool incendiary=incendiaryBeforeThink(e);
    edict_t* grenadeOwner=incendiary?e->v.owner:nullptr;int grenadeOwnerSerial=valid(grenadeOwner)?grenadeOwner->serialnumber:0;
    auto s=getEffect(e);
    if(s && s->slowUntil && (!aliveEnemy(e)||globals->time>=s->slowUntil))restoreSlow(*s);
    bool active=slowed(e);
    float expected=active?s->baseFrame*balance::SlowFactor:0;
    if(active)e->v.framerate=expected;
    raven.pfnThink(e);
    incendiaryAfterThink(e,incendiary,grenadeOwner,grenadeOwnerSerial);
    for(const auto& probe:hordeProbes)if(probe.entity==e && probe.fallback && !probe.combat && aliveEnemy(e))hordeWalkAnimation(e);
    if(active && aliveEnemy(e)) {
        if(std::fabs(e->v.framerate-expected)>.0001f)s->baseFrame=e->v.framerate;
        e->v.framerate=s->baseFrame*balance::SlowFactor;blue(e);
    }
}
static void hookStartFrame() {
#ifdef CREMATION_ENABLE_VALIDATION
    tacticalHudProbeFrame();
    dmcPreviewFrame();
    tacticalSaveProbeFrame();
    czTacticalProbeFrame();
    vostokProbeFrame();
    truthProbeFrame();
    czAnimationProbeFrame();
#endif
    czSmokeFrame();
    czBombFrame();
    tacticalHudFrame();
    mansionRadarFrame();
    heartFrame();
#ifdef CREMATION_ENABLE_VALIDATION
    heartProbeFrame();
    czProbeFrame();
#endif
    czVisualCleanup();
#ifdef CREMATION_ENABLE_VALIDATION
    qzProbeFrame();
#endif
    cultistFrame();
    zombieVoiceFrame();
    rpgFrame();
    linkFrame();
    anatomicalGibFrame();
    davidFrame();
#ifdef CREMATION_ENABLE_VALIDATION
    davidProbeFrame();
    enemyCombatProbeFrame();
#endif
    if(raven.pfnStartFrame)raven.pfnStartFrame();
    // Drops are blocked at their creation/command paths. Keep a low-frequency
    // sweep as a fallback for entities created by native death routines.
    if(globals->time-lastDroppedItemCleanup>=.5f){lastDroppedItemCleanup=globals->time;
        for(int i=1;i<globals->maxEntities;++i){auto e=engine.pfnPEntityOfEntIndex(i);if(!valid(e))continue;
            if(mansionGrenadePickup(e))continue;
            if(valid(e->v.aiment)&&(e->v.aiment->v.flags&FL_CLIENT))continue;
            const char* name=text(e->v.classname);if(!std::strncmp(name,"weapon_",7)||!std::strcmp(name,"weaponbox"))engine.pfnRemoveEntity(e);
        }
    }
    necroFrame(); // Shared fire zones used by incendiary grenades.
    float dt=std::max(0.f,globals->time-lastFrame);lastFrame=globals->time;
    mansionFrame(dt);
    reactorFrame();
#ifdef CREMATION_ENABLE_VALIDATION
    mansionRulesProbeFrame();
#endif
    if(dt>0) {
        // Process long frames in bounded steps without changing disc travel speed.
        int steps=std::min(32,std::max(1,int(std::ceil(dt/.025f))));
        for(int i=0;i<steps;++i) {
            shockStep(dt/steps);
            advanceDiscs(dt/steps);
            advanceNails(dt/steps);
            bounce::frame(dt/steps);
        }
    }
    for(auto& probe:hordeProbes) {
        if(!valid(probe.entity)||probe.entity->serialnumber!=probe.serial)continue;
        Vector displacement=probe.entity->v.origin-probe.previous;
        float elapsed=std::max(.001f,globals->time-probe.previousTime);
        if(length2d(displacement)>96.f+1200.f*elapsed)probe.jumped=true;
        else probe.walked+=length2d(displacement);
        probe.combat=hordeAlert(probe);
        // Idle-only collision-checked patrol; native AI owns all combat thereafter.
        if(!probe.fallback && globals->time-hordeStartedAt>3 && probe.walked<8 && length2d(probe.patrol-probe.origin)>32 && length2d(probe.patrol-probe.origin)<140)probe.fallback=true;
        if(probe.fallback && aliveEnemy(probe.entity) && !qzIs(probe.entity)) {
            if(!probe.combat) {
                Vector target=probe.returning?probe.origin:probe.patrol;
                if(length2d(target-probe.entity->v.origin)<8){probe.returning=!probe.returning;target=probe.returning?probe.origin:probe.patrol;}
                Vector direction=target-probe.entity->v.origin;
                float yaw=std::atan2(direction.y,direction.x)*180.f/3.14159265f;
                const char* name=text(probe.entity->v.classname);
                float speed=std::strstr(name,"fast")?124.f:(std::strstr(name,"poision")?52.f:72.f);
                float step=std::min(length2d(direction),speed*std::min(elapsed,.05f));
                Vector beforeStep=probe.entity->v.origin;
                if(hookWalkMove(probe.entity,yaw,step,0)) {probe.walked+=length2d(probe.entity->v.origin-beforeStep);probe.entity->v.angles.y=yaw;hordeWalkAnimation(probe.entity);}
            }
        }
        probe.previous=probe.entity->v.origin;probe.previousTime=globals->time;
    }
    if(hordeReportAt && globals->time>=hordeReportAt) {
        int moving=0,normalMoved=0,fastMoved=0,poisonMoved=0;for(const auto& probe:hordeProbes)if(valid(probe.entity)&&probe.entity->serialnumber==probe.serial&&probe.walked>32){++moving;const char* name=text(probe.entity->v.classname);if(std::strstr(name,"fast"))++fastMoved;else if(std::strstr(name,"poision"))++poisonMoved;else ++normalMoved;}
        char report[160];std::snprintf(report,sizeof report,"zombies=%u moved_over_32=%d normal=%d fast=%d poison=%d",unsigned(hordeProbes.size()),moving,normalMoved,fastMoved,poisonMoved);
        startupTrace("HordeProgress",report);
        int jumped=0,nearSpawn=0;float closest=100000;
        for(const auto& probe:hordeProbes)if(valid(probe.entity)) {
            if(probe.jumped)++jumped;
            float distance=length2d(probe.entity->v.origin-Vector(432,-1601,-256));
            closest=std::min(closest,distance);if(distance<256)++nearSpawn;
        }
        std::snprintf(report,sizeof report,"teleports=%d near_spawn_256=%d closest=%.1f",jumped,nearSpawn,closest);
        startupTrace("HordeMovement",report);
        for(int index:{0,20,21})if(index<int(hordeProbes.size())) {
            auto& probe=hordeProbes[index];auto e=probe.entity;
            std::snprintf(report,sizeof report,"%s start=%.0f,%.0f now=%.0f,%.0f",text(e->v.targetname),probe.origin.x,probe.origin.y,e->v.origin.x,e->v.origin.y);startupTrace("HordeSample",report);
        }
        if(hordeReportsRemaining==3)for(const auto& probe:hordeProbes)if(probe.walked<=32 && valid(probe.entity)) {
            auto e=probe.entity;
            std::snprintf(report,sizeof report,"%s origin=%.0f,%.0f,%.0f mins=%.0f,%.0f,%.0f maxs=%.0f,%.0f,%.0f",text(e->v.targetname),e->v.origin.x,e->v.origin.y,e->v.origin.z,e->v.mins.x,e->v.mins.y,e->v.mins.z,e->v.maxs.x,e->v.maxs.y,e->v.maxs.z);startupTrace("HordeBlocked",report);
        }
        hordeReportAt=--hordeReportsRemaining>0?globals->time+5.f:0;
    }
    expireEffects();
}
static void hookPreThink(edict_t* p) {
    rpgNormalize(p);
    int i=engine.pfnIndexOfEdict(p);if(i>0 && i<33)players[i].buttons=p->v.button;
    if(shockRifle(p)||necromancer(p)||ricochet(p)||nailgun(p)||rpgPlayer(p)||bounce::isBouncePlayer(p)||bounce::isTfcPlayer(p)||bounce::engineerBuilding(p))p->v.button&=~(Attack|Attack2|Reload);
    raven.pfnPlayerPreThink(p);
}
static void playerDefaults(edict_t* p);
static void hookPostThink(edict_t* p) {
    if(!firstPlayerPostThinkTime)firstPlayerPostThinkTime=globals->time;
    int i=engine.pfnIndexOfEdict(p);if(i<=0||i>=33) {raven.pfnPlayerPostThink(p);return;}
    auto& state=players[i];
    bool carrier=bounce::isBouncePlayer(p);
    if(shockRifle(p)||necromancer(p)||ricochet(p)||nailgun(p)||rpgPlayer(p)||carrier||bounce::isTfcPlayer(p)||bounce::engineerBuilding(p))p->v.button&=~(Attack|Attack2|Reload);
    raven.pfnPlayerPostThink(p);
    mansionDoorRepairPostThink(p,state.buttons);
    dmcPlayerFrame(p);playerDefaults(p);
    rpgNormalize(p);
    rpgPostThink(p,state.buttons);
    shockPostThink(p,state.buttons);
    if((disabledShotgunActive(p)||necromancer(p))&&engine.pfnClientCommand)engine.pfnClientCommand(p,const_cast<char*>("weapon_bounce_shotgun\n"));
    bool activeNail=nailgun(p);
    if(activeNail) {
        p->v.viewmodel=engine.pfnAllocString(NailViewModel);
        p->v.weaponmodel=engine.pfnAllocString(NailPlayerModel);
    }
    bool activePipe=pipeWrench(p);
    if(activePipe) {
        p->v.viewmodel=engine.pfnAllocString(PipeWrenchViewModel);
        p->v.weaponmodel=engine.pfnAllocString(PipeWrenchPlayerModel);
    }
    // The bounce shotgun keeps its own clip, so its input is resolved after the
    // inherited inventory logic, with the buttons the player actually pressed.
    if(carrier && !state.wasFlak && msgWeaponList) {
        // Native WeaponList order can announce crossbow before shotgun. Publish
        // the final shared ammo index when deploying, after all native lists.
        int fields[8];bounce::hudListBytes(fields);
        engine.pfnMessageBegin(MSG_ONE,msgWeaponList,nullptr,p);
        engine.pfnWriteString(bounce::WeaponName);
        for(int field:fields)engine.pfnWriteByte(field);
        engine.pfnMessageEnd();
    }
    state.wasFlak=carrier;
    if(carrier) {
        bounce::playerPostThink(p,bounce::engineerBuilding(p)?0:state.buttons);
        static float reportAt=0;
        if((state.buttons&(Attack|Attack2)) && globals->time>=reportAt) {
            reportAt=globals->time+.5f;
            auto st=bounce::status(p);char report[256];
            std::snprintf(report,sizeof report,"shells=%d ammo_index=%d offset=%d pellets=%d grenades=%d health=%.0f model=%s",sharedShells(p,0),shotgunAmmoIndex,ammoOffset,st.activePellets,st.activeGrenades,p->v.health,text(p->v.viewmodel));
            startupTrace("FlakAttack",report);
        }
    }
    bounce::tfcPostThink(p,(!ShotgunsPlayable||bounce::engineerBuilding(p))?0:state.buttons);
    recharge(state);
    bool active=ricochet(p) && !bounce::engineerBuilding(p);
    if(active)p->v.viewmodel=engine.pfnAllocString(state.redMode?RicochetRedModel:"models/v_9mmhandgun.mdl");
    if(msgAmmoX && (state.sentCharges!=state.charges || (active&&!state.wasActive))) {
        engine.pfnMessageBegin(MSG_ONE,msgAmmoX,nullptr,p);
        engine.pfnWriteByte(balance::HudAmmoCharges);engine.pfnWriteByte(state.charges);engine.pfnMessageEnd();
        state.sentCharges=state.charges;
    }
    if(active && !state.wasActive) {animation(p,4);state.nextFire=std::max(state.nextFire,globals->time+.45f);state.animUntil=globals->time+.45f;}
    if(active && globals->time>=state.nextFire && (state.buttons&(Attack|Attack2))) {
        bool secondary=(state.buttons&Attack2)!=0;
        if(fire(p,secondary)) {
            state.nextFire=globals->time+(secondary?balance::SecondaryCooldown:balance::PrimaryCooldown);
            state.animUntil=globals->time+.4f;
        }
    }
    if(active && globals->time>=state.animUntil && p->v.weaponanim!=0)animation(p,0);
    if(activeNail&&!state.wasNailgun) {sound(p,"weapons/ut2004/link_equip.wav");animation(p,4);state.nailNextFire=std::max(state.nailNextFire,globals->time+.42f);state.animUntil=globals->time+.42f;}
    linkPostThink(p,activeNail&&globals->time>=state.nailNextFire,(state.buttons&Attack2)!=0);
    if(activeNail&&globals->time>=state.nailNextFire&&(state.buttons&Attack)&&!(state.buttons&Attack2)) {
        if(fireNailgun(p,false)) {state.nailNextFire=globals->time+(1.f/7.5f);state.animUntil=globals->time+.2f;}
    }
    if(activeNail&&globals->time>=state.animUntil&&p->v.weaponanim!=0)animation(p,0);
    state.wasNailgun=activeNail;
    state.wasActive=active;
    if(bounce::isBouncePlayer(p))p->v.fov=0;
    // Preserve the engine-visible input after inventory processing is complete.
    p->v.button=state.buttons;
}
#include "weapon_registry.inc"
static int hookRegisterMsg(const char* name,int size) {
    int id=engine.pfnRegUserMsg(name,size);
    if(!std::strcmp(name,"WeaponList"))msgWeaponList=id;
    if(!std::strcmp(name,"CurWeapon"))msgCurWeapon=id;
    if(!std::strcmp(name,"ItemPickup"))msgItemPickup=id;
    if(!std::strcmp(name,"AmmoX"))msgAmmoX=id;
    if(!std::strcmp(name,"AmmoPickup"))msgAmmoPickup=id;
    if(!std::strcmp(name,"SetFOV"))msgSetFov=id;
    bounce::setMessageIds(msgCurWeapon,msgAmmoX);
    return id;
}
static void hookMessageBegin(int dest,int type,const float* origin,edict_t* player) {
    currentMsg=type;currentByte=0;curWeaponId=0;carrierCurWeapon=false;messagePlayer=player;
    if(type==msgWeaponList){weaponListPending={};weaponListPending.dest=dest;weaponListPending.target=player;return;}
    engine.pfnMessageBegin(dest,type,origin,player);
}
static void hookMessageEnd() {
    if(currentMsg==msgWeaponList)weaponRegistryPublish();
    else engine.pfnMessageEnd();
    currentMsg=0;messagePlayer=nullptr;
}
static void hookWriteString(const char* value) {
    if(currentMsg==msgWeaponList){weaponListPending.name=value;return;}
    if(currentMsg==msgItemPickup){
        if(!std::strcmp(value,"weapon_gauss"))value="weapon_shockrifle";
        else if(!std::strcmp(value,"weapon_9mmhandgun")||!std::strcmp(value,"weapon_glock"))value="weapon_ricochet";
        else if(bounce::isCarrierClass(value))value=bounce::WeaponName;
        else if(!std::strcmp(value,"weapon_9mmAR")||!std::strcmp(value,"weapon_mp5"))value="weapon_nailgun";
        else if(!std::strcmp(value,"weapon_crowbar"))value="weapon_pipewrench";
    }
    engine.pfnWriteString(value);
}
static void hookWriteByte(int value) {
    if(currentMsg==msgAmmoPickup&&singleGrenadePickup&&currentByte==1)value=1;
    if(currentMsg==msgWeaponList){if(weaponListPending.count<8)weaponListPending.bytes[weaponListPending.count++]=value;return;}
    if(currentMsg==msgSetFov && currentByte==0){
        if(bounce::isBouncePlayer(messagePlayer))value=0;
    }
    if(currentMsg==msgCurWeapon) {
        if(currentByte==1) {curWeaponId=value;carrierCurWeapon=(value==bounce::carrierWeaponId());}
        if(currentByte==2 && curWeaponId==GlockId)value=255;
        if(currentByte==2 && curWeaponId==Mp5Id)value=255;
        if(currentByte==2 && (curWeaponId==8||curWeaponId==11||curWeaponId==9))value=255;
        if(currentByte==2 && curWeaponId==7)value=bounce::tfcClip(messagePlayer);
        if(currentByte==2 && carrierCurWeapon && valid(messagePlayer))value=bounce::status(messagePlayer).clip;
    }
    if(currentMsg==SvcWeaponAnim && !ownAnimation && currentByte==0) {
        if(shockRifle(messagePlayer))value=0;
        else if(necromancer(messagePlayer))value=0;
        else if(ricochet(messagePlayer))value=value==7?4:value==8?3:0;
        else if(pipeWrench(messagePlayer))value=pipeWrenchSequence(value);
        else if(bounce::isBouncePlayer(messagePlayer)||bounce::isTfcPlayer(messagePlayer))value=bounce::remapAnimation(value);
    }
    ++currentByte;engine.pfnWriteByte(value);
}
static void hookClientCommand(edict_t* p) {
    if(engine.pfnCmd_Argv){const char* command=engine.pfnCmd_Argv(0);if(command&&(!std::strcmp(command,"drop")||!std::strcmp(command,"weapon_awp")||!std::strcmp(command,"weapon_deagle")||!std::strcmp(command,"weapon_egon")||!std::strcmp(command,"weapon_necromancer")||!std::strcmp(command,"weapon_hornetgun")))return;}
    if(engine.pfnCmd_Argv&&disabledShotgunClass(engine.pfnCmd_Argv(0)))return;
    if(engine.pfnCmd_Argv && bounce::engineerCommand(p,engine.pfnCmd_Argv(0)))return;
    // Weapon commands travel as the HUD name, which is the item classname, so
    // the inherited SelectItem resolves them without aliasing.
    raven.pfnClientCommand(p);
    if(engine.pfnCmd_Argv && !std::strncmp(engine.pfnCmd_Argv(0),"weapon_",7)) {
        char report[256];std::snprintf(report,sizeof report,"%s -> %s",engine.pfnCmd_Argv(0),p->v.viewmodel?text(p->v.viewmodel):"none");
        startupTrace("WeaponSelection",report);
    }
}
static void hookTouch(edict_t* a,edict_t* b) {
    if(mansionTouchGrenade(a,b)||mansionTouchGrenade(b,a))return;
    auto johnnyAxeTouch=[](edict_t* axe,edict_t* other){
        if(!valid(axe)||!axe->v.classname||std::strcmp(text(axe->v.classname),"cremation_johnny_axe"))return false;
        if(valid(other)&&other==axe->v.owner)return true;
        if(valid(other)&&(other->v.flags&FL_CLIENT)&&other->pvPrivateData&&other->v.takedamage&&other->v.health>0){
            auto table=*reinterpret_cast<void***>(other->pvPrivateData);
            auto take=reinterpret_cast<TakeDamageFn>(table[ravenholm_abi::slot::TakeDamage]);
            auto owner=valid(axe->v.owner)?axe->v.owner:axe;
            take(other->pvPrivateData,&axe->v,&owner->v,axe->v.dmg,1<<2); // DMG_SLASH
        }
        engine.pfnRemoveEntity(axe);return true;
    };
    if(johnnyAxeTouch(a,b)||johnnyAxeTouch(b,a))return;
    if(qzTouch(a,b)||qzTouch(b,a))return;
    auto fullShockAmmo=[](edict_t* item,edict_t* player){return valid(item)&&valid(player)&&(player->v.flags&FL_CLIENT)&&(!std::strcmp(text(item->v.classname),"ammo_gaussclip")||!std::strcmp(text(item->v.classname),"ammo_shockcore"))&&shockAmmoIndex>=0&&sharedAmmo(player,shockAmmoIndex,0)>=50;};
    if(fullShockAmmo(a,b)||fullShockAmmo(b,a))return;
    if((valid(a)&&disabledShotgunClass(text(a->v.classname)))||(valid(b)&&disabledShotgunClass(text(b->v.classname))))return;
    if(valid(a)&&a->v.targetname&&!std::strcmp(text(a->v.targetname),"cremation_anatomical_gib")){
        if(globals->time>=a->v.fuser1&&length(a->v.velocity)>40){static const char* clips[]={"debris/flesh1.wav","debris/flesh2.wav","debris/flesh3.wav"};engine.pfnEmitSound(a,CHAN_BODY,clips[a->v.body%3],.65f,ATTN_NORM,0,100);a->v.fuser1=globals->time+.2f;}return;
    }
    if(rpgPickup(a,b)||rpgPickup(b,a))return;
    if(rpgTouch(a,b))return;
    if(bounce::engineerMetal(a,b)||bounce::engineerMetal(b,a))return;
    if(raven.pfnTouch)raven.pfnTouch(a,b);
}
static const char* hookDescription() {return "Cremation";}
static void extensionPrecache(){
        enemyCombatReset();
        davidStates.clear();davidNextDecision=0;davidPrecache();
        invalidFastSequenceLogged=false;
        startupTrace("worldspawn");
        char modulePath[MAX_PATH]{};
        GetModuleFileNameA(selfModule,modulePath,MAX_PATH);
        std::string configRoot=modulePath;
        auto slash=configRoot.find_last_of("/\\");
        if(slash!=std::string::npos)configRoot.resize(slash);
        slash=configRoot.find_last_of("/\\");
        if(slash!=std::string::npos)configRoot.resize(slash);
        auto reports=weapon_config::loadAll(configRoot+"/config/weapons");
        for(const auto& report:reports) {
            startupTrace("WeaponConfig",report.c_str());
            if(engine.pfnAlertMessage)engine.pfnAlertMessage(at_console,"Cremation: %s\n",report.c_str());
        }
        char summary[256];
        std::snprintf(summary,sizeof summary,"charges=%d recharge=%.3f flak_cost=%d/%d pellets=%d spread=%.3f gib=%d",
            balance::MaxCharges,balance::RechargeTime,bounce_balance::PrimaryAmmoCost,bounce_balance::SecondaryAmmoCost,
            bounce_balance::Pellets,bounce_balance::SpreadMultiplier,int(bounce_balance::OverkillGib));
        startupTrace("WeaponBalance",summary);
        // World spawn is the legal precache phase, before clients enter.
        msgTacticalHUD=engine.pfnRegUserMsg("TacticalHUD",17);msgJudgmentHUD=engine.pfnRegUserMsg("JudgmentHUD",6);msgMansionDoors=engine.pfnRegUserMsg("MnDoors",8);msgCremationWaveHUD=engine.pfnRegUserMsg("CremationWaveHUD",6);
        czPrecache();
        heartPrecache();
        mansionRadarPrecache();
        trailSprite=engine.pfnPrecacheModel(const_cast<char*>("sprites/ricochet/lgtning.spr"));
        flashSprite=engine.pfnPrecacheModel(const_cast<char*>("sprites/ricochet/discreturn.spr"));
        for(auto path:{"models/ricochet/disc.mdl","models/ricochet/disc_hard.mdl","models/p_9mmhandgun.mdl"})engine.pfnPrecacheModel(const_cast<char*>(path));
        for(auto path:{"ricochet/throw.wav","ricochet/altfire.wav","ricochet/freeze.wav","ricochet/blast.wav","ricochet/bounce.wav"})engine.pfnPrecacheSound(const_cast<char*>(path));
        for(auto path:{RicochetRedModel,NailViewModel,NailPlayerModel,"models/cremation/w_nailgun.mdl","models/cremation/nail.mdl",PipeWrenchViewModel,PipeWrenchPlayerModel,"models/cremation/w_pipe_wrench.mdl"})engine.pfnPrecacheModel(const_cast<char*>(path));
        for(auto path:{"weapons/ut2004/link_fire.wav","weapons/ut2004/link_altfire.wav","weapons/ut2004/link_equip.wav","weapons/spike1.wav","weapons/spike2.wav","weapons/pwrench_big_hit1.wav","weapons/pwrench_big_hit2.wav","weapons/pwrench_big_hitbod1.wav","weapons/pwrench_big_hitbod2.wav","weapons/pwrench_big_miss.wav","weapons/pwrench_hit1.wav","weapons/pwrench_hit2.wav","weapons/pwrench_hitbod1.wav","weapons/pwrench_hitbod2.wav","weapons/pwrench_hitbod3.wav","weapons/pwrench_miss1.wav","weapons/pwrench_miss2.wav"})engine.pfnPrecacheSound(const_cast<char*>(path));
        // GoldSrc retains these pointers for the resource list sent at signon.
        // Each path must remain alive after worldspawn has returned.
        for(auto path:{"weapons/doom3/chainsaw/attack.wav","weapons/doom3/chainsaw/dry.wav",
                       "weapons/doom3/chainsaw/idle_01.wav","weapons/doom3/chainsaw/idle_02.wav",
                       "weapons/doom3/chainsaw/pull.wav","weapons/doom3/chainsaw/put_away.wav"})
            engine.pfnPrecacheSound(const_cast<char*>(path));
        engine.pfnPrecacheModel(const_cast<char*>(LinkSkeletonModel));
        for(auto path:{PlasmaBody,PlasmaCore,PlasmaHalo,LinkGlowSprite,LinkHotSprite})engine.pfnPrecacheModel(const_cast<char*>(path));
        linkBeamSprite=engine.pfnPrecacheModel(const_cast<char*>("sprites/xbeam1.spr"));
        msgLinkMuzzle=engine.pfnRegUserMsg("LinkMuzzle",8);msgLinkShot=engine.pfnRegUserMsg("LinkShot",13);msgFlakMuzzle=engine.pfnRegUserMsg("FlakMuz",1);engine.pfnPrecacheModel(const_cast<char*>("sprites/muzzleflash1.spr"));
        msgGibBlood=engine.pfnRegUserMsg("GibBlood",14);bloodStamps.clear();bloodHits.clear();
        for(auto path:{"sprites/bloodspray.spr","sprites/blood.spr"})engine.pfnPrecacheModel(const_cast<char*>(path));
        shockPrecache();
        qzPrecache();
        engine.pfnPrecacheModel(const_cast<char*>(RandyModel));
        engine.pfnPrecacheModel(const_cast<char*>(RandyCharred));
        necroPrecache(); // Fire/charred assets are also used by grenades and old saves.
        // S&W 629 Opposing Force viewmodel animation-event sounds.
        for(auto path:{"weapons/629_hammer.wav","weapons/629_revolver_in.wav","weapons/629_revolver_out.wav","weapons/629_shells_land.wav","weapons/629_shell_in.wav","weapons/629_shell_release.wav"})engine.pfnPrecacheSound(const_cast<char*>(path));
        cultistPrecache();
        zombieVoicePrecache();
        rpgPrecache();
        bounce::precache();
    for(auto path:{"models/cremation/vostok/tree_noleaves2.mdl","models/cremation/vostok/jeep-russian.mdl","models/cremation/truth/tree_japan_maple_red.mdl","models/cremation/truth/tree_japan_maple_orange.mdl","models/cremation/truth/bush_japan_bush01.mdl","models/cremation/truth/rock_japan_rocks2.mdl"})engine.pfnPrecacheModel(const_cast<char*>(path));
}
#ifdef CREMATION_ENABLE_VALIDATION
static void enemyCombatProbePrecache();
#endif
static int hookSpawn(edict_t* e) {
    if(valid(e)&&(!std::strcmp(text(e->v.classname),"weapon_deagle")||!std::strcmp(text(e->v.classname),"weapon_egon"))){engine.pfnRemoveEntity(e);return 0;}
    if(valid(e)&&disabledShotgunClass(text(e->v.classname))){engine.pfnRemoveEntity(e);return 0;}
    if(valid(e)&&(!std::strcmp(text(e->v.classname),"weapon_necromancer")||!std::strcmp(text(e->v.classname),"weapon_hornetgun"))){engine.pfnRemoveEntity(e);return 0;}
    if(valid(e)&&!std::strcmp(text(e->v.classname),"weaponbox")){engine.pfnRemoveEntity(e);return 0;}
    if(vostokCoverSpawn(e))return 0;
    if(czIs(e)){int result=raven.pfnSpawn(e);zombieHecuVision(e);czSpawn(e);enemyCombatInstall(e);return result;}
    if(qzIs(e)){int result=raven.pfnSpawn(e);zombieHecuVision(e);qzSpawn(e);enemyCombatInstall(e);return result;}
    const char* classname=text(e->v.classname);
    bool johnnyNpc=classname&&!std::strcmp(classname,"monster_johnny");
    bool maxNpc=classname&&!std::strcmp(classname,"monster_max");
    bool kenNpc=classname&&!std::strcmp(classname,"monster_ken");
    bool randyNpc=randyClass(e);
    bool ricochetItem=classname && (!std::strcmp(classname,"weapon_ricochet")||!std::strcmp(classname,"weapon_9mmhandgun")||!std::strcmp(classname,"weapon_glock"));
    bool bounceItem=bounce::isCarrierClass(classname);
    bool nailItem=classname && (!std::strcmp(classname,"weapon_nailgun")||!std::strcmp(classname,"weapon_9mmAR")||!std::strcmp(classname,"weapon_mp5"));
    bool wrenchItem=classname && (!std::strcmp(classname,"weapon_pipewrench")||!std::strcmp(classname,"weapon_crowbar"));
    bool shockItem=classname&&(!std::strcmp(classname,"weapon_shockrifle")||!std::strcmp(classname,"weapon_gauss"));
    bool minionItem=classname&&!std::strcmp(classname,"monster_minion");
    bool widenZombieVision=zombieVisionClass(classname);
    bool world=classname && !std::strcmp(classname,"worldspawn");
    if(world)extensionPrecache();
    int result=raven.pfnSpawn(e);
    if(widenZombieVision)zombieHecuVision(e);
    if(johnnyNpc)e->v.classname=engine.pfnAllocString("monster_johnny");
    if(maxNpc)e->v.classname=engine.pfnAllocString("monster_max");
    if(kenNpc)e->v.classname=engine.pfnAllocString("monster_ken");
    if(randyNpc&&valid(e)){
        e->v.classname=engine.pfnAllocString("monster_randy");randySetModel(e,RandyModel);
        if(e->pvPrivateData)reinterpret_cast<void(__thiscall*)(void*)>(ravenholm_abi::address(ravenModule,ravenholm_abi::ResetSequenceInfoRva))(e->pvPrivateData);
        startupTrace("RandySpawn","original armored zstin model; native fast-zombie AI");
    }
    if(world)mansionPrecache();
#ifdef CREMATION_ENABLE_VALIDATION
    if(world)enemyCombatProbePrecache();
#endif
    if(world)startupInventoryPrecache();
    if(world)reactorPrecache();
    if(shockItem){e->v.classname=engine.pfnAllocString("weapon_shockrifle");engine.pfnSetModel(e,"models/cremation/w_shockrifle.mdl");}
    if(minionItem){e->v.classname=engine.pfnAllocString("monster_minion");engine.pfnSetModel(e,"models/cremation/minion.mdl");necroInstall(e);}
    if(classname && !std::strcmp(classname,"monster_human_grunt")) {
        startupTrace("HECUAlliance",cremationAllyHecu(e->pvPrivateData)?"zombie_faction=7":"FAILED");
        cultistInstall(e);
    }
    // CGlock::Spawn rewrites its classname to weapon_9mmhandgun. Restore the
    // HUD name so Ravenholm SelectItem can find the actual inventory entity.
    if(ricochetItem)e->v.classname=engine.pfnAllocString("weapon_ricochet");
    // Native CMP5::Spawn canonicalizes the name; restore the selectable HUD name after it.
    if(nailItem){e->v.classname=engine.pfnAllocString("weapon_nailgun");engine.pfnSetModel(e,"models/cremation/w_nailgun.mdl");}
    if(wrenchItem) {e->v.classname=engine.pfnAllocString("weapon_pipewrench");engine.pfnSetModel(e,"models/cremation/w_pipe_wrench.mdl");}
    // The inherited crossbow spawn writes its own view/world models; replace
    // them once it has finished with our own copies.
    if(bounceItem) {e->v.classname=engine.pfnAllocString(bounce::WeaponName);bounce::weaponSpawned(e);}
    if(classname && !std::strcmp(classname,"monster_human_grunt") && e->v.targetname &&
       !std::strncmp(text(e->v.targetname),"cremation_hecu_",15)) {
        int variant=std::atoi(text(e->v.targetname)+15)-1;
        // Preserve the native weapon bodygroup, select one of the four heads.
        e->v.body=(e->v.body/4)*4+(variant%4);
        char report[160];std::snprintf(report,sizeof report,"%s weapons=%d body=%d model=%s health=%.0f",text(e->v.targetname),e->v.weapons,e->v.body,text(e->v.model),e->v.health);
        startupTrace("HECU",report);
    }
    if(disabledShotgunClass(text(e->v.classname))){e->v.flags|=FL_KILLME;e->v.solid=SOLID_NOT;e->v.effects|=EF_NODRAW;}
    humanGibInstall(e);
    weaponBloodInstall(e);
    davidSpawn(e);
    enemyCombatInstall(e);
    return result;
}
#include "judgment_boss.inc"
static void hookSetAbsBox(edict_t* e){if(judgmentIs(e)){judgmentAbsBox(e);return;}if(raven.pfnSetAbsBox)raven.pfnSetAbsBox(e);}
#include "mansion_waves.inc"
#include "mansion_revolver_ammo.inc"
#include "mansion_cz_respawn.inc"
#ifdef CREMATION_ENABLE_VALIDATION
#include "judgment_validation.inc"
#include "hotd2_death_validation.inc"
#include "enemy_combat_validation.inc"
#include "randy_validation.inc"
#include "david_validation.inc"
#endif
#include "mansion_radar.inc"
static const char* startupInventory[]={"item_suit","item_longjump","weapon_357","weapon_ricochet","weapon_pipewrench"};
static void startupInventoryPrecache(){
    // Stations and upstairs grenade pickups remain precached even though they
    // are no longer part of the starting inventory.
    for(auto name:{"weapon_shockrifle","weapon_bounce_shotgun","weapon_nailgun","weapon_rpg","weapon_handgrenade","ammo_357"}){
        auto item=engine.pfnCreateNamedEntity(engine.pfnAllocString(name));if(!valid(item))continue;hookSpawn(item);engine.pfnRemoveEntity(item);
    }
    for(auto name:startupInventory){auto item=engine.pfnCreateNamedEntity(engine.pfnAllocString(name));if(!valid(item))continue;hookSpawn(item);engine.pfnRemoveEntity(item);}
}
static void startupInventoryGive(edict_t* player){
    for(auto name:startupInventory){
        auto item=engine.pfnCreateNamedEntity(engine.pfnAllocString(name));if(!valid(item))continue;
        item->v.origin=player->v.origin;item->v.spawnflags|=1<<30;hookSpawn(item);item->v.effects|=EF_NODRAW;
        if(raven.pfnTouch)raven.pfnTouch(item,player);
        bool attached=valid(item)&&(item->v.owner==player||item->v.aiment==player);
        bool consumed=!valid(item)||item->v.solid==SOLID_NOT;
        char report[160];std::snprintf(report,sizeof report,"%s attached=%d consumed=%d",name,int(attached),int(consumed));startupTrace("StartupInventory",report);
        if(valid(item)&&!attached)engine.pfnRemoveEntity(item);
    }
    if(valid(player)&&engine.pfnClientCommand)engine.pfnClientCommand(player,const_cast<char*>("weapon_ricochet\n"));
}
static void playerDefaults(edict_t* p){
    if(!valid(p)||p->v.health<=0)return;
    if(std::strcmp(engine.pfnGetPhysicsKeyValue(p,"slj"),"1")){
        auto item=engine.pfnCreateNamedEntity(engine.pfnAllocString("item_longjump"));if(valid(item)){item->v.origin=p->v.origin;hookSpawn(item);raven.pfnTouch(item,p);if(valid(item))engine.pfnRemoveEntity(item);}
    }
    engine.pfnSetPhysicsKeyValue(p,"defuse","1");
}
#include "mansion_reactor.inc"
#include "czero_objective.inc"
#include "mansion_navigation.inc"
#include "tactical_hud.inc"
#ifdef CREMATION_ENABLE_VALIDATION
#include "tactical_hud_validation.inc"
#include "reactor_validation.inc"
#include "mansion_rules_validation.inc"
#include "reactor_preview.inc"
#endif
#ifdef CREMATION_ENABLE_VALIDATION
#include "czero_validation.inc"
#include "heart_validation.inc"
#include "czero_animation_validation.inc"
#include "vostok_validation.inc"
#include "truth_validation.inc"
#include "czero_tactical_validation.inc"
#endif
#include "tactical_save.inc"
#ifdef CREMATION_ENABLE_VALIDATION
#include "tactical_save_validation.inc"
#include "dmc_preview.inc"
#endif
static void hookActivate(edict_t* list,int count,int maxClients) {
    weaponRegistryReset();
    tacticalHudNext=0;
    shockStates.fill(ShockState{});shockOrbs.clear();
    anatomicalGibs.clear();humanNativeGibs.clear();humanGibContext=false;
    necroStates.fill(NecroState{});necroFires.clear();incendiaryGrenades.clear();
    rpgCharges.fill(RpgCharge{});rpgMissiles.clear();linkLocks.fill(LinkLock{});linkSkeletons.clear();
    discs.clear();nails.clear();effects.assign(globals->maxEntities,Effect{});players.fill(PlayerState{});lastFrame=globals->time;lastDroppedItemCleanup=globals->time;
    bounce::reset();
    raven.pfnServerActivate(list,count,maxClients);
    startupTrace("ServerActivate",text(globals->mapname));
    mansionActivate();
    mansionCZRespawnActivate();
    reactorActivate();
    mansionRevolverAmmoActivate();
    firstPlayerPostThinkTime=0;
    int ricochetCount=0,bounceCount=0,nailCount=0,wrenchCount=0;
    hordeProbes.clear();hordeReportAt=globals->time+5.f;hordeReportsRemaining=3;hordeStartedAt=globals->time;
    for(int i=0;i<count;++i)if(valid(&list[i])) {
        const char* name=text(list[i].v.classname);
        if(list[i].v.targetname && !std::strncmp(text(list[i].v.targetname),"cremation_zombie_",17)) {
            hordeProbes.push_back({&list[i],list[i].serialnumber,list[i].v.origin,list[i].v.origin,globals->time});
            hordeProbes.back().initialHealth=list[i].v.health;
            char marker[64];std::snprintf(marker,sizeof marker,"cremation_patrol_%02d_0",std::atoi(text(list[i].v.targetname)+17));
            for(int j=0;j<count;++j)if(list[j].v.targetname && !std::strcmp(text(list[j].v.targetname),marker)){hordeProbes.back().patrol=list[j].v.origin;break;}
        }
        if(name && !std::strcmp(name,"weapon_ricochet"))++ricochetCount;
        if(name && !std::strcmp(name,bounce::WeaponName))++bounceCount;
        if(name && !std::strcmp(name,"weapon_nailgun"))++nailCount;
        if(name && !std::strcmp(name,"weapon_pipewrench"))++wrenchCount;
    }
    char inventory[120];std::snprintf(inventory,sizeof(inventory),"ricochet=%d bounce_shotgun=%d nailgun=%d pipewrench=%d",ricochetCount,bounceCount,nailCount,wrenchCount);
    startupTrace("MapWeapons",inventory);
    engine.pfnAlertMessage(at_console,const_cast<char*>("Cremation loaded: Ricochet, Flak Cannon, Nailgun and Opposing Force Pipe Wrench.\n"));
#ifdef CREMATION_ENABLE_VALIDATION
    qzProbeBegin(list,count);
    davidProbeBegin(list,count);
    enemyCombatProbeBegin(list,count);
#endif
}
static void hookDeactivate() {
    for(auto& s:effects) {
        if(s.slowUntil)restoreSlow(s);
        if(s.pushUntil && valid(s.entity) && s.entity->serialnumber==s.serial && s.entity->v.movetype==MOVETYPE_TOSS)s.entity->v.movetype=s.movement;
    }
    effects.clear();discs.clear();nails.clear();rpgCharges.fill(RpgCharge{});rpgMissiles.clear();linkLocks.fill(LinkLock{});linkSkeletons.clear();bounce::reset();
    if(raven.pfnServerDeactivate)raven.pfnServerDeactivate();
}
static void hookSave(edict_t* e,SAVERESTOREDATA* data) {
    // Save the normal appearance/movement. Temporary discs/statuses expire on load.
    auto s=getEffect(e);bool slow=s && s->slowUntil;bool push=s && s->pushUntil;
    int mode=e->v.rendermode,fx=e->v.renderfx,movement=e->v.movetype;float alpha=e->v.renderamt,frame=e->v.framerate;Vector color=e->v.rendercolor;
    if(slow) {e->v.rendermode=s->mode;e->v.renderfx=s->fx;e->v.renderamt=s->alpha;e->v.rendercolor=s->color;e->v.framerate=s->baseFrame;}
    if(push)e->v.movetype=s->movement;
    bool constructionFreeze=bounce::engineerOwnFreeze(e);
    if(constructionFreeze)e->v.flags&=~FL_FROZEN;
    float savedGrenadeDamage=e->v.dmg;
    for(auto& g:incendiaryGrenades)if(g.entity==e&&g.serial==e->serialnumber)e->v.dmg=g.originalDamage;
    raven.pfnSave(e,data);
    davidSave(e,data);
    if(tacticalSaved(e)&&raven.pfnSaveWriteFields)raven.pfnSaveWriteFields(data,"CremationTactical1",&e->v,tacticalFields,ARRAYSIZE(tacticalFields));
    e->v.dmg=savedGrenadeDamage;
    if(constructionFreeze)e->v.flags|=FL_FROZEN;
    e->v.rendermode=mode;e->v.renderfx=fx;e->v.renderamt=alpha;e->v.rendercolor=color;e->v.framerate=frame;e->v.movetype=movement;
}
static int hookRestore(edict_t* e,SAVERESTOREDATA* data,int globalEntity) {
    incendiaryGrenades.erase(std::remove_if(incendiaryGrenades.begin(),incendiaryGrenades.end(),[e](const IncendiaryGrenade& g){return g.entity==e;}),incendiaryGrenades.end());
    bool restoringWorld=engine.pfnIndexOfEdict(e)==0;
    if(restoringWorld)extensionPrecache();
    int result=raven.pfnRestore(e,data,globalEntity);
    davidRestore(e,data);
    if(restoringWorld){mansionPrecache();startupInventoryPrecache();reactorPrecache();}
    if(tacticalSaved(e)&&raven.pfnSaveReadFields)raven.pfnSaveReadFields(data,"CremationTactical1",&e->v,tacticalFields,ARRAYSIZE(tacticalFields));
    const char* restoredClass=text(e->v.classname);
    if(randyClass(e)){
        const char* model=text(e->v.model);
        if(model&&!std::strcmp(model,"models/zombie_fast.mdl"))randySetModel(e,RandyModel);
        else if(model&&!std::strcmp(model,"models/cremation/charred/zombie_fast.mdl"))randySetModel(e,RandyCharred);
        e->v.classname=engine.pfnAllocString("monster_randy");
    }
    if(czIs(e)){czInstall(e);enemyCombatInstall(e);e->v.nextthink=globals->time+.05f;return result;}
    if(qzIs(e)){qzInstall(e);weaponBloodInstall(e);enemyCombatInstall(e);Vector mins(-16,-16,0),maxs(16,16,QZHeight);engine.pfnSetSize(e,&mins.x,&maxs.x);e->v.view_ofs=Vector(0,0,QZEye);e->v.nextthink=globals->time+.1f;return result;}
    if(qzGib(e)){e->v.nextthink=std::max(globals->time+.1f,e->v.dmgtime);return result;}
    if(restoredClass&&!std::strcmp(restoredClass,"monster_minion"))necroInstall(e);
    if(restoredClass&&!std::strcmp(restoredClass,"weapon_hornetgun"))e->v.classname=engine.pfnAllocString("weapon_necromancer");
    if(e->v.targetname&&!std::strcmp(text(e->v.targetname),"cremation_necro_fx"))e->v.flags|=FL_KILLME;
    if(restoredClass && !std::strcmp(restoredClass,"monster_human_grunt")) {cremationAllyHecu(e->pvPrivateData);cultistInstall(e);}
    if(restoredClass && (!std::strcmp(restoredClass,"weapon_9mmhandgun")||!std::strcmp(restoredClass,"weapon_glock")))e->v.classname=engine.pfnAllocString("weapon_ricochet");
    if(restoredClass && (!std::strcmp(restoredClass,"weapon_9mmAR")||!std::strcmp(restoredClass,"weapon_mp5")))e->v.classname=engine.pfnAllocString("weapon_nailgun");
    if(restoredClass&&!std::strcmp(restoredClass,"weapon_awp")){e->v.classname=engine.pfnAllocString("weapon_357");engine.pfnSetModel(e,"models/w_357.mdl");}
    if(restoredClass && !std::strcmp(restoredClass,"weapon_crowbar"))e->v.classname=engine.pfnAllocString("weapon_pipewrench");
    if(restoredClass && !std::strcmp(restoredClass,bounce::CarrierName))e->v.classname=engine.pfnAllocString(bounce::WeaponName);
    if(e->v.targetname&&!std::strcmp(text(e->v.targetname),"cremation_anatomical_gib"))e->v.flags|=FL_KILLME;
    if(e->v.targetname&&!std::strcmp(text(e->v.targetname),DavidGibTag))e->v.flags|=FL_KILLME;
    if(restoredClass&&(!std::strcmp(restoredClass,"weapon_357")||!std::strcmp(restoredClass,"weapon_python")))e->v.classname=engine.pfnAllocString("weapon_357");
    if(restoredClass&&!std::strcmp(restoredClass,"weapon_gauss"))e->v.classname=engine.pfnAllocString("weapon_shockrifle");
    if(e->v.targetname&&!std::strcmp(text(e->v.targetname),"cremation_shock_core"))e->v.flags|=FL_KILLME;
    humanGibInstall(e);
    weaponBloodInstall(e);
    if(auto s=getEffect(e))*s=Effect{};
    if(!std::strcmp(text(e->v.classname),"cremation_disc")) {e->v.flags|=FL_KILLME;e->v.effects|=EF_NODRAW;}
    if(!std::strcmp(text(e->v.classname),"cremation_nail")) {e->v.flags|=FL_KILLME;e->v.effects|=EF_NODRAW;}
    bounce::removeRestored(e);
    enemyCombatInstall(e);
    if(e->v.targetname && (!std::strcmp(text(e->v.targetname),"ut2004_rpg_missile")||!std::strcmp(text(e->v.targetname),"cremation_link_skeleton_fx")||!std::strcmp(text(e->v.targetname),"cremation_link_plasma_fx"))) {e->v.flags|=FL_KILLME;e->v.effects|=EF_NODRAW;}
    int i=engine.pfnIndexOfEdict(e);if(i>0&&i<33) {players[i]=PlayerState{};rpgCharges[i]=RpgCharge{};linkLocks[i]=LinkLock{};}
    return result;
}
static void hookCmdStart(const edict_t* player,const struct usercmd_s* cmd,unsigned int seed) {
    // GoldSrc hands the game DLL the same shared random seed the donor shotgun
    // would use for this usercmd; the pellet pattern is built from it later in
    // PostThink, so the volley stays fixed once fired.
    bounce::cmdStart(const_cast<edict_t*>(player),seed);
    if(raven.pfnCmdStart)raven.pfnCmdStart(player,cmd,seed);
}
static void hookPutInServer(edict_t* player) {
    if(raven.pfnClientPutInServer)raven.pfnClientPutInServer(player);
    startupInventoryGive(player);
    int i=engine.pfnIndexOfEdict(player);if(i>0&&i<33) {rpgCharges[i]=RpgCharge{};rpgCharges[i].startPending=false;}
    startupTrace("ClientPutInServer","player entered map");
    if(engine.pfnClientCommand)engine.pfnClientCommand(player,const_cast<char*>("cl_lw 0\n"));
}
static void hookUpdateClientData(const edict_t* player,int sendWeapons,clientdata_s* data) {
    if(raven.pfnUpdateClientData)raven.pfnUpdateClientData(player,sendWeapons,data);
    // Only advertise actual inventory bits belonging to the playable registry.
    if(data)data->weapons&=weaponRegistryMask|(1u<<31);
    // Public network state only: do not access CBasePlayer's private m_iFOV.
    if(bounce::isBouncePlayer(const_cast<edict_t*>(player)))data->fov=0;
}
static uint32 hookFunctionFromName(const char* name) {
    FARPROC result=GetProcAddress(ravenModule,name);
    return result?uint32(reinterpret_cast<uintptr_t>(result)):engine.pfnFunctionFromName(name);
}
static const char* hookNameForFunction(uint32 address) {
    // Savegame callback names must resolve in the original DLL, including methods.
    auto base=reinterpret_cast<unsigned char*>(ravenModule);
    auto dos=reinterpret_cast<IMAGE_DOS_HEADER*>(base);
    auto nt=reinterpret_cast<IMAGE_NT_HEADERS*>(base+dos->e_lfanew);
    auto directory=nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT];
    auto exports=reinterpret_cast<IMAGE_EXPORT_DIRECTORY*>(base+directory.VirtualAddress);
    auto names=reinterpret_cast<DWORD*>(base+exports->AddressOfNames);
    for(DWORD i=0;i<exports->NumberOfNames;++i) {
        const char* name=reinterpret_cast<const char*>(base+names[i]);
        if(reinterpret_cast<uintptr_t>(GetProcAddress(ravenModule,name))==address)return name;
    }
    return engine.pfnNameForFunction(address);
}

extern "C" void __stdcall GiveFnptrsToDll(enginefuncs_t* incoming,globalvars_t* globalState) {
    engine=*incoming;hookedEngine=engine;globals=globalState;
    if(FILE* file=std::fopen("Cremation/startup-diagnostics.txt","w")) {std::fputs("Cremation DLL initialized\n",file);std::fclose(file);}
    if(!ravenModule) {
        char path[MAX_PATH];GetModuleFileNameA(selfModule,path,MAX_PATH);
        auto slash=std::strrchr(path,'\\');if(slash)slash[1]=0;
        std::strcat(path,"hl_ravenholm.dll");ravenModule=LoadLibraryA(path);
        if(!ravenModule) {engine.pfnAlertMessage(at_error,const_cast<char*>("Cremation cannot load hl_ravenholm.dll\n"));return;}
    }
    for(size_t i=0;i<sizeof(forwardTargets)/sizeof(forwardTargets[0]);++i)forwardTargets[i]=GetProcAddress(ravenModule,forwardNames[i]);
    hookedEngine.pfnWalkMove=hookWalkMove;hookedEngine.pfnMoveToOrigin=hookMoveToOrigin;
    hookedEngine.pfnGetModelPtr=hookModelPtr;
    hookedEngine.pfnSetModel=hookGibSetModel;
    davidOriginalTrace=engine.pfnTraceLine;originalBloodTrace=davidTraceLine;engine.pfnTraceLine=weaponBloodTrace;hookedEngine.pfnTraceLine=weaponBloodTrace;
    hookedEngine.pfnEmitSound=hookEmitSound;
    hookedEngine.pfnRegUserMsg=hookRegisterMsg;hookedEngine.pfnMessageBegin=hookMessageBegin;hookedEngine.pfnMessageEnd=hookMessageEnd;
    hookedEngine.pfnWriteString=hookWriteString;hookedEngine.pfnWriteByte=hookWriteByte;
#ifdef CREMATION_ENABLE_VALIDATION
    hookedEngine.pfnCmd_Argv=weaponProbeArgv;hookedEngine.pfnCmd_Args=weaponProbeArgs;hookedEngine.pfnCmd_Argc=weaponProbeArgc;
#endif
    hookedEngine.pfnFunctionFromName=hookFunctionFromName;hookedEngine.pfnNameForFunction=hookNameForFunction;
    attachBounce();
    ammoOffset=ravenAmmoOffset(ravenModule);
    auto give=reinterpret_cast<void(__stdcall*)(enginefuncs_t*,globalvars_t*)>(GetProcAddress(ravenModule,"GiveFnptrsToDll"));
    give(&hookedEngine,globals);
}
static void installHooks(DLL_FUNCTIONS* table) {
    raven=*table;
    table->pfnSetAbsBox=hookSetAbsBox;
    table->pfnTouch=hookTouch;table->pfnSpawn=hookSpawn;table->pfnThink=hookThink;table->pfnStartFrame=hookStartFrame;
    table->pfnPlayerPreThink=hookPreThink;table->pfnPlayerPostThink=hookPostThink;
    table->pfnClientCommand=hookClientCommand;table->pfnServerActivate=hookActivate;table->pfnServerDeactivate=hookDeactivate;
    table->pfnCmdStart=hookCmdStart;
    table->pfnClientPutInServer=hookPutInServer;
    table->pfnUpdateClientData=hookUpdateClientData;
    table->pfnSave=hookSave;table->pfnRestore=hookRestore;table->pfnGetGameDescription=hookDescription;
}
extern "C" int GetEntityAPI2(DLL_FUNCTIONS* table,int* version) {
    if(!ravenModule)return 0;
    auto get=reinterpret_cast<int(*)(DLL_FUNCTIONS*,int*)>(GetProcAddress(ravenModule,"GetEntityAPI2"));
    if(!get || !get(table,version))return 0;
    installHooks(table);return 1;
}
extern "C" int GetEntityAPI(DLL_FUNCTIONS* table,int version) {return GetEntityAPI2(table,&version);}
extern "C" void weapon_ricochet(entvars_t* vars) {
    auto create=reinterpret_cast<void(*)(entvars_t*)>(GetProcAddress(ravenModule,"weapon_9mmhandgun"));create(vars);
}
extern "C" void cremation_disc(entvars_t* vars) {
    auto create=reinterpret_cast<void(*)(entvars_t*)>(GetProcAddress(ravenModule,"info_target"));create(vars);
}
extern "C" void weapon_bounce_shotgun(entvars_t* vars) {
    // Native carrier: the unused crossbow slot. Inventory identity comes from
    // the map entity's classname, so the item stays independent of the shotgun.
    auto create=reinterpret_cast<void(*)(entvars_t*)>(GetProcAddress(ravenModule,"weapon_crossbow"));create(vars);
}
static void spawnNativeAlias(entvars_t* vars,const char* nativeName,const char* customName) {
    if(!vars||!ravenModule)return;
    auto create=reinterpret_cast<void(*)(entvars_t*)>(GetProcAddress(ravenModule,nativeName));
    if(!create)return;
    vars->classname=engine.pfnAllocString(const_cast<char*>(customName));
    create(vars);
    vars->classname=engine.pfnAllocString(const_cast<char*>(customName));
}
extern "C" void weapon_awp(entvars_t* vars) {spawnNativeAlias(vars,"weapon_python","weapon_357");}
extern "C" void weapon_deagle(entvars_t* vars) {spawnNativeAlias(vars,"weapon_egon","weapon_deagle");}
extern "C" void weapon_nailgun(entvars_t* vars) {
    spawnNativeAlias(vars,"weapon_9mmAR","weapon_nailgun");
}
extern "C" void weapon_pipewrench(entvars_t* vars) {
    spawnNativeAlias(vars,"weapon_crowbar","weapon_pipewrench");

}
extern "C" void cremation_pellet(entvars_t* vars) {
    auto create=reinterpret_cast<void(*)(entvars_t*)>(GetProcAddress(ravenModule,"info_target"));create(vars);
}
extern "C" void cremation_grenade(entvars_t* vars) {
    auto create=reinterpret_cast<void(*)(entvars_t*)>(GetProcAddress(ravenModule,"info_target"));create(vars);
}
extern "C" void cremation_nail(entvars_t* vars) {
    auto create=reinterpret_cast<void(*)(entvars_t*)>(GetProcAddress(ravenModule,"info_target"));create(vars);
}
#ifndef CREMATION_TEST
BOOL WINAPI DllMain(HINSTANCE module,DWORD reason,LPVOID) {if(reason==DLL_PROCESS_ATTACH) {selfModule=module;DisableThreadLibraryCalls(module);}return TRUE;}
#endif

extern "C" void cremation_sentry(entvars_t* vars) {
    auto create=reinterpret_cast<void(*)(entvars_t*)>(GetProcAddress(ravenModule,"monster_sentry"));create(vars);
}
extern "C" void cremation_sentry_base(entvars_t* vars) {
    auto create=reinterpret_cast<void(*)(entvars_t*)>(GetProcAddress(ravenModule,"info_target"));create(vars);
}

extern "C" void weapon_necromancer(entvars_t* vars){spawnNativeAlias(vars,"weapon_hornetgun","weapon_necromancer");}
extern "C" void monster_minion(entvars_t* vars){spawnNativeAlias(vars,"monster_zombie","monster_minion");}
extern "C" void monster_johnny(entvars_t* vars){spawnNativeAlias(vars,"monster_zombie","monster_johnny");}
extern "C" void monster_ken(entvars_t* vars){spawnNativeAlias(vars,"monster_zombie","monster_ken");}
extern "C" void monster_randy(entvars_t* vars){spawnNativeAlias(vars,"monster_fastzombie","monster_randy");}
extern "C" void monster_max(entvars_t* vars){spawnNativeAlias(vars,"monster_zombie","monster_max");}
extern "C" void monster_quake_zombie(entvars_t* vars){spawnNativeAlias(vars,"monster_zombie","monster_quake_zombie");}
extern "C" void monster_czero_zombie(entvars_t* vars){spawnNativeAlias(vars,"monster_human_grunt","monster_czero_zombie");}
extern "C" void quake_zombie_gib(entvars_t* vars){spawnNativeAlias(vars,"info_target","quake_zombie_gib");}

extern "C" void weapon_winchester1887(entvars_t* vars){spawnNativeAlias(vars,"weapon_357","weapon_winchester1887");}

extern "C" void weapon_shockrifle(entvars_t* vars){spawnNativeAlias(vars,"weapon_gauss","weapon_shockrifle");}
extern "C" void ammo_shockcore(entvars_t* vars){spawnNativeAlias(vars,"ammo_gaussclip","ammo_gaussclip");}
