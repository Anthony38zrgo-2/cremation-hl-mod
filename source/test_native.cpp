// Load the actual Win32 server DLL and Ravenholm entity constructor, without
// launching Half-Life, creating a window, or sending input to the user's game.
#include "extdll.h"
#undef min
#undef max
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include "raven_ammo.h"
#include "hecu_alliance.h"

static void* allocated=nullptr;
static void* allocate(edict_t* e,int32 count) {allocated=std::calloc(1,count);e->pvPrivateData=allocated;return allocated;}
static void* privateData(edict_t* e) {return e->pvPrivateData;}
static void alert(ALERT_TYPE,char* format,...) {std::fprintf(stderr,"DLL: %s",format);}

int main(int argc,char** argv) {
    assert(argc==2);
    char path[MAX_PATH];GetFullPathNameA(argv[1],MAX_PATH,path,nullptr);
    HMODULE module=LoadLibraryA(path);
    if(!module){std::fprintf(stderr,"LoadLibrary failed: %lu\n",GetLastError());return 1;}
    auto give=reinterpret_cast<void(__stdcall*)(enginefuncs_t*,globalvars_t*)>(GetProcAddress(module,"GiveFnptrsToDll"));
    auto api=reinterpret_cast<int(*)(DLL_FUNCTIONS*,int*)>(GetProcAddress(module,"GetEntityAPI2"));
    auto zombie=reinterpret_cast<void(*)(entvars_t*)>(GetProcAddress(module,"monster_zombie"));
    assert(give&&api&&zombie&&GetProcAddress(module,"weapon_ricochet")&&GetProcAddress(module,"cremation_disc"));
    enginefuncs_t engine{};globalvars_t state{};
    engine.pfnPvAllocEntPrivateData=allocate;engine.pfnPvEntPrivateData=privateData;engine.pfnAlertMessage=alert;
    give(&engine,&state);
    DLL_FUNCTIONS callbacks{};int version=140;
    assert(api(&callbacks,&version)==1);
    assert(callbacks.pfnSpawn&&callbacks.pfnThink&&callbacks.pfnSave&&callbacks.pfnRestore&&callbacks.pfnStartFrame);
    assert(!std::strcmp(callbacks.pfnGetGameDescription(),"Cremation"));
    edict_t entity;std::memset(&entity,0,sizeof(entity));entity.v.pContainingEntity=&entity;
    zombie(&entity.v);
    assert(entity.pvPrivateData && reinterpret_cast<void**>(entity.pvPrivateData)[1]==&entity.v);
    auto table=*reinterpret_cast<void***>(entity.pvPrivateData);
    using Classify=int(__thiscall*)(void*);
    int classification=reinterpret_cast<Classify>(table[ravenholm_abi::slot::Classify])(entity.pvPrivateData);
    std::printf("Ravenholm zombie classification: %d\n",classification);
    assert(classification==7); // Valve CLASS_ALIEN_MONSTER.
    HMODULE donor=GetModuleHandleA("hl_ravenholm.dll");assert(donor);
    assert(ravenAmmoOffset(donor)==1256);
    std::puts("PASS: player ammo array located from Ravenholm save-field metadata.");
    auto base=reinterpret_cast<unsigned char*>(donor);
    auto dos=reinterpret_cast<IMAGE_DOS_HEADER*>(base);
    auto nt=reinterpret_cast<IMAGE_NT_HEADERS*>(base+dos->e_lfanew);
    auto damageAddress=reinterpret_cast<unsigned char*>(table[ravenholm_abi::slot::TakeDamage]);
    assert(damageAddress>=base && damageAddress<base+nt->OptionalHeader.SizeOfImage);
    auto killedAddress=reinterpret_cast<unsigned char*>(table[ravenholm_abi::slot::Killed]);
    assert(killedAddress>=base && killedAddress<base+nt->OptionalHeader.SizeOfImage);
    std::puts("PASS: zombie Killed virtual slot 13 resolves inside Ravenholm for native GIB_ALWAYS.");
    std::free(allocated);
    auto grunt=reinterpret_cast<void(*)(entvars_t*)>(GetProcAddress(module,"monster_human_grunt"));
    assert(grunt);entity.pvPrivateData=nullptr;grunt(&entity.v);
    table=*reinterpret_cast<void***>(entity.pvPrivateData);
    assert(reinterpret_cast<Classify>(table[ravenholm_abi::slot::Classify])(entity.pvPrivateData)==4);
    assert(cremationAllyHecu(entity.pvPrivateData));
    assert(reinterpret_cast<Classify>(table[ravenholm_abi::slot::Classify])(entity.pvPrivateData)==7);
    std::puts("PASS: native HECU classification changes from military 4 to zombie faction 7.");
    std::free(allocated);
    std::puts("PASS: actual Win32 DLL loads; API 140 and exports resolve; Ravenholm zombie constructor and base ABI match.");
    FreeLibrary(module);
}
