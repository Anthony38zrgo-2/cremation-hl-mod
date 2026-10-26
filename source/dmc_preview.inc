static void dmcPreviewFrame(){
    static bool checked=false,enabled=false,shot=false;static int stage=0;static float began=0;
    if(!checked){char env[8]{};GetEnvironmentVariableA("CREMATION_DMC_PREVIEW",env,sizeof env);enabled=env[0]=='1';checked=true;}
    if(!enabled||stage==4||!firstPlayerPostThinkTime||globals->time<firstPlayerPostThinkTime+2)return;
    auto p=engine.pfnPEntityOfEntIndex(1),display=mansionFind("reactor_display_1");if(!valid(p)||!valid(display))return;
    auto state=mansionState();if(valid(state))state->v.impulse=1;
    Vector at(-1840,-1870,95),angle(18,90,0);engine.pfnSetOrigin(p,&at.x);p->v.velocity=Vector(0,0,0);p->v.movetype=MOVETYPE_NOCLIP;p->v.viewmodel=p->v.weaponmodel=0;p->v.view_ofs=Vector(0,0,28);
    p->v.v_angle=p->v.angles=angle;p->v.fixangle=1;
    if(!began){began=globals->time;shot=false;engine.pfnSetModel(display,dmcModels[stage]);display->v.impulse=stage+1;display->v.effects&=~EF_NODRAW;
        display->v.rendercolor=stage==0?Vector(128,128,255):stage==1?Vector(255,128,0):Vector(128,128,128);
        display->v.rendermode=stage==2?kRenderTransColor:kRenderNormal;display->v.renderamt=stage==2?30:stage==3?255:100;display->v.renderfx=stage==3?kRenderFxNone:kRenderFxGlowShell;}
    if(globals->time-began>.5f&&!shot){engine.pfnClientCommand(p,const_cast<char*>("snapshot\n"));shot=true;}
    if(globals->time-began>1.f){++stage;began=0;if(stage==4)startupTrace("DMCPreviewComplete");}
}
