// Opt-in actual-client integration scenario. Fixture repositioning is confined
// to this probe; each entrance/ladder traversal runs the real NPC movement.
static void czTacticalProbeFrame(){
    static bool checked=false,enabled=false;static int stage=0,checks=0;static float began=0;
    static edict_t* npc=nullptr;static edict_t* bomb=nullptr;static edict_t* victim=nullptr;static std::vector<edict_t*> fixtures;
    if(!checked){char env[8]{};GetEnvironmentVariableA("CREMATION_TACTICAL_VALIDATE",env,sizeof env);enabled=env[0]=='1';checked=true;}
    if(!enabled||stage==99||!firstPlayerPostThinkTime||globals->time<firstPlayerPostThinkTime+2)return;
    auto p=engine.pfnPEntityOfEntIndex(1);if(!valid(p))return;
    auto pass=[&](bool ok,const char* name){++checks;startupTrace(ok?"TacticalTest PASS":"TacticalTest FAIL",name);};
    auto spawn=[&](const char* cls,const Vector& at){auto e=engine.pfnCreateNamedEntity(engine.pfnAllocString(cls));if(valid(e)){e->v.origin=at;hookSpawn(e);engine.pfnSetOrigin(e,&at.x);e->v.nextthink=globals->time+1000;e->v.flags|=FL_ONGROUND;}return e;};
    auto place=[&](const Vector& at){engine.pfnSetOrigin(p,&at.x);p->v.velocity=Vector(0,0,0);p->v.movetype=MOVETYPE_NOCLIP;};
    auto clear=[&](){for(auto f:fixtures)if(valid(f))engine.pfnRemoveEntity(f);fixtures.clear();};
    auto startNPC=[&](const Vector& at,int phase,int entrance){npc=spawn(CZClass,at);npc->v.targetname=engine.pfnAllocString("mansion_enemy_tactical");npc->v.weapons=1;npc->v.iuser1=phase;npc->v.iuser2=entrance;npc->v.fuser3=globals->time;npc->v.nextthink=globals->time+.05f;};
    auto state=mansionState();if(valid(state)&&state->v.impulse!=2)state->v.impulse=1;
    if(stage==0){
        pass(!std::strcmp(engine.pfnGetPhysicsKeyValue(p,"slj"),"1"),"longjump_from_start");
        pass(!std::strcmp(engine.pfnGetPhysicsKeyValue(p,"defuse"),"1"),"defuse_kit_from_start");
        pass(linkDps(0)==21&&linkDps(1.5f)==42&&linkDps(3)==84&&linkDps(6)==336,"exponential_damage_doubles_each_1_5_seconds");
        place(Vector(500,-256,92));p->v.health=1000;auto ds=dmcState(p,true);ds->v.fuser1=ds->v.fuser2=ds->v.fuser3=ds->v.fuser4=0;
        victim=spawn("monster_zombie",Vector(800,-256,24));victim->v.health=1000;fixtures.push_back(victim);
        auto t=*reinterpret_cast<void***>(victim->pvPrivateData);dmcGive(p,0);float old=victim->v.health;reinterpret_cast<TakeDamageFn>(t[11])(victim->pvPrivateData,&p->v,&p->v,10,0);
        pass(victim->v.health==old-40,"quad_multiplies_native_monster_damage");ds->v.fuser1=0;
        dmcGive(p,1);auto pt=*reinterpret_cast<void***>(p->pvPrivateData);old=p->v.health;reinterpret_cast<TakeDamageFn>(pt[11])(p->pvPrivateData,&victim->v,&victim->v,20,1<<1);pass(p->v.health==old,"invulnerability_blocks_damage");ds->v.fuser2=0;
        dmcGive(p,3);reinterpret_cast<TakeDamageFn>(pt[11])(p->pvPrivateData,&victim->v,&victim->v,20,1<<20);pass(p->v.health==old,"biosuit_blocks_acid");ds->v.fuser4=0;
        dmcGive(p,2);dmcPlayerFrame(p);pass(dmcInvisible(p)&&(p->v.flags&FL_NOTARGET),"invisibility_hides_from_monsters");ds->v.fuser3=0;dmcPlayerFrame(p);pass(!(p->v.flags&FL_NOTARGET)&&p->v.rendermode==kRenderNormal,"artifact_expiry_restores_player");
        auto closeEnemy=spawn("monster_zombie",Vector(800,-192,24));auto distantEnemy=spawn("monster_zombie",Vector(1100,-256,24));fixtures.push_back(closeEnemy);fixtures.push_back(distantEnemy);closeEnemy->v.health=distantEnemy->v.health=1000;
        linkChain(p,victim,10);pass(closeEnemy->v.health==990&&distantEnemy->v.health==1000,"link_chain_only_hits_nearby_enemy");
        clear();npc=spawn(CZClass,Vector(352,-400,24));fixtures.push_back(npc);npc->v.weapons=1;
        pass(!czThrowSmoke(npc)&&!(npc->v.spawnflags&CZSmokeThrown),"no_smoke_at_spawn_without_cluster");
        for(auto at:{Vector(256,-256,24),Vector(288,-256,24),Vector(256,-320,24),Vector(288,-320,24)})fixtures.push_back(spawn("monster_zombie",at));
        pass(czThrowSmoke(npc)&&!czThrowSmoke(npc),"one_smoke_for_four_zombie_cluster");
        place(Vector(352,-560,60));p->v.v_angle=Vector(0,90,0);
        pass(czThrowFlash(npc,p)&&!czThrowFlash(npc,p),"one_flash_after_visible_player");
        npc->v.targetname=engine.pfnAllocString("mansion_enemy_tactical_melee");npc->v.iuser1=1;npc->v.dmgtime=0;npc->v.pain_finished=0;place(Vector(352,-448,60));float health=p->v.health;
        czObjectiveThink(npc,.05f);pass(p->v.health==health-20&&npc->v.sequence==76,"knife_defends_during_infiltration");
        began=globals->time;stage=1;return;
    }
    if(stage==1){if(globals->time-began<2)return;
        int smoke=0,flash=0;for(int i=1;i<globals->maxEntities;++i){auto e=engine.pfnPEntityOfEntIndex(i);if(!valid(e))continue;if(!std::strcmp(text(e->v.targetname),"cz_smoke_cloud"))++smoke;if(!std::strcmp(text(e->v.targetname),"cz_flash_grenade"))++flash;}
        pass(smoke==1&&flash==0,"both_grenades_detonated_once");clear();
        for(auto at:{Vector(64,-240,56),Vector(96,-240,56),Vector(64,-280,56),Vector(96,-280,56),Vector(-312,-800,48),Vector(-344,-800,48),Vector(-312,-768,56),Vector(-344,-768,56)})fixtures.push_back(spawn("monster_zombie",at));
        npc=spawn(CZClass,Vector(208,-400,24));fixtures.push_back(npc);czChooseEntrance(npc);pass(npc->v.iuser1==6,"crowded_doors_select_vertical_ladder");clear();
        place(Vector(1400,-256,100));startNPC(Vector(192,-400,24),1,0);began=globals->time;stage=2;return;
    }
    if(stage==2||stage==7||stage==8){
        static float report=0;if(globals->time>=report&&valid(npc)){report=globals->time+3;auto& path=czPath(npc);char line[220];std::snprintf(line,sizeof line,"stage=%d phase=%d xyz=%.1f,%.1f,%.1f flags=%d next=%.2f now=%.2f path=%u/%u",stage,npc->v.iuser1,npc->v.origin.x,npc->v.origin.y,npc->v.origin.z,int(npc->v.flags),npc->v.nextthink,globals->time,unsigned(path.cursor),unsigned(path.nodes.size()));startupTrace("TacticalRouteProgress",line);if(path.cursor<path.nodes.size()){Vector a=npc->v.origin+Vector(0,0,36),b=czNavPoint(path.nodes[path.cursor])+Vector(0,0,36);TraceResult tr{};engine.pfnTraceHull(&a.x,&b.x,0,1,npc,&tr);char detail[256];std::snprintf(detail,sizeof detail,"next=%.1f,%.1f,%.1f hit=%s/%d frac=%.2f start=%d mins=%.0f,%.0f,%.0f maxs=%.0f,%.0f,%.0f",b.x,b.y,b.z,valid(tr.pHit)?text(tr.pHit->v.classname):"none",valid(tr.pHit)?engine.pfnIndexOfEdict(tr.pHit):-1,tr.flFraction,tr.fStartSolid,npc->v.mins.x,npc->v.mins.y,npc->v.mins.z,npc->v.maxs.x,npc->v.maxs.y,npc->v.maxs.z);startupTrace("TacticalRouteBlock",detail);}}
        p->v.health=1000;
        if(!valid(npc)||!aliveEnemy(npc)){pass(false,"objective_npc_survives_route");stage=99;startupTrace("TacticalTestComplete");return;}
        if(globals->time-began>40){char row[180];std::snprintf(row,sizeof row,"stage=%d phase=%d at=%.1f,%.1f,%.1f",stage,npc->v.iuser1,npc->v.origin.x,npc->v.origin.y,npc->v.origin.z);startupTrace("TacticalRouteTimeout",row);pass(false,"physical_entrance_route");stage=99;startupTrace("TacticalTestComplete");return;}
        if(npc->v.iuser1!=5)return;
        bomb=npc->v.chain;pass(valid(bomb)&&bomb->v.dmgtime>globals->time,"bomb_planted_after_physical_route");
        pass(!std::strcmp(text(npc->v.model),"models/cremation/czero/zombie_glock.mdl"),"glock_acquired_only_after_planting");
        if(stage==2){pass(npc->v.origin.z<100&&length2d(npc->v.origin-Vector(-304,-272,56))<40,"front_door_enters_ground_hostage_zone");npc->v.nextthink=globals->time+1000;
            place(bomb->v.origin+Vector(40,0,31));began=globals->time;stage=3;return;}
        if(stage==7){pass(npc->v.origin.z<100&&length2d(npc->v.origin-Vector(-304,-272,56))<40,"south_door_enters_ground_hostage_zone");engine.pfnRemoveEntity(npc);engine.pfnRemoveEntity(bomb);
            startNPC(Vector(-328,352,24),6,0);began=globals->time;stage=8;return;}
        pass(npc->v.origin.z>160&&length2d(npc->v.origin-Vector(-624,-176,192))<40,"ladder_enters_upper_hostage_zone");engine.pfnRemoveEntity(npc);bomb->v.dmgtime=globals->time+.5f;dmcGive(p,1);began=globals->time;stage=9;return;
    }
    if(stage>=3&&stage<=6){place(bomb->v.origin+Vector(40,0,31));p->v.health=1000;players[1].buttons|=IN_USE;
        if(stage==3&&globals->time-began>.5f){pass(bomb->v.frags==0,"living_planter_blocks_defuse");auto t=*reinterpret_cast<void***>(npc->pvPrivateData);reinterpret_cast<TakeDamageFn>(t[11])(npc->pvPrivateData,&p->v,&p->v,1000000,1<<13);began=globals->time;stage=4;}
        else if(stage==4&&globals->time-began>1.5f){pass(bomb->v.frags>1,"dead_planter_allows_defuse");players[1].buttons&=~IN_USE;began=globals->time;stage=5;}
        else if(stage==5){players[1].buttons&=~IN_USE;if(globals->time-began>.3f){pass(bomb->v.frags==0,"release_use_resets_defuse");began=globals->time;stage=6;}}
        else if(stage==6&&globals->time-began>5.5f){pass(bomb->v.impulse==1,"five_second_kit_defuse");engine.pfnRemoveEntity(bomb);place(Vector(1400,-256,100));startNPC(Vector(-312,-880,24),1,1);began=globals->time;stage=7;}
        return;
    }
    if(stage==9&&globals->time-began>1){pass(valid(state)&&state->v.impulse==2&&p->v.health<=0,"bomb_explosion_is_game_over_even_with_invulnerability");stage=99;startupTrace("TacticalTestComplete");}
}
