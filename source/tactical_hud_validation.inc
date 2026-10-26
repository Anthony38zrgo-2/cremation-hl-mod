// Test-process only: uses the real bomb, USE handling and expiration timers.
static void tacticalHudProbeFrame(){
    static bool checked=false,enabled=false,shot=false;static int stage=0;static float began=0;
    static edict_t* npc=nullptr;static edict_t* bomb=nullptr;
    if(!checked){char env[8]{};GetEnvironmentVariableA("CREMATION_HUD_VALIDATE",env,sizeof env);enabled=env[0]=='1';checked=true;}
    if(!enabled||stage==7||!firstPlayerPostThinkTime||globals->time<firstPlayerPostThinkTime+2)return;
    auto p=engine.pfnPEntityOfEntIndex(1);if(!valid(p))return;
    auto state=mansionState();if(valid(state)&&state->v.impulse!=2)state->v.impulse=1;
    if(stage<6){Vector at(320,-288,60),angle(25,146,0);engine.pfnSetOrigin(p,&at.x);p->v.velocity=Vector(0,0,0);p->v.movetype=MOVETYPE_NOCLIP;p->v.viewmodel=p->v.weaponmodel=0;p->v.v_angle=p->v.angles=angle;p->v.fixangle=1;p->v.health=1000;}
    auto create=[&](){npc=engine.pfnCreateNamedEntity(engine.pfnAllocString(CZClass));npc->v.origin=Vector(272,-256,24.1f);hookSpawn(npc);npc->v.targetname=engine.pfnAllocString("hud_probe_npc");czPlant(npc);npc->v.nextthink=globals->time+1000;bomb=npc->v.chain;};
    auto screenshot=[&](const char* label){engine.pfnClientCommand(p,const_cast<char*>("snapshot\n"));startupTrace("HUDProbeShot",label);};
    if(!stage){create();for(int k=0;k<4;++k)dmcGive(p,k);began=globals->time;stage=1;shot=false;return;}
    players[1].buttons&=~IN_USE;
    if(stage==1){if(globals->time-began>.6f&&!shot){screenshot("planted");shot=true;}
        if(globals->time-began>1){auto table=*reinterpret_cast<void***>(npc->pvPrivateData);reinterpret_cast<TakeDamageFn>(table[11])(npc->pvPrivateData,&p->v,&p->v,1000000,1<<13);stage=2;began=globals->time;shot=false;}}
    else if(stage==2){players[1].buttons|=IN_USE;
        if(globals->time-began>2.4f&&!shot){screenshot("defusing");shot=true;}
        if(globals->time-began>2.9f){stage=3;began=globals->time;shot=false;}}
    else if(stage==3){if(globals->time-began>.6f&&!shot){screenshot("cancelled");shot=true;}
        if(globals->time-began>1){stage=4;began=globals->time;shot=false;}}
    else if(stage==4){players[1].buttons|=IN_USE;
        if(globals->time-began>5.6f&&!shot){screenshot("defused");shot=true;}
        if(globals->time-began>6){auto s=dmcState(p);if(valid(s))s->v.fuser1=s->v.fuser2=s->v.fuser3=s->v.fuser4=0;stage=5;began=globals->time;shot=false;}}
    else if(stage==5){if(globals->time-began>.6f&&!shot){screenshot("expired");shot=true;}
        if(globals->time-began>1){create();bomb->v.dmgtime=globals->time+.6f;stage=6;began=globals->time;shot=false;}}
    else if(stage==6&&globals->time-began>1.4f){screenshot("exploded");stage=7;startupTrace("HUDProbeComplete");}
}
