// Cremation server extension. Ravenholm keeps ownership of monsters, inventory,
// player state, saves and the shotgun; this module adds the disc weapon and the
// bouncing shotgun, and forwards every inherited export unchanged.
#include "extdll.h"
#undef min
#undef max
#include <algorithm>
#include <array>
#include <vector>
#include <string>
#include <cstring>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include "ricochet_balance.h"
#include "bounce_shotgun.h"
#include "entity_state.h"

// GoldSrc's file-based export reader assumes that export names live in .rdata.
// Reserve space there before linking; finalization never moves names elsewhere.
#pragma section(".rdata",read)
extern "C" __declspec(allocate(".rdata")) const unsigned char cremation_export_reserve[32768] = "CREMATION_EXPORT_RESERVE_BEGIN";

static enginefuncs_t engine{}, hookedEngine{};
static globalvars_t* globals=nullptr;
static DLL_FUNCTIONS raven{};
static HMODULE selfModule=nullptr, ravenModule=nullptr;
#include "forwarders.inc"
static constexpr int SvcTemp=23, SvcWeaponAnim=35, GlockId=2;
static constexpr int Attack=1, Attack2=1<<11, Reload=1<<13;
static int trailSprite=0, flashSprite=0;
static int msgWeaponList=0,msgCurWeapon=0,msgItemPickup=0,msgAmmoX=0,msgSetFov=0;
static int currentMsg=0,currentByte=0,curWeaponId=0;
static bool customWeaponList=false,customBounceList=false,carrierCurWeapon=false;
static edict_t* messagePlayer=nullptr;
static bool ownAnimation=false;
static float lastFrame=0;
static void startupTrace(const char* stage,const char* detail="") {
    if(FILE* file=std::fopen("Cremation/startup-diagnostics.txt","a")) {
        std::fprintf(file,"%s %s\n",stage,detail);std::fclose(file);
    }
}

static const char* text(string_t s) { return engine.pfnSzFromIndex(s); }
static bool valid(edict_t* e) { return e && !e->free && !(e->v.flags&FL_KILLME); }
static bool aliveEnemy(edict_t* e) {
    return valid(e) && (e->v.flags&FL_MONSTER) && e->v.deadflag==DEAD_NO && e->v.health>0;
}
static float length(const Vector& a) { return std::sqrt(a.x*a.x+a.y*a.y+a.z*a.z); }
static float length2d(const Vector& a) { return std::sqrt(a.x*a.x+a.y*a.y); }
static Vector normal(const Vector& a) { float n=length(a); return n>.001f ? a*(1.f/n):Vector(0,0,0); }
static float dot(const Vector&a,const Vector&b) { return a.x*b.x+a.y*b.y+a.z*b.z; }
static Vector center(edict_t* e) { return (e->v.absmin+e->v.absmax)*.5f; }
static bool ricochet(edict_t* e) {
    return valid(e) && e->v.health>0 && e->v.viewmodel && !std::strcmp(text(e->v.viewmodel),"models/v_9mmhandgun.mdl");
}
static void coords(const Vector& p) { engine.pfnWriteCoord(p.x);engine.pfnWriteCoord(p.y);engine.pfnWriteCoord(p.z); }
static void sound(edict_t* e,const char* name) { engine.pfnEmitSound(e,CHAN_WEAPON,name,1,ATTN_NORM,0,100); }

struct Effect {
    edict_t* entity=nullptr;
    int serial=0;
    float slowUntil=0,pushUntil=0,baseFrame=1;
    int mode=0,fx=0,movement=MOVETYPE_STEP;
    float alpha=0;
    Vector color{0,0,0};
};
static std::vector<Effect> effects;
struct PlayerState { int buttons=0; float nextFire=0,animUntil=0; bool wasActive=false; };
static std::array<PlayerState,33> players{};
struct Projectile {
    edict_t* entity=nullptr; int serial=0;
    edict_t* owner=nullptr; int ownerSerial=0;
    Vector velocity{0,0,0}; float born=0,nextSteer=0; int bounces=0;
    bool explosive=false;
};
static std::vector<Projectile> discs;

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
    auto fn=reinterpret_cast<TakeDamageFn>(table[11]);
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
static void steer(Projectile& d) {
    if(d.explosive || d.bounces || globals->time<d.nextSteer)return;
    d.nextSteer=globals->time+.1f;
    edict_t* best=nullptr;float bestDistance=1024;
    Vector forward=normal(d.velocity);
    for(int i=globals->maxClients+1;i<globals->maxEntities;++i) {
        auto e=engine.pfnPEntityOfEntIndex(i);if(!aliveEnemy(e))continue;
        Vector delta=center(e)-d.entity->v.origin;float distance=length(delta);
        if(distance<bestDistance && dot(forward,normal(delta))>.6f && canSee(d.entity->v.origin,e,d.owner)) {best=e;bestDistance=distance;}
    }
    if(best)d.velocity=normal(forward+normal(center(best)-d.entity->v.origin)*.25f)*balance::DiscSpeed;
}
static void advanceDiscs(float dt) {
    for(auto& d:discs) {
        if(!valid(d.entity) || d.entity->serialnumber!=d.serial) {d.entity=nullptr;continue;}
        if(!valid(d.owner) || d.owner->serialnumber!=d.ownerSerial || d.owner->v.health<=0 || globals->time-d.born>balance::DiscLifetime) {removeDisc(d);continue;}
        steer(d);
        float remaining=dt;
        // Bound bounce iterations; trace the entire traveled segment to avoid tunneling.
        for(int sub=0;sub<4 && remaining>0 && d.entity;++sub) {
            Vector from=d.entity->v.origin,to=from+d.velocity*remaining;
            TraceResult tr{};engine.pfnTraceLine(from,to,0,d.owner,&tr);
            if(tr.fStartSolid || tr.fAllSolid) {removeDisc(d);break;}
            if(tr.flFraction>=1) {engine.pfnSetOrigin(d.entity,to);break;}
            Vector at=tr.vecEndPos+tr.vecPlaneNormal*2.f;
            engine.pfnSetOrigin(d.entity,at);
            if(d.explosive) {burst(d,at);removeDisc(d);break;}
            if(aliveEnemy(tr.pHit)) {freeze(tr.pHit);removeDisc(d);break;}
            if(++d.bounces>=3) {removeDisc(d);break;}
            d.velocity=d.velocity-tr.vecPlaneNormal*(2.f*dot(d.velocity,tr.vecPlaneNormal));
            sound(d.entity,"ricochet/bounce.wav");
            remaining*=1.f-tr.flFraction;
        }
        if(d.entity) {d.entity->v.angles.y+=dt*720;d.entity->v.velocity=Vector(0,0,0);}
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
static int bounceSpawn(edict_t* e) { return raven.pfnSpawn ? raven.pfnSpawn(e) : 0; }
static void bounceAnimation(edict_t* p,int sequence) { animation(p,sequence); }
static void bouncePush(edict_t* e,const Vector& dir,float horizontal,float vertical,float duration) {
    applyPush(e,dir,horizontal,vertical,duration);
}
static void attachBounce() {
    bounce::Host host{};
    host.engine=&engine;host.globals=globals;
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
    e->v.rendercolor=explosive?Vector(255,125,35):Vector(50,145,255);
    engine.pfnSetModel(e,explosive?"models/ricochet/disc_hard.mdl":"models/ricochet/disc.mdl");
    Vector mins(-4,-4,-4),maxs(4,4,4);engine.pfnSetSize(e,mins,maxs);engine.pfnSetOrigin(e,origin);
    discs.push_back({e,e->serialnumber,player,player->serialnumber,direction*balance::DiscSpeed,globals->time,0,0,explosive});
    engine.pfnMessageBegin(MSG_BROADCAST,SvcTemp,nullptr,nullptr);
    engine.pfnWriteByte(TE_BEAMFOLLOW);engine.pfnWriteShort(engine.pfnIndexOfEdict(e));engine.pfnWriteShort(trailSprite);
    engine.pfnWriteByte(4);engine.pfnWriteByte(3);
    engine.pfnWriteByte(explosive?255:60);engine.pfnWriteByte(explosive?150:145);engine.pfnWriteByte(explosive?40:255);
    engine.pfnWriteByte(190);engine.pfnMessageEnd();
}
static bool fire(edict_t* p,bool secondary) {
    int needed=secondary?3:1;
    if(discs.size()+needed>balance::MaxProjectiles)return false;
    if(secondary) {launch(p,0,true);launch(p,-balance::TripleSpread,true);launch(p,balance::TripleSpread,true);}
    else launch(p,0,false);
    animation(p,secondary?5:3);
    sound(p,secondary?"ricochet/altfire.wav":"ricochet/throw.wav");
    p->v.punchangle.x-=secondary?2.f:1.f;
    return true;
}
static int hookWalkMove(edict_t* e,float yaw,float distance,int mode) {
    if(pushed(e))distance=0;
    else if(slowed(e))distance*=balance::SlowFactor;
    return engine.pfnWalkMove(e,yaw,distance,mode);
}
static void hookMoveToOrigin(edict_t* e,const float* goal,float distance,int mode) {
    if(pushed(e))distance=0;
    else if(slowed(e))distance*=balance::SlowFactor;
    engine.pfnMoveToOrigin(e,goal,distance,mode);
}
static void hookThink(edict_t* e) {
    auto s=getEffect(e);
    if(s && s->slowUntil && (!aliveEnemy(e)||globals->time>=s->slowUntil))restoreSlow(*s);
    bool active=slowed(e);
    float expected=active?s->baseFrame*balance::SlowFactor:0;
    if(active)e->v.framerate=expected;
    raven.pfnThink(e);
    if(active && aliveEnemy(e)) {
        if(std::fabs(e->v.framerate-expected)>.0001f)s->baseFrame=e->v.framerate;
        e->v.framerate=s->baseFrame*balance::SlowFactor;blue(e);
    }
}
static void hookStartFrame() {
    if(raven.pfnStartFrame)raven.pfnStartFrame();
    float dt=std::max(0.f,globals->time-lastFrame);lastFrame=globals->time;
    if(dt>0) {
        // Process long frames in bounded steps without changing disc travel speed.
        int steps=std::min(32,std::max(1,int(std::ceil(dt/.025f))));
        for(int i=0;i<steps;++i) {
            advanceDiscs(dt/steps);
            bounce::frame(dt/steps);
        }
    }
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
static void hookPreThink(edict_t* p) {
    int i=engine.pfnIndexOfEdict(p);if(i>0 && i<33)players[i].buttons=p->v.button;
    if(ricochet(p)||bounce::isBouncePlayer(p))p->v.button&=~(Attack|Attack2|Reload);
    raven.pfnPlayerPreThink(p);
}
static void hookPostThink(edict_t* p) {
    int i=engine.pfnIndexOfEdict(p);if(i<=0||i>=33) {raven.pfnPlayerPostThink(p);return;}
    auto& state=players[i];
    bool carrier=bounce::isBouncePlayer(p);
    if(ricochet(p)||carrier)p->v.button&=~(Attack|Attack2|Reload);
    raven.pfnPlayerPostThink(p);
    // The bounce shotgun keeps its own clip, so its input is resolved after the
    // inherited inventory logic, with the buttons the player actually pressed.
    if(carrier)bounce::playerPostThink(p,state.buttons);
    bool active=ricochet(p);
    if(active && !state.wasActive) {animation(p,7);state.nextFire=std::max(state.nextFire,globals->time+.45f);state.animUntil=globals->time+.45f;}
    if(active && globals->time>=state.nextFire && (state.buttons&(Attack|Attack2))) {
        bool secondary=(state.buttons&Attack2)!=0;
        if(fire(p,secondary)) {
            state.nextFire=globals->time+(secondary?balance::SecondaryCooldown:balance::PrimaryCooldown);
            state.animUntil=globals->time+.4f;
        }
    }
    if(active && globals->time>=state.animUntil && p->v.weaponanim!=0)animation(p,0);
    state.wasActive=active;
    if(bounce::isBouncePlayer(p))p->v.fov=0;
    // Preserve the engine-visible input after inventory processing is complete.
    p->v.button=state.buttons;
}
static int hookRegisterMsg(const char* name,int size) {
    int id=engine.pfnRegUserMsg(name,size);
    if(!std::strcmp(name,"WeaponList"))msgWeaponList=id;
    if(!std::strcmp(name,"CurWeapon"))msgCurWeapon=id;
    if(!std::strcmp(name,"ItemPickup"))msgItemPickup=id;
    if(!std::strcmp(name,"AmmoX"))msgAmmoX=id;
    if(!std::strcmp(name,"SetFOV"))msgSetFov=id;
    bounce::setMessageIds(msgCurWeapon,msgAmmoX);
    return id;
}
static void hookMessageBegin(int dest,int type,const float* origin,edict_t* player) {
    currentMsg=type;currentByte=0;customWeaponList=false;customBounceList=false;curWeaponId=0;carrierCurWeapon=false;messagePlayer=player;
    engine.pfnMessageBegin(dest,type,origin,player);
}
static void hookMessageEnd() {
    bool bounceList=customBounceList;edict_t* target=messagePlayer;
    engine.pfnMessageEnd();currentMsg=0;messagePlayer=nullptr;
    // The magazine counters live in the weapon module; announce them once the
    // HUD knows the weapon exists.
    if(bounceList && target)bounce::pickedUp(target);
}
static void hookWriteString(const char* value) {
    bool pistol=!std::strcmp(value,"weapon_ricochet")||!std::strcmp(value,"weapon_9mmhandgun")||!std::strcmp(value,"weapon_glock");
    bool carrier=bounce::isCarrierClass(value);
    if(currentMsg==msgWeaponList && pistol) {customWeaponList=true;value="weapon_ricochet";}
    else if(currentMsg==msgWeaponList && carrier) {
        customBounceList=true;
        if(std::strcmp(value,bounce::WeaponName))value=bounce::WeaponName;
    }
    else if(currentMsg==msgItemPickup && pistol)value="weapon_ricochet";
    else if(currentMsg==msgItemPickup && carrier)value=bounce::WeaponName;
    engine.pfnWriteString(value);
}
static void hookWriteByte(int value) {
    if(currentMsg==msgSetFov && currentByte==0 && bounce::isBouncePlayer(messagePlayer))value=0;
    if(currentMsg==msgWeaponList && customWeaponList) {
        static const int fields[]={255,255,255,255,1,0,GlockId,7};
        if(currentByte<8)value=fields[currentByte];
    }
    if(currentMsg==msgWeaponList && customBounceList) {
        static int fields[8];
        if(currentByte==0)bounce::hudListBytes(fields);
        if(currentByte<8)value=fields[currentByte];
    }
    if(currentMsg==msgCurWeapon) {
        if(currentByte==1) {curWeaponId=value;carrierCurWeapon=(value==bounce::carrierWeaponId());}
        if(currentByte==2 && curWeaponId==GlockId)value=255;
        if(currentByte==2 && carrierCurWeapon && valid(messagePlayer))value=bounce::status(messagePlayer).clip;
    }
    if(currentMsg==SvcWeaponAnim && !ownAnimation && currentByte==0) {
        if(ricochet(messagePlayer))value=value==7?7:value==8?6:0;
        else if(bounce::isBouncePlayer(messagePlayer))value=bounce::remapAnimation(value);
    }
    ++currentByte;engine.pfnWriteByte(value);
}
static void hookClientCommand(edict_t* p) {
    // Weapon commands travel as the HUD name, which is the item classname, so
    // the inherited SelectItem resolves them without aliasing.
    raven.pfnClientCommand(p);
}
static const char* hookDescription() {return "Cremation";}
static int hookSpawn(edict_t* e) {
    const char* classname=text(e->v.classname);
    bool ricochetItem=classname && (!std::strcmp(classname,"weapon_ricochet")||!std::strcmp(classname,"weapon_9mmhandgun")||!std::strcmp(classname,"weapon_glock"));
    bool bounceItem=classname && !std::strcmp(classname,bounce::WeaponName);
    bool world=classname && !std::strcmp(classname,"worldspawn");
    if(world) {
        startupTrace("worldspawn");
        // World spawn is the legal precache phase, before clients enter.
        trailSprite=engine.pfnPrecacheModel(const_cast<char*>("sprites/ricochet/lgtning.spr"));
        flashSprite=engine.pfnPrecacheModel(const_cast<char*>("sprites/ricochet/discreturn.spr"));
        for(auto path:{"models/ricochet/disc.mdl","models/ricochet/disc_hard.mdl","models/p_9mmhandgun.mdl"})engine.pfnPrecacheModel(const_cast<char*>(path));
        for(auto path:{"ricochet/throw.wav","ricochet/altfire.wav","ricochet/freeze.wav","ricochet/blast.wav","ricochet/bounce.wav"})engine.pfnPrecacheSound(const_cast<char*>(path));
        bounce::precache();
    }
    int result=raven.pfnSpawn(e);
    // CGlock::Spawn rewrites its classname to weapon_9mmhandgun. Restore the
    // HUD name so Ravenholm SelectItem can find the actual inventory entity.
    if(ricochetItem)e->v.classname=engine.pfnAllocString("weapon_ricochet");
    // The inherited crossbow spawn writes its own view/world models; replace
    // them once it has finished with our own copies.
    if(bounceItem) {e->v.classname=engine.pfnAllocString(bounce::WeaponName);bounce::weaponSpawned(e);}
    return result;
}
static void hookActivate(edict_t* list,int count,int maxClients) {
    discs.clear();effects.assign(globals->maxEntities,Effect{});players.fill(PlayerState{});lastFrame=globals->time;
    bounce::reset();
    raven.pfnServerActivate(list,count,maxClients);
    startupTrace("ServerActivate",text(globals->mapname));
    int ricochetCount=0,bounceCount=0;
    for(int i=0;i<count;++i)if(valid(&list[i])) {
        const char* name=text(list[i].v.classname);
        if(name && !std::strcmp(name,"weapon_ricochet"))++ricochetCount;
        if(name && !std::strcmp(name,bounce::WeaponName))++bounceCount;
    }
    char inventory[80];std::snprintf(inventory,sizeof(inventory),"ricochet=%d bounce_shotgun=%d",ricochetCount,bounceCount);
    startupTrace("MapWeapons",inventory);
    engine.pfnAlertMessage(at_console,const_cast<char*>("Cremation loaded: Ricochet (slot 2) and Bounce Shotgun (slot 3).\n"));
}
static void hookDeactivate() {
    for(auto& s:effects) {
        if(s.slowUntil)restoreSlow(s);
        if(s.pushUntil && valid(s.entity) && s.entity->serialnumber==s.serial && s.entity->v.movetype==MOVETYPE_TOSS)s.entity->v.movetype=s.movement;
    }
    effects.clear();discs.clear();bounce::reset();
    if(raven.pfnServerDeactivate)raven.pfnServerDeactivate();
}
static void hookSave(edict_t* e,SAVERESTOREDATA* data) {
    // Save the normal appearance/movement. Temporary discs/statuses expire on load.
    auto s=getEffect(e);bool slow=s && s->slowUntil;bool push=s && s->pushUntil;
    int mode=e->v.rendermode,fx=e->v.renderfx,movement=e->v.movetype;float alpha=e->v.renderamt,frame=e->v.framerate;Vector color=e->v.rendercolor;
    if(slow) {e->v.rendermode=s->mode;e->v.renderfx=s->fx;e->v.renderamt=s->alpha;e->v.rendercolor=s->color;e->v.framerate=s->baseFrame;}
    if(push)e->v.movetype=s->movement;
    raven.pfnSave(e,data);
    e->v.rendermode=mode;e->v.renderfx=fx;e->v.renderamt=alpha;e->v.rendercolor=color;e->v.framerate=frame;e->v.movetype=movement;
}
static int hookRestore(edict_t* e,SAVERESTOREDATA* data,int globalEntity) {
    int result=raven.pfnRestore(e,data,globalEntity);
    const char* restoredClass=text(e->v.classname);
    if(restoredClass && (!std::strcmp(restoredClass,"weapon_9mmhandgun")||!std::strcmp(restoredClass,"weapon_glock")))e->v.classname=engine.pfnAllocString("weapon_ricochet");
    if(auto s=getEffect(e))*s=Effect{};
    if(!std::strcmp(text(e->v.classname),"cremation_disc")) {e->v.flags|=FL_KILLME;e->v.effects|=EF_NODRAW;}
    bounce::removeRestored(e);
    int i=engine.pfnIndexOfEdict(e);if(i>0&&i<33)players[i]=PlayerState{};
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
    if(engine.pfnClientCommand)engine.pfnClientCommand(player,const_cast<char*>("cl_lw 0\n"));
}
static void hookUpdateClientData(const edict_t* player,int sendWeapons,clientdata_s* data) {
    if(raven.pfnUpdateClientData)raven.pfnUpdateClientData(player,sendWeapons,data);
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
    hookedEngine.pfnRegUserMsg=hookRegisterMsg;hookedEngine.pfnMessageBegin=hookMessageBegin;hookedEngine.pfnMessageEnd=hookMessageEnd;
    hookedEngine.pfnWriteString=hookWriteString;hookedEngine.pfnWriteByte=hookWriteByte;
    hookedEngine.pfnFunctionFromName=hookFunctionFromName;hookedEngine.pfnNameForFunction=hookNameForFunction;
    attachBounce();
    auto give=reinterpret_cast<void(__stdcall*)(enginefuncs_t*,globalvars_t*)>(GetProcAddress(ravenModule,"GiveFnptrsToDll"));
    give(&hookedEngine,globals);
}
static void installHooks(DLL_FUNCTIONS* table) {
    raven=*table;
    table->pfnSpawn=hookSpawn;table->pfnThink=hookThink;table->pfnStartFrame=hookStartFrame;
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
extern "C" void cremation_pellet(entvars_t* vars) {
    auto create=reinterpret_cast<void(*)(entvars_t*)>(GetProcAddress(ravenModule,"info_target"));create(vars);
}
extern "C" void cremation_grenade(entvars_t* vars) {
    auto create=reinterpret_cast<void(*)(entvars_t*)>(GetProcAddress(ravenModule,"info_target"));create(vars);
}
#ifndef CREMATION_TEST
BOOL WINAPI DllMain(HINSTANCE module,DWORD reason,LPVOID) {if(reason==DLL_PROCESS_ATTACH) {selfModule=module;DisableThreadLibraryCalls(module);}return TRUE;}
#endif
