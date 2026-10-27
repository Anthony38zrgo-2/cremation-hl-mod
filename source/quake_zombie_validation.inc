// Opt-in headless engine acceptance probe. Never runs in normal sessions.
// The launcher used by tools/check_quake_zombie_runtime.py sets the environment
// flag only for its bounded child hlds process. No map/config files are changed.
static edict_t* qzProbeZombie=nullptr;
static edict_t* qzProbeTarget=nullptr;
static float qzProbeAt=0;
static int qzProbePhase=-1;
static int qzProbeTicks=0;
static size_t qzProbeGibs=0;
static void qzProbeResult(bool passed,const char* detail){startupTrace(passed?"QuakeZombieTest PASS":"QuakeZombieTest FAIL",detail);}
static void qzProbeBegin(edict_t* list,int count){
    qzProbePhase=-1;char enabled[8]{};
    if(!GetEnvironmentVariableA("CREMATION_QZ_VALIDATE",enabled,sizeof enabled)||std::strcmp(enabled,"1"))return;
    qzDiagnostic=true;
    qzProbeZombie=nullptr;
    for(int i=0;i<count;++i)if(qzIs(&list[i])){qzProbeZombie=&list[i];break;}
    if(!qzProbeZombie){qzProbeResult(false,"missing entity");return;}
    auto e=qzProbeZombie;
    qzProbeTarget=engine.pfnCreateNamedEntity(engine.pfnAllocString("info_target"));
    if(!valid(qzProbeTarget)){qzProbeResult(false,"target allocation");return;}
    auto t=qzProbeTarget;t->v.classname=engine.pfnAllocString("monster_minion");
    t->v.health=1000;t->v.takedamage=DAMAGE_AIM;t->v.solid=SOLID_BBOX;t->v.movetype=MOVETYPE_NONE;
    engine.pfnSetModel(t,"models/hgrunt.mdl");Vector mins(-16,-16,0),maxs(16,16,72);engine.pfnSetSize(t,&mins.x,&maxs.x);
    Vector at=e->v.origin+Vector(128,0,0);engine.pfnSetOrigin(t,&at.x);
    e->v.enemy=t;e->v.dmg_save=float(t->serialnumber);qzFace(e,t->v.origin);qzSequence(e,3,QZAttack);
    // Prove server-side hitboxes follow the resized rendered head, rather than
    // merely checking the broad-phase entity bbox or reading MDL metadata.
    qzSequence(e,0,QZIdle);
    Vector a=e->v.origin+Vector(80,0,96),b=e->v.origin+Vector(-80,0,96);TraceResult head{},above{};
    engine.pfnTraceLine(&a.x,&b.x,0,nullptr,&head);
    a.z=b.z=e->v.origin.z+118;engine.pfnTraceLine(&a.x,&b.x,0,nullptr,&above);
    qzProbeResult(head.pHit==e&&head.iHitgroup==1&&above.pHit!=e,"resized anatomical head hitbox hit; air above model missed");
    qzSequence(e,3,QZAttack);
    qzProbePhase=0;qzProbeAt=globals->time+4;
}
static void qzProbeFrame(){
    if(qzProbePhase<0||globals->time<qzProbeAt)return;
    auto e=qzProbeZombie;auto t=qzProbeTarget;
    if(!qzIs(e)||!valid(t)){qzProbeResult(false,"lost probe entity");qzProbePhase=-1;return;}
    if(qzProbePhase==0){
        char detail[200];std::snprintf(detail,sizeof detail,"target=%s health=%.0f state=%d nextshot=%.2f time=%.2f",text(t->v.classname),t->v.health,e->v.impulse,e->v.teleport_time,globals->time);startupTrace("QuakeZombieProbe",detail);
        qzProbeResult(t->v.health<1000,"real thrown chunk hit target and applied damage");
        auto table=*reinterpret_cast<void***>(e->pvPrivateData);
        reinterpret_cast<TakeDamageFn>(table[ravenholm_abi::slot::TakeDamage])(e->pvPrivateData,&t->v,&t->v,25,1<<1);
        qzProbeResult(e->v.impulse==QZDown&&e->v.health==60,"real virtual TakeDamage entered knockdown");
        qzProbePhase=1;qzProbeAt=globals->time+2;
    }else if(qzProbePhase==1){
        qzProbeResult(e->v.impulse==QZDown&&e->v.solid==SOLID_NOT&&e->v.button==10,"down pose held non-solid");
        qzProbePhase=2;qzProbeAt=globals->time+7;
    }else if(qzProbePhase==2){
        qzProbeResult(e->v.impulse!=QZDown&&e->v.impulse!=QZRise&&e->v.solid==SOLID_SLIDEBOX,"real timed recovery restored solid body");
        auto table=*reinterpret_cast<void***>(e->pvPrivateData);
        size_t before=anatomicalGibs.size();
        reinterpret_cast<TakeDamageFn>(table[ravenholm_abi::slot::TakeDamage])(e->pvPrivateData,&t->v,&t->v,60,1<<6);
        qzProbeResult(e->v.impulse==QZDead&&e->v.health==0&&e->v.solid==SOLID_NOT,"60 damage permanently gibbed entity");
        qzProbeResult(anatomicalGibs.size()==before+6,"same six anatomical gib pieces as other monsters");
        auto energy=engine.pfnCreateNamedEntity(engine.pfnAllocString(QZClass));
        if(!valid(energy)){qzProbeResult(false,"energy test allocation");qzProbePhase=-1;return;}
        energy->v.origin=e->v.origin;energy->v.angles=e->v.angles;raven.pfnSpawn(energy);qzSpawn(energy);
        qzProbeZombie=energy;qzProbeTicks=0;qzProbeGibs=anatomicalGibs.size();qzProbePhase=3;qzProbeAt=globals->time+.1f;
    }else if(qzProbePhase==3){
        auto table=*reinterpret_cast<void***>(e->pvPrivateData);
        reinterpret_cast<TakeDamageFn>(table[ravenholm_abi::slot::TakeDamage])(e->pvPrivateData,&t->v,&t->v,4,(1<<10)|(1<<12));++qzProbeTicks;
        qzProbeAt=globals->time+.1f;
        if(qzProbeTicks==15){
            qzProbeResult(e->v.health==0&&e->v.impulse==QZDead&&anatomicalGibs.size()==qzProbeGibs,"Link beam ticks accumulate to non-gib death");
            LinkLock lock{};lock.serial=e->serialnumber;lock.lastOrigin=e->v.origin;lock.lastAngles=e->v.angles;lock.lastHeight=QZHeight;
            size_t count=linkSkeletons.size();linkDeathEffect(lock,e);
            qzProbeResult(lock.deathShown&&linkSkeletons.size()==count+1&&linkSkeletons.back().matched,"shared Link Gun skeleton death effect matched");
            qzProbePhase=-1;
        }
    }
}

