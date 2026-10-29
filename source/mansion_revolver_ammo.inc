// Eight native .357 pickups, four on each floor. A saved marker prevents
// duplication or replenishing consumed boxes when a save is loaded.
static void mansionRevolverAmmoActivate(){
    if(!mansionMap())return;
    if(!valid(mansionFind("mansion_revolver_ammo_state"))){
    auto marker=engine.pfnCreateNamedEntity(engine.pfnAllocString("info_target"));
    if(!valid(marker))return;
    hookSpawn(marker);marker->v.targetname=engine.pfnAllocString("mansion_revolver_ammo_state");
    static const Vector spots[]={
        Vector(-400,-650,200),Vector(-600,-400,200),
        Vector(-350,50,200),Vector(-530,150,200),
        Vector(-450,-600,64),Vector(-600,-300,64),
        Vector(-550,110,64),Vector(-500,0,64)
    };
    for(int i=0;i<8;++i){
        auto ammo=engine.pfnCreateNamedEntity(engine.pfnAllocString("ammo_357"));
        if(!valid(ammo))continue;
        ammo->v.origin=spots[i];ammo->v.spawnflags|=1<<30;
        hookSpawn(ammo);
        char name[64];std::snprintf(name,sizeof name,"mansion_revolver_ammo_%02d",i);
        ammo->v.targetname=engine.pfnAllocString(name);
        char report[128];std::snprintf(report,sizeof report,"index=%d floor=%d origin=%.0f,%.0f,%.0f",i,i<4?2:1,spots[i].x,spots[i].y,spots[i].z);
        startupTrace("MansionRevolverAmmo",report);
    }
    }
    if(!valid(mansionFind("mansion_grenade_state"))){
        auto state=engine.pfnCreateNamedEntity(engine.pfnAllocString("info_target"));
        if(!valid(state))return;
        hookSpawn(state);state->v.targetname=engine.pfnAllocString("mansion_grenade_state");
        static const Vector spots[]={Vector(-570,-650,200),Vector(-420,-450,200),Vector(-600,40,200),Vector(-420,150,200)};
        for(int i=0;i<4;++i){
            auto item=engine.pfnCreateNamedEntity(engine.pfnAllocString("weapon_handgrenade"));if(!valid(item))continue;
            char name[48];std::snprintf(name,sizeof name,"mansion_grenade_%d",i);
            item->v.targetname=engine.pfnAllocString(name);item->v.origin=spots[i];item->v.spawnflags|=1<<30;hookSpawn(item);
            startupTrace("MansionGrenade",name);
        }
    }

}
static bool mansionTouchGrenade(edict_t* item,edict_t* player){
    if(!mansionGrenadePickup(item)||!valid(player)||!(player->v.flags&FL_CLIENT))return false;
    if(grenadeAmmoIndex<0)return true;
    int index=std::atoi(text(item->v.targetname)+16);
    int before=sharedAmmo(player,grenadeAmmoIndex,0);
    singleGrenadePickup=true;raven.pfnTouch(item,player);singleGrenadePickup=false;
    int after=sharedAmmo(player,grenadeAmmoIndex,0);
    if(after>before){
        sharedAmmo(player,grenadeAmmoIndex,std::max(0,after-before-1));
        if(valid(item))item->v.targetname=0;
        auto state=mansionFind("mansion_grenade_state");if(valid(state))state->v.frags=float(int(state->v.frags)|(1<<index));
        if(msgAmmoX){engine.pfnMessageBegin(MSG_ONE,msgAmmoX,nullptr,player);engine.pfnWriteByte(grenadeAmmoIndex);engine.pfnWriteByte(before+1);engine.pfnMessageEnd();}
        startupTrace("MansionGrenadePickup","amount=1");
    }
    return true;
}
#ifdef CREMATION_ENABLE_VALIDATION
static void mansionRevolverAmmoProbe(edict_t* player){
    int count=0,upper=0,lower=0;
    edict_t* first=nullptr;
    for(int i=0;i<8;++i){
        char name[64];std::snprintf(name,sizeof name,"mansion_revolver_ammo_%02d",i);
        auto ammo=mansionFind(name);
        if(!valid(ammo)||ammo->v.solid!=SOLID_TRIGGER||(ammo->v.effects&EF_NODRAW))continue;
        if(engine.pfnPointContents(&ammo->v.origin.x)==CONTENTS_SOLID)continue;
        ++count;if(ammo->v.origin.z>150)++upper;else ++lower;
        if(!first)first=ammo;
    }
    sharedAmmo(player,revolverAmmoIndex,6);
    int before=sharedAmmo(player,revolverAmmoIndex,0);
    if(first)raven.pfnTouch(first,player);
    int after=sharedAmmo(player,revolverAmmoIndex,0);
    char report[160];std::snprintf(report,sizeof report,"boxes=%d upper=%d lower=%d before=%d after=%d",count,upper,lower,before,after);
    startupTrace("MansionRevolverAmmoProbe",report);
}
#endif
