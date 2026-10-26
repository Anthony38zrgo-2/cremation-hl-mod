// Actual client review of the exterior; disabled in ordinary play.
static void truthProbeFrame(){
    static bool checked=false,enabled=false;static int stage=0;static float began=0;static bool shot=false;
    if(!checked){char env[8]{};GetEnvironmentVariableA("CREMATION_TRUTH_VALIDATE",env,sizeof env);enabled=env[0]=='1';checked=true;}
    if(!enabled||stage==6||!firstPlayerPostThinkTime||globals->time<firstPlayerPostThinkTime+2)return;
    auto p=engine.pfnPEntityOfEntIndex(1);if(!valid(p)||p->v.health<=0)return;
    const Vector cameras[]={Vector(660,-1050,230),Vector(-1180,520,230),Vector(1000,-256,-180),Vector(1350,1500,260),Vector(-2110,-2000,260),Vector(-352,900,300)};
    const Vector angles[]={Vector(5,139,0),Vector(5,322,0),Vector(5,180,0),Vector(15,230,0),Vector(15,44,0),Vector(4,90,0)};
    auto state=mansionState();if(valid(state))state->v.impulse=1;
    p->v.movetype=MOVETYPE_NOCLIP;p->v.velocity=Vector(0,0,0);p->v.viewmodel=0;p->v.weaponmodel=0;
    engine.pfnSetOrigin(p,&cameras[stage].x);p->v.v_angle=p->v.angles=angles[stage];p->v.fixangle=1;p->v.view_ofs=Vector(0,0,28);
    if(!began){began=globals->time;shot=false;
        if(!stage){int count=0;for(int i=1;i<globals->maxEntities;++i){auto e=engine.pfnPEntityOfEntIndex(i);
            if(!vostokCover(e)||!std::strstr(text(e->v.targetname),"truth_"))continue;
            ++count;startupTrace(e->v.modelindex>0?"TruthTest PASS":"TruthTest FAIL","original_prop_loaded");}
            startupTrace(count==9?"TruthTest PASS":"TruthTest FAIL","two_trees_five_bushes_two_rocks");}}
    if(globals->time-began>.65f&&!shot){engine.pfnClientCommand(p,const_cast<char*>("snapshot\n"));shot=true;}
    if(globals->time-began>1.25f){++stage;began=0;if(stage==6)startupTrace("TruthTestComplete");}
}
