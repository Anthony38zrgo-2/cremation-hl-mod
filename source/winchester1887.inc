static constexpr const char* WinchesterView="models/cremation/v_winchester1887.mdl";
struct WinchesterState{float next=0,idle=0;bool active=false;};
static std::array<WinchesterState,33> winchesterStates{};
static bool winchester(edict_t* p){return valid(p)&&p->v.health>0&&p->v.viewmodel&&(!std::strcmp(text(p->v.viewmodel),"models/v_357.mdl")||!std::strcmp(text(p->v.viewmodel),WinchesterView));}
static void winchesterPrecache(){for(auto p:{WinchesterView,"models/cremation/p_winchester1887.mdl","models/cremation/w_winchester1887.mdl"})engine.pfnPrecacheModel(const_cast<char*>(p));for(auto p:{"weapons/m1887/sbarrel1.wav","weapons/m1887/dbarrel1.wav","weapons/m1887/scock1.wav","weapons/m1887/m1887_after.wav","weapons/m1887/m1887_draw.wav","weapons/m1887/m1887_insert.wav","weapons/m1887/m1887_start.wav","weapons/m1887/reload3.wav"})engine.pfnPrecacheSound(const_cast<char*>(p));}
static void winchesterPostThink(edict_t* p,int buttons){int index=engine.pfnIndexOfEdict(p);auto& s=winchesterStates[index];bool active=winchester(p);if(active){p->v.viewmodel=engine.pfnAllocString(WinchesterView);p->v.weaponmodel=engine.pfnAllocString("models/cremation/p_winchester1887.mdl");
 if(!s.active){
  if(msgWeaponList){const int fields[]={shotgunAmmoIndex,125,255,255,2,4,3,1};engine.pfnMessageBegin(MSG_ONE,msgWeaponList,nullptr,p);engine.pfnWriteString("weapon_winchester1887");for(int v:fields)engine.pfnWriteByte(v);engine.pfnMessageEnd();}
  animation(p,6);s.next=globals->time+1.03f;s.idle=s.next;}
 if((buttons&Attack)&&globals->time>=s.next&&sharedShells(p,0)>0&&sharedShells(p,1)>=0){
  animation(p,1);sound(p,"weapons/m1887/sbarrel1.wav");p->v.effects|=EF_MUZZLEFLASH;p->v.punchangle.x-=5;engine.pfnMakeVectors(&p->v.v_angle.x);Vector from=p->v.origin+p->v.view_ofs,forward=globals->v_forward,right=globals->v_right,up=globals->v_up;
  for(int i=0;i<8;++i){Vector dir=normal(forward+right*engine.pfnRandomFloat(-.07f,.07f)+up*engine.pfnRandomFloat(-.07f,.07f)),end=from+dir*2048;TraceResult tr;engine.pfnTraceLine(&from.x,&end.x,0,p,&tr);if(valid(tr.pHit)&&tr.pHit->v.takedamage&&tr.pHit->pvPrivateData){auto table=*reinterpret_cast<void***>(tr.pHit->pvPrivateData);reinterpret_cast<TakeDamageFn>(table[ravenholm_abi::slot::TakeDamage])(tr.pHit->pvPrivateData,&p->v,&p->v,8.f,1<<1);}else if(tr.flFraction<1){engine.pfnMessageBegin(MSG_PVS,SvcTemp,&tr.vecEndPos.x,nullptr);engine.pfnWriteByte(TE_GUNSHOT);coords(tr.vecEndPos);engine.pfnMessageEnd();}}
  s.next=globals->time+1.04f;s.idle=s.next;
 }
 if(globals->time>=s.idle&&p->v.weaponanim!=0)animation(p,0);
 }s.active=active;
}
