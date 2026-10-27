// Isolated native-engine coverage; activated only by check_enemy_combat.py.
static const char* enemyCombatProbeClasses[]={"monster_zombie","monster_zombie","monster_zombie","monster_quake_zombie","monster_fastzombie","monster_poisionzombie","monster_human_grunt","monster_czero_zombie","monster_headcrab"};
static edict_t* enemyCombatProbeActors[9]{};
static edict_t *enemyCombatProbePlayer=nullptr,*enemyCombatProbeMinion=nullptr;
static float enemyCombatProbeAt=0;
static bool enemyCombatProbeActive=false,enemyCombatProbeFailed=false;
static bool enemyCombatProbeEnabled(){char flag[8]{};return GetEnvironmentVariableA("CREMATION_ENEMY_COMBAT_VALIDATE",flag,sizeof flag)&&!std::strcmp(flag,"1");}
static void enemyCombatProbeResult(bool ok,const char* detail){if(!ok)enemyCombatProbeFailed=true;startupTrace(ok?"EnemyCombatTest PASS":"EnemyCombatTest FAIL",detail);}
static void enemyCombatProbePrecache(){
 if(!enemyCombatProbeEnabled())return;
 for(auto name:enemyCombatProbeClasses){auto e=engine.pfnCreateNamedEntity(engine.pfnAllocString(name));if(valid(e)){hookSpawn(e);engine.pfnRemoveEntity(e);}}
}
static void enemyCombatProbeBegin(edict_t* list,int count){
 if(!enemyCombatProbeEnabled())return;
 char run[64]{};GetEnvironmentVariableA("CREMATION_ENEMY_COMBAT_RUN",run,sizeof run);startupTrace("EnemyCombatTest BEGIN",run);
 enemyCombatProbeActive=true;enemyCombatProbeAt=globals->time+1.2f;
 for(int i=1;i<count;++i)if(aliveEnemy(&list[i])){list[i].v.health=0;list[i].v.deadflag=DEAD_DEAD;list[i].v.solid=SOLID_NOT;list[i].v.effects|=EF_NODRAW;list[i].v.nextthink=globals->time+100;}
 auto p=engine.pfnCreateFakeClient("CombatProbe");enemyCombatProbePlayer=p;
 if(valid(p)){auto ctor=reinterpret_cast<void(*)(entvars_t*)>(GetProcAddress(ravenModule,"player"));if(ctor)ctor(&p->v);char reject[128]{};raven.pfnClientConnect(p,"CombatProbe","127.0.0.1",reject);raven.pfnClientPutInServer(p);p->v.flags|=FL_NOTARGET|FL_FROZEN;p->v.health=1000;p->v.armorvalue=0;}
 for(int i=0;i<9;++i){
  auto e=engine.pfnCreateNamedEntity(engine.pfnAllocString(i==7?"monster_human_grunt":enemyCombatProbeClasses[i]));enemyCombatProbeActors[i]=e;if(!valid(e))continue;
  e->v.origin=Vector(-160+(i%3)*96,800+(i/3)*96,-319.9f);e->v.targetname=engine.pfnAllocString(i==2?"mansion_enemy_johnny_combatprobe":"mansion_enemy_fixed_combatprobe");
  float chance=steveChance.value;steveChance.value=i==1?1.f:0.f;
  if(i==7){raven.pfnSpawn(e);e->v.classname=engine.pfnAllocString(enemyCombatProbeClasses[i]);czSpawn(e);enemyCombatInstall(e);}
  else hookSpawn(e);
  steveChance.value=chance;e->v.nextthink=globals->time+100;
 }
 auto m=engine.pfnCreateNamedEntity(engine.pfnAllocString("monster_minion"));enemyCombatProbeMinion=m;if(valid(m)){m->v.owner=p;m->v.origin=Vector(-160,1184,-319.9f);hookSpawn(m);m->v.nextthink=globals->time+100;}
}
static int enemyCombatProbeDamage(edict_t* victim,edict_t* source,edict_t* inflictor,int bits=1<<10,float amount=2){
 auto table=*reinterpret_cast<void***>(victim->pvPrivateData);return reinterpret_cast<TakeDamageFn>(table[ravenholm_abi::slot::TakeDamage])(victim->pvPrivateData,&inflictor->v,&source->v,amount,bits);
}
static void enemyCombatProbeFrame(){
 if(!enemyCombatProbeActive||globals->time<enemyCombatProbeAt)return;enemyCombatProbeActive=false;
 auto p=enemyCombatProbePlayer;auto m=enemyCombatProbeMinion;bool allocated=valid(p)&&valid(m);
 for(auto e:enemyCombatProbeActors)allocated=allocated&&valid(e)&&e->pvPrivateData&&enemyCombatHostile(e);
 char allocation[400]{};size_t allocationUsed=0;
 for(int i=0;i<9;++i){auto e=enemyCombatProbeActors[i];int wrote=std::snprintf(allocation+allocationUsed,sizeof(allocation)-allocationUsed,"%s=%d/%d/%d ",enemyCombatProbeClasses[i],int(valid(e)),valid(e)&&e->pvPrivateData,int(enemyCombatHostile(e)));
  if(wrote>0)allocationUsed=std::min(sizeof(allocation)-1,allocationUsed+size_t(wrote));}
 enemyCombatProbeResult(allocated,"all nine enemy types, player and player-owned summon allocated with active hooks");if(!allocated){startupTrace("EnemyCombatTest INCOMPLETE",allocation);return;}
 bool allied=true,blocked=true;
 for(auto victim:enemyCombatProbeActors)for(auto attacker:enemyCombatProbeActors)if(victim!=attacker){
  auto vt=*reinterpret_cast<void***>(victim->pvPrivateData);allied=allied&&reinterpret_cast<int(__thiscall*)(void*,void*)>(vt[63])(victim->pvPrivateData,attacker->pvPrivateData)==-2;
  for(int bits:{1<<1,1<<2,1<<6,1<<20,1<<3}){float health=victim->v.health;int body=victim->v.body,sequence=victim->v.sequence;
   blocked=enemyCombatProbeDamage(victim,attacker,attacker,bits,20)==0&&victim->v.health==health&&victim->v.body==body&&victim->v.sequence==sequence&&blocked;
  }
 }
 enemyCombatProbeResult(allied,"all 72 directed cross-type relationships are allies (no target selection for infighting)");
 enemyCombatProbeResult(blocked,"360 native bullet/melee/blast/acid/fire hits cause no health, anatomy or pain changes");
 auto johnny=enemyCombatProbeActors[2];auto js=davidState(johnny);enemyCombatProbeResult(js&&!js->saved.johnnyDefended&&!js->johnnyGuarding,"friendly hits do not consume Johnny's one-time guard");
 if(js){auto vt=*reinterpret_cast<void***>(johnny->pvPrivateData);TraceResult tr{};tr.pHit=johnny;tr.iHitgroup=1;Vector dir(1,0,0);
  reinterpret_cast<void(__thiscall*)(void*,entvars_t*,float,Vector,TraceResult*,int)>(vt[10])(johnny->pvPrivateData,&enemyCombatProbeActors[6]->v,20,dir,&tr,1<<1);
  enemyCombatProbeResult(!js->saved.johnnyDefended&&!js->johnnyGuarding,"native TraceAttack is rejected before Johnny's guard and multi-damage");
 }
 auto grenade=engine.pfnCreateNamedEntity(engine.pfnAllocString("info_target"));auto nested=engine.pfnCreateNamedEntity(engine.pfnAllocString("info_target"));auto world=engine.pfnPEntityOfEntIndex(0);
 if(valid(grenade)&&valid(nested)){raven.pfnSpawn(grenade);raven.pfnSpawn(nested);grenade->v.owner=enemyCombatProbeActors[6];nested->v.owner=grenade;
  auto victim=enemyCombatProbeActors[0];float before=victim->v.health;
  enemyCombatProbeDamage(victim,world,nested,1<<6,20);enemyCombatProbeResult(victim->v.health==before,"NPC grenade owner chains block splash damage even with world attacker");
  grenade->v.owner=p;enemyCombatProbeDamage(victim,world,nested);enemyCombatProbeResult(victim->v.health<before,"player projectile owner chains retain damage");
  grenade->v.owner=enemyCombatProbeActors[6];before=victim->v.health;enemyCombatProbeDamage(victim,p,grenade);enemyCombatProbeResult(victim->v.health<before,"explicit player damage attribution wins over inflictor owner");
 }
 else enemyCombatProbeResult(false,"projectile fixture allocation");
 if(valid(grenade))engine.pfnRemoveEntity(grenade);if(valid(nested))engine.pfnRemoveEntity(nested);
 bool playerDamage=true,playerRelationship=true;
 for(auto victim:enemyCombatProbeActors){if(auto s=davidState(victim))s->saved.johnnyDefended=1;
  auto vt=*reinterpret_cast<void***>(victim->pvPrivateData);playerRelationship=playerRelationship&&reinterpret_cast<int(__thiscall*)(void*,void*)>(vt[63])(victim->pvPrivateData,p->pvPrivateData)>0;
  float before=victim->v.health;enemyCombatProbeDamage(victim,p,p);playerDamage=playerDamage&&victim->v.health<before;
 }
 enemyCombatProbeResult(playerRelationship,"all nine enemy types preserve native hostility toward the player");
 enemyCombatProbeResult(playerDamage,"player damage reaches all nine native/adapted enemy damage paths");
 float before=p->v.health;enemyCombatProbeDamage(p,enemyCombatProbeActors[0],enemyCombatProbeActors[0]);enemyCombatProbeResult(p->v.health<before,"enemy damage to the player remains active");
 auto victim=enemyCombatProbeActors[0];before=victim->v.health;enemyCombatProbeDamage(victim,m,m);bool summonAttacks=victim->v.health<before;
 before=m->v.health;enemyCombatProbeDamage(m,victim,victim);enemyCombatProbeResult(summonAttacks&&m->v.health<before&&!enemyCombatHostile(m),"player summon remains outside hostile alliance and can exchange damage with enemies");
 before=victim->v.health;enemyCombatProbeDamage(victim,world,world);enemyCombatProbeResult(victim->v.health<before,"world/environmental damage remains active");
 enemyCombatProbeResult(!enemyCombatBlocked(victim->pvPrivateData,&victim->v,&victim->v),"scripted self damage remains available");
 if(enemyCombatEnemyOffset>=0){auto handle=static_cast<char*>(victim->pvPrivateData)+enemyCombatEnemyOffset;auto other=enemyCombatProbeActors[6];
  *reinterpret_cast<edict_t**>(handle)=other;*reinterpret_cast<int*>(handle+4)=other->serialnumber;victim->v.enemy=other;enemyCombatClearTargets(victim);
  bool cleared=*reinterpret_cast<edict_t**>(handle)==nullptr&&victim->v.enemy==nullptr;
  *reinterpret_cast<edict_t**>(handle)=p;*reinterpret_cast<int*>(handle+4)=p->serialnumber;victim->v.enemy=p;enemyCombatClearTargets(victim);
  enemyCombatProbeResult(cleared&&*reinterpret_cast<edict_t**>(handle)==p&&victim->v.enemy==p,"saved/native ally aggro is cleared without discarding the player target");
  *reinterpret_cast<edict_t**>(handle)=nullptr;*reinterpret_cast<int*>(handle+4)=0;victim->v.enemy=nullptr;
 }else enemyCombatProbeResult(false,"native enemy handle metadata");
 for(auto e:enemyCombatProbeActors)enemyCombatInstall(e);
 enemyCombatProbeResult(enemyCombatAllied(victim,enemyCombatProbeActors[6]),"reinstall is idempotent after actor adaptation");
 startupTrace(enemyCombatProbeFailed?"EnemyCombatTest INCOMPLETE":"EnemyCombatTest COMPLETE","hostile alliance, damage ownership, player allies and stale aggro");
}
