// Actual-engine regression probe, enabled only in an explicit test process.
static void czProbeFrame(){
    static bool checked=false,enabled=false;static int stage=0;static float began=0;
    static edict_t* npc=nullptr;static std::vector<edict_t*> smokers;
    if(!checked){char env[8]{};GetEnvironmentVariableA("CREMATION_CZERO_VALIDATE",env,sizeof env);enabled=env[0]=='1';checked=true;}
    if(!enabled||stage==5||!firstPlayerPostThinkTime||globals->time<firstPlayerPostThinkTime+2)return;
    auto p=engine.pfnPEntityOfEntIndex(1);if(!valid(p)||p->v.health<=0)return;
    auto pass=[](bool ok,const char* name){startupTrace(ok?"CZTest PASS":"CZTest FAIL",name);};
    auto create=[](const Vector& at){auto e=engine.pfnCreateNamedEntity(engine.pfnAllocString(CZClass));if(valid(e)){e->v.origin=at;hookSpawn(e);engine.pfnSetOrigin(e,&at.x);e->v.flags|=FL_ONGROUND;}return e;};
    if(stage==1||stage==2){Vector camera(740,-380,80);engine.pfnSetOrigin(p,&camera.x);p->v.velocity=Vector(0,0,0);p->v.v_angle=p->v.angles=Vector(15,38,0);p->v.fixangle=1;}
    if(stage==0){
        auto controller=mansionState();if(valid(controller))controller->v.impulse=1;
        npc=create(Vector(900,-256,24.1f));pass(czIs(npc)&&aliveEnemy(npc),"native_monster_spawn");if(!valid(npc)){stage=5;return;}
        auto h=static_cast<unsigned char*>(engine.pfnGetModelPtr(npc));pass(h&&*reinterpret_cast<int*>(h+140)==34&&*reinterpret_cast<int*>(h+156)==20,"first_folder_body_hitboxes");
        auto table=*reinterpret_cast<void***>(npc->pvPrivateData);pass(reinterpret_cast<int(__thiscall*)(void*)>(table[ravenholm_abi::slot::Classify])(npc->pvPrivateData)==7,"zombie_alliance");
        pass(linkEnemy(npc)&&humanoidRemains(npc),"link_and_anatomical_effect_eligibility");
        Vector camera(740,-380,80);engine.pfnSetOrigin(p,&camera.x);p->v.velocity=Vector(0,0,0);p->v.movetype=MOVETYPE_NOCLIP;
        p->v.v_angle=p->v.angles=Vector(8,38,0);p->v.fixangle=1;p->v.health=1000;p->v.armorvalue=0;
        npc->v.teleport_time=globals->time+2;npc->v.angles.y=180;czGunFrame(npc);
        for(Vector pos:{Vector(-352,1792,24.1f),Vector(-352,-2304,24.1f),Vector(-2400,-256,24.1f)}){
            auto normal=engine.pfnCreateNamedEntity(engine.pfnAllocString("monster_zombie"));normal->v.origin=pos;hookSpawn(normal);normal->v.targetname=engine.pfnAllocString("mansion_enemy_radar_probe");normal->v.nextthink=globals->time+30;
        }
        began=globals->time;stage=1;return;
    }
    if(stage==1&&globals->time-began>.5f){engine.pfnClientCommand(p,const_cast<char*>("snapshot\n"));began=globals->time;stage=2;return;}
    if(stage==2&&globals->time-began>.3f){
        Vector at(640,-240,61);engine.pfnSetOrigin(p,&at.x);p->v.movetype=MOVETYPE_NONE;p->v.health=1000;
        Vector melee(700,-240,24.1f);engine.pfnSetOrigin(npc,&melee.x);npc->v.health=1000;npc->v.flags|=FL_ONGROUND;npc->v.dmgtime=0;float before=p->v.health;npc->v.dmgtime=0;czFire(npc,p);
        pass(p->v.health==before-20,"knife_melee_damage");
        auto blocker=engine.pfnCreateNamedEntity(engine.pfnAllocString("monster_zombie"));Vector blocked(670,-240,24.1f);blocker->v.origin=blocked;hookSpawn(blocker);engine.pfnSetOrigin(blocker,&blocked.x);blocker->v.health=1000;blocker->v.nextthink=globals->time+10;
        before=p->v.health;npc->v.dmgtime=0;czFire(npc,p);pass(p->v.health==before&&blocker->v.health==1000,"allies_block_without_friendly_damage");engine.pfnRemoveEntity(blocker);
        pass(npc->v.frags==0&&npc->v.impulse==0,"knife_without_ammo_or_reload");
        Vector distant(900,-256,24.1f);engine.pfnSetOrigin(npc,&distant.x);before=p->v.health;npc->v.dmgtime=0;czFire(npc,p);
        pass(p->v.health==before,"no_ranged_knife_damage");
        auto table=*reinterpret_cast<void***>(npc->pvPrivateData);auto hurt=reinterpret_cast<TakeDamageFn>(table[ravenholm_abi::slot::TakeDamage]);
        bool damaged=true;
        for(int bits:{0,1<<1,1<<7,1<<6,1<<10}){before=npc->v.health;hurt(npc->pvPrivateData,&p->v,&p->v,5.f,bits);damaged=damaged&&npc->v.health<before;}
        pass(damaged,"bullet_plasma_melee_blast_energy_damage");
        before=npc->v.health;necroHurt(npc,p,5,true);pass(npc->v.health<before,"necromancer_fire_damage");
        freeze(npc);czThink(npc);pass(slowed(npc)&&npc->v.framerate==balance::SlowFactor,"ricochet_freeze");
        before=npc->v.health;shockHurt(npc,p,p,5,false,Vector(1,0,0));pass(npc->v.health<before&&pushed(npc)&&npc->v.movetype==MOVETYPE_TOSS,"shock_damage_and_push");
        float magazine=npc->v.frags;czThink(npc);pass(npc->v.frags==magazine,"no_shooting_during_push");
        auto fx=getEffect(npc);if(fx){restoreSlow(*fx);fx->pushUntil=0;}npc->v.movetype=MOVETYPE_STEP;
        npc->v.health=100;necroHurt(npc,p,10000,true);czThink(npc);czVisualCleanup();
        pass(!aliveEnemy(npc),"native_death");bool gun=false;
        pass(!std::strcmp(text(npc->v.model),"models/cremation/czero/zombie_knife_charred.mdl"),"charred_death_preserves_rig");
        for(int i=1;i<globals->maxEntities;++i){auto a=engine.pfnPEntityOfEntIndex(i);if(valid(a)&&a->v.owner==npc&&!std::strcmp(text(a->v.targetname),"czombie_knife_visual"))gun=true;}
        pass(!gun,"no_orphan_or_ground_knife");if(valid(npc))engine.pfnRemoveEntity(npc);
        npc=create(Vector(900,-256,24.1f));auto gibCount=anatomicalGibs.size();
        auto death=*reinterpret_cast<void***>(npc->pvPrivateData);reinterpret_cast<void(__thiscall*)(void*,entvars_t*,int)>(death[13])(npc->pvPrivateData,&p->v,2);
        pass(!aliveEnemy(npc)&&(anatomicalGibs.size()>gibCount||engine.pfnCVarGetFloat("violence_hgibs")==0),"native_anatomical_gibs");
        const Vector directions[]={Vector(0,1,0),Vector(1,0,0),Vector(0,-1,0),Vector(-1,0,0)};
        bool pending=true;
        for(int side=0;side<4;++side){auto e=create(Vector(-352,-256,24.1f)+directions[side]*2048);e->v.weapons=side;
            e->v.targetname=engine.pfnAllocString("mansion_enemy_smoke_probe");pending=pending&&czSmokeCanLeave(e)&&!mansionRadarEligible(e);
            czThrowSmoke(e);smokers.push_back(e);
        }
        pass(pending,"npc_hidden_on_radar_and_free_to_leave_spawn");
        began=globals->time;stage=3;return;
    }
    if(stage==3&&globals->time-began>1.45f){
        int clouds=0;bool locations=true;
        for(int i=1;i<globals->maxEntities;++i){auto e=engine.pfnPEntityOfEntIndex(i);if(!valid(e)||std::strcmp(text(e->v.targetname),"cz_smoke_cloud"))continue;
            ++clouds;Vector d=e->v.origin-Vector(-352,-256,36);float nearest=10000;
            for(Vector direction:{Vector(0,1,0),Vector(1,0,0),Vector(0,-1,0),Vector(-1,0,0)})nearest=std::min(nearest,length2d(d-direction*1632));
            locations=locations&&nearest<110;
        }
        pass(clouds==0,"no_smoke_thrown_at_empty_bridge_entries");
        bool one=true,waiting=true;
        for(auto e:smokers){czThrowSmoke(e);one=one&&!(e->v.spawnflags&CZSmokeThrown);waiting=waiting&&czSmokeCanLeave(e);}
        int grenades=0;for(int i=1;i<globals->maxEntities;++i){auto e=engine.pfnPEntityOfEntIndex(i);if(valid(e)&&!std::strcmp(text(e->v.targetname),"cz_smoke_grenade"))++grenades;}
        pass(one&&grenades==0&&waiting,"unspent_smoke_remains_available");
        pass(!czSmokeBlocks(Vector(1200,-256,80),Vector(1450,-256,80)),"clear_air_does_not_block_npc_vision");
        began=globals->time;stage=31;return;
    }
    if(stage==31&&globals->time-began>.6f){
        bool released=true;for(auto e:smokers)released=released&&czSmokeCanLeave(e);pass(released,"no_grenade_wait_without_cluster");
        Vector camera(1150,-440,100);engine.pfnSetOrigin(p,&camera.x);p->v.v_angle=p->v.angles=Vector(12,55,0);p->v.fixangle=1;p->v.viewmodel=0;
        engine.pfnClientCommand(p,const_cast<char*>("snapshot\n"));
        npc=create(Vector(496.25f,-800,-350));npc->v.flags&=~FL_ONGROUND;npc->v.velocity=Vector(0,0,0);
        began=globals->time;stage=4;return;
    }
    if(stage==4&&globals->time-began>1.f){pass(!aliveEnemy(npc),"radioactive_pit_instakill");stage=5;startupTrace("CZTestComplete");}
}
