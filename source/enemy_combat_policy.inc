// One policy for hostile NPCs. Damage hooks call this before native retaliation,
// guard, pain, anatomy or loot; players and their summons retain normal damage.
static std::unordered_map<void*,EntityHandle> enemyCombatEntities;
static int enemyCombatEnemyOffset=-2;
static int cultistFindEnemyOffset();
static edict_t* enemyCombatEntity(void* object){
 if(!object)return nullptr;
 auto it=enemyCombatEntities.find(object);
 if(it!=enemyCombatEntities.end()){auto e=it->second.resolve();if(e&&e->pvPrivateData==object)return e;enemyCombatEntities.erase(it);}
 for(int i=1;i<globals->maxEntities;++i){auto e=engine.pfnPEntityOfEntIndex(i);if(valid(e)&&e->pvPrivateData==object){enemyCombatEntities[object]=EntityHandle(e);return e;}}
 return nullptr;
}
static edict_t* enemyCombatPrincipal(edict_t* entity){
 edict_t* seen[8]{};int count=0;
 while(valid(entity)&&count<8){
  if(entity->v.flags&FL_CLIENT)return entity;
  for(int i=0;i<count;++i)if(seen[i]==entity)return entity;
  seen[count++]=entity;auto owner=entity->v.owner;
  if(!valid(owner)||engine.pfnIndexOfEdict(owner)<=0||owner==entity)return entity;
  entity=owner;
 }
 return entity;
}
static bool enemyCombatHostile(edict_t* entity){
 if(!valid(entity)||!(entity->v.flags&FL_MONSTER)||(entity->v.flags&FL_CLIENT)||!entity->pvPrivateData)return false;
 const char* name=text(entity->v.classname);
 if(name&&(!std::strcmp(name,"monster_minion")||!std::strcmp(name,"cremation_sentry")))return false;
 auto principal=enemyCombatPrincipal(entity);if(valid(principal)&&(principal->v.flags&FL_CLIENT))return false;
 auto table=*reinterpret_cast<void***>(entity->pvPrivateData);
 int faction=reinterpret_cast<int(__thiscall*)(void*)>(table[ravenholm_abi::slot::Classify])(entity->pvPrivateData);
 return faction!=0&&faction!=2&&faction!=3&&faction!=11&&faction!=12&&faction!=14;
}
static bool enemyCombatAllied(edict_t* a,edict_t* b){return enemyCombatHostile(a)&&enemyCombatHostile(b);}
static bool enemyCombatBlocked(void* victim,entvars_t* inflictor,entvars_t* attacker){
 auto target=enemyCombatEntity(victim);if(!enemyCombatHostile(target))return false;
 auto source=attacker?attacker->pContainingEntity:nullptr;
 // Explicit player/ally credit wins over projectile ownership. Only world or
 // missing attackers need the inflictor's owner chain to establish provenance.
 if(!valid(source)||engine.pfnIndexOfEdict(source)<=0)source=inflictor?inflictor->pContainingEntity:nullptr;
 source=enemyCombatPrincipal(source);
 return source!=target&&enemyCombatAllied(target,source);
}
static int __fastcall enemyCombatRelationship(void* object,void*,void* other){
 if(enemyCombatAllied(enemyCombatEntity(object),enemyCombatEntity(other)))return -2; // R_AL
 return reinterpret_cast<int(__thiscall*)(void*,void*)>(ravenholm_abi::address(ravenModule,ravenholm_abi::MonsterRelationshipRva))(object,other);
}
static void enemyCombatClearTargets(edict_t* entity){
 if(!valid(entity)||!entity->pvPrivateData)return;
 auto registered=enemyCombatEntities.find(entity->pvPrivateData);
 if(registered==enemyCombatEntities.end()||registered->second.resolve()!=entity)return;
 if(!enemyCombatHostile(entity))return;
 if(enemyCombatAllied(entity,entity->v.enemy))entity->v.enemy=nullptr;
 if(enemyCombatEnemyOffset==-2)enemyCombatEnemyOffset=cultistFindEnemyOffset();
 if(enemyCombatEnemyOffset<0)return;
 auto handle=static_cast<char*>(entity->pvPrivateData)+enemyCombatEnemyOffset;
 auto target=*reinterpret_cast<edict_t**>(handle);auto pointer=reinterpret_cast<uintptr_t>(target);
 auto start=reinterpret_cast<uintptr_t>(engine.pfnPEntityOfEntIndex(0));
 if(pointer<start||pointer>=start+sizeof(edict_t)*globals->maxEntities||(pointer-start)%sizeof(edict_t))return;
 if(valid(target)&&target->serialnumber==*reinterpret_cast<int*>(handle+4)&&enemyCombatAllied(entity,target)){
  *reinterpret_cast<edict_t**>(handle)=nullptr;*reinterpret_cast<int*>(handle+4)=0;
 }
}
static void enemyCombatInstall(edict_t* entity){
 if(!valid(entity)||!entity->pvPrivateData)return;
 enemyCombatEntities[entity->pvPrivateData]=EntityHandle(entity);
 if(!enemyCombatHostile(entity))return;
 auto base=reinterpret_cast<unsigned char*>(ravenModule);
 // Native Classify calls at +0x20 verify the original relationship ABI.
 if(!base||std::memcmp(ravenholm_abi::address(ravenModule,ravenholm_abi::MonsterRelationshipRva),"\x55\x8b\xec\x8b\x01\x56\xff\x50\x20",9)){startupTrace("EnemyCombat ERROR","unsupported relationship ABI");return;}
 auto table=*reinterpret_cast<void***>(entity->pvPrivateData);auto replacement=reinterpret_cast<void*>(&enemyCombatRelationship);
 auto slot=table+ravenholm_abi::slot::Relationship;
 if(*slot!=replacement){DWORD old;if(!VirtualProtect(slot,sizeof(void*),PAGE_READWRITE,&old)){startupTrace("EnemyCombat ERROR","relationship hook protection");return;}
  *slot=replacement;DWORD ignored;VirtualProtect(slot,sizeof(void*),old,&ignored);
 }
 enemyCombatClearTargets(entity);
}
static void enemyCombatReset(){enemyCombatEntities.clear();enemyCombatEnemyOffset=-2;}
