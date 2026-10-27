// UT99 Shock Rifle: authoritative hitscan, flying core and beam/core combo.
static constexpr const char* ShockView="models/cremation/v_shockrifle.mdl";
static int shockAmmoIndex=-1,msgShockFx=0,shockSprite=0;
struct ShockState{float next=0,idle=0;bool active=false;};static std::array<ShockState,33> shockStates{};
struct ShockOrb{edict_t* entity;int serial;edict_t* owner;int ownerSerial;Vector velocity;float expires;float nextLight=0;};static std::vector<ShockOrb> shockOrbs;
static bool shockRifle(edict_t* p){return valid(p)&&p->v.health>0&&p->v.viewmodel&&(!std::strcmp(text(p->v.viewmodel),"models/v_gauss.mdl")||!std::strcmp(text(p->v.viewmodel),ShockView));}
static void shockPrecache(){
 for(auto path:{ShockView,"models/cremation/p_shockrifle.mdl","models/cremation/w_shockrifle.mdl"})engine.pfnPrecacheModel(const_cast<char*>(path));
 shockSprite=engine.pfnPrecacheModel(const_cast<char*>("sprites/cremation/shock_core.spr"));
 for(auto path:{"weapons/shockrifle/fire.wav","weapons/shockrifle/altfire.wav","weapons/shockrifle/equip.wav","weapons/shockrifle/impact.wav","weapons/shockrifle/combo.wav"})engine.pfnPrecacheSound(const_cast<char*>(path));msgShockFx=engine.pfnRegUserMsg("ShockFx",8);
}
static void shockFx(edict_t* p,int mode,const Vector& at){if(!msgShockFx)return;for(int i=1;i<=globals->maxClients;++i){auto viewer=engine.pfnPEntityOfEntIndex(i);if(!valid(viewer)||!(viewer->v.flags&FL_CLIENT))continue;engine.pfnMessageBegin(MSG_ONE_UNRELIABLE,msgShockFx,nullptr,viewer);engine.pfnWriteByte(engine.pfnIndexOfEdict(p));engine.pfnWriteByte(mode);coords(at);engine.pfnMessageEnd();}}
static void shockHurt(edict_t* e,edict_t* inflictor,edict_t* owner,float amount,bool blast,const Vector& direction){
 if(!valid(e)||!e->pvPrivateData||!e->v.takedamage||amount<=0)return;auto table=*reinterpret_cast<void***>(e->pvPrivateData);reinterpret_cast<TakeDamageFn>(table[ravenholm_abi::slot::TakeDamage])(e->pvPrivateData,&inflictor->v,&owner->v,amount,(1<<10)|(blast?(1<<6):0));if(aliveEnemy(e))applyPush(e,direction,blast?260.f:180.f,blast?130.f:45.f,.15f);
}
static void shockExplode(ShockOrb& orb,edict_t* attacker,bool combo,edict_t* direct=nullptr,const Vector& surface=Vector(0,0,0)){
 auto entity=orb.entity;if(!valid(entity)||entity->serialnumber!=orb.serial){orb.entity=nullptr;return;}Vector at=entity->v.origin+surface*4;float radius=combo?250.f:70.f,maximum=combo?165.f:55.f;
 sound(entity,combo?"weapons/shockrifle/combo.wav":"weapons/shockrifle/impact.wav");
 engine.pfnMessageBegin(MSG_PVS,SvcTemp,&at.x,nullptr);engine.pfnWriteByte(TE_DLIGHT);coords(at);engine.pfnWriteByte(combo?30:12);engine.pfnWriteByte(180);engine.pfnWriteByte(50);engine.pfnWriteByte(255);engine.pfnWriteByte(4);engine.pfnWriteByte(10);engine.pfnMessageEnd();

 engine.pfnMessageBegin(MSG_PVS,SvcTemp,&at.x,nullptr);engine.pfnWriteByte(TE_SPRITE);coords(at);engine.pfnWriteShort(shockSprite);engine.pfnWriteByte(combo?20:7);engine.pfnWriteByte(240);engine.pfnMessageEnd();
 engine.pfnMessageBegin(MSG_PVS,SvcTemp,&at.x,nullptr);engine.pfnWriteByte(TE_BEAMCYLINDER);coords(at);coords(at+Vector(0,0,radius));engine.pfnWriteShort(trailSprite);engine.pfnWriteByte(0);engine.pfnWriteByte(12);engine.pfnWriteByte(3);engine.pfnWriteByte(combo?24:12);engine.pfnWriteByte(0);engine.pfnWriteByte(180);engine.pfnWriteByte(50);engine.pfnWriteByte(255);engine.pfnWriteByte(220);engine.pfnWriteByte(0);engine.pfnMessageEnd();
 for(int i=1;i<globals->maxEntities;++i){auto e=engine.pfnPEntityOfEntIndex(i);if(!valid(e)||!e->v.takedamage||e==entity)continue;Vector to=center(e);float distance=length(to-at);if(e!=direct&&distance>radius)continue;TraceResult tr;engine.pfnTraceLine(&at.x,&to.x,0,entity,&tr);if(e!=direct&&tr.flFraction<1&&tr.pHit!=e)continue;float amount=e==direct?maximum:maximum*std::max(0.f,1-distance/radius);shockHurt(e,entity,attacker,amount,true,normal(to-at));}
 entity->v.flags|=FL_KILLME;entity->v.effects|=EF_NODRAW;orb.entity=nullptr;
}
static void shockPostThink(edict_t* p,int buttons){
 int i=engine.pfnIndexOfEdict(p);if(i<1||i>32)return;auto& s=shockStates[i];bool active=shockRifle(p);
 if(shockAmmoIndex>=0&&shockAmmoIndex<32&&ammoOffset>=0&&p->pvPrivateData){int* ammo=reinterpret_cast<int*>(static_cast<char*>(p->pvPrivateData)+ammoOffset)+shockAmmoIndex;if(*ammo>50)*ammo=50;}
 if(active){p->v.viewmodel=engine.pfnAllocString(ShockView);p->v.weaponmodel=engine.pfnAllocString("models/cremation/p_shockrifle.mdl");
 if(!s.active){animation(p,3);sound(p,"weapons/shockrifle/equip.wav");s.next=std::max(s.next,globals->time+.5f);s.idle=s.next;}
 if(globals->time>=s.next&&(buttons&(Attack|Attack2))&&shockAmmoIndex>=0&&sharedAmmo(p,shockAmmoIndex,0)>0){
 bool alt=(buttons&Attack2)!=0;Vector aim=p->v.v_angle+p->v.punchangle;engine.pfnMakeVectors(&aim.x);Vector dir=globals->v_forward,eye=p->v.origin+p->v.view_ofs;bool fired=false;
 if(alt&&shockOrbs.size()<64){Vector origin=eye+dir*16;TraceResult obstruction;engine.pfnTraceLine(&eye.x,&origin.x,0,p,&obstruction);if(obstruction.flFraction<1)origin=obstruction.vecEndPos-dir*2;
 auto orb=plasmaLayer(p,origin,"sprites/cremation/shock_core.spr",.3f,255);if(orb){orb->v.targetname=engine.pfnAllocString("cremation_shock_core");shockOrbs.push_back({orb,orb->serialnumber,p,p->serialnumber,dir*500.f,globals->time+10});engine.pfnMessageBegin(MSG_BROADCAST,SvcTemp,nullptr,nullptr);engine.pfnWriteByte(TE_BEAMFOLLOW);engine.pfnWriteShort(engine.pfnIndexOfEdict(orb));engine.pfnWriteShort(trailSprite);engine.pfnWriteByte(3);engine.pfnWriteByte(4);engine.pfnWriteByte(180);engine.pfnWriteByte(50);engine.pfnWriteByte(255);engine.pfnWriteByte(160);engine.pfnMessageEnd();shockFx(p,1,origin);fired=true;}
 }else if(!alt){Vector end=eye+dir*8192;TraceResult tr;engine.pfnTraceLine(&eye.x,&end.x,0,p,&tr);float nearest=8192*tr.flFraction;ShockOrb* hit=nullptr;
 for(auto& orb:shockOrbs)if(valid(orb.entity)&&orb.entity->serialnumber==orb.serial){Vector delta=orb.entity->v.origin-eye;float t=dot(delta,dir);if(t<0||t>nearest)continue;if(length(delta-dir*t)<=12){hit=&orb;nearest=t;}}
 Vector at=hit?hit->entity->v.origin:Vector(tr.vecEndPos);shockFx(p,0,at);if(hit){int remaining=sharedAmmo(p,shockAmmoIndex,0);sharedAmmo(p,shockAmmoIndex,std::min(2,std::max(0,remaining-1)));shockExplode(*hit,p,true);}else if(valid(tr.pHit))shockHurt(tr.pHit,p,p,40,false,dir);fired=true;
 }
 if(fired){sharedAmmo(p,shockAmmoIndex,1);animation(p,alt?2:1);sound(p,alt?"weapons/shockrifle/altfire.wav":"weapons/shockrifle/fire.wav");p->v.punchangle.x-=alt?1.f:2.f;s.next=globals->time+(alt?.85f:.71f);s.idle=globals->time+.48f;}
 }
 if(globals->time>=s.idle&&p->v.weaponanim!=0)animation(p,0);
 }s.active=active;
}
static void shockStep(float dt){
 for(auto& orb:shockOrbs){auto e=orb.entity;if(!valid(e)||e->serialnumber!=orb.serial){orb.entity=nullptr;continue;}if(!valid(orb.owner)||orb.owner->serialnumber!=orb.ownerSerial){e->v.flags|=FL_KILLME;orb.entity=nullptr;continue;}if(globals->time>=orb.expires){shockExplode(orb,orb.owner,false);continue;}
 Vector from=e->v.origin,to=from+orb.velocity*dt;TraceResult best;engine.pfnTraceLine(&from.x,&to.x,0,orb.owner,&best);
 // Sweep side rays too, so the luminous sphere cannot skim through narrow edges.
 for(Vector offset:{Vector(6,0,0),Vector(-6,0,0),Vector(0,6,0),Vector(0,-6,0),Vector(0,0,6),Vector(0,0,-6)}){Vector a=from+offset,b=to+offset;TraceResult tr;engine.pfnTraceLine(&a.x,&b.x,0,orb.owner,&tr);if(tr.flFraction<best.flFraction){best=tr;best.vecEndPos=from+(to-from)*tr.flFraction;}}
 if(globals->time>=orb.nextLight){orb.nextLight=globals->time+.1f;engine.pfnMessageBegin(MSG_PVS,SvcTemp,&e->v.origin.x,nullptr);engine.pfnWriteByte(TE_DLIGHT);coords(e->v.origin);engine.pfnWriteByte(8);engine.pfnWriteByte(180);engine.pfnWriteByte(50);engine.pfnWriteByte(255);engine.pfnWriteByte(2);engine.pfnWriteByte(5);engine.pfnMessageEnd();}
 engine.pfnSetOrigin(e,&best.vecEndPos.x);e->v.frame=std::fmod(globals->time*16,16.f);if(best.flFraction<1||best.fStartSolid)shockExplode(orb,orb.owner,false,best.pHit,best.vecPlaneNormal);
 }
 shockOrbs.erase(std::remove_if(shockOrbs.begin(),shockOrbs.end(),[](const ShockOrb& o){return !o.entity;}),shockOrbs.end());
}
