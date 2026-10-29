// One HUD entry per native inventory ID. Buffer WeaponList so removed weapons
// can be omitted entirely, instead of publishing aliases that overwrite IDs.
struct PendingWeaponList {
    int dest=MSG_ONE,count=0;
    edict_t* target=nullptr;
    std::string name;
    std::array<int,8> bytes{};
};
static PendingWeaponList weaponListPending;
static unsigned int weaponRegistryMask=0;
static int revolverAmmoIndex=-1;
static std::array<std::string,32> weaponRegistryNames;
static void weaponRegistryReset(){weaponRegistryMask=0;revolverAmmoIndex=-1;weaponRegistryNames.fill("");weaponListPending={};}
static void weaponRegistrySend(edict_t* target,int dest,const char* name,const int* fields){
    engine.pfnMessageBegin(dest,msgWeaponList,nullptr,target);
    engine.pfnWriteString(name);
    for(int i=0;i<8;++i)engine.pfnWriteByte(fields[i]);
    engine.pfnMessageEnd();
}
static void weaponRegistryPublish(){
    auto& list=weaponListPending;
    if(list.count!=8)return;
    auto fields=list.bytes;
    const char* raw=list.name.c_str();
    const char* name=nullptr;
    int id=fields[6];
    if(id<1||id>=31)return;
    if(!std::strcmp(raw,"weapon_shotgun")){
        shotgunAmmoIndex=fields[0];bounce::setShellAmmoIndex(shotgunAmmoIndex);
        // Crossbow may have been announced before buckshot was known.
        if(weaponRegistryMask&(1u<<bounce::carrierWeaponId())){
            int flak[8];bounce::hudListBytes(flak);
            weaponRegistrySend(list.target,list.dest,bounce::WeaponName,flak);
            if(list.target)bounce::pickedUp(list.target);
        }
        return;
    }
    if(!std::strcmp(raw,"weapon_357")||!std::strcmp(raw,"weapon_python")){
        // Preserve the original Python ammo, flags, ID and native behaviour.
        name="weapon_357";revolverAmmoIndex=fields[0];fields[4]=1;fields[5]=1;
    }else if(!std::strcmp(raw,"weapon_gauss")||!std::strcmp(raw,"weapon_shockrifle")){
        name="weapon_shockrifle";shockAmmoIndex=fields[0];
        fields={-1,50,255,255,3,0,id,0};
    }else if(!std::strcmp(raw,"weapon_9mmhandgun")||!std::strcmp(raw,"weapon_glock")||!std::strcmp(raw,"weapon_ricochet")){
        name="weapon_ricochet";
        fields={balance::HudAmmoCharges,balance::MaxCharges,255,255,1,0,id,0};
    }else if(!std::strcmp(raw,"weapon_9mmAR")||!std::strcmp(raw,"weapon_mp5")||!std::strcmp(raw,"weapon_nailgun")){
        name="weapon_nailgun";nailAmmoIndex=fields[0];
        fields={nailAmmoIndex,250,255,255,2,0,id,0};
    }else if(bounce::isCarrierClass(raw)){
        name=bounce::WeaponName;bounce::hudListBytes(fields.data());fields[6]=id;
    }else if(!std::strcmp(raw,"weapon_crowbar")||!std::strcmp(raw,"weapon_pipewrench")){
        name="weapon_pipewrench";fields[4]=0;fields[5]=0;
    }else if(!std::strcmp(raw,"weapon_rpg")){
        name="weapon_rpg";rpgAmmoIndex=fields[0];fields[1]=30;fields[4]=3;fields[5]=1;
    }else if(!std::strcmp(raw,"weapon_handgrenade")){
        name="weapon_handgrenade";grenadeAmmoIndex=fields[0];fields[4]=4;fields[5]=0;
    }
    if(!name){
        char report[160];std::snprintf(report,sizeof report,"hidden=%s id=%d",raw,id);startupTrace("WeaponRegistry",report);
        return;
    }
    if(!weaponRegistryNames[id].empty()&&weaponRegistryNames[id]!=name){
        startupTrace("WeaponRegistryConflict",name);return;
    }
    weaponRegistryNames[id]=name;weaponRegistryMask|=1u<<id;
    weaponRegistrySend(list.target,list.dest,name,fields.data());
    if(bounce::isCarrierClass(raw)&&list.target)bounce::pickedUp(list.target);
    char report[256];std::snprintf(report,sizeof report,"raw=%s name=%s id=%d group=%d position=%d ammo=%d",raw,name,id,fields[4]+1,fields[5]+1,fields[0]);startupTrace("WeaponRegistry",report);
}
#ifdef CREMATION_ENABLE_VALIDATION
static void mansionRevolverAmmoProbe(edict_t* player);
static const char* weaponProbeCommand=nullptr;
static const char* weaponProbeArgv(int i){return weaponProbeCommand?(i==0?weaponProbeCommand:""):engine.pfnCmd_Argv(i);}
static const char* weaponProbeArgs(){return weaponProbeCommand?"":engine.pfnCmd_Args();}
static int weaponProbeArgc(){return weaponProbeCommand?1:engine.pfnCmd_Argc();}
static void weaponRegistryProbe(edict_t* player){
    raven.pfnPlayerPreThink(player);
    for(int id=1;id<31;++id){
        if(weaponRegistryNames[id].empty()||!(player->v.weapons&(1u<<id)))continue;
        weaponProbeCommand=weaponRegistryNames[id].c_str();
        raven.pfnClientCommand(player);
        weaponProbeCommand=nullptr;
        hookPostThink(player);
        char report[256];std::snprintf(report,sizeof report,"name=%s id=%d owned=%d model=%s",weaponRegistryNames[id].c_str(),id,(player->v.weapons&(1u<<id))?1:0,text(player->v.viewmodel));
        startupTrace("WeaponRegistrySelection",report);
    }
    mansionRevolverAmmoProbe(player);
}
#endif
