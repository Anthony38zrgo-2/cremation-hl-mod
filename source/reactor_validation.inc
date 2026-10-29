// Explicit dedicated-test process only. No test code activates in normal play.
static void reactorTestFrame(){
    static bool enabled=false,checked=false,clientMode=false;
    static int stage=0;static float began=0,nextMove=0,apex=0;
    static edict_t* player=nullptr;static std::vector<std::pair<edict_t*,int>> monsters;
    static edict_t* linkVictim=nullptr;
    if(!checked){char env[8]{};GetEnvironmentVariableA("CREMATION_REACTOR_VALIDATE",env,sizeof env);clientMode=!std::strcmp(env,"2");enabled=clientMode||!std::strcmp(env,"1");checked=true;}
    if(!enabled||stage==7)return;
    auto pass=[](bool ok,const char* name){startupTrace(ok?"ReactorTest PASS":"ReactorTest FAIL",name);};
    auto location=[](const char* name,float zOffset=0.f){auto marker=mansionFind(name);return valid(marker)?marker->v.origin+Vector(0,0,zOffset):Vector(0,0,0);};
    auto onRoof=[&](const char* name){auto goal=location(name);return length2d(player->v.origin-goal)<100&&std::fabs(player->v.origin.z-(goal.z+31))<24&&(player->v.flags&FL_ONGROUND);};
    if(stage==0){
        if(clientMode){player=engine.pfnPEntityOfEntIndex(1);if(!valid(player)||!(player->v.flags&FL_CLIENT)||player->v.health<=0||!firstPlayerPostThinkTime||globals->time<firstPlayerPostThinkTime+2)return;}
        else{player=engine.pfnCreateFakeClient("ReactorProbe");if(!valid(player)){pass(false,"fake_client");stage=7;return;}
            char reject[128]{};if(raven.pfnClientConnect)raven.pfnClientConnect(player,"ReactorProbe","127.0.0.1",reject);
            raven.pfnClientPutInServer(player);startupInventoryGive(player);}
        // The test window can receive keyboard input while the user works.
        // Limit only test-process wish movement; ballistic velocity stays native.
        engine.pfnSetClientMaxspeed(player,1.f);
        engine.pfnClientCommand(player,const_cast<char*>("-forward;-back;-moveleft;-moveright;-jump;-duck;-attack;-attack2\n"));
        Vector at=location("reactor_jump_pad",37);engine.pfnSetOrigin(player,&at.x);player->v.velocity=Vector(0,0,0);player->v.flags|=FL_ONGROUND;
        began=nextMove=globals->time;apex=64;stage=1;return;
    }
    if(stage==1){
        if(!clientMode&&globals->time>=nextMove){nextMove+=.02f;Vector angle(0,0,0);engine.pfnRunPlayerMove(player,&angle.x,0,0,0,0,0,20);}
        apex=std::max(apex,player->v.origin.z);
        if(globals->time-began<1.7f)return;
        pass(apex>260,"curved_launch_apex");
        pass(onRoof("reactor_jump_destination"),"roof_landing");
        char info[160];std::snprintf(info,sizeof info,"landing=%.1f,%.1f,%.1f apex=%.1f",player->v.origin.x,player->v.origin.y,player->v.origin.z,apex);startupTrace("ReactorTestDetail",info);
        int indices[]={shotgunAmmoIndex,nailAmmoIndex,shockAmmoIndex,rpgAmmoIndex};
        int* ammo=reinterpret_cast<int*>(static_cast<char*>(player->pvPrivateData)+ammoOffset);
        bool grants=true,full=true;const int caps[]={125,250,50,30};
        for(int k=0;k<4;++k){if(indices[k]<0||indices[k]>=32){grants=false;continue;}ammo[indices[k]]=0;grants=reactorGive(player,k)&&ammo[indices[k]]>0&&grants;ammo[indices[k]]=caps[k];full=!reactorGive(player,k)&&full;}
        pass(grants,"four_mod_ammo_rewards");pass(full,"full_ammo_does_not_consume_reward");
        if(shotgunAmmoIndex>=0)ammo[shotgunAmmoIndex]=0;
        auto station=mansionFind("reactor_pickup_0"),display=mansionFind("reactor_display_0");
        if(valid(display)){display->v.impulse=1;engine.pfnSetModel(display,reactorWorldModels[0]);display->v.effects&=~EF_NODRAW;}
        Vector inside=location("reactor_pickup_0",3);engine.pfnSetOrigin(player,&inside.x);player->v.velocity=Vector(0,0,0);stage=2;began=globals->time;return;
    }
    if(stage==2&&globals->time-began>.2f){
        auto station=mansionFind("reactor_pickup_0"),display=mansionFind("reactor_display_0");
        pass(valid(display)&&(display->v.effects&EF_NODRAW)&&valid(station)&&station->v.dmgtime>globals->time+9,"proximity_pickup_and_respawn_timer");
        Vector safe(-448,-584,233);engine.pfnSetOrigin(player,&safe.x);
        if(valid(station))station->v.dmgtime=globals->time;
        stage=3;began=globals->time;return;
    }
    if(stage==3&&globals->time-began>.2f){
        auto display=mansionFind("reactor_display_0");pass(valid(display)&&!(display->v.effects&EF_NODRAW)&&display->v.impulse>=1&&display->v.impulse<=4,"random_reward_respawn");
        pass(reactorGoldSprite>0,"gold_square_particles_precached");
        auto station=mansionFind("reactor_pickup_0");pass(valid(station)&&length2d(station->v.origin-location("reactor_jump_destination"))==48&&!valid(mansionFind("reactor_pickup_1")),"single_reward_near_door");
        Vector portal=location("reactor_return_portal",-4);engine.pfnSetOrigin(player,&portal.x);player->v.velocity=Vector(0,0,0);stage=6;began=globals->time;return;
    }
    if(stage==6&&globals->time-began>.2f){
        pass(length2d(player->v.origin-Vector(-784,-272,61))<100&&player->v.health>0&&player->v.origin.z>55,"return_portal_rear_safe");
        Vector pad=location("reactor_powerup_jump_pad",37);engine.pfnSetOrigin(player,&pad.x);player->v.velocity=Vector(0,0,0);player->v.flags|=FL_ONGROUND;
        reactorJumpCooldown[engine.pfnIndexOfEdict(player)]=0;apex=65;began=globals->time;stage=8;return;
    }
    if(stage==8){
        apex=std::max(apex,player->v.origin.z);if(globals->time-began<1.7f)return;
        pass(apex>260,"mirrored_southwest_jump");
        pass(onRoof("reactor_powerup_jump_destination"),"southwest_roof_landing");
        char landing[180];std::snprintf(landing,sizeof landing,"southwest=%.1f,%.1f,%.1f flags=%d health=%.1f",player->v.origin.x,player->v.origin.y,player->v.origin.z,int(player->v.flags),player->v.health);startupTrace("ReactorTestDetail",landing);
        pass(!reactorGive(player,4),"grenades_excluded_from_station_pool");
        pass(!std::strcmp(engine.pfnGetPhysicsKeyValue(player,"slj"),"1"),"longjump_default_not_in_pickup_pool");
        int* ammo=reinterpret_cast<int*>(static_cast<char*>(player->pvPrivateData)+ammoOffset);ammo[shotgunAmmoIndex]=0;
        auto display=mansionFind("reactor_display_1");if(valid(display)){display->v.impulse=1;display->v.effects&=~EF_NODRAW;engine.pfnSetModel(display,reactorWorldModels[0]);}
        auto station=mansionFind("reactor_powerup_0");if(valid(station))station->v.dmgtime=0;
        player->v.health=50;Vector inside=location("reactor_powerup_0",3);engine.pfnSetOrigin(player,&inside.x);player->v.velocity=Vector(0,0,0);
        stage=9;began=globals->time;return;
    }
    if(stage==9&&globals->time-began>.2f){
        auto display=mansionFind("reactor_display_1"),station=mansionFind("reactor_powerup_0");
        pass(valid(display)&&(display->v.effects&EF_NODRAW)&&sharedAmmo(player,shotgunAmmoIndex,0)>0&&valid(station)&&station->v.dmgtime>globals->time+9,"southwest_weapon_proximity_pickup");
        if(valid(station))station->v.dmgtime=globals->time;
        Vector portal=location("reactor_powerup_portal",-4);engine.pfnSetOrigin(player,&portal.x);player->v.velocity=Vector(0,0,0);reactorPortalCooldown[engine.pfnIndexOfEdict(player)]=0;
        stage=10;began=globals->time;return;
    }
    if(stage==10&&globals->time-began>.2f){
        auto display=mansionFind("reactor_display_1");pass(valid(display)&&!(display->v.effects&EF_NODRAW),"southwest_weapon_respawn");
        pass(length2d(player->v.origin-Vector(-784,-272,61))<100&&player->v.health>0,"powerup_house_return_portal");
        auto ds=dmcState(player);if(valid(ds))ds->v.fuser1=ds->v.fuser2=ds->v.fuser3=ds->v.fuser4=0;dmcPlayerFrame(player);
        linkVictim=engine.pfnCreateNamedEntity(engine.pfnAllocString("monster_zombie"));
        if(valid(linkVictim)){
            linkVictim->v.origin=Vector(900,-256,24.1f);hookSpawn(linkVictim);
            linkVictim->v.health=linkVictim->v.max_health=1000;linkVictim->v.nextthink=globals->time+10;linkVictim->v.movetype=MOVETYPE_NONE;
            engine.pfnSetOrigin(linkVictim,&linkVictim->v.origin.x);
        }
        Vector firing(640,-240,61);engine.pfnSetOrigin(player,&firing.x);player->v.velocity=Vector(0,0,0);
        player->v.v_angle=player->v.angles=player->v.punchangle=Vector(0,0,0);player->v.fixangle=1;
        fireNailgun(player,false);stage=11;began=globals->time;return;
    }
    if(stage==11&&globals->time-began>.5f){
        pass(valid(linkVictim)&&std::fabs(1000-linkVictim->v.health-15.75f)<.05f,"link_primary_damage_15_75");
        for(int ramp=0;ramp<2;++ramp){
            if(valid(linkVictim))linkVictim->v.health=1000;
            auto& state=linkLocks[engine.pfnIndexOfEdict(player)];linkClearGlows(state);state=LinkLock{};
            state.target=linkVictim;state.serial=valid(linkVictim)?linkVictim->serialnumber:0;state.acquired=true;
            state.connectedTime=ramp?6.f:0.f;state.lastTick=globals->time-.1f;state.nextAmmo=globals->time+1;
            linkPostThink(player,true,true);
            pass(valid(linkVictim)&&std::fabs(1000-linkVictim->v.health-(ramp?33.6f:2.1f))<.05f,ramp?"link_beam_maximum_336_dps":"link_beam_initial_21_dps");
            linkClearGlows(state);state=LinkLock{};
        }
        if(valid(linkVictim))engine.pfnRemoveEntity(linkVictim);
        Vector safe(-448,-584,233);engine.pfnSetOrigin(player,&safe.x);player->v.velocity=Vector(0,0,0);
        for(int k=0;k<5;++k){auto e=engine.pfnCreateNamedEntity(engine.pfnAllocString(mansion::classname(k)));if(!valid(e))continue;
            Vector at(496.25f,-800+k*120.f,-350);e->v.origin=at;hookSpawn(e);engine.pfnSetOrigin(e,&at.x);
            e->v.flags&=~FL_ONGROUND;e->v.velocity=Vector(0,0,0);e->v.nextthink=globals->time+10;
            monsters.push_back({e,e->serialnumber});}
        stage=4;began=globals->time;return;
    }
    if(stage==4&&globals->time-began>1.f){
        int trimCount=0;bool nonsolid=true;
        for(int i=1;i<globals->maxEntities;++i){auto e=engine.pfnPEntityOfEntIndex(i);if(!valid(e)||std::strncmp(text(e->v.targetname),"reactor_trim_",13))continue;++trimCount;nonsolid=nonsolid&&e->v.solid==SOLID_NOT;}
        pass(nonsolid&&trimCount==7,"all_seven_glowing_trims_nonsolid");
        bool dead=monsters.size()==5;for(auto pair:monsters)if(valid(pair.first)&&pair.first->serialnumber==pair.second&&aliveEnemy(pair.first))dead=false;
        pass(dead,"bottom_instakills_all_five_enemy_types");
        auto ne=location("reactor_jump_destination"),sw=location("reactor_powerup_jump_destination");
        pass(!reactorInside(ne.x,ne.y)&&!reactorInside(sw.x,sw.y)&&!reactorInside(-448,-584)&&reactorInside(1000,300),"island_and_mansion_safe");
        Vector bottom(1000,200,-370);engine.pfnSetOrigin(player,&bottom.x);player->v.flags|=FL_GODMODE;
        stage=5;began=globals->time;return;
    }
    if(stage==5&&globals->time-began>.2f){pass(player->v.health<=0||player->v.deadflag!=DEAD_NO,"bottom_instakills_player_even_with_godmode");stage=7;startupTrace("ReactorTestComplete");}
}
