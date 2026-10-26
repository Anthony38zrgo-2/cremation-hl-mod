// Deterministic RNG substitution only inside the explicit isolated test process.
static decltype(engine.pfnRandomLong) heartTestRandomOriginal=nullptr;
static int heartTestRoll=10;
static int heartTestRandom(int lo,int hi){return lo==1&&hi==100?heartTestRoll:heartTestRandomOriginal(lo,hi);}
static void heartProbeFrame(){
    static bool checked=false,enabled=false;static int stage=0;static float began=0;
    static edict_t* expiry=nullptr;static int expirySerial=0;static Vector first(0,0,0);
    if(!checked){char env[8]{};GetEnvironmentVariableA("CREMATION_HEART_VALIDATE",env,sizeof env);enabled=env[0]=='1';checked=true;}
    if(!enabled||stage==4||!firstPlayerPostThinkTime||globals->time<firstPlayerPostThinkTime+2)return;
    auto p=engine.pfnPEntityOfEntIndex(1);if(!valid(p)||p->v.health<=0)return;
    auto pass=[](bool ok,const char* name){startupTrace(ok?"HeartTest PASS":"HeartTest FAIL",name);};
    auto count=[](){int n=0;for(int i=1;i<globals->maxEntities;++i)if(heartIs(engine.pfnPEntityOfEntIndex(i)))++n;return n;};
    if(stage==0){
        auto controller=mansionState();if(valid(controller))controller->v.impulse=1;
        Vector at(640,-240,61);engine.pfnSetOrigin(p,&at.x);p->v.movetype=MOVETYPE_NONE;p->v.velocity=Vector(0,0,0);
        heartTestRandomOriginal=engine.pfnRandomLong;engine.pfnRandomLong=heartTestRandom;
        auto victim=engine.pfnCreateNamedEntity(engine.pfnAllocString(CZClass));victim->v.origin=Vector(900,-256,24.1f);hookSpawn(victim);
        int wins=0;for(int i=1;i<=100;++i){victim->v.spawnflags&=~HeartRolled;heartTestRoll=i;if(heartRollDeath(victim))++wins;}
        pass(wins==10,"exact_ten_percent_roll_boundary");
        heartTestRoll=10;victim->v.spawnflags&=~HeartRolled;bool once=heartRollDeath(victim);pass(once&&!heartRollDeath(victim),"one_roll_per_enemy");engine.pfnRemoveEntity(victim);
        heartTestRoll=10;
        auto quake=engine.pfnCreateNamedEntity(engine.pfnAllocString(QZClass));quake->v.origin=Vector(900,-256,24.1f);hookSpawn(quake);
        auto qt=*reinterpret_cast<void***>(quake->pvPrivateData);auto hit=reinterpret_cast<TakeDamageFn>(qt[11]);int previous=count();
        hit(quake->pvPrivateData,&p->v,&p->v,25,1<<1);
        pass(aliveEnemy(quake)&&count()==previous&&!(quake->v.spawnflags&HeartRolled),"quake_knockdown_does_not_roll");
        hit(quake->pvPrivateData,&p->v,&p->v,10000,1<<10);
        pass(!aliveEnemy(quake)&&count()==previous+1,"quake_direct_damage_death_drops");
        previous=count();reinterpret_cast<void(__thiscall*)(void*,entvars_t*,int)>(qt[13])(quake->pvPrivateData,&p->v,2);
        pass(count()==previous,"corpse_gib_does_not_drop_twice");engine.pfnRemoveEntity(quake);
        bool all=true;
        for(int kind=0;kind<6;++kind){auto e=engine.pfnCreateNamedEntity(engine.pfnAllocString(mansion::classname(kind)));e->v.origin=Vector(900,-256,24.1f);hookSpawn(e);int before=count();
            auto t=*reinterpret_cast<void***>(e->pvPrivateData);reinterpret_cast<void(__thiscall*)(void*,entvars_t*,int)>(t[13])(e->pvPrivateData,&p->v,2);all=all&&count()==before+1;
        }pass(all,"all_six_enemy_death_hooks_drop");
        auto minion=engine.pfnCreateNamedEntity(engine.pfnAllocString("monster_minion"));minion->v.origin=Vector(900,-256,24);hookSpawn(minion);
        pass(!heartRollDeath(minion),"allies_do_not_drop");engine.pfnRemoveEntity(minion);engine.pfnRandomLong=heartTestRandomOriginal;
        for(int i=1;i<globals->maxEntities;++i){auto e=engine.pfnPEntityOfEntIndex(i);if(heartIs(e))heartRemove(e);}
        auto h=heartSpawn(p->v.origin+Vector(0,0,8));p->v.health=50;bool consumed=heartConsume(h,p);
        pass(consumed&&p->v.health==75&&!heartIs(h),"pickup_heals_25_and_consumes_once");
        h=heartSpawn(p->v.origin+Vector(0,0,8));p->v.health=90;heartConsume(h,p);pass(p->v.health==100,"health_capped_at_100");
        h=heartSpawn(p->v.origin+Vector(0,0,8));pass(!heartConsume(h,p)&&heartIs(h),"full_health_does_not_consume");heartRemove(h);
        h=heartSpawn(Vector(700,-256,65));p->v.health=50;p->v.deadflag=DEAD_DYING;pass(!heartConsume(h,p),"dead_player_cannot_consume");p->v.deadflag=DEAD_NO;heartRemove(h);
        expiry=heartSpawn(Vector(760,-256,68));expirySerial=expiry->serialnumber;began=globals->time;
        pass(std::fabs(expiry->v.dmgtime-began-10)<.01f,"lifetime_ten_seconds");
        p->v.health=100;heartFrame();first=expiry->v.origin;stage=1;return;
    }
    if(stage==1&&globals->time-began>.6f){
        pass(heartIs(expiry)&&length(expiry->v.origin-first)>1&&expiry->v.renderfx==kRenderFxGlowShell&&expiry->v.rendercolor.x==255,"floating_red_glow");
        int squares=0;bool moving=true;
        for(int i=1;i<globals->maxEntities;++i){auto e=engine.pfnPEntityOfEntIndex(i);if(valid(e)&&e->v.owner==expiry&&!std::strcmp(text(e->v.targetname),"cremation_heart_square")){++squares;moving=moving&&length(e->v.origin-expiry->v.origin)>10;}}
        pass(squares==6&&moving,"six_orbiting_square_particles");
        Vector camera(700,-340,62);engine.pfnSetOrigin(p,&camera.x);p->v.v_angle=p->v.angles=Vector(8,54,0);p->v.fixangle=1;p->v.viewmodel=0;
        engine.pfnClientCommand(p,const_cast<char*>("snapshot\n"));stage=2;return;
    }
    if(stage==2&&globals->time-began>10.15f){
        pass(!heartIs(expiry)||expiry->serialnumber!=expirySerial,"automatic_expiration");
        int orphans=0;for(int i=1;i<globals->maxEntities;++i){auto e=engine.pfnPEntityOfEntIndex(i);if(valid(e)&&!std::strcmp(text(e->v.targetname),"cremation_heart_square"))++orphans;}
        pass(orphans==0,"particle_cleanup_on_expiration");stage=4;startupTrace("HeartTestComplete");
    }
}
