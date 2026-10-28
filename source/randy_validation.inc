// Compare the adapted actor with the real native fast zombie in isolated HLDS.
static void randyProbeRun(edict_t* player,const Vector& at){
 auto pass=[](bool ok,const char* name){startupTrace(ok?"RandyTest PASS":"RandyTest FAIL",name);};
 float damage[2][2]{},health[2]{};Vector original=player->v.origin;
 for(int adapted=0;adapted<2;++adapted){
  auto npc=engine.pfnCreateNamedEntity(engine.pfnAllocString(adapted?"monster_randy":"monster_fastzombie"));
  if(!valid(npc)){pass(false,"exported entity allocation");continue;}
  npc->v.origin=at;npc->v.angles=Vector(0,0,0);
  if(adapted)hookSpawn(npc);else raven.pfnSpawn(npc);
  npc->v.nextthink=globals->time+1000;health[adapted]=npc->v.health;
  auto table=*reinterpret_cast<void***>(npc->pvPrivateData);
  if(adapted){
   pass(!std::strcmp(text(npc->v.model),RandyModel)&&!davidState(npc),"Randy model uses native fast AI rather than David hooks");
   auto model=static_cast<unsigned char*>(engine.pfnGetModelPtr(npc));
   pass(model&&*reinterpret_cast<int*>(model+156)==15,"original Randy regional hitboxes loaded by GoldSrc");
  }
  Vector target=npc->v.origin+Vector(42,0,36);engine.pfnSetOrigin(player,&target.x);
  for(int hit=0;hit<2;++hit){player->v.health=1000;player->v.armorvalue=0;MonsterEvent_t event{};event.event=hit+1;
   reinterpret_cast<void(__thiscall*)(void*,MonsterEvent_t*)>(table[ravenholm_abi::slot::HandleAnimEvent])(npc->pvPrivateData,&event);
   damage[adapted][hit]=1000-player->v.health;
  }
  if(adapted){
   reinterpret_cast<void(__thiscall*)(void*,int)>(table[ravenholm_abi::slot::SetActivity])(npc->pvPrivateData,3);
   pass(npc->v.sequence==1,"native fast locomotion selects Randy run sequence");
   reinterpret_cast<TakeDamageFn>(table[ravenholm_abi::slot::TakeDamage])(npc->pvPrivateData,&player->v,&player->v,10000,1<<6);
   pass(!aliveEnemy(npc),"native damage kills Randy and releases wave count");
  }
  if(valid(npc))engine.pfnRemoveEntity(npc);
 }
 char report[180];std::snprintf(report,sizeof report,"native=%.1f/%.1f Randy=%.1f/%.1f health=%.1f/%.1f",damage[0][0],damage[0][1],damage[1][0],damage[1][1],health[0],health[1]);startupTrace("RandyProbe",report);
 pass(health[0]>0&&health[0]==health[1],"native fast-zombie health preserved");
 pass(damage[0][0]>0&&damage[0][1]>0&&damage[0][0]==damage[1][0]&&damage[0][1]==damage[1][1],"both native fast-zombie claw contacts preserve actual damage");
 engine.pfnSetOrigin(player,&original.x);player->v.health=1000;startupTrace("RandyTestComplete");
}
