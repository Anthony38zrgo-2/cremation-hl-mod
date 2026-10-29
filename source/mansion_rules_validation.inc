// Dedicated-process regression checks for the mansion pickup/respawn rules.
static void mansionRulesProbeFrame(){
    static bool checked=false,enabled=false;
    static int stage=0;
    static float began=0,pickedAt=0;
    static edict_t* player=nullptr;
    static std::vector<edict_t*> horde;
    if(!checked){char value[8]{};GetEnvironmentVariableA("CREMATION_RULES_VALIDATE",value,sizeof value);enabled=!std::strcmp(value,"1");checked=true;}
    if(!enabled||stage==99||!mansionMap())return;
    auto pass=[](bool ok,const char* name){startupTrace(ok?"MansionRules PASS":"MansionRules FAIL",name);};
    auto countSupplies=[](bool grenades){int count=0;for(int i=1;i<globals->maxEntities;++i){auto e=engine.pfnPEntityOfEntIndex(i);if(!valid(e)||!e->v.targetname)continue;const char* n=text(e->v.targetname);if(grenades?mansionGrenadePickup(e):(!std::strncmp(n,"mansion_revolver_ammo_",22)&&!std::strcmp(text(e->v.classname),"ammo_357")))++count;}return count;};
    auto aliveCZ=[](){edict_t* result=nullptr;for(int i=1;i<globals->maxEntities;++i){auto e=engine.pfnPEntityOfEntIndex(i);if(mansionActor(e)&&czIs(e)&&aliveEnemy(e)){e->v.nextthink=globals->time+60;result=e;}}return result;};
    auto wave=mansionState();if(!valid(wave))return;
    if(stage==0){
        wave->v.impulse=2;
        player=engine.pfnCreateFakeClient("MansionRules");if(!valid(player)){pass(false,"fake_client");stage=99;return;}
        char reject[128]{};raven.pfnClientConnect(player,"MansionRules","127.0.0.1",reject);raven.pfnClientPutInServer(player);startupInventoryGive(player);raven.pfnPlayerPreThink(player);
        player->v.flags|=FL_GODMODE|FL_NOTARGET;
        unsigned expected=(1u<<1)|(1u<<2)|(1u<<3)|(1u<<31);
        pass(unsigned(player->v.weapons)==expected,"only_three_starting_weapons");
        pass(countSupplies(false)==8&&countSupplies(true)==4,"eight_ammo_and_four_grenades");
        mansionRevolverAmmoActivate();
        pass(countSupplies(false)==8&&countSupplies(true)==4,"uncollected_supplies_not_duplicated");
        auto ammo=mansionFind("mansion_revolver_ammo_00");if(valid(ammo))hookTouch(ammo,player);
        auto grenade=mansionFind("mansion_grenade_0");int before=sharedAmmo(player,grenadeAmmoIndex,0);if(valid(grenade))hookTouch(grenade,player);
        pass(sharedAmmo(player,grenadeAmmoIndex,0)==before+1,"each_grenade_gives_exactly_one");
        began=globals->time;stage=1;return;
    }
    if(stage==1&&globals->time-began>.6f){
        mansionRevolverAmmoActivate();pass(countSupplies(false)==7&&countSupplies(true)==3,"collected_supplies_stay_collected");
        int indices[]={shotgunAmmoIndex,nailAmmoIndex,shockAmmoIndex,rpgAmmoIndex};
        bool grants=true;for(int i=0;i<4;++i){int* ammo=reinterpret_cast<int*>(static_cast<char*>(player->pvPrivateData)+ammoOffset);ammo[indices[i]]=0;grants=reactorGive(player,i)&&grants;}
        pass(grants&&!reactorGive(player,4),"four_weapons_only_in_reward_pool");
        bool timers=true;
        for(int i=0;i<2;++i){
            auto station=mansionFind(i?"reactor_powerup_0":"reactor_pickup_0");char name[40];std::snprintf(name,sizeof name,"reactor_display_%d",i);auto display=mansionFind(name);
            if(!valid(station)||!valid(display)){timers=false;continue;}
            display->v.impulse=i+1;display->v.effects&=~EF_NODRAW;engine.pfnSetModel(display,reactorWorldModels[i]);
            int* ammo=reinterpret_cast<int*>(static_cast<char*>(player->pvPrivateData)+ammoOffset);ammo[indices[i]]=0;
            Vector at=station->v.origin;engine.pfnSetOrigin(player,&at.x);reactorFrame();
            timers=timers&&(display->v.effects&EF_NODRAW)&&std::fabs(station->v.dmgtime-globals->time-10.f)<.05f;
        }
        Vector safe(-448,-584,233);engine.pfnSetOrigin(player,&safe.x);pickedAt=globals->time;
        pass(timers,"both_stations_have_ten_second_cooldown");
        for(int i=0;i<6;++i){
            auto e=engine.pfnCreateNamedEntity(engine.pfnAllocString("monster_zombie"));if(!valid(e))continue;
            e->v.origin=Vector(-1500,-2000+i*100.f,24);hookSpawn(e);e->v.nextthink=globals->time+60;e->v.movetype=MOVETYPE_NONE;
            char name[48];std::snprintf(name,sizeof name,"mansion_enemy_rules_%d",i);e->v.targetname=engine.pfnAllocString(name);horde.push_back(e);
        }
        wave->v.impulse=0;wave->v.frags=1;wave->v.armorvalue=40;mansionAliveAt=-1;
        bool spawned=mansionEmit(wave);auto npc=aliveCZ();
        if(valid(npc)){auto table=*reinterpret_cast<void***>(npc->pvPrivateData);reinterpret_cast<TakeDamageFn>(table[ravenholm_abi::slot::TakeDamage])(npc->pvPrivateData,&player->v,&player->v,1000.f,1<<6);}
        auto queue=mansionCZRespawnState();pass(spawned&&valid(queue)&&queue->v.impulse==1,"tactical_zombie_death_queues_revival");
        wave->v.impulse=2;began=globals->time;stage=2;return;
    }
    if(stage==2){
        wave->v.impulse=0;mansionCZRespawnFrame();wave->v.impulse=2;
        if(globals->time-began<3.15f)return;
        auto npc=aliveCZ();pass(valid(npc)&&wave->v.armorvalue==41,"tactical_zombie_revives_without_advancing_wave");
        if(!horde.empty()&&valid(horde.back()))engine.pfnRemoveEntity(horde.back());
        mansionAliveAt=-1;wave->v.impulse=0;
        if(valid(npc)){auto table=*reinterpret_cast<void***>(npc->pvPrivateData);reinterpret_cast<TakeDamageFn>(table[ravenholm_abi::slot::TakeDamage])(npc->pvPrivateData,&player->v,&player->v,1000.f,1<<6);}
        auto queue=mansionCZRespawnState();pass(valid(queue)&&queue->v.impulse==2&&mansionAlive()==5,"revival_stops_at_five_remaining_enemies");
        wave->v.impulse=2;began=globals->time;stage=3;return;
    }
    if(stage==3&&globals->time-began>3.3f){
        wave->v.impulse=0;mansionCZRespawnFrame();wave->v.impulse=2;
        pass(!valid(aliveCZ()),"no_revival_after_threshold");stage=4;
    }
    if(stage==4&&globals->time-pickedAt>9.f){
        bool hidden=true;for(int i=0;i<2;++i){char n[40];std::snprintf(n,sizeof n,"reactor_display_%d",i);auto d=mansionFind(n);hidden=hidden&&valid(d)&&(d->v.effects&EF_NODRAW);}
        pass(hidden,"stations_do_not_respawn_before_ten_seconds");stage=5;
    }
    if(stage==5&&globals->time-pickedAt>10.1f){
        bool visible=true;for(int i=0;i<2;++i){char n[40];std::snprintf(n,sizeof n,"reactor_display_%d",i);auto d=mansionFind(n);visible=visible&&valid(d)&&!(d->v.effects&EF_NODRAW)&&d->v.impulse>=1&&d->v.impulse<=4;if(valid(d)&&d->v.impulse>=1&&d->v.impulse<=4)visible=visible&&!std::strcmp(text(d->v.model),reactorWorldModels[d->v.impulse-1]);}
        pass(visible,"both_stations_respawn_weapons_after_ten_seconds");stage=99;startupTrace("MansionRulesComplete");
    }
}
