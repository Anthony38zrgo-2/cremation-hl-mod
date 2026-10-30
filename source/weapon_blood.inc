// Weapon impacts use the same engine particle-trail primitive as studio gib flags.
static int msgGibBlood=0;
struct BloodHit{int serial=0;float time=-100;Vector point{0,0,0},direction{0,0,1};};static std::vector<BloodHit> bloodHits;
static decltype(engine.pfnTraceLine) originalBloodTrace=nullptr;
static void weaponBloodTrace(const float* from,const float* to,int flags,edict_t* skip,TraceResult* tr){
 originalBloodTrace(from,to,flags,skip,tr);if(!tr||!valid(tr->pHit)||tr->flFraction>=1||!tr->pHit->v.takedamage)return;int index=engine.pfnIndexOfEdict(tr->pHit);if(index<1||index>=globals->maxEntities)return;if(index>=int(bloodHits.size()))bloodHits.resize(globals->maxEntities);bloodHits[index]={tr->pHit->serialnumber,globals->time,tr->vecEndPos,normal(Vector(from[0]-to[0],from[1]-to[1],from[2]-to[2]))};
}
struct BloodDamageTable{void** table;TakeDamageFn original;void* killed;};static std::vector<BloodDamageTable> bloodDamageTables;
struct BloodStamp{int serial=0;float next=0;};static std::vector<BloodStamp> bloodStamps;
static int __fastcall weaponBloodDamage(void* object,void*,entvars_t* inflictor,entvars_t* attacker,float amount,int bits){
 if(enemyCombatBlocked(object,inflictor,attacker))return 0;
 amount*=dmcDamageScale(attacker)*davidMeleeScale(attacker);
 auto table=*reinterpret_cast<void***>(object);TakeDamageFn original=nullptr;for(auto& t:bloodDamageTables)if(t.table==table){original=t.original;break;}if(!original)return 0;
 edict_t* victim=nullptr;for(int i=1;i<globals->maxEntities;++i){auto e=engine.pfnPEntityOfEntIndex(i);if(valid(e)&&e->pvPrivateData==object){victim=e;break;}}
 // Herir a un enemigo lo compromete: aunque no lo vea, ya no retomara rutas.
 if(valid(victim)&&attacker)mansionEngageNotify(victim,attacker);
 bool wasAlive=aliveEnemy(victim);Vector lootAt=valid(victim)?center(victim):Vector(0,0,0);
 bool eligible=valid(victim)&&victim->v.health>0&&amount>0&&attacker&&(attacker->flags&FL_CLIENT);float before=eligible?victim->v.health:0;int serial=eligible?victim->serialnumber:0,color=-1;Vector at(0,0,0),direction(0,0,1);
 if(eligible){color=reinterpret_cast<int(__thiscall*)(void*)>(table[ravenholm_abi::slot::BloodColor])(object);at=center(victim);Vector source=(inflictor&&inflictor!=attacker)?inflictor->origin:attacker->origin+attacker->view_ofs;direction=normal(source-at);at=at+direction*std::min(12.f,length(victim->v.maxs-victim->v.mins)*.15f);int index=engine.pfnIndexOfEdict(victim);if(index<int(bloodHits.size())){auto& hit=bloodHits[index];if(hit.serial==serial&&globals->time-hit.time<=.025f){at=hit.point;direction=hit.direction;}}}
 int result=original(object,inflictor,attacker,amount,bits);
 if(wasAlive&&!aliveEnemy(victim)&&heartRollDeath(victim))heartSpawn(lootAt);
 if(eligible&&color>=0&&victim&&!victim->free&&victim->serialnumber==serial&&(victim->v.health<before||(result>0&&!std::strcmp(text(victim->v.classname),"monster_quake_zombie")))&&msgGibBlood){
  int index=engine.pfnIndexOfEdict(victim);if(index>=int(bloodStamps.size()))bloodStamps.resize(globals->maxEntities);auto& stamp=bloodStamps[index];
  if(stamp.serial!=serial){stamp.serial=serial;stamp.next=0;}
  bool red=color==247;if(globals->time>=stamp.next&&engine.pfnCVarGetFloat(red?"violence_hblood":"violence_ablood")!=0){stamp.next=globals->time+.04f;engine.pfnMessageBegin(MSG_PVS,msgGibBlood,&at.x,nullptr);coords(at);coords(direction);engine.pfnWriteByte(red?2:3);engine.pfnWriteByte(2*std::min(5,std::max(2,int(amount/12)+2)));engine.pfnMessageEnd();}
 }return result;
}
static void __fastcall weaponBloodKilled(void* object,void*,entvars_t* attacker,int gib){
 auto table=*reinterpret_cast<void***>(object);void* original=nullptr;for(auto& t:bloodDamageTables)if(t.table==table){original=t.killed;break;}
 edict_t* victim=nullptr;for(int i=1;i<globals->maxEntities;++i){auto e=engine.pfnPEntityOfEntIndex(i);if(valid(e)&&e->pvPrivateData==object){victim=e;break;}}
 zombieVoicePlay(victim,ZombieVoiceEvent::Death);
 bool drop=heartRollDeath(victim);Vector at=valid(victim)?center(victim):Vector(0,0,0);
 if(original)reinterpret_cast<void(__thiscall*)(void*,entvars_t*,int)>(original)(object,attacker,gib);
 if(drop)heartSpawn(at);
}
static void weaponBloodInstall(edict_t* e){
 if(!valid(e)||!e->pvPrivateData||!(e->v.flags&FL_MONSTER))return;auto table=*reinterpret_cast<void***>(e->pvPrivateData);for(auto& t:bloodDamageTables)if(t.table==table)return;DWORD old;if(VirtualProtect(table+ravenholm_abi::slot::TakeDamage,sizeof(void*)*3,PAGE_READWRITE,&old)){bloodDamageTables.push_back({table,reinterpret_cast<TakeDamageFn>(table[ravenholm_abi::slot::TakeDamage]),table[ravenholm_abi::slot::Killed]});table[ravenholm_abi::slot::TakeDamage]=reinterpret_cast<void*>(&weaponBloodDamage);table[ravenholm_abi::slot::Killed]=reinterpret_cast<void*>(&weaponBloodKilled);DWORD ignored;VirtualProtect(table+ravenholm_abi::slot::TakeDamage,sizeof(void*)*3,old,&ignored);}
}
