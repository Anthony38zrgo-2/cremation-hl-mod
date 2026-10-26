// Explicit test-only studio pose captures. Does not run in normal play.
static void czAnimationProbeFrame(){
    static bool checked=false,enabled=false;static int stage=0;static float began=0;static bool shot=false;static edict_t* npc=nullptr;
    if(!checked){char env[8]{};GetEnvironmentVariableA("CREMATION_CZERO_ANIMATIONS",env,sizeof env);enabled=env[0]=='1';checked=true;}
    if(!enabled||stage==10||!firstPlayerPostThinkTime||globals->time<firstPlayerPostThinkTime+2)return;
    auto player=engine.pfnPEntityOfEntIndex(1);if(!valid(player)||player->v.health<=0)return;
    if(!valid(npc)){auto state=mansionState();if(valid(state))state->v.impulse=1;
        npc=engine.pfnCreateNamedEntity(engine.pfnAllocString(CZClass));npc->v.origin=Vector(900,-256,24.1f);hookSpawn(npc);
        npc->v.movetype=MOVETYPE_NONE;npc->v.nextthink=globals->time+1000;npc->v.angles=Vector(0,180,0);npc->v.effects|=EF_NOINTERP;czGunFrame(npc);
    }
    const int sequences[]={75,3,3,3,3,4,4,76,76,57};
    const float frames[]={0,0,64,128,192,64,128,64,160,128};
    Vector camera(650,-430,76);engine.pfnSetOrigin(player,&camera.x);player->v.movetype=MOVETYPE_NOCLIP;player->v.velocity=Vector(0,0,0);
    player->v.viewmodel=0;player->v.weaponmodel=0;player->v.v_angle=player->v.angles=Vector(8,35,0);player->v.fixangle=1;
    npc->v.sequence=sequences[stage];npc->v.frame=frames[stage];npc->v.framerate=0;npc->v.animtime=globals->time;czGunFrame(npc);
    if(!began){began=globals->time;shot=false;}
    if(globals->time-began>.3f&&!shot){engine.pfnClientCommand(player,const_cast<char*>("snapshot\n"));shot=true;}
    if(globals->time-began>.55f){++stage;began=0;if(stage==10)startupTrace("CZAnimationComplete");}
}
