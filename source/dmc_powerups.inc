// Valve DMC quake_items.cpp: four 30-second artifacts; native saved timers.
static const char* dmcModels[]={"models/cremation/dmc/pow_quad.mdl","models/cremation/dmc/pow_invuln.mdl","models/cremation/dmc/pow_invis.mdl","models/cremation/dmc/suit.mdl"};
static const char* dmcNames[]={"item_artifact_super_damage","item_artifact_invulnerability","item_artifact_invisibility","item_artifact_envirosuit"};
static const char* dmcSounds[]={"cremation/dmc/damage.wav","cremation/dmc/protect.wav","cremation/dmc/inv1.wav","cremation/dmc/suit.wav"};
static edict_t* dmcState(edict_t* player,bool create=false){
    auto e=engine.pfnFindEntityByString(nullptr,"targetname","dmc_player_state");
    while(valid(e)&&engine.pfnIndexOfEdict(e)>0){if(e->v.owner==player)return e;e=engine.pfnFindEntityByString(e,"targetname","dmc_player_state");}
    if(!create)return nullptr;e=engine.pfnCreateNamedEntity(engine.pfnAllocString("info_target"));if(!valid(e))return nullptr;raven.pfnSpawn(e);
    e->v.targetname=engine.pfnAllocString("dmc_player_state");e->v.owner=player;e->v.solid=SOLID_NOT;e->v.movetype=MOVETYPE_NONE;return e;
}
static float dmcUntil(edict_t* p,int kind){auto s=dmcState(p);if(!valid(s))return 0;return kind==0?s->v.fuser1:kind==1?s->v.fuser2:kind==2?s->v.fuser3:s->v.fuser4;}
static bool dmcInvisible(edict_t* p){return valid(p)&&(p->v.flags&FL_CLIENT)&&dmcUntil(p,2)>globals->time;}
static float dmcDamageScale(entvars_t* a){return a&&(a->flags&FL_CLIENT)&&dmcUntil(engine.pfnFindEntityByVars(a),0)>globals->time?4.f:1.f;}
static void dmcPrecache(){for(auto p:dmcModels)engine.pfnPrecacheModel(const_cast<char*>(p));for(auto p:dmcSounds)engine.pfnPrecacheSound(const_cast<char*>(p));}
static bool dmcGive(edict_t* p,int kind){
    if(!valid(p)||p->v.health<=0||kind<0||kind>3)return false;
    auto s=dmcState(p,true);if(!valid(s))return false;float* timer=kind==0?&s->v.fuser1:kind==1?&s->v.fuser2:kind==2?&s->v.fuser3:&s->v.fuser4;
    *timer=globals->time+30;engine.pfnEmitSound(p,CHAN_ITEM,dmcSounds[kind],1,ATTN_NORM,0,100);startupTrace("DMCPowerup",dmcNames[kind]);return true;
}
struct DMCPlayerTable{void** base;TakeDamageFn damage;};
static std::vector<std::unique_ptr<DMCPlayerTable>> dmcPlayerTables;
static bool dmcPoisonHeadcrabHit(entvars_t* inflictor,entvars_t* attacker){
    for(auto vars:{inflictor,attacker})if(vars){auto e=engine.pfnFindEntityByVars(vars);if(valid(e)&&!std::strcmp(text(e->v.classname),"monster_poison_headcrab"))return true;}
    return false;
}
static int __fastcall dmcPlayerDamage(void* object,void*,entvars_t* inflictor,entvars_t* attacker,float amount,int bits){
    DMCPlayerTable* entry=nullptr;auto table=*reinterpret_cast<void***>(object);
    for(auto& t:dmcPlayerTables)if(t->base==table){entry=t.get();break;}if(!entry)return 0;
    edict_t* victim=nullptr;
    for(int i=1;i<=globals->maxClients;++i){auto p=engine.pfnPEntityOfEntIndex(i);if(!valid(p)||p->pvPrivateData!=object)continue;victim=p;
        // Objective floor and bomb remain lethal, even under artifacts.
        if(amount<99999){if(dmcUntil(p,1)>globals->time)return 0;
            if(dmcUntil(p,3)>globals->time&&(bits&((1<<3)|(1<<14)|(1<<16)|(1<<17)|(1<<18)|(1<<20))))return 0;}break;
    }
    float scaled=amount*dmcDamageScale(attacker)*davidMeleeScale(attacker);
    // A poison headcrab should leave the player at critical health, not kill
    // them. In this single-player-only mod a death issues `reload`, which can
    // dump a player back to the main menu when there is no save to load.
    if(victim&&victim->v.health>1.f&&dmcPoisonHeadcrabHit(inflictor,attacker))scaled=std::min(scaled,victim->v.health-1.f);
    return entry->damage(object,inflictor,attacker,scaled,bits);
}
static void dmcInstallPlayer(edict_t* p){
    if(!valid(p)||!p->pvPrivateData)return;auto base=*reinterpret_cast<void***>(p->pvPrivateData);
    for(auto& t:dmcPlayerTables)if(base==t->base)return;
    constexpr auto damageSlot=ravenholm_abi::slot::TakeDamage;
    DWORD old;if(!VirtualProtect(base+damageSlot,sizeof(void*),PAGE_READWRITE,&old))return;
    auto t=std::make_unique<DMCPlayerTable>();t->base=base;t->damage=reinterpret_cast<TakeDamageFn>(base[damageSlot]);
    dmcPlayerTables.push_back(std::move(t));base[damageSlot]=reinterpret_cast<void*>(&dmcPlayerDamage);DWORD unused;VirtualProtect(base+damageSlot,sizeof(void*),old,&unused);
}
static void dmcPlayerFrame(edict_t* p){
    dmcInstallPlayer(p);bool invis=dmcInvisible(p),quad=dmcUntil(p,0)>globals->time,invul=dmcUntil(p,1)>globals->time;constexpr int owned=1<<26;
    if(p->v.health<=0){auto s=dmcState(p);if(valid(s))s->v.fuser1=s->v.fuser2=s->v.fuser3=s->v.fuser4=0;invis=quad=invul=false;}
    auto state=dmcState(p,invis);
    if(invis&&valid(state)&&!state->v.iuser1){state->v.iuser1=1;state->v.iuser2=(p->v.flags&FL_NOTARGET)?1:0;p->v.flags|=FL_NOTARGET;}
    else if(!invis&&valid(state)&&state->v.iuser1){if(!state->v.iuser2)p->v.flags&=~FL_NOTARGET;state->v.iuser1=0;}
    if(invis||quad||invul){p->v.spawnflags|=owned;p->v.renderfx=kRenderFxGlowShell;p->v.rendermode=invis?kRenderTransColor:kRenderNormal;p->v.renderamt=invis?1:100;
        p->v.rendercolor=quad?(invul?Vector(255,125,255):Vector(128,128,255)):(invul?Vector(255,128,0):Vector(128,128,128));
    }else if(p->v.spawnflags&owned){p->v.spawnflags&=~owned;p->v.renderfx=kRenderFxNone;p->v.rendermode=kRenderNormal;p->v.renderamt=255;}
    if(dmcUntil(p,3)>globals->time)p->v.air_finished=globals->time+12;
}
