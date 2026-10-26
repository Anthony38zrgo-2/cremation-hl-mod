// Isolated engine checks and review captures; opt-in process only.
static void vostokProbeFrame(){
    static bool checked=false,enabled=false;static int stage=0;static float began=0,oldFrame=0;static bool shot=false;static edict_t* prop=nullptr;
    if(!checked){char env[8]{};GetEnvironmentVariableA("CREMATION_VOSTOK_VALIDATE",env,sizeof env);enabled=env[0]=='1';checked=true;}
    if(!enabled||stage==4||!firstPlayerPostThinkTime||globals->time<firstPlayerPostThinkTime+2)return;
    auto p=engine.pfnPEntityOfEntIndex(1);if(!valid(p)||p->v.health<=0)return;
    const Vector origins[]={Vector(-864,64,24),Vector(-864,-624,24),Vector(-448,344,24),Vector(336,-640,24)};
    const Vector cameras[]={Vector(-1030,-160,125),Vector(-1150,-1000,240),Vector(-900,560,240),Vector(470,-1050,240)};
    const Vector angles[]={Vector(15,54,0),Vector(0,53,0),Vector(0,334,0),Vector(0,108,0)};
    auto pass=[](bool ok,const char* name){startupTrace(ok?"VostokTest PASS":"VostokTest FAIL",name);};
    auto state=mansionState();if(valid(state))state->v.impulse=1;
    p->v.movetype=MOVETYPE_NOCLIP;p->v.velocity=Vector(0,0,0);p->v.viewmodel=0;p->v.weaponmodel=0;
    engine.pfnSetOrigin(p,&cameras[stage].x);p->v.v_angle=p->v.angles=angles[stage];p->v.fixangle=1;p->v.view_ofs=Vector(0,0,28);
    if(!began){
        began=globals->time;shot=false;
        prop=nullptr;
        for(int i=1;i<globals->maxEntities;++i){auto e=engine.pfnPEntityOfEntIndex(i);
            if(vostokCover(e)&&length(e->v.origin-origins[stage])<1){prop=e;oldFrame=e->v.frame;break;}}
        if(!stage){int count=0;for(int i=1;i<globals->maxEntities;++i){auto e=engine.pfnPEntityOfEntIndex(i);if(vostokCover(e)&&!std::strstr(text(e->v.targetname),"truth_")){
            ++count;pass(e->v.modelindex>0&&e->v.frame>0&&e->v.framerate>0,"original_model_loaded_and_animating");}}
            pass(count==4,"one_jeep_three_trees");}
        Vector a=origins[stage]+Vector(-75,0,40),b=origins[stage]+Vector(75,0,40);TraceResult tr{};
        engine.pfnTraceLine(&a.x,&b.x,1,p,&tr);pass(tr.flFraction<1,"cover_blocks_bullets");
        engine.pfnTraceHull(&a.x,&b.x,1,1,p,&tr);pass(tr.flFraction<1,"cover_blocks_player_and_monsters");
    }
    if(globals->time-began>.37f&&!shot){
        pass(valid(prop)&&std::fabs(prop->v.frame-oldFrame)>.01f,"animation_advances_over_time");
        engine.pfnClientCommand(p,const_cast<char*>("snapshot\n"));shot=true;}
    if(globals->time-began>1.f){++stage;began=0;if(stage==4)startupTrace("VostokTestComplete");}
}
