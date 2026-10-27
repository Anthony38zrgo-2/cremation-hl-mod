#pragma once
#include <fstream>
#include <sstream>
#include "blood_cultist_assets.h"
struct CultistVoiceState {
    edict_t* entity=nullptr;int serial=0,lastClip=-1;bool deathSpoken=false;
    float busyUntil=0,nextSearch=0;edict_t* enemy=nullptr;int enemySerial=0;
    bool enemyAlive=false;int family=0;
};
static std::vector<CultistVoiceState> cultistVoices;
static std::vector<std::string> cultistSentenceNames;
static int cultistEnemyOffset=-1;
static float cultistNextScan=0,cultistChatterAt=0;
static void* cultistDamageObject=nullptr;
static int cultistDamageBits=0;
using CultistDamageFn=int(__thiscall*)(void*,entvars_t*,entvars_t*,float,int);
struct CultistDamageTable {void** table;CultistDamageFn original;};
static std::vector<CultistDamageTable> cultistDamageTables;
static bool cultistHecu(edict_t* e) {
    return e&&!e->free&&e->v.classname&&!std::strcmp(text(e->v.classname),"monster_human_grunt");
}
static CultistVoiceState* cultistState(edict_t* e) {
    if(!cultistHecu(e))return nullptr;
    int index=engine.pfnIndexOfEdict(e);
    if(index<1||index>=int(cultistVoices.size()))return nullptr;
    auto& state=cultistVoices[index];
    if(state.entity!=e||state.serial!=e->serialnumber) {
        state=CultistVoiceState{};state.entity=e;state.serial=e->serialnumber;
        state.family=(e->v.weapons&8)?0:1;
        state.nextSearch=globals->time+4.f+(index%7);
    }
    return &state;
}
static bool cultistPlay(edict_t* e,CultistEvent event) {
    auto state=cultistState(e);if(!state)return false;
    bool death=event==CultistEvent::Death||(event==CultistEvent::Burning&&e->v.health<=0);
    bool urgent=death||event==CultistEvent::Pain||event==CultistEvent::Burning;
    if(state->deathSpoken)return false;
    if(!death&&globals->time<state->busyUntil) {
        if(!urgent)return false;
        // A hurt cry may interrupt chatter but not another hurt cry.
        if(state->lastClip>=0) {
            auto previous=cultistClips[state->lastClip].event;
            if(previous==CultistEvent::Pain||previous==CultistEvent::Burning)return false;
        }
    }
    std::vector<int> choices;
    for(int i=0;i<int(sizeof(cultistClips)/sizeof(cultistClips[0]));++i)
        if(cultistClips[i].event==event&&cultistClips[i].family==state->family&&i!=state->lastClip)choices.push_back(i);
    if(choices.empty())return false;
    int pick=engine.pfnRandomLong?engine.pfnRandomLong(0,int(choices.size())-1):0;
    int selected=choices[pick];auto& clip=cultistClips[selected];
    engine.pfnEmitSound(e,CHAN_VOICE,clip.path,1,ATTN_NORM,0,100);
    state->busyUntil=globals->time+clip.duration+.15f;state->lastClip=selected;
    state->nextSearch=globals->time+8.f+(engine.pfnIndexOfEdict(e)%7);
    if(death)state->deathSpoken=true;
    return true;
}
static bool cultistGroupEvent(const std::string& name,CultistEvent& event) {
    if(name.compare(0,3,"HG_"))return false;
    if(!name.compare(0,7,"HG_PAIN"))event=CultistEvent::Pain;
    else if(!name.compare(0,6,"HG_DIE"))event=CultistEvent::Death;
    else if(!name.compare(0,7,"HG_BURN"))event=CultistEvent::Burning;
    else if(!name.compare(0,7,"HG_KILL"))event=CultistEvent::Victory;
    else if(!name.compare(0,8,"HG_ALERT")||!name.compare(0,7,"HG_GREN")||
            !name.compare(0,8,"HG_THROW")||!name.compare(0,9,"HG_CHARGE")||
            !name.compare(0,8,"HG_TAUNT")||!name.compare(0,8,"HG_MONST"))event=CultistEvent::Alert;
    else event=CultistEvent::Search;
    return true;
}
static bool cultistVoiceSound(edict_t* e,int channel,const char* sample,int flags) {
    if(!cultistHecu(e)||!sample||(flags&(1<<5)))return false;
    CultistEvent event;
    if(!std::strncmp(sample,"hgrunt/gr_pain",14))event=CultistEvent::Pain;
    else if(!std::strncmp(sample,"hgrunt/gr_die",13))event=CultistEvent::Death;
    else if(sample[0]=='!') {
        std::string sentence=sample+1;
        if(!sentence.empty()&&sentence[0]>='0'&&sentence[0]<='9') {
            int index=std::atoi(sentence.c_str());
            if(index<0||index>=int(cultistSentenceNames.size()))return false;
            sentence=cultistSentenceNames[index];
        }
        if(!cultistGroupEvent(sentence,event))return false;
    } else return false;
    if(cultistDamageObject==e->pvPrivateData&&(cultistDamageBits&((1<<3)|(1<<21))))event=CultistEvent::Burning;
    cultistPlay(e,event);return true; // Suppress the original voice even during cooldown.
}
static int cultistFindEnemyOffset() {
    auto base=reinterpret_cast<unsigned char*>(ravenModule);
    auto nt=reinterpret_cast<IMAGE_NT_HEADERS*>(base+reinterpret_cast<IMAGE_DOS_HEADER*>(base)->e_lfanew);
    size_t size=nt->OptionalHeader.SizeOfImage;auto section=IMAGE_FIRST_SECTION(nt);
    for(int j=0;j<nt->FileHeader.NumberOfSections;++j) {
        if(!(section[j].Characteristics&IMAGE_SCN_MEM_WRITE))continue;
        size_t end=std::min(size,size_t(section[j].VirtualAddress+section[j].Misc.VirtualSize));
        for(size_t i=section[j].VirtualAddress;i+16<=end;i+=4) {
            auto f=reinterpret_cast<const uint32_t*>(base+i);uintptr_t low=reinterpret_cast<uintptr_t>(base),name=f[1];
            if(f[0]==4&&f[3]==1&&f[2]<4096&&name>=low&&name+9<low+size&&
               !std::memcmp(reinterpret_cast<void*>(name),"m_hEnemy",9))return int(f[2]);
        }
    }
    return -1;
}
static edict_t* cultistEnemy(edict_t* e) {
    if(!e->pvPrivateData||cultistEnemyOffset<0)return nullptr;
    auto bytes=static_cast<char*>(e->pvPrivateData)+cultistEnemyOffset;
    auto enemy=*reinterpret_cast<edict_t**>(bytes);int serial=*reinterpret_cast<int*>(bytes+4);
    uintptr_t start=reinterpret_cast<uintptr_t>(engine.pfnPEntityOfEntIndex(0));
    uintptr_t pointer=reinterpret_cast<uintptr_t>(enemy);
    if(pointer<start||pointer>=start+sizeof(edict_t)*globals->maxEntities||(pointer-start)%sizeof(edict_t))return nullptr;
    return valid(enemy)&&enemy->serialnumber==serial?enemy:nullptr;
}
static int __fastcall cultistTakeDamage(void* object,void*,entvars_t* inflictor,entvars_t* attacker,float amount,int bits) {
    if(enemyCombatBlocked(object,inflictor,attacker))return 0;
    auto table=*reinterpret_cast<void***>(object);CultistDamageFn original=nullptr;
    for(auto& installed:cultistDamageTables)if(installed.table==table) {original=installed.original;break;}
    if(!original)return 0;
    edict_t* entity=nullptr;
    for(auto& state:cultistVoices)if(valid(state.entity)&&state.serial==state.entity->serialnumber&&state.entity->pvPrivateData==object) {entity=state.entity;break;}
    float health=entity?entity->v.health:0;
    auto oldObject=cultistDamageObject;int oldBits=cultistDamageBits;
    cultistDamageObject=object;cultistDamageBits=bits;
    int result=original(object,inflictor,attacker,amount,bits);
    if(entity&&!entity->free&&health>0&&entity->v.health<health) {
        CultistEvent event=(bits&((1<<3)|(1<<21)))?CultistEvent::Burning:
            (entity->v.health<=0?CultistEvent::Death:CultistEvent::Pain);
        cultistPlay(entity,event);
    }
    cultistDamageObject=oldObject;cultistDamageBits=oldBits;return result;
}
static void cultistInstall(edict_t* e) {
    if(!cultistState(e)||!e->pvPrivateData)return;
    auto table=*reinterpret_cast<void***>(e->pvPrivateData);
    for(auto& installed:cultistDamageTables)if(installed.table==table)return;
    auto original=reinterpret_cast<CultistDamageFn>(table[ravenholm_abi::slot::TakeDamage]);DWORD protection=0;
    if(VirtualProtect(&table[ravenholm_abi::slot::TakeDamage],sizeof(void*),PAGE_READWRITE,&protection)) {
        cultistDamageTables.push_back({table,original});table[ravenholm_abi::slot::TakeDamage]=reinterpret_cast<void*>(&cultistTakeDamage);
        DWORD ignored=0;VirtualProtect(&table[ravenholm_abi::slot::TakeDamage],sizeof(void*),protection,&ignored);
    }
}
static void cultistPrecache() {
    cultistVoices.assign(globals->maxEntities,CultistVoiceState{});cultistSentenceNames.clear();
    cultistEnemyOffset=cultistFindEnemyOffset();cultistNextScan=0;cultistChatterAt=0;
    for(auto& clip:cultistClips)engine.pfnPrecacheSound(const_cast<char*>(clip.path));
    char directory[MAX_PATH]{};engine.pfnGetGameDir(directory);
    std::ifstream file(std::string(directory)+"/sound/sentences.txt");std::string line;
    while(std::getline(file,line)) {
        std::istringstream stream(line);std::string name;stream>>name;
        if(!name.empty()&&((name[0]>='A'&&name[0]<='Z')||(name[0]>='a'&&name[0]<='z')))cultistSentenceNames.push_back(name);
    }
    char report[128];std::snprintf(report,sizeof report,"clips=%d sentences=%d enemy_offset=%d",int(sizeof(cultistClips)/sizeof(cultistClips[0])),int(cultistSentenceNames.size()),cultistEnemyOffset);
    startupTrace("BloodCultistVoices",report);
}
static void cultistFrame() {
    if(globals->time<cultistNextScan)return;cultistNextScan=globals->time+.2f;
    for(auto& state:cultistVoices) {
        auto e=state.entity;
        if(!cultistHecu(e)||e->serialnumber!=state.serial||e->v.deadflag!=DEAD_NO||e->v.health<=0)continue;
        auto enemy=cultistEnemy(e);bool alive=valid(enemy)&&enemy->v.health>0&&enemy->v.deadflag==DEAD_NO;
        if(state.enemyAlive&&state.enemy&&valid(state.enemy)&&state.enemy->serialnumber==state.enemySerial&&state.enemy->v.health<=0) {
            if(!cultistPlay(e,CultistEvent::Victory))continue;
        } else if(alive&&(enemy!=state.enemy||enemy->serialnumber!=state.enemySerial))cultistPlay(e,CultistEvent::Alert);
        state.enemy=enemy;state.enemySerial=enemy?enemy->serialnumber:0;state.enemyAlive=alive;
        if(!alive&&globals->time>=state.nextSearch&&globals->time>=cultistChatterAt) {
            // Search/idle chatter, not a kill taunt while the target is alive.
            if(cultistPlay(e,CultistEvent::Search))cultistChatterAt=globals->time+2.f;
        }
    }
}
