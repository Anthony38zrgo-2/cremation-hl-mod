static void tacticalSaveProbeFrame(){
    static int mode=-1,stage=0;static float began=0;
    if(mode<0){char env[8]{};GetEnvironmentVariableA("CREMATION_SAVE_PROBE",env,sizeof env);mode=std::atoi(env);}
    if(!mode||stage==2||!firstPlayerPostThinkTime||globals->time<firstPlayerPostThinkTime+.5f)return;
    auto p=engine.pfnPEntityOfEntIndex(1);if(!valid(p))return;
    if(mode==1&&stage==0){auto state=mansionState();if(valid(state))state->v.impulse=1;
        auto e=engine.pfnCreateNamedEntity(engine.pfnAllocString(CZClass));e->v.origin=Vector(900,-256,24.1f);hookSpawn(e);e->v.targetname=engine.pfnAllocString("save_probe_npc");e->v.weapons=1;e->v.iuser2=1;
        e->v.spawnflags|=CZSmokeThrown|CZFlashThrown;czPlant(e);e->v.nextthink=globals->time+1000;
        auto bomb=e->v.chain;bomb->v.dmgtime=globals->time+60;dmcGive(p,0);dmcGive(p,3);
        engine.pfnClientCommand(p,const_cast<char*>("save cremation_tactical_probe\n"));stage=1;began=globals->time;
    }else if(mode==1&&stage==1&&globals->time-began>1){startupTrace("SaveProbeCreated");stage=2;}
    else if(mode==2&&stage==0){auto e=mansionFind("save_probe_npc"),b=mansionFind("cz_planted_bomb");
        auto pass=[](bool ok,const char* name){startupTrace(ok?"SaveProbe PASS":"SaveProbe FAIL",name);};
        pass(czIs(e)&&e->v.iuser1==5&&e->v.iuser2==1,"npc_objective_phase_and_entrance");
        pass(valid(e)&&(e->v.spawnflags&(CZSmokeThrown|CZFlashThrown))==(CZSmokeThrown|CZFlashThrown),"grenade_inventory_not_refilled");
        pass(valid(b)&&b->v.owner==e&&b->v.dmgtime>globals->time+20&&b->v.dmgtime<globals->time+61,"bomb_owner_and_remaining_timer");
        pass(dmcUntil(p,0)>globals->time+10&&dmcUntil(p,0)<globals->time+31&&dmcUntil(p,3)>globals->time+10,"artifact_timers_survive_load");
        pass(!std::strcmp(engine.pfnGetPhysicsKeyValue(p,"slj"),"1")&&!std::strcmp(engine.pfnGetPhysicsKeyValue(p,"defuse"),"1"),"player_defaults_survive_load");
        startupTrace("SaveProbeComplete");stage=2;
    }
}
