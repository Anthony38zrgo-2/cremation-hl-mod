// Original Condition Zero Desert Eagle. Egon is only the native inventory carrier; this
// weapon uses the shared .357 reserve but keeps its own seven-round magazine.
static constexpr const char* DeagleView="models/cremation/deagle/v_deagle.mdl";
static constexpr const char* DeaglePlayer="models/cremation/deagle/p_deagle.mdl";
static constexpr const char* DeagleWorld="models/cremation/deagle/w_deagle.mdl";
static int DeagleHudId=10; // Updated from the native Egon WeaponList.
struct DeagleState {
    int clip=7;
    bool initialized=false,active=false,reloading=false,wasReload=false;
    float nextFire=0,reloadFinish=0,idleAt=0;
};
static std::array<DeagleState,33> deagleStates{};
static bool deaglePlayer(edict_t* p){
    if(!valid(p)||p->v.health<=0||!p->v.viewmodel)return false;
    const char* model=text(p->v.viewmodel);
    return !std::strcmp(model,"models/v_egon.mdl")||!std::strcmp(model,DeagleView);
}
static bool deagleItemClass(const char* name){return name&&(!std::strcmp(name,"weapon_deagle")||!std::strcmp(name,"weapon_egon"));}
static void deaglePrecache(){
    for(auto path:{DeagleView,DeaglePlayer,DeagleWorld})engine.pfnPrecacheModel(const_cast<char*>(path));
    for(auto path:{"weapons/de_clipout.wav","weapons/de_clipin.wav","weapons/de_deploy.wav","weapons/deagle-1.wav","weapons/deagle-2.wav"})engine.pfnPrecacheSound(const_cast<char*>(path));
}
static void deagleHud(edict_t* p){
    int i=engine.pfnIndexOfEdict(p);if(i<1||i>32)return;auto& s=deagleStates[i];
    if(msgCurWeapon&&deaglePlayer(p)){engine.pfnMessageBegin(MSG_ONE,msgCurWeapon,nullptr,p);engine.pfnWriteByte(1);engine.pfnWriteByte(DeagleHudId);engine.pfnWriteByte(s.clip);engine.pfnMessageEnd();}
    if(msgAmmoX&&awpAmmoIndex>=0){engine.pfnMessageBegin(MSG_ONE,msgAmmoX,nullptr,p);engine.pfnWriteByte(awpAmmoIndex);engine.pfnWriteByte(sharedAmmo(p,awpAmmoIndex,0));engine.pfnMessageEnd();}
}
static void deagleReload(edict_t* p,DeagleState& s){
    if(s.reloading||s.clip>=7||awpAmmoIndex<0||sharedAmmo(p,awpAmmoIndex,0)<=0)return;
    animation(p,4);s.reloading=true;s.reloadFinish=globals->time+2.2f;
}
static void deagleShoot(edict_t* p,DeagleState& s){
    if(s.clip<=0)return;
    --s.clip;s.nextFire=globals->time+.28f;s.idleAt=globals->time+1.03f;
    animation(p,s.clip==0?3:engine.pfnRandomLong(1,2));sound(p,engine.pfnRandomLong(0,1)?"weapons/deagle-1.wav":"weapons/deagle-2.wav");
    p->v.effects|=EF_MUZZLEFLASH;p->v.punchangle.x-=3.5f;
    Vector aim=p->v.v_angle+p->v.punchangle;engine.pfnMakeVectors(&aim.x);
    Vector direction=normal(globals->v_forward+globals->v_right*engine.pfnRandomFloat(-.008f,.008f)+globals->v_up*engine.pfnRandomFloat(-.008f,.008f));
    Vector start=p->v.origin+p->v.view_ofs,end=start+direction*4096.f;TraceResult tr{};engine.pfnTraceLine(start,end,0,p,&tr);
    if(valid(tr.pHit)&&tr.pHit->v.takedamage>0&&tr.pHit->pvPrivateData){auto table=*reinterpret_cast<void***>(tr.pHit->pvPrivateData);reinterpret_cast<TakeDamageFn>(table[ravenholm_abi::slot::TakeDamage])(tr.pHit->pvPrivateData,&p->v,&p->v,48.f,1<<1);}
    else if(tr.flFraction<1.f){engine.pfnMessageBegin(MSG_PVS,SvcTemp,&tr.vecEndPos.x,nullptr);engine.pfnWriteByte(TE_GUNSHOT);coords(tr.vecEndPos);engine.pfnMessageEnd();}
    deagleHud(p);
}
static void deaglePostThink(edict_t* p,int buttons){
    int i=engine.pfnIndexOfEdict(p);if(i<1||i>32)return;auto& s=deagleStates[i];bool active=deaglePlayer(p);
    if(!active){s.active=false;s.reloading=false;s.wasReload=false;return;}
    p->v.viewmodel=engine.pfnAllocString(DeagleView);p->v.weaponmodel=engine.pfnAllocString(DeaglePlayer);
    if(!s.initialized){s.initialized=true;s.clip=7;deagleHud(p);}
    if(!s.active){s.active=true;s.nextFire=std::max(s.nextFire,globals->time+1.f);animation(p,5);}
    if((buttons&Reload)&&!s.wasReload)deagleReload(p,s);s.wasReload=(buttons&Reload)!=0;
    if(s.reloading){if(globals->time>=s.reloadFinish){int reserve=sharedAmmo(p,awpAmmoIndex,0),load=std::min(7-s.clip,reserve);if(load>0){sharedAmmo(p,awpAmmoIndex,load);s.clip+=load;}s.reloading=false;s.nextFire=globals->time+.15f;s.idleAt=0;animation(p,0);deagleHud(p);}}
    else if(s.idleAt>0&&globals->time>=s.idleAt){animation(p,0);s.idleAt=0;}
    if(!s.reloading&&(buttons&Attack)&&globals->time>=s.nextFire){if(s.clip>0)deagleShoot(p,s);else deagleReload(p,s);}
}
