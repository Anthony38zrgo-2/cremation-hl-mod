#define CREMATION_TEST
#include "ricochet_proxy.cpp"
#include <cassert>
#include <cstdio>

static edict_t entities[64];
static std::vector<std::string> strings{""};
static std::vector<int> bytes;
static float walked=0,thinkRate=0,savedRate=0;
static int savedMode=0,savedMovement=0,traceMode=0,seenButtons=0;
static const char* cmd="weapon_ricochet";
static const char* writtenString="";
static int str(const char* s) {strings.emplace_back(s);return int(strings.size()-1);}
static const char* lookup(int i) {return strings.at(i).c_str();}
static int indexOf(const edict_t* e) {return int(e-entities);}
static edict_t* at(int i) {return i>=0&&i<64?&entities[i]:nullptr;}
static void setOrigin(edict_t* e,const float* p) {e->v.origin=Vector(p[0],p[1],p[2]);}
static void setSize(edict_t* e,const float* a,const float* b) {e->v.mins=Vector(a[0],a[1],a[2]);e->v.maxs=Vector(b[0],b[1],b[2]);}
static edict_t* create(int classname) {
    for(int i=8;i<64;++i)if(entities[i].free) {auto e=&entities[i];std::memset(e,0,sizeof(*e));e->serialnumber=i+100;e->v.classname=classname;return e;}
    return nullptr;
}
static void remove(edict_t* e) {e->free=true;}
static bool emulateGlockSpawn=false;
static int spawn(edict_t* e) {
    if(emulateGlockSpawn)e->v.classname=str("weapon_9mmhandgun");
    return 0;
}
static void trace(const float* a,const float* b,int,edict_t*,TraceResult* tr) {
    *tr=TraceResult{};tr->flFraction=1;tr->vecEndPos=Vector(b[0],b[1],b[2]);
    if(traceMode==1 && a[0]<200 && b[0]>=200) {
        tr->flFraction=(200-a[0])/(b[0]-a[0]);tr->pHit=&entities[2];
        tr->vecEndPos=Vector(200,a[1]+(b[1]-a[1])*tr->flFraction,a[2]+(b[2]-a[2])*tr->flFraction);
        tr->vecPlaneNormal=Vector(-1,0,0);
    }
}
static void emit(edict_t*,int,const char*,float,float,int,int){}
static void begin(int,int,const float*,edict_t*) {bytes.clear();}
static void end(){}
static void integer(int i) {bytes.push_back(i);}
static void scalar(float){}
static void stringWrite(const char* value){writtenString=value;}
static void setModel(edict_t* e,const char*) {e->v.modelindex=42;}
static int walk(edict_t*,float,float distance,int) {walked=distance;return 1;}
static void move(edict_t*,const float*,float distance,int) {walked=distance;}
static void think(edict_t* e) {thinkRate=e->v.framerate;hookWalkMove(e,0,100,0);}
static void nothing(edict_t*){}
static void post(edict_t* e) {seenButtons=e->v.button;}
static void save(edict_t* e,SAVERESTOREDATA*) {savedRate=e->v.framerate;savedMode=e->v.rendermode;savedMovement=e->v.movetype;}
static const char* argv(int) {return cmd;}
static const char* args() {return cmd;}
static int commandsSeen=0;
static void command(edict_t*) {++commandsSeen;}
static void playSound(edict_t*,const char*) {}
static void animate(edict_t*,int) {}
static void clientData(const edict_t*,int,clientdata_s* data) {data->fov=45;}
struct FakeMonster {
    edict_t* entity;
    virtual void v0(){} virtual void v1(){} virtual void v2(){} virtual void v3(){}
    virtual void v4(){} virtual void v5(){} virtual void v6(){} virtual void v7(){}
    virtual void v8(){} virtual void v9(){} virtual void v10(){}
    virtual int take(entvars_t*,entvars_t*,float amount,int) {entity->v.health-=amount;return 1;}
};
int main() {
    globalvars_t state{};globals=&state;state.time=1;state.maxClients=1;state.maxEntities=64;
    for(auto& e:entities)e.free=true;
    engine.pfnSzFromIndex=lookup;engine.pfnAllocString=str;engine.pfnIndexOfEdict=indexOf;engine.pfnPEntityOfEntIndex=at;
    engine.pfnCreateNamedEntity=create;engine.pfnRemoveEntity=remove;engine.pfnSetModel=setModel;engine.pfnSetSize=setSize;engine.pfnSetOrigin=setOrigin;
    engine.pfnTraceLine=trace;engine.pfnEmitSound=emit;engine.pfnMessageBegin=begin;engine.pfnMessageEnd=end;
    engine.pfnWriteByte=integer;engine.pfnWriteShort=integer;engine.pfnWriteCoord=scalar;engine.pfnWriteString=stringWrite;
    engine.pfnWalkMove=walk;engine.pfnMoveToOrigin=move;engine.pfnCmd_Argv=argv;engine.pfnCmd_Args=args;
    raven.pfnSpawn=spawn;raven.pfnThink=think;raven.pfnPlayerPreThink=nothing;raven.pfnPlayerPostThink=post;raven.pfnSave=save;raven.pfnClientCommand=command;
    effects.resize(64);
    auto player=&entities[1];std::memset(player,0,sizeof(*player));player->serialnumber=1;player->v.health=100;player->v.view_ofs=Vector(0,0,64);player->v.viewmodel=str("models/v_9mmhandgun.mdl");
    {
        edict_t item;std::memset(&item,0,sizeof(item));item.v.classname=str("weapon_ricochet");
        emulateGlockSpawn=true;hookSpawn(&item);emulateGlockSpawn=false;
        assert(!std::strcmp(text(item.v.classname),"weapon_ricochet"));
    }
    auto enemy=&entities[2];std::memset(enemy,0,sizeof(*enemy));enemy->serialnumber=2;enemy->v.flags=FL_MONSTER;enemy->v.health=50;enemy->v.takedamage=1;
    enemy->v.absmin=Vector(200,-16,0);enemy->v.absmax=Vector(232,16,72);enemy->v.framerate=1;enemy->v.movetype=MOVETYPE_STEP;
    FakeMonster target{};target.entity=enemy;enemy->pvPrivateData=&target;
    assert(fire(player,false));assert(discs.size()==1);assert(std::fabs(length(discs[0].velocity)-1000)<.1f);
    traceMode=1;advanceDiscs(.25f);assert(discs.empty());assert(slowed(enemy));assert(enemy->v.health==50);
    assert(enemy->v.rendermode==kRenderTransTexture && enemy->v.rendercolor.z==255 && enemy->v.renderamt==175);
    hookThink(enemy);assert(std::fabs(walked-25)<.01f && std::fabs(thinkRate-.25f)<.001f);
    hookThink(enemy);assert(std::fabs(enemy->v.framerate-.25f)<.001f); // No cumulative slowdown.
    hookMoveToOrigin(enemy,enemy->v.origin,80,0);assert(walked==20);
    traceMode=0;player->v.punchangle=Vector(0,0,0);assert(fire(player,true));assert(discs.size()==3);
    assert(std::fabs(std::atan2(discs[1].velocity.y,discs[1].velocity.x)*180/3.14159265f+7)<.01f);
    assert(std::fabs(std::atan2(discs[2].velocity.y,discs[2].velocity.x)*180/3.14159265f-7)<.01f);
    for(auto& d:discs)burst(d,Vector(190,0,36));
    assert(enemy->v.health>=38 && enemy->v.health<50);assert(enemy->v.movetype==MOVETYPE_TOSS);
    assert(length2d(enemy->v.velocity)>600 && length2d(enemy->v.velocity)<=1200.1f && enemy->v.velocity.z>=150);
    hookWalkMove(enemy,0,100,0);assert(walked==0);
    hookSave(enemy,nullptr);assert(savedMode==kRenderNormal && savedMovement==MOVETYPE_STEP && savedRate==1);
    assert(enemy->v.rendermode==kRenderTransTexture && enemy->v.movetype==MOVETYPE_TOSS);
    state.time=9;lastFrame=state.time;hookStartFrame();assert(!slowed(enemy));assert(enemy->v.rendermode==kRenderNormal && enemy->v.framerate==1 && enemy->v.movetype==MOVETYPE_STEP);
    for(auto& d:discs)removeDisc(d);discs.clear();
    msgWeaponList=10;hookMessageBegin(MSG_ONE,10,nullptr,player);hookWriteString("weapon_9mmhandgun");for(int i=0;i<8;++i)hookWriteByte(0);hookMessageEnd();
    assert((bytes==std::vector<int>{255,255,255,255,1,0,2,7}));
    hookMessageBegin(MSG_ONE,10,nullptr,player);hookWriteString("weapon_ricochet");
    for(int i=0;i<8;++i)hookWriteByte(0);hookMessageEnd();
    assert((bytes==std::vector<int>{255,255,255,255,1,0,2,7}));
    msgCurWeapon=11;hookMessageBegin(MSG_ONE,11,nullptr,player);hookWriteByte(1);hookWriteByte(2);hookWriteByte(17);hookMessageEnd();assert(bytes[2]==255);
    hookClientCommand(player);assert(commandsSeen==1);
    // Bounce Shotgun wiring: the inherited crossbow inventory is rewritten to
    // the new weapon's own HUD identity, ammo indices and bucket position.
    {
        bounce::Host host{};
        host.engine=&engine;host.globals=globals;host.spawn=spawn;host.sound=playSound;
        host.animation=animate;host.push=[](edict_t*,const Vector&,float,float,float){};
        bounce::attach(host);bounce::setMessageIds(msgCurWeapon,0);
        int hud[8];bounce::hudListBytes(hud);
        msgWeaponList=10;msgItemPickup=12;
        hookMessageBegin(MSG_ONE,10,nullptr,player);
        hookWriteString("weapon_crossbow");
        for(int i=0;i<8;++i)hookWriteByte(0);
        hookMessageEnd();
        assert((bytes==std::vector<int>(hud,hud+8)));
        assert(writtenString=="weapon_bounce_shotgun");
        hookMessageBegin(MSG_ONE,11,nullptr,player);hookWriteByte(1);hookWriteByte(6);hookWriteByte(3);hookMessageEnd();
        assert(bytes[2]==bounce::status(player).clip);
        hookMessageBegin(MSG_ONE,12,nullptr,player);hookWriteString("weapon_crossbow");hookMessageEnd();
        assert(writtenString=="weapon_bounce_shotgun");
        // A real shotgun keeps its own bytes untouched.
        hookMessageBegin(MSG_ONE,10,nullptr,player);hookWriteString("weapon_shotgun");
        for(int i=0;i<8;++i)hookWriteByte(i+1);hookMessageEnd();
        assert((bytes==std::vector<int>{1,2,3,4,5,6,7,8}));
        player->v.viewmodel=str(bounce::ViewModel);
        msgSetFov=13;
        hookMessageBegin(MSG_ONE,13,nullptr,player);hookWriteByte(45);hookMessageEnd();
        assert((bytes==std::vector<int>{0}));
        raven.pfnUpdateClientData=clientData;clientdata_s data{};
        hookUpdateClientData(player,1,&data);assert(data.fov==0);
        player->v.viewmodel=str("models/v_shotgun.mdl");
        hookUpdateClientData(player,1,&data);assert(data.fov==45);
        player->v.viewmodel=str("models/v_9mmhandgun.mdl");
    }
    players.fill(PlayerState{});player->v.button=Attack;hookPreThink(player);hookPostThink(player);assert(seenButtons==0 && discs.empty());
    state.time+=.5f;player->v.button=Attack;hookPreThink(player);hookPostThink(player);assert(discs.size()==1 && seenButtons==0);
    state.time+=.1f;player->v.button=Attack;hookPreThink(player);hookPostThink(player);assert(discs.size()==1);
    player->v.viewmodel=str("models/v_shotgun.mdl");player->v.button=Attack;hookPreThink(player);hookPostThink(player);assert(seenButtons==Attack);
    while(discs.size()<24)launch(player,0,false);assert(!fire(player,true));
    std::puts("PASS: primary, homing hit, 7s slow, movement/animation scaling and blue transparency.");
    std::puts("PASS: 3-disc +/-7deg volley, <=12 damage, strong capped knockback, restoration/save snapshot.");
    std::puts("PASS: slot-2 HUD/selection, pistol suppression, cooldown, projectile cap and shotgun input.");
    std::puts("PASS: Bounce Shotgun HUD bytes, ammo indices, clip rewrite and carrier name rewrite.");
    std::puts("PASS: Ricochet keeps its selectable classname after native Spawn; modified shotgun FOV messages/data cannot zoom.");
}
