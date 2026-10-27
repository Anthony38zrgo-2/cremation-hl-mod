#define CREMATION_TEST
#include "ricochet_proxy.cpp"
#include <cassert>

static edict_t ents[64];
static std::vector<std::string> names{""};
static int name(const char* s){names.emplace_back(s);return int(names.size()-1);}
static const char* lookup(int i){return names.at(i).c_str();}
static edict_t* entity(int i){return i>=0&&i<64?&ents[i]:nullptr;}
static int indexOf(const edict_t* e){return int(e-ents);}
static void emit(edict_t*,int,const char*,float,float,int,int){}
static void model(edict_t* e,const char* s){e->v.model=name(s);}
static void origin(edict_t* e,const float* p){e->v.origin=Vector(p[0],p[1],p[2]);}
static void size(edict_t* e,const float* a,const float* b){e->v.mins=Vector(a[0],a[1],a[2]);e->v.maxs=Vector(b[0],b[1],b[2]);}
static void trace(const float*,const float* to,int,edict_t*,TraceResult* tr){*tr={};tr->flFraction=1;tr->vecEndPos=Vector(to[0],to[1],to[2]);}
static int blocked=0;
static int walk(edict_t*,float,float,int){return blocked?0:1;}
static int spawn(edict_t*){return 0;}
static void* noModel(edict_t*){return nullptr;}
static float randomFloat(float a,float){return a;}
static int randomLong(int a,int){return a;}
static edict_t* create(int cls){for(int i=8;i<64;++i)if(ents[i].free){auto e=&ents[i];*e={};e->v.classname=cls;e->serialnumber=i+100;return e;}return nullptr;}
struct Object{void** table;entvars_t* vars;};
static int __fastcall targetClass(void*,void*){return 11;}
static int __fastcall targetDamage(void* o,void*,entvars_t*,entvars_t*,float amount,int){reinterpret_cast<Object*>(o)->vars->health-=amount;return 1;}
static int __fastcall allyClass(void*,void*){return 7;}

int main(){
    globalvars_t state{};globals=&state;state.time=1;state.maxEntities=64;state.maxClients=1;
    for(auto& e:ents)e.free=true;
    engine.pfnSzFromIndex=lookup;engine.pfnAllocString=name;engine.pfnPEntityOfEntIndex=entity;engine.pfnIndexOfEdict=indexOf;
    engine.pfnEmitSound=emit;engine.pfnSetModel=model;engine.pfnSetOrigin=origin;engine.pfnSetSize=size;
    engine.pfnTraceLine=trace;engine.pfnWalkMove=walk;engine.pfnRandomFloat=randomFloat;engine.pfnRandomLong=randomLong;engine.pfnCreateNamedEntity=create;
    engine.pfnGetModelPtr=noModel;raven.pfnSpawn=spawn;
    std::array<void*,128> base{};Object obj{base.data(),&ents[2].v};
    auto e=&ents[2];e->free=false;e->pvPrivateData=&obj;e->v.pContainingEntity=e;
    qzSpawn(e);assert(qzIs(e)&&e->v.health==60&&e->v.impulse==QZIdle);
    assert(e->v.maxs.z==104&&e->v.view_ofs.z==80);
    assert(obj.table!=base.data()&&base[11]==nullptr); // instance-local table
    assert(reinterpret_cast<int(__thiscall*)(void*)>(obj.table[ravenholm_abi::slot::Classify])(&obj)==7);
    qzInstall(e);assert(base[11]==nullptr);
    qzDamage(&obj,nullptr,nullptr,nullptr,8,(1<<1));assert(e->v.health==60&&e->v.impulse==QZIdle);
    state.time+=.1f;qzDamage(&obj,nullptr,nullptr,nullptr,25,(1<<1));assert(e->v.impulse==QZDown);
    for(int i=0;i<10;++i){state.time+=.1f;qzThink(e);}assert(e->v.button==10&&e->v.solid==SOLID_NOT);
    float hold=state.time;state.time=hold+4.9f;qzThink(e);assert(e->v.button==10);
    blocked=1;state.time=hold+5.1f;qzThink(e);assert(e->v.impulse==QZDown&&e->v.button==10&&e->v.solid==SOLID_NOT);
    blocked=0;state.time+=5.1f;qzThink(e);assert(e->v.impulse==QZRise&&e->v.solid==SOLID_SLIDEBOX);
    for(int i=0;i<20;++i){state.time+=.1f;qzThink(e);}assert(e->v.impulse==QZIdle&&e->v.health==60);
    // Small hits on separate frames cannot accumulate a permanent kill.
    for(int i=0;i<10;++i){state.time+=.1f;qzDamage(&obj,nullptr,nullptr,nullptr,8,(1<<1));}assert(e->v.health==60);
    auto target=&ents[3];target->free=false;target->serialnumber=93;target->v.classname=name("monster_minion");target->v.health=100;
    target->v.takedamage=DAMAGE_AIM;target->v.origin=Vector(200,0,0);target->v.maxs=Vector(16,16,56);
    std::array<void*,128> targetTable{};targetTable[8]=reinterpret_cast<void*>(&targetClass);targetTable[11]=reinterpret_cast<void*>(&targetDamage);
    Object targetObj{targetTable.data(),&target->v};target->pvPrivateData=&targetObj;
    assert(qzEnemy(e)==target);
    qzSequence(e,3,QZAttack);qzThrow(e,target);
    auto gib=&ents[8];assert(qzGib(gib)&&gib->v.dmg==10&&gib->v.dmgtime==state.time+2.5f);
    float flight=length2d(center(target)-gib->v.origin)/600.f;
    float impactZ=gib->v.origin.z+gib->v.velocity.z*flight-400.f*flight*flight;
    assert(std::fabs(impactZ-center(target).z)<.01f);
    qzTouch(gib,e);assert(!(gib->v.flags&FL_KILLME));
    qzTouch(gib,target);assert(target->v.health==90&&(gib->v.flags&FL_KILLME));
    targetTable[8]=reinterpret_cast<void*>(&allyClass);gib->v.flags=0;qzTouch(gib,target);assert(target->v.health==90);
    // Same-frame pellets combine to gib, even though each hit is sublethal.
    state.time+=1;qzDamage(&obj,nullptr,nullptr,nullptr,30,(1<<1));qzDamage(&obj,nullptr,nullptr,nullptr,30,(1<<1));
    assert(e->v.health==0&&e->v.impulse==QZDead&&e->v.deadflag==DEAD_DEAD);
    assert(anatomicalGibs.size()==6);
    for(auto& g:anatomicalGibs)assert(!std::strcmp(text(g.entity->v.model),"models/hgibs.mdl")&&g.expires==state.time+25);
    anatomicalGibs.clear();e->v.effects=0;qzSpawn(e);
    for(int i=0;i<15;++i){state.time+=.1f;qzDamage(&obj,nullptr,nullptr,nullptr,4,(1<<10)|(1<<12));}
    assert(e->v.health==0&&e->v.sequence==12&&anatomicalGibs.empty());
    LinkLock lock{};lock.lastOrigin=e->v.origin;lock.lastAngles=e->v.angles;lock.lastHeight=104;lock.serial=e->serialnumber;
    linkDeathEffect(lock,e);assert(lock.deathShown&&linkSkeletons.size()==1&&(e->v.effects&EF_NODRAW));
    e->v.effects=0;qzSpawn(e);entvars_t plasma{};plasma.classname=name("cremation_nail");
    for(int i=0;i<7;++i){state.time+=.1f;qzDamage(&obj,nullptr,&plasma,nullptr,9,0);}
    assert(e->v.health==0&&anatomicalGibs.size()==6);
    assert(!qzIs(target)&&base[11]==nullptr);
    puts("PASS: independent table, faction, resized hull, pain/recovery, throws, shared gibs, Link beam/plasma cumulative damage and skeleton FX.");
}

