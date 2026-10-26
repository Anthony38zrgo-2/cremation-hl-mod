// Test-process-only camera placements for review screenshots.
static void reactorPreviewFrame(){
    static bool checked=false,enabled=false;static int stage=0;static float began=0;static bool shot=false;
    if(!checked){char env[8]{};GetEnvironmentVariableA("CREMATION_REACTOR_PREVIEW",env,sizeof env);enabled=!std::strcmp(env,"1");checked=true;}
    if(!enabled||stage==4||!firstPlayerPostThinkTime||globals->time<firstPlayerPostThinkTime+2)return;
    auto player=engine.pfnPEntityOfEntIndex(1);if(!valid(player)||player->v.health<=0)return;
    static const Vector positions[]={Vector(-324,-872,88),Vector(-324,344,88),Vector(570,-256,90),Vector(600,-600,-220)};
    static const Vector angles[]={Vector(35,90,0),Vector(35,270,0),Vector(25,0,0),Vector(25,90,0)};
    player->v.movetype=MOVETYPE_NOCLIP;player->v.velocity=Vector(0,0,0);
    player->v.viewmodel=0;player->v.weaponmodel=0;
    engine.pfnSetOrigin(player,&positions[stage].x);player->v.angles=angles[stage];player->v.v_angle=angles[stage];player->v.fixangle=1;player->v.punchangle=Vector(0,0,0);player->v.view_ofs=Vector(0,0,28);
    if(!began){began=globals->time;engine.pfnSetOrigin(player,&positions[stage].x);player->v.angles=angles[stage];player->v.v_angle=angles[stage];player->v.fixangle=1;shot=false;}
    if(globals->time-began>.65f&&!shot){
        if(stage==0)for(int i=1;i<globals->maxEntities;++i){auto e=engine.pfnPEntityOfEntIndex(i);if(!valid(e)||!(e->v.effects&EF_BRIGHTFIELD))continue;
            char row[240];std::snprintf(row,sizeof row,"class=%s name=%s origin=%.1f,%.1f,%.1f model=%s",text(e->v.classname),text(e->v.targetname),e->v.origin.x,e->v.origin.y,e->v.origin.z,text(e->v.model));startupTrace("BrightfieldProbe",row);}
        engine.pfnClientCommand(player,const_cast<char*>("snapshot\n"));shot=true;startupTrace("ReactorPreviewShot",stage==0?"overview":stage==1?"reward_house":stage==2?"powerup_house":"pit_floor");}
    if(globals->time-began>1.4f){++stage;began=0;if(stage==4)startupTrace("ReactorPreviewComplete");}
}
