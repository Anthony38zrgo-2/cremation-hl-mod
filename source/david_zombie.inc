// David adapts the live Ravenholm CZombie. The donor owns navigation, schedules,
// hitgroup multipliers, health, melee damage, ordinary death and whole-body gibs.
#include "david_assets.h"
#include "david_logic.h"
#include "monsterevent.h"
#include <cstddef>
static constexpr const char* DavidModel="models/cremation/hotd2/david/david.mdl";
static constexpr const char* DavidCharred="models/cremation/hotd2/david/david_charred.mdl";
static constexpr const char* DavidGibTag="cremation_david_gib";
static cvar_t davidEnable={const_cast<char*>("cremation_david"),const_cast<char*>("1"),FCVAR_SERVER,1,nullptr};
static cvar_t davidRadius={const_cast<char*>("david_crowd_radius"),const_cast<char*>("256"),FCVAR_SERVER,256,nullptr};
static cvar_t davidClearRadius={const_cast<char*>("david_clear_radius"),const_cast<char*>("320"),FCVAR_SERVER,320,nullptr};
static cvar_t davidHeadThreshold={const_cast<char*>("david_head_fraction"),const_cast<char*>("0.35"),FCVAR_SERVER,.35f,nullptr};
static cvar_t davidLimbThreshold={const_cast<char*>("david_limb_fraction"),const_cast<char*>("0.25"),FCVAR_SERVER,.25f,nullptr};
static cvar_t davidLocalizedDamage={const_cast<char*>("cremation_david_local_damage"),const_cast<char*>("0"),FCVAR_SERVER,0,nullptr};
static bool davidCvarsRegistered=false;
struct DavidSaved {int version=2;int severMask=0;float regional[16]{};float initialHealth=0;int prone=0;};
static TYPEDESCRIPTION davidSaveFields[]={
 {FIELD_INTEGER,const_cast<char*>("version"),offsetof(DavidSaved,version),1,0},
 {FIELD_INTEGER,const_cast<char*>("severMask"),offsetof(DavidSaved,severMask),1,0},
 {FIELD_FLOAT,const_cast<char*>("regional"),offsetof(DavidSaved,regional),16,0},
 {FIELD_FLOAT,const_cast<char*>("initialHealth"),offsetof(DavidSaved,initialHealth),1,0},
 {FIELD_INTEGER,const_cast<char*>("prone"),offsetof(DavidSaved,prone),1,0}
};
struct DavidBonePose {Vector origin{0,0,0},axis[3];};
struct DavidState {
 edict_t* entity=nullptr;int serial=0;DavidSaved saved;
 edict_t* target=nullptr;int targetSerial=0;float movedAt=-100,clearSince=-1,cooldown=0;bool running=false;
 int nextAttack=0;bool attackEventConsumed=false;
 float pendingAt=-100;entvars_t* pendingAttacker=nullptr;float pending[16]{};
 float directAt=-100;entvars_t* directAttacker=nullptr;int directNode=0;Vector directPoint{0,0,0},directDirection{0,0,1};
 int lastNode=0;Vector lastPoint{0,0,0};float lastTrace=-100;
 float poseAt=-100,poseFrame=0;int poseSequence=-1,poseBody=-1;Vector poseOrigin{0,0,0},poseAngles{0,0,0};
 DavidBonePose pose[16];bool inDamage=false,killedDuringDamage=false;float healthAtKilled=0;
};
static std::vector<DavidState> davidStates;
struct DavidTable {void** source;std::array<void*,125> slots;void* trace;TakeDamageFn damage;void* killed;void* event;void* move;void* activity;};
static std::vector<std::unique_ptr<DavidTable>> davidTables;
static float davidNextDecision=0;
static bool davidClass(edict_t* e){return valid(e)&&e->v.classname&&!std::strcmp(text(e->v.classname),"monster_zombie");}
static bool davidModel(edict_t* e){return valid(e)&&e->v.model&&(!std::strcmp(text(e->v.model),DavidModel)||!std::strcmp(text(e->v.model),DavidCharred));}
static DavidState* davidState(edict_t* e,bool create=false){
 if(!valid(e)||!engine.pfnIndexOfEdict)return nullptr;int i=engine.pfnIndexOfEdict(e);if(i<1||i>=globals->maxEntities)return nullptr;
 if(i>=int(davidStates.size())){if(!create)return nullptr;davidStates.resize(globals->maxEntities);}
 auto& s=davidStates[i];if(s.entity!=e||s.serial!=e->serialnumber){if(!create)return nullptr;s=DavidState{};s.entity=e;s.serial=e->serialnumber;}
 return &s;
}
static edict_t* davidObject(void* object){auto v=reinterpret_cast<entvars_t**>(object)[1];return v?v->pContainingEntity:nullptr;}
static DavidTable* davidTable(void* object){auto table=*reinterpret_cast<void***>(object);for(auto& t:davidTables)if(t->slots.data()==table)return t.get();return nullptr;}
static int davidBody(edict_t* e,int node){return (e->v.body/david_assets::bodyBase[node])%david_assets::bodyCount[node];}
static void davidSetBody(edict_t* e,int node,int state){int base=david_assets::bodyBase[node];e->v.body+=(state-davidBody(e,node))*base;}
static bool davidVisiblePart(edict_t* e,int node){return node==1||node==9||davidBody(e,node)<david_assets::bodyCount[node]-1;}
static bool davidCrawling(const DavidState& s){return s.saved.prone!=0||david_logic::legGone(s.saved.severMask);}
static void davidEnterProne(DavidState& s){if(s.saved.prone||!valid(s.entity))return;s.saved.prone=1;s.running=false;s.clearSince=-1;Vector lo(-16,-16,0),hi(16,16,24);engine.pfnSetSize(s.entity,&lo.x,&hi.x);s.entity->v.view_ofs=Vector(0,0,18);s.entity->v.sequence=david_assets::seqCrawlIdle;s.entity->v.frame=0;s.entity->v.animtime=globals->time;s.entity->v.framerate=1;s.poseAt=-100;}
static void davidUpdateProne(DavidState& s){auto e=s.entity;if(s.saved.prone||!aliveEnemy(e))return;float maximum=s.saved.initialHealth>0?s.saved.initialHealth:e->v.max_health;if(maximum>0&&e->v.health<maximum*.5f)davidEnterProne(s);}
static void davidPrecache(){
 if(!davidCvarsRegistered&&engine.pfnCVarRegister){for(auto v:{&davidEnable,&davidRadius,&davidClearRadius,&davidHeadThreshold,&davidLimbThreshold,&davidLocalizedDamage})engine.pfnCVarRegister(v);davidCvarsRegistered=true;}
 for(auto m:{DavidModel,DavidCharred})engine.pfnPrecacheModel(const_cast<char*>(m));
 for(auto sound:{"debris/flesh1.wav","debris/flesh2.wav","debris/flesh3.wav"})engine.pfnPrecacheSound(const_cast<char*>(sound));
 for(int node=2;node<16;++node)if(node!=9){char path[128];std::snprintf(path,sizeof path,"models/cremation/hotd2/david/part_%02d.mdl",node);engine.pfnPrecacheModel(const_cast<char*>(text(engine.pfnAllocString(path))));}
}
static void davidPose(DavidState& s){
 auto e=s.entity;if(!engine.pfnGetBonePosition)return;
 if(s.poseAt==globals->time&&s.poseFrame==e->v.frame&&s.poseSequence==e->v.sequence&&s.poseBody==e->v.body&&length(s.poseOrigin-e->v.origin)<.0001f&&length(s.poseAngles-e->v.angles)<.0001f)return;
 for(int node=1;node<16;++node){auto& p=s.pose[node];Vector angles,forward,right,up;engine.pfnGetBonePosition(e,david_assets::bone[node],&p.origin.x,&angles.x);engine.pfnAngleVectors(&angles.x,&forward.x,&right.x,&up.x);p.axis[0]=forward;p.axis[1]=right*-1;p.axis[2]=up;}
 s.poseAt=globals->time;s.poseFrame=e->v.frame;s.poseSequence=e->v.sequence;s.poseBody=e->v.body;s.poseOrigin=e->v.origin;s.poseAngles=e->v.angles;
}
static int davidNodeAt(DavidState& s,const Vector& point){
 davidPose(s);float best=1e9f;int found=0;
 for(int n=1;n<16;++n)if(davidVisiblePart(s.entity,n)){
  auto& p=s.pose[n];Vector diff=point-p.origin;float distance=0;const float* box=david_assets::partBounds[n][davidBody(s.entity,n)];
  for(int k=0;k<3;++k){float x=dot(diff,p.axis[k]);float gap=std::max(box[k]-x,std::max(0.f,x-box[k+3]));distance+=gap*gap;}
  if(distance<best){best=distance;found=n;}
 }return best<=2.25f?found:0;
}
static decltype(engine.pfnTraceLine) davidOriginalTrace=nullptr;
static bool davidTracing=false;
static void davidTraceLine(const float* from,const float* to,int flags,edict_t* skip,TraceResult* tr){
 if(!davidOriginalTrace)return;
 if(davidTracing||(flags&1)||davidStates.empty()||(davidLocalizedDamage.value!=0&&!engine.pfnGetBonePosition)){davidOriginalTrace(from,to,flags,skip,tr);return;}
 davidTracing=true;
 // Engine studio hitboxes are independent of bodygroups. Remove only Davids
 // from this synchronous broad phase, then test either the single general hull
 // or the optional localized bone boxes.
 std::vector<std::pair<edict_t*,int>> solids;
 for(auto& s:davidStates)if(valid(s.entity)&&s.entity->serialnumber==s.serial&&s.entity!=skip&&s.entity->v.solid!=SOLID_NOT&&davidModel(s.entity)){
  solids.push_back({s.entity,s.entity->v.solid});s.entity->v.solid=SOLID_NOT;
 }
 davidOriginalTrace(from,to,flags,skip,tr);
 for(auto p:solids)p.first->v.solid=p.second;
 Vector a(from[0],from[1],from[2]),b(to[0],to[1],to[2]);float best=tr->flFraction;int nodeHit=0;DavidState* stateHit=nullptr;Vector hitNormal;
 if(!tr->fAllSolid&&!tr->fStartSolid)for(auto p:solids){
  auto s=davidState(p.first);if(!s)continue;
  if(davidLocalizedDamage.value==0){
   Vector lo=p.first->v.origin+p.first->v.mins,hi=p.first->v.origin+p.first->v.maxs;
   float box[6]={lo.x,lo.y,lo.z,hi.x,hi.y,hi.z},fraction;int axis;float sign;
   if(david_logic::segmentBox(from,to,box,fraction,axis,sign)&&fraction<best){best=fraction;stateHit=s;nodeHit=0;hitNormal=Vector(axis==0?sign:0,axis==1?sign:0,axis==2?sign:0);}
   continue;
  }
  Vector origin=p.first->v.origin;float coarse[6]={origin.x-160,origin.y-160,origin.z-80,origin.x+160,origin.y+160,origin.z+160};float cf,cs;int ca;
  if(!david_logic::segmentBox(from,to,coarse,cf,ca,cs)||cf>best)continue;
  davidPose(*s);
  for(int n=1;n<16;++n)if(davidVisiblePart(p.first,n)){
   auto& pose=s->pose[n];Vector ar=a-pose.origin,br=b-pose.origin;float av[3],bv[3];for(int k=0;k<3;++k){av[k]=dot(ar,pose.axis[k]);bv[k]=dot(br,pose.axis[k]);}
   float fraction;int axis;float sign;const float* box=david_assets::partBounds[n][davidBody(p.first,n)];
   if(david_logic::segmentBox(av,bv,box,fraction,axis,sign)&&fraction<best){best=fraction;stateHit=s;nodeHit=n;hitNormal=pose.axis[axis]*sign;}
  }
 }
 if(stateHit){auto e=stateHit->entity;tr->flFraction=best;tr->vecEndPos=a+(b-a)*best;tr->vecPlaneNormal=hitNormal;tr->flPlaneDist=dot(tr->vecEndPos,hitNormal);tr->pHit=e;tr->iHitgroup=davidLocalizedDamage.value!=0?david_assets::hitgroup[nodeHit]:0;tr->fInOpen=TRUE;
  if(davidLocalizedDamage.value!=0){stateHit->lastNode=nodeHit;stateHit->lastPoint=tr->vecEndPos;stateHit->lastTrace=globals->time;
   edict_t* owner=valid(skip)&&skip->v.flags&FL_CLIENT?skip:(valid(skip)&&valid(skip->v.owner)&&skip->v.owner->v.flags&FL_CLIENT?skip->v.owner:nullptr);
   if(owner){stateHit->directAt=globals->time;stateHit->directAttacker=&owner->v;stateHit->directNode=nodeHit;stateHit->directPoint=tr->vecEndPos;stateHit->directDirection=normal(b-a);}
  }
 }
 davidTracing=false;
}
static bool davidHostile(edict_t* enemy,edict_t* player){
 if(!aliveEnemy(enemy)||!enemy->pvPrivateData||!player->pvPrivateData)return false;
 // Player sentries are scripted info_target entities with FL_MONSTER; they
 // have no CBaseMonster tail in their vtable and are allied support.
 if(bounce::isSentry(enemy)||necroMinion(enemy))return false;
 auto table=*reinterpret_cast<void***>(enemy->pvPrivateData);
 // Verified donor MyMonsterPointer returns this at base slot 17. Reject
 // decorative/base entities before accessing the monster-only relation slot.
 if(reinterpret_cast<void*(__thiscall*)(void*)>(table[17])(enemy->pvPrivateData)!=enemy->pvPrivateData)return false;
 // IRelationship at verified native CBaseMonster slot 63; all registered NPC
 // variants retain this donor slot even when their damage/Think are extended.
 return reinterpret_cast<int(__thiscall*)(void*,void*)>(table[63])(enemy->pvPrivateData,player->pvPrivateData)>0;
}
static bool davidCrowded(DavidState& s,float radius){
 auto target=s.target;if(!valid(target))return true;
 for(int i=1;i<globals->maxEntities;++i){auto other=engine.pfnPEntityOfEntIndex(i);if(other==s.entity||other==target||!aliveEnemy(other))continue;
  Vector delta=other->v.origin-target->v.origin;if(length2d(delta)>radius||std::fabs(delta.z)>80)continue;
  if(!davidHostile(other,target))continue;
  TraceResult tr{};Vector from=target->v.origin+target->v.view_ofs,to=center(other);
  // Ignore monsters: a separating wall blocks occupancy, another enemy does not.
  engine.pfnTraceLine(&from.x,&to.x,1,target,&tr);if(!tr.fStartSolid&&tr.flFraction>=.999f)return true;
 }return false;
}
static bool davidBusy(edict_t* e){int seq=e->v.sequence;return (seq>=3&&seq<=19)||seq==david_assets::seqProneAttackRight||seq==david_assets::seqProneAttackLeft||seq==david_assets::seqProneAttackBoth;}
static bool davidCanRun(DavidState& s){
 auto e=s.entity,t=s.target;bool player=valid(t)&&t->serialnumber==s.targetSerial&&(t->v.flags&FL_CLIENT)&&t->v.health>0&&t->v.deadflag==DEAD_NO;
 if(!aliveEnemy(e)||!player)return false;
 if(!david_logic::runnerEligible(aliveEnemy(e),player,true,globals->time-s.movedAt<.45f,!davidCrawling(s)&&!(s.saved.severMask&(1<<2)),davidBusy(e),length2d(t->v.origin-e->v.origin)))return false;
 Vector from=e->v.origin+e->v.view_ofs,to=t->v.origin+t->v.view_ofs;TraceResult tr{};engine.pfnTraceLine(&from.x,&to.x,1,e,&tr);return !tr.fStartSolid&&tr.flFraction>=.999f;
}
static void davidDecision(){
 if(globals->time<davidNextDecision)return;davidNextDecision=globals->time+.2f;
 std::array<bool,33> reserved{};
 // Keep a valid existing reservation before assigning newcomers by edict order.
 for(auto& s:davidStates)if(s.running){auto e=s.entity;bool keep=valid(e)&&e->serialnumber==s.serial&&davidEnable.value!=0&&davidCanRun(s)&&!davidCrowded(s,std::max(64.f,davidRadius.value));int target=keep?engine.pfnIndexOfEdict(s.target):0;
  if(target<1||target>=33||reserved[target])keep=false;
  if(keep)reserved[target]=true;else{s.running=false;s.clearSince=-1;s.cooldown=globals->time+1;}
 }
 for(auto& s:davidStates){if(s.running||!valid(s.entity)||s.entity->serialnumber!=s.serial)continue;
  if(davidEnable.value==0||!davidCanRun(s)||davidCrowded(s,std::max(davidRadius.value+16,davidClearRadius.value))){s.clearSince=-1;continue;}
  if(s.clearSince<0)s.clearSince=globals->time;
  int target=engine.pfnIndexOfEdict(s.target);if(target>0&&target<33&&!reserved[target]&&globals->time>=s.cooldown&&globals->time-s.clearSince>=.4f){s.running=true;reserved[target]=true;}
 }
}
static void __fastcall davidActivity(void* object,void*,int requested){
 auto t=davidTable(object);auto e=davidObject(object);auto s=davidState(e);if(!t)return;int attackSequence=-1;bool proneDeath=false;
 if(s){
  bool alive=aliveEnemy(s->entity),crawl=davidCrawling(*s);
  bool deathActivity=requested==36||requested==37||requested==38||requested==39||requested==66;
  if(s->saved.prone&&deathActivity)proneDeath=true;
  if(alive){
  if(requested==3||requested==4||requested==48||requested==49){
   int desired=crawl?david_assets::seqCrawl:(s->running?david_assets::seqRun:david_assets::seqWalk);
   // MoveExecute is called for every route step. Reapplying SetActivity there
   // resets the animation timer and made walk/run stutter. Preserve attacks,
   // pain and death clips until they finish; only switch when locomotion changes.
   if(e->v.sequence==desired)return;
   if(davidBusy(e)&&e->v.frame<254)return;
   requested=crawl?48:(s->running?4:3);
  }
  else if(s->saved.prone&&requested!=30&&requested!=31)requested=18;
  else if(crawl&&(requested==1||requested==47||requested==61||requested==26||requested==27||requested==28||requested==29||requested==70||requested==71||requested==72||requested==73||requested==74||requested==75||requested==76))requested=18;
  // The donor always requests ACT_MELEE_ATTACK1 (30). Select every authored
  // attack explicitly so the left/right single-arm clips are reliably used;
  // the third entry preserves the original two-arm strike as well.
  if(requested==30){
   if(s->saved.prone){static constexpr int attacks[]={david_assets::seqProneAttackRight,david_assets::seqProneAttackLeft,david_assets::seqProneAttackBoth};attackSequence=attacks[s->nextAttack%3];}
   else{static constexpr int attacks[]={david_assets::seqAttackRight,david_assets::seqAttackLeft,david_assets::seqAttackBoth};attackSequence=attacks[s->nextAttack%3];}
   s->nextAttack=(s->nextAttack+1)%3;
  }
  else if(requested==31)attackSequence=s->saved.prone?david_assets::seqProneAttackBoth:david_assets::seqAttackBoth;
  }
 }
 bool slow=valid(e)&&slowed(e);float rate=slow?e->v.framerate:0;
 reinterpret_cast<void(__thiscall*)(void*,int)>(t->activity)(object,requested);
 if(slow&&valid(e))e->v.framerate=rate;
 if(attackSequence>=0&&s&&valid(e)){
  e->v.sequence=attackSequence;e->v.frame=0;e->v.animtime=globals->time;s->attackEventConsumed=false;
 }
 if(proneDeath&&valid(e)&&!(e->v.effects&EF_NODRAW)){e->v.sequence=david_assets::seqProneDie;e->v.frame=0;e->v.animtime=globals->time;e->v.framerate=1;}
}
static void __fastcall davidMove(void* object,void*,void* target,const Vector& dir,float interval){
 auto t=davidTable(object);auto s=davidState(davidObject(object));if(!t)return;
 if(s){s->movedAt=globals->time;s->target=nullptr;
  for(int i=1;i<globals->maxEntities;++i){auto e=engine.pfnPEntityOfEntIndex(i);if(valid(e)&&e->pvPrivateData==target){s->target=e;s->targetSerial=e->serialnumber;break;}}
  if(!valid(s->target)||!(s->target->v.flags&FL_CLIENT))s->running=false;
  davidActivity(object,nullptr,s->running?4:3);
 }
 reinterpret_cast<void(__thiscall*)(void*,void*,const Vector&,float)>(t->move)(object,target,dir,interval);
}
static void __fastcall davidEvent(void* object,void*,MonsterEvent_t* event){
 auto t=davidTable(object);auto s=davidState(davidObject(object));if(!t)return;MonsterEvent_t changed=*event;
 if(s&&event->event>=1&&event->event<=3){if(s->attackEventConsumed)return;changed.event=david_logic::attackEvent(event->event,s->saved.severMask);if(!changed.event)return;s->attackEventConsumed=true;}
 reinterpret_cast<void(__thiscall*)(void*,MonsterEvent_t*)>(t->event)(object,&changed);
}
static void davidPiece(DavidState& s,int node,const Vector& impulse){
 int local=0;for(auto& g:anatomicalGibs)if(g.localized&&valid(g.entity)&&g.entity->serialnumber==g.serial)++local;
 if(local>=128)return;
 // Do not suppress all local wounds merely because the previous wave left
 // ordinary 25-second gibs. Reserve actual free edicts for gameplay instead.
 int freeSlots=0;for(int i=globals->maxClients+1;i<globals->maxEntities&&freeSlots<32;++i){auto slot=engine.pfnPEntityOfEntIndex(i);if(!slot||slot->free)++freeSlots;}
 if(freeSlots<32)return;auto e=s.entity;Vector at,angles;
 engine.pfnGetBonePosition(e,david_assets::bone[node],&at.x,&angles.x);
 auto gib=engine.pfnCreateNamedEntity(engine.pfnAllocString("info_target"));if(!valid(gib))return;raven.pfnSpawn(gib);
 char model[128];std::snprintf(model,sizeof model,"models/cremation/hotd2/david/part_%02d.mdl",node);engine.pfnSetModel(gib,model);
 gib->v.targetname=engine.pfnAllocString(DavidGibTag);gib->v.body=std::min(davidBody(e,node),david_assets::bodyCount[node]-2);gib->v.sequence=0;gib->v.frame=0;gib->v.framerate=0;gib->v.animtime=globals->time;gib->v.angles=angles;
 gib->v.movetype=MOVETYPE_NONE;gib->v.solid=SOLID_NOT;gib->v.takedamage=DAMAGE_NO;gib->v.nextthink=0;Vector zero(0,0,0);engine.pfnSetSize(gib,&zero.x,&zero.x);engine.pfnSetOrigin(gib,&at.x);
 Vector velocity=e->v.velocity+normal(impulse)*100+Vector(0,0,100);if(length(velocity)>260)velocity=normal(velocity)*260;
 AnatomicalGib g{gib,gib->serialnumber,globals->time+1,globals->time,globals->time,velocity};g.localized=true;anatomicalGibs.push_back(g);
}
static void davidSever(DavidState& s,int node,const Vector& impulse){
 if(node==1||node==9||(s.saved.severMask&(1<<node))||!davidVisiblePart(s.entity,node))return;
 auto e=s.entity;for(int n=2;n<16;++n)if(n!=9&&david_logic::descendant(n,node,david_assets::parent)&&davidVisiblePart(e,n))davidPiece(s,n,impulse);
 s.saved.severMask|=1<<node;
 for(int n=2;n<16;++n)if(n!=9&&david_logic::descendant(n,node,david_assets::parent)){
  bool hasChild=false;for(int child=1;child<16;++child)if(david_assets::parent[child]==n)hasChild=true;
  if(n==node&&hasChild)davidSetBody(e,n,2);else davidSetBody(e,n,david_assets::bodyCount[n]-1);
 }
 s.running=false;s.clearSince=-1;s.poseAt=-100;
 if(node==2&&e->pvPrivateData){
  bool notify=e->v.deadflag==DEAD_NO&&!s.killedDuringDamage;e->v.health=0;
  if(notify){auto table=*reinterpret_cast<void***>(e->pvPrivateData);reinterpret_cast<void(__thiscall*)(void*,entvars_t*,int)>(table[13])(e->pvPrivateData,s.pendingAttacker,1);}
  // The donor may defer its deadflag/health bookkeeping until MonsterThink.
  // Decapitation is immediately lethal, never an alive headless frame.
  if(valid(e)){e->v.health=0;if(e->v.deadflag==DEAD_NO)e->v.deadflag=DEAD_DYING;e->v.nextthink=globals->time+.1f;}
 }
}
static float davidTraceWeight(float amount,int group){
 // Skill values used by native TraceAttack; read only after the ABI guard.
 static const int offsets[8]={0,0x11dcd8,0x11dcdc,0x11dce0,0x11dce8,0x11dce8,0x11dce4,0x11dce4};
 if(group>0&&group<8&&ravenModule){float mul=*reinterpret_cast<float*>(reinterpret_cast<unsigned char*>(ravenModule)+offsets[group]);if(std::isfinite(mul)&&mul>=0&&mul<=100)amount*=mul;}
 return amount;
}
static void __fastcall davidTraceAttack(void* object,void*,entvars_t* attacker,float amount,Vector direction,TraceResult* tr,int bits){
 auto t=davidTable(object);auto s=davidState(davidObject(object));if(!t)return;
 reinterpret_cast<void(__thiscall*)(void*,entvars_t*,float,Vector,TraceResult*,int)>(t->trace)(object,attacker,amount,direction,tr,bits);
 if(davidLocalizedDamage.value==0||!s||!tr||amount<=0||bits&((1<<6)|(1<<3)|(1<<21)))return;
 int n=s->lastTrace==globals->time&&length(s->lastPoint-tr->vecEndPos)<.1f?s->lastNode:davidNodeAt(*s,tr->vecEndPos);if(!n)return;
 if(s->pendingAt!=globals->time||s->pendingAttacker!=attacker){std::fill_n(s->pending,16,0.f);s->pendingAt=globals->time;s->pendingAttacker=attacker;}
 s->pending[n]+=davidTraceWeight(amount,tr->iHitgroup);s->directAt=-100;s->directDirection=direction;
}
static int __fastcall davidDamage(void* object,void*,entvars_t* inflictor,entvars_t* attacker,float amount,int bits){
 auto t=davidTable(object);auto e=davidObject(object);auto s=davidState(e);if(!t)return 0;if(!s||s->inDamage)return t->damage(object,inflictor,attacker,amount,bits);
 if(davidLocalizedDamage.value==0){bool wasAlive=aliveEnemy(e);std::fill_n(s->pending,16,0.f);s->pendingAt=-100;s->directAt=-100;s->inDamage=true;s->killedDuringDamage=false;int result=t->damage(object,inflictor,attacker,amount,bits);s->inDamage=false;if(wasAlive&&aliveEnemy(e))davidUpdateProne(*s);return result;}
 bool wasAlive=aliveEnemy(e);float before=e->v.health;float regional[16]{};bool area=(bits&((1<<6)|(1<<3)|(1<<21)))!=0;
 if(!area&&s->pendingAt==globals->time&&s->pendingAttacker==attacker)std::copy_n(s->pending,16,regional);
 else if(!area&&s->directAt==globals->time&&s->directAttacker==attacker&&s->directNode>0)regional[s->directNode]=1;
 std::fill_n(s->pending,16,0.f);s->pendingAt=-100;s->directAt=-100;s->inDamage=true;s->killedDuringDamage=false;
 int result=t->damage(object,inflictor,attacker,amount,bits);s->inDamage=false;
 if(wasAlive&&aliveEnemy(e))davidUpdateProne(*s);
 if(!wasAlive||!valid(e)||s->serial!=e->serialnumber||(e->v.effects&EF_NODRAW))return result;
 // BecomeDead can replace health with corpse durability before TakeDamage
 // returns. Use the lethal health captured at Killed, not that recycled value.
 float loss=std::max(0.f,before-(s->killedDuringDamage?s->healthAtKilled:e->v.health)),total=0;for(float value:regional)total+=value;
 if(loss<=0||total<=0)return result;s->pendingAttacker=attacker;
 for(int n=1;n<16;++n)if(regional[n]>0){s->saved.regional[n]+=loss*regional[n]/total;float fraction=n==2?davidHeadThreshold.value:(n==1?.5f:davidLimbThreshold.value);float threshold=std::max(1.f,s->saved.initialHealth*std::max(.05f,fraction));
  if(n==1){int stage=std::min(3,int(s->saved.regional[n]/std::max(1.f,threshold*.5f)));if(stage>davidBody(e,n))davidSetBody(e,n,stage);}
  else if(n!=9&&!(s->saved.severMask&(1<<n))){
   if(s->saved.regional[n]>=threshold)davidSever(*s,n,s->directDirection);
   else if(n==2){int stage=s->saved.regional[n]>=threshold*.75f?2:(s->saved.regional[n]>=threshold*.45f?1:0);if(stage>davidBody(e,n))davidSetBody(e,n,stage);}
   else if(s->saved.regional[n]>=threshold*.45f&&davidBody(e,n)==0&&david_assets::bodyCount[n]>2)davidSetBody(e,n,1);
  }
 }
 return result;
}
static void __fastcall davidKilled(void* object,void*,entvars_t* attacker,int gib){
 auto t=davidTable(object);auto s=davidState(davidObject(object));if(!t)return;if(s){s->running=false;s->clearSince=-1;if(s->inDamage){s->killedDuringDamage=true;s->healthAtKilled=s->entity->v.health;}}
 auto e=davidObject(object);int body=e?e->v.body:0;
 // A corpse has no remaining health, so every non-gib David death uses the
 // prone death clip, including a lethal hit that skipped the live threshold.
 if(s&&valid(e)&&!s->saved.prone)davidEnterProne(*s);
 reinterpret_cast<void(__thiscall*)(void*,entvars_t*,int)>(t->killed)(object,attacker,gib);
 if(valid(e)&&davidModel(e)&&!(e->v.effects&EF_NODRAW)){e->v.body=body;if(s&&s->saved.prone){e->v.sequence=david_assets::seqProneDie;e->v.frame=0;e->v.animtime=globals->time;e->v.framerate=1;}}
}
static bool davidInstall(edict_t* e){
 if(!davidClass(e)||!e->pvPrivateData)return false;auto source=*reinterpret_cast<void***>(e->pvPrivateData);
 for(auto& t:davidTables){if(source==t->slots.data())return true;if(source==t->source){*reinterpret_cast<void***>(e->pvPrivateData)=t->slots.data();return true;}}
 auto base=reinterpret_cast<unsigned char*>(ravenModule);
 // Fail closed on a different donor build rather than treating HLSDK layouts as proof.
 if(!base||source[10]!=base+0x1c390||source[57]!=base+0xa7290||source[73]!=base+0x55fa0||source[93]!=base+0x54a90||source[63]!=base+0x565f0){startupTrace("David ERROR","unsupported Ravenholm zombie ABI; kept native zombie");return false;}
 auto t=std::make_unique<DavidTable>();t->source=source;std::copy_n(source,125,t->slots.begin());
 // Existing shared wrappers resolve by table identity. Unwrap then reinstall
 // around our clone so each wrapper finds its own original without recursion.
 for(auto& b:bloodDamageTables)if(b.table==source){t->slots[11]=reinterpret_cast<void*>(b.original);t->slots[13]=b.killed;break;}
 for(auto& h:humanGibTables)if(h.table==source){t->slots[11]=h.damage;t->slots[13]=h.killed;break;}
 t->trace=t->slots[10];t->damage=reinterpret_cast<TakeDamageFn>(t->slots[11]);t->killed=t->slots[13];t->event=t->slots[57];t->move=t->slots[73];t->activity=t->slots[93];
 t->slots[10]=reinterpret_cast<void*>(&davidTraceAttack);t->slots[11]=reinterpret_cast<void*>(&davidDamage);t->slots[13]=reinterpret_cast<void*>(&davidKilled);t->slots[57]=reinterpret_cast<void*>(&davidEvent);t->slots[73]=reinterpret_cast<void*>(&davidMove);t->slots[93]=reinterpret_cast<void*>(&davidActivity);
 *reinterpret_cast<void***>(e->pvPrivateData)=t->slots.data();davidTables.push_back(std::move(t));humanGibInstall(e);weaponBloodInstall(e);return true;
}
static void davidSpawn(edict_t* e,bool restoring=false){
 if(!davidClass(e)||(!restoring&&davidEnable.value==0)||!davidInstall(e))return;
 auto s=davidState(e,true);
 if(!restoring){s->saved.initialHealth=std::max(1.f,e->v.health);s->saved.prone=0;e->v.body=0;engine.pfnSetModel(e,DavidModel);e->v.sequence=0;e->v.frame=0;e->v.animtime=globals->time;}
 if(davidLocalizedDamage.value==0){s->saved.severMask=0;std::fill_n(s->saved.regional,16,0.f);e->v.body=0;}
 Vector lo(-16,-16,0),hi(16,16,s->saved.prone?24:72);engine.pfnSetSize(e,&lo.x,&hi.x);e->v.view_ofs=Vector(0,0,s->saved.prone?18:64);
 if(restoring&&s->saved.prone&&aliveEnemy(e)&&e->v.sequence!=david_assets::seqCrawl&&e->v.sequence!=david_assets::seqCrawlIdle&&e->v.sequence!=david_assets::seqProneAttackRight&&e->v.sequence!=david_assets::seqProneAttackLeft&&e->v.sequence!=david_assets::seqProneAttackBoth){e->v.sequence=david_assets::seqCrawlIdle;e->v.frame=0;e->v.animtime=globals->time;}
 startupTrace("David",restoring?"restored original wounds/rig":"spawn original CZombie damage + David model/rig");
}
static void davidFrame(){
 for(auto& s:davidStates)if(valid(s.entity)&&s.entity->serialnumber==s.serial&&davidModel(s.entity)&&aliveEnemy(s.entity))davidUpdateProne(s);
 davidDecision();for(auto& s:davidStates)if(valid(s.entity)&&s.entity->serialnumber==s.serial&&davidModel(s.entity)&&aliveEnemy(s.entity)){
  int seq=s.entity->v.sequence;bool locomotion=seq==0||seq==1||seq==2||seq==20||seq==21;
  if(locomotion&&globals->time-s.movedAt<.45f){int desired=davidCrawling(s)?david_assets::seqCrawl:(s.running?david_assets::seqRun:david_assets::seqWalk);if(seq!=desired)davidActivity(s.entity->pvPrivateData,nullptr,s.running?4:3);}
 }
}
static void davidSave(edict_t* e,SAVERESTOREDATA* data){auto s=davidState(e);if(s&&davidModel(e)&&raven.pfnSaveWriteFields)raven.pfnSaveWriteFields(data,"CremationDavid1",&s->saved,davidSaveFields,ARRAYSIZE(davidSaveFields));}
static void davidRestore(edict_t* e,SAVERESTOREDATA* data){
 if(!davidClass(e))return;bool saved=davidModel(e);
 if(!saved){davidSpawn(e);return;}
 auto s=davidState(e,true);if(raven.pfnSaveReadFields)raven.pfnSaveReadFields(data,"CremationDavid1",&s->saved,davidSaveFields,ARRAYSIZE(davidSaveFields));
 if(s->saved.version<2)s->saved.prone=0;s->saved.version=2;
 if(s->saved.initialHealth<=0)s->saved.initialHealth=std::max(1.f,e->v.max_health>0?e->v.max_health:e->v.health);
 bool alive=aliveEnemy(e),visibleCorpse=!alive&&!(e->v.effects&EF_NODRAW),crossedThreshold=alive&&e->v.health<s->saved.initialHealth*.5f;
 // Older saves stored a random spawn prone flag; living restores now derive
 // the pose exclusively from the current-health threshold.
 if(alive)s->saved.prone=0;else if(visibleCorpse)s->saved.prone=1;
 s->running=false;s->target=nullptr;s->movedAt=-100;s->clearSince=-1;s->cooldown=globals->time+1;davidSpawn(e,true);
 if(crossedThreshold)davidEnterProne(*s);
 else if(visibleCorpse){e->v.sequence=david_assets::seqProneDie;e->v.frame=0;e->v.animtime=globals->time;e->v.framerate=1;}
}
