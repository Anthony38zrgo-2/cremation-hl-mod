// Original Condition Zero AWP. The stock .357 Python is only the inventory carrier; all firing,
// scoping, reload timing, and presentation are owned here.
static constexpr const char* AWPView="models/cremation/awp/v_awp.mdl";
static constexpr const char* AWPPlayer="models/cremation/awp/p_awp.mdl";
static constexpr const char* AWPWorld="models/cremation/awp/w_awp.mdl";
// Use the actual Python carrier ID; updated from the native WeaponList.
static int AWPHudId=3;
struct AWPState {
    int clip=10,zoomLevel=0;
    bool initialized=false,active=false,scoped=false,wasSecondary=false,wasReload=false,reloading=false;
    float nextFire=0,reloadFinish=0,idleAt=0;
};
static int awpFov(const AWPState& s){return s.zoomLevel==1?40:s.zoomLevel==2?10:90;}
static std::array<AWPState,33> awpStates{};
static int awpAmmoIndex=-1;
static bool awpPlayer(edict_t* p){
    if(!valid(p)||p->v.health<=0||!p->v.viewmodel)return false;
    const char* model=text(p->v.viewmodel);
    return !std::strcmp(model,"models/v_357.mdl")||!std::strcmp(model,AWPView);
}
static bool awpItemClass(const char* name){return name&&(!std::strcmp(name,"weapon_awp")||!std::strcmp(name,"weapon_357")||!std::strcmp(name,"weapon_python"));}
static void awpPrecache(){
    for(auto path:{AWPView,AWPPlayer,AWPWorld})engine.pfnPrecacheModel(const_cast<char*>(path));
    for(auto path:{"weapons/awp1.wav","weapons/awp_deploy.wav","weapons/awp_clipin.wav","weapons/awp_clipout.wav","weapons/boltup.wav","weapons/boltpull1.wav","weapons/boltdown.wav","weapons/zoom.wav"})engine.pfnPrecacheSound(const_cast<char*>(path));
}
static void awpSendAmmo(edict_t* p){
    if(!msgAmmoX||awpAmmoIndex<0)return;
    engine.pfnMessageBegin(MSG_ONE,msgAmmoX,nullptr,p);engine.pfnWriteByte(awpAmmoIndex);engine.pfnWriteByte(sharedAmmo(p,awpAmmoIndex,0));engine.pfnMessageEnd();
}
static void awpSetZoom(edict_t* p,int level){
    auto& s=awpStates[engine.pfnIndexOfEdict(p)];s.zoomLevel=level;s.scoped=level!=0;p->v.fov=float(awpFov(s));
    if(msgSetFov){engine.pfnMessageBegin(MSG_ONE,msgSetFov,nullptr,p);engine.pfnWriteByte(awpFov(s));engine.pfnMessageEnd();}
}
static void awpHud(edict_t* p){
    int i=engine.pfnIndexOfEdict(p);if(i<1||i>32)return;auto& s=awpStates[i];
    if(msgCurWeapon&&awpPlayer(p)){engine.pfnMessageBegin(MSG_ONE,msgCurWeapon,nullptr,p);engine.pfnWriteByte(1);engine.pfnWriteByte(AWPHudId);engine.pfnWriteByte(s.clip);engine.pfnMessageEnd();}
    awpSendAmmo(p);
}
static void awpStartReload(edict_t* p,AWPState& s){
    if(s.reloading||s.clip>=10||awpAmmoIndex<0||sharedAmmo(p,awpAmmoIndex,0)<=0)return;
    animation(p,4);
    s.reloading=true;s.reloadFinish=globals->time+2.95f;
}
static void awpTraceShot(edict_t* p,AWPState& s){
    if(s.clip<=0)return;
    --s.clip;s.nextFire=globals->time+1.35f;s.idleAt=globals->time+1.3f;
    int seq=1+int(engine.pfnRandomLong(0,2));animation(p,seq);sound(p,"weapons/awp1.wav");
    p->v.effects|=EF_MUZZLEFLASH;p->v.punchangle.x-=7.f;
    Vector aim=p->v.v_angle+p->v.punchangle;engine.pfnMakeVectors(&aim.x);
    Vector direction=normal(globals->v_forward+globals->v_right*engine.pfnRandomFloat(s.scoped?-.0007f:-.004f,s.scoped?.0007f:.004f)+globals->v_up*engine.pfnRandomFloat(s.scoped?-.0007f:-.004f,s.scoped?.0007f:.004f));
    Vector start=p->v.origin+p->v.view_ofs,end=start+direction*8192.f;TraceResult tr{};engine.pfnTraceLine(start,end,0,p,&tr);
    if(valid(tr.pHit)&&tr.pHit->v.takedamage>0&&tr.pHit->pvPrivateData){
        auto table=*reinterpret_cast<void***>(tr.pHit->pvPrivateData);
        reinterpret_cast<TakeDamageFn>(table[ravenholm_abi::slot::TakeDamage])(tr.pHit->pvPrivateData,&p->v,&p->v,115.f,1<<1);
    }else if(tr.flFraction<1.f){
        engine.pfnMessageBegin(MSG_PVS,SvcTemp,&tr.vecEndPos.x,nullptr);engine.pfnWriteByte(TE_GUNSHOT);coords(tr.vecEndPos);engine.pfnMessageEnd();
    }
    awpHud(p);
}
static void awpPostThink(edict_t* p,int buttons){
    int i=engine.pfnIndexOfEdict(p);if(i<1||i>32)return;auto& s=awpStates[i];bool active=awpPlayer(p);
    if(!active){if(s.active&&s.scoped)awpSetZoom(p,0);s.active=false;s.reloading=false;s.wasSecondary=false;s.wasReload=false;return;}
    p->v.viewmodel=engine.pfnAllocString(AWPView);p->v.weaponmodel=engine.pfnAllocString(AWPPlayer);
    if(!s.initialized){s.initialized=true;s.clip=10;awpHud(p);}
    if(!s.active){s.active=true;s.nextFire=std::max(s.nextFire,globals->time+1.f);animation(p,5);}
    if((buttons&Attack2)&&!s.wasSecondary&&!s.reloading){awpSetZoom(p,(s.zoomLevel+1)%3);sound(p,"weapons/zoom.wav");}
    s.wasSecondary=(buttons&Attack2)!=0;
    if((buttons&Reload)&&!s.wasReload)awpStartReload(p,s);
    s.wasReload=(buttons&Reload)!=0;
    if(s.reloading){
        if(globals->time>=s.reloadFinish){int reserve=sharedAmmo(p,awpAmmoIndex,0);int load=std::min(10-s.clip,reserve);if(load>0){sharedAmmo(p,awpAmmoIndex,load);s.clip+=load;}s.reloading=false;s.nextFire=globals->time+.2f;s.idleAt=0;animation(p,0);awpHud(p);}
    }
    if(!s.reloading&&s.idleAt>0&&globals->time>=s.idleAt){animation(p,0);s.idleAt=0;}
    if(!s.reloading&&(buttons&Attack)&&globals->time>=s.nextFire){
        if(s.clip>0)awpTraceShot(p,s);else awpStartReload(p,s);
    }
    p->v.fov=float(awpFov(s));
}
