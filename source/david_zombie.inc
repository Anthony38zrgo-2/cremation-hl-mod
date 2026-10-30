// David adapts the live Ravenholm CZombie. The donor owns navigation, schedules,
// hitgroup multipliers, health, melee damage, ordinary death and whole-body gibs.
#include "david_assets.h"
#include "steve_assets.h"
#include "johnny_assets.h"
#include "max_assets.h"
#include "ken_assets.h"
#include "david_logic.h"
#include "monsterevent.h"
#include <cstddef>
static constexpr const char* DavidModel="models/cremation/hotd2/david/david.mdl";
static constexpr const char* DavidCharred="models/cremation/hotd2/david/david_charred.mdl";
static constexpr const char* SteveModel="models/cremation/hotd2/steve/steve.mdl";
static constexpr const char* SteveCharred="models/cremation/hotd2/steve/steve_charred.mdl";
static constexpr const char* JohnnyModel="models/cremation/hotd2/johnny/johnny.mdl";
static constexpr const char* JohnnyCharred="models/cremation/hotd2/johnny/johnny_charred.mdl";
static constexpr const char* MaxModel="models/cremation/hotd2/max/max.mdl";
static constexpr const char* MaxCharred="models/cremation/hotd2/max/max_charred.mdl";
static constexpr const char* KenModel="models/cremation/hotd2/ken/ken.mdl";
static constexpr const char* KenCharred="models/cremation/hotd2/ken/ken_charred.mdl";
static constexpr const char* MaxSawSound="cremation/hotd2/max/chain_saw_22.wav";
static constexpr const char* JohnnyAxe="models/cremation/hotd2/johnny/axe.mdl";
static constexpr const char* JohnnyAxeLeft="models/cremation/hotd2/johnny/axe_left.mdl";
static constexpr bool steveLayoutCompatible(){for(int n=0;n<16;++n)if(steve_assets::bone[n]!=david_assets::bone[n]||steve_assets::parent[n]!=david_assets::parent[n]||steve_assets::bodyBase[n]!=david_assets::bodyBase[n]||steve_assets::bodyCount[n]!=david_assets::bodyCount[n])return false;return true;}
static_assert(steveLayoutCompatible(),"Steve must retain the shared David rig/bodygroup encoding");
static constexpr bool johnnyLayoutCompatible(){for(int n=0;n<16;++n)if(johnny_assets::bone[n]!=david_assets::bone[n]||johnny_assets::parent[n]!=david_assets::parent[n])return false;return true;}
static_assert(johnnyLayoutCompatible(),"Johnny must retain the shared David 16-bone hierarchy");
static constexpr bool maxLayoutCompatible(){for(int n=0;n<16;++n)if(max_assets::bone[n]!=david_assets::bone[n]||max_assets::parent[n]!=david_assets::parent[n])return false;return true;}
static_assert(maxLayoutCompatible(),"Max must retain the shared David 16-bone hierarchy");
static constexpr bool kenLayoutCompatible(){for(int n=0;n<16;++n)if(ken_assets::bone[n]!=david_assets::bone[n]||ken_assets::parent[n]!=david_assets::parent[n]||ken_assets::bodyBase[n]!=david_assets::bodyBase[n]||ken_assets::bodyCount[n]!=david_assets::bodyCount[n])return false;return true;}
static_assert(kenLayoutCompatible(),"Ken must retain David's rig and bodygroup layout");
static constexpr const char* DavidGibTag="cremation_david_gib";
static bool mansionOnBridgeRoute(edict_t* e);
static cvar_t davidEnable={const_cast<char*>("cremation_david"),const_cast<char*>("1"),FCVAR_SERVER,1,nullptr};
static cvar_t davidRadius={const_cast<char*>("david_crowd_radius"),const_cast<char*>("256"),FCVAR_SERVER,256,nullptr};
static cvar_t davidClearRadius={const_cast<char*>("david_clear_radius"),const_cast<char*>("320"),FCVAR_SERVER,320,nullptr};
static cvar_t davidHeadThreshold={const_cast<char*>("david_head_fraction"),const_cast<char*>("0.35"),FCVAR_SERVER,.35f,nullptr};
static cvar_t davidLimbThreshold={const_cast<char*>("david_limb_fraction"),const_cast<char*>("0.25"),FCVAR_SERVER,.25f,nullptr};
static cvar_t davidLocalizedDamage={const_cast<char*>("cremation_david_local_damage"),const_cast<char*>("0"),FCVAR_SERVER,0,nullptr};
static cvar_t davidRunDwell={const_cast<char*>("david_run_dwell"),const_cast<char*>("1.2"),FCVAR_SERVER,1.2f,nullptr};
static cvar_t davidStuckFlee={const_cast<char*>("david_stuck_release"),const_cast<char*>("6"),FCVAR_SERVER,6.0f,nullptr};
static cvar_t steveChance={const_cast<char*>("cremation_steve_chance"),const_cast<char*>("0.5"),FCVAR_SERVER,.5f,nullptr};
static cvar_t kenChance={const_cast<char*>("cremation_ken_chance"),const_cast<char*>("0.3333333"),FCVAR_SERVER,1.f/3.f,nullptr};
static cvar_t johnnyChance={const_cast<char*>("cremation_johnny_chance"),const_cast<char*>("0.1"),FCVAR_SERVER,.1f,nullptr};
static bool davidCvarsRegistered=false;
struct DavidSaved {int version=6;int severMask=0;float regional[16]{};float initialHealth=0;int prone=0;int variant=0;int johnnyAxes=3;int johnnyDefended=0;float johnnyGuardRemaining=0;};
static TYPEDESCRIPTION davidSaveFields[]={
 {FIELD_INTEGER,const_cast<char*>("version"),offsetof(DavidSaved,version),1,0},
 {FIELD_INTEGER,const_cast<char*>("severMask"),offsetof(DavidSaved,severMask),1,0},
 {FIELD_FLOAT,const_cast<char*>("regional"),offsetof(DavidSaved,regional),16,0},
 {FIELD_FLOAT,const_cast<char*>("initialHealth"),offsetof(DavidSaved,initialHealth),1,0},
 {FIELD_INTEGER,const_cast<char*>("prone"),offsetof(DavidSaved,prone),1,0},
 {FIELD_INTEGER,const_cast<char*>("variant"),offsetof(DavidSaved,variant),1,0},
 {FIELD_INTEGER,const_cast<char*>("johnnyAxes"),offsetof(DavidSaved,johnnyAxes),1,0},
 {FIELD_INTEGER,const_cast<char*>("johnnyDefended"),offsetof(DavidSaved,johnnyDefended),1,0},
 {FIELD_FLOAT,const_cast<char*>("johnnyGuardRemaining"),offsetof(DavidSaved,johnnyGuardRemaining),1,0}
};
struct DavidBonePose {Vector origin{0,0,0},axis[3];};
struct DavidState {
 edict_t* entity=nullptr;int serial=0;DavidSaved saved;
 edict_t* target=nullptr;int targetSerial=0;float movedAt=-100,clearSince=-1,cooldown=0;bool running=false;
 // Histeresis de la decision caminar/correr (anti-baile): el estado deseado
 // debe mantenerse sin cambiar durante david_run_dwell antes de aplicar.
 bool runWant=false;float runWantAt=-1000;float stuckSince=-1000;
 bool attackEventConsumed=false,dying=false;int attackSelections[3]{},meleeEvents=0,attackRequests[2]{};
 int deathActivity=0;bool deathStarted=false;
 float pendingAt=-100;entvars_t* pendingAttacker=nullptr;float pending[16]{};
 float directAt=-100;entvars_t* directAttacker=nullptr;int directNode=0;Vector directPoint{0,0,0},directDirection{0,0,1};
 int lastNode=0;Vector lastPoint{0,0,0};float lastTrace=-100;
 float poseAt=-100,poseFrame=0;int poseSequence=-1,poseBody=-1;Vector poseOrigin{0,0,0},poseAngles{0,0,0};
 float johnnyGuardUntil=0,johnnyThrowUntil=0,johnnyNextThrow=0;int johnnyThrowSide=0;bool johnnyGuarding=false,johnnyThrowing=false,johnnyThrowReleased=false;
 edict_t* johnnyThrowTarget=nullptr;int johnnyThrowTargetSerial=0;Vector johnnyThrowAim{0,0,0};
 DavidBonePose pose[16];bool inDamage=false,killedDuringDamage=false;float healthAtKilled=0;
};
static std::vector<DavidState> davidStates;
struct DavidTable {void** source;std::array<void*,125> slots;void* trace;TakeDamageFn damage;void* killed;void* event;void* move;void* activity;void* deathActivity;};
static std::vector<std::unique_ptr<DavidTable>> davidTables;
static float davidNextDecision=0;
static bool davidClass(edict_t* e){return valid(e)&&e->v.classname&&(!std::strcmp(text(e->v.classname),"monster_zombie")||!std::strcmp(text(e->v.classname),"monster_johnny")||!std::strcmp(text(e->v.classname),"monster_max")||!std::strcmp(text(e->v.classname),"monster_ken"));}
static bool steveModel(edict_t* e){return valid(e)&&e->v.model&&(!std::strcmp(text(e->v.model),SteveModel)||!std::strcmp(text(e->v.model),SteveCharred));}
static bool johnnyModel(edict_t* e){return valid(e)&&e->v.model&&(!std::strcmp(text(e->v.model),JohnnyModel)||!std::strcmp(text(e->v.model),JohnnyCharred));}
static bool maxModel(edict_t* e){return valid(e)&&e->v.model&&(!std::strcmp(text(e->v.model),MaxModel)||!std::strcmp(text(e->v.model),MaxCharred));}
static bool kenModel(edict_t* e){return valid(e)&&e->v.model&&(!std::strcmp(text(e->v.model),KenModel)||!std::strcmp(text(e->v.model),KenCharred));}
static edict_t* kenMeleeActor=nullptr;
static edict_t* steveMeleeActor=nullptr;
static edict_t* maxMeleeActor=nullptr;
static float davidMeleeScale(entvars_t* attacker){if(kenMeleeActor&&valid(kenMeleeActor)&&attacker==&kenMeleeActor->v&&kenModel(kenMeleeActor))return 2.f;if(maxMeleeActor&&valid(maxMeleeActor)&&attacker==&maxMeleeActor->v&&maxModel(maxMeleeActor))return 1.25f;return steveMeleeActor&&valid(steveMeleeActor)&&attacker==&steveMeleeActor->v&&steveModel(steveMeleeActor)?1.1f:1.f;}
static bool davidModel(edict_t* e){return valid(e)&&e->v.model&&(!std::strcmp(text(e->v.model),DavidModel)||!std::strcmp(text(e->v.model),DavidCharred)||steveModel(e)||johnnyModel(e)||maxModel(e)||kenModel(e));}
static DavidState* davidState(edict_t* e,bool create=false){
 if(!valid(e)||!engine.pfnIndexOfEdict)return nullptr;int i=engine.pfnIndexOfEdict(e);if(i<1||i>=globals->maxEntities)return nullptr;
 if(i>=int(davidStates.size())){if(!create)return nullptr;davidStates.resize(globals->maxEntities);}
 auto& s=davidStates[i];if(s.entity!=e||s.serial!=e->serialnumber){if(!create)return nullptr;s=DavidState{};s.entity=e;s.serial=e->serialnumber;}
 return &s;
}
static edict_t* davidObject(void* object){auto v=reinterpret_cast<entvars_t**>(object)[1];return v?v->pContainingEntity:nullptr;}
static DavidTable* davidTable(void* object){auto table=*reinterpret_cast<void***>(object);for(auto& t:davidTables)if(t->slots.data()==table)return t.get();return nullptr;}
static int davidBodyCount(edict_t* e,int node){return maxModel(e)?max_assets::bodyCount[node]:(johnnyModel(e)?johnny_assets::bodyCount[node]:(steveModel(e)?steve_assets::bodyCount[node]:david_assets::bodyCount[node]));}
static int davidBody(edict_t* e,int node){int base=maxModel(e)?max_assets::bodyBase[node]:(johnnyModel(e)?johnny_assets::bodyBase[node]:(steveModel(e)?steve_assets::bodyBase[node]:david_assets::bodyBase[node]));return base?(e->v.body/base)%davidBodyCount(e,node):0;}
static void davidSetBody(edict_t* e,int node,int state){int base=maxModel(e)?max_assets::bodyBase[node]:(johnnyModel(e)?johnny_assets::bodyBase[node]:(steveModel(e)?steve_assets::bodyBase[node]:david_assets::bodyBase[node]));if(base)e->v.body+=(state-davidBody(e,node))*base;}
static bool davidVisiblePart(edict_t* e,int node){return node==1||node==9||davidBody(e,node)<davidBodyCount(e,node)-1;}
static const float* davidPartBounds(edict_t* e,int node,int state){return kenModel(e)?ken_assets::partBounds[node][state]:maxModel(e)?max_assets::partBounds[node][state]:(johnnyModel(e)?johnny_assets::partBounds[node][state]:(steveModel(e)?steve_assets::partBounds[node][state]:david_assets::partBounds[node][state]));}
static bool davidCrawling(const DavidState& s){return david_logic::legGone(s.saved.severMask);}
static void davidPrecache(){
 if(!davidCvarsRegistered&&engine.pfnCVarRegister){for(auto v:{&davidEnable,&davidRadius,&davidClearRadius,&davidHeadThreshold,&davidLimbThreshold,&davidLocalizedDamage,&davidRunDwell,&davidStuckFlee,&steveChance,&kenChance,&johnnyChance})engine.pfnCVarRegister(v);davidCvarsRegistered=true;}
 for(auto m:{DavidModel,DavidCharred,SteveModel,SteveCharred,JohnnyModel,JohnnyCharred,JohnnyAxe,JohnnyAxeLeft,MaxModel,MaxCharred,KenModel,KenCharred})engine.pfnPrecacheModel(const_cast<char*>(m));
 engine.pfnPrecacheSound(const_cast<char*>(MaxSawSound));
 for(auto sound:{"debris/flesh1.wav","debris/flesh2.wav","debris/flesh3.wav"})engine.pfnPrecacheSound(const_cast<char*>(sound));
 for(auto variant:{"david","steve","johnny","max","ken"})for(int node=2;node<16;++node)if(node!=9){char path[128];std::snprintf(path,sizeof path,"models/cremation/hotd2/%s/part_%02d.mdl",variant,node);engine.pfnPrecacheModel(const_cast<char*>(text(engine.pfnAllocString(path))));}
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
  auto& p=s.pose[n];Vector diff=point-p.origin;float distance=0;const float* box=davidPartBounds(s.entity,n,davidBody(s.entity,n));
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
 // Engine studio hitboxes are independent of bodygroups. First use a cheap
 // segment/AABB test so only Davids along this trace leave the engine's broad
 // phase; exact hull or localized bone checks run only for those candidates.
 std::vector<std::pair<edict_t*,int>> solids;
 for(auto& s:davidStates)if(valid(s.entity)&&s.entity->serialnumber==s.serial&&s.entity!=skip&&s.entity->v.solid!=SOLID_NOT&&davidModel(s.entity)){
  float broad[6];
  if(davidLocalizedDamage.value==0){Vector lo=s.entity->v.origin+s.entity->v.mins,hi=s.entity->v.origin+s.entity->v.maxs;broad[0]=lo.x;broad[1]=lo.y;broad[2]=lo.z;broad[3]=hi.x;broad[4]=hi.y;broad[5]=hi.z;}
  else {Vector o=s.entity->v.origin;broad[0]=o.x-160;broad[1]=o.y-160;broad[2]=o.z-80;broad[3]=o.x+160;broad[4]=o.y+160;broad[5]=o.z+160;}
  float fraction,sign;int axis;
  if(!david_logic::segmentBox(from,to,broad,fraction,axis,sign))continue;
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
   float fraction;int axis;float sign;const float* box=davidPartBounds(p.first,n,davidBody(p.first,n));
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
 if(reinterpret_cast<void*(__thiscall*)(void*)>(table[ravenholm_abi::slot::MyMonsterPointer])(enemy->pvPrivateData)!=enemy->pvPrivateData)return false;
 // IRelationship at verified native CBaseMonster slot 63; all registered NPC
 // variants retain this donor slot even when their damage/Think are extended.
 return reinterpret_cast<int(__thiscall*)(void*,void*)>(table[ravenholm_abi::slot::Relationship])(enemy->pvPrivateData,player->pvPrivateData)>0;
}
static bool davidCrowded(DavidState& s,float radius){
 auto target=s.target;if(!valid(target))return true;
 for(auto other:getActiveEnemies()){if(other==s.entity||other==target||!aliveEnemy(other))continue;
  Vector delta=other->v.origin-target->v.origin;if(length2d(delta)>radius||std::fabs(delta.z)>80)continue;
  if(!davidHostile(other,target))continue;
  TraceResult tr{};Vector from=target->v.origin+target->v.view_ofs,to=center(other);
  // Ignore monsters: a separating wall blocks occupancy, another enemy does not.
  engine.pfnTraceLine(&from.x,&to.x,1,target,&tr);if(!tr.fStartSolid&&tr.flFraction>=.999f)return true;
 }return false;
}
static bool davidBusy(edict_t* e){
 int seq=e->v.sequence;
 if(johnnyModel(e))return seq>=3&&seq<=20;
 return (seq>=3&&seq<=19)||(seq>=22&&seq<=25);
}
static bool davidCanRun(DavidState& s){
 auto e=s.entity,t=s.target;bool player=valid(t)&&t->serialnumber==s.targetSerial&&(t->v.flags&FL_CLIENT)&&t->v.health>0&&t->v.deadflag==DEAD_NO;
 if(!aliveEnemy(e)||!player)return false;
 float dist=length2d(t->v.origin-e->v.origin);
 float minDist=s.running?80.f:128.f;
 if(!david_logic::runnerEligible(aliveEnemy(e),player,true,globals->time-s.movedAt<.45f,!davidCrawling(s)&&!(s.saved.severMask&(1<<2)),davidBusy(e),s.running?std::max(dist,128.f):dist))return false;
 if(dist<minDist)return false;
 Vector from=e->v.origin+e->v.view_ofs,to=t->v.origin+t->v.view_ofs;TraceResult tr{};engine.pfnTraceLine(&from.x,&to.x,1,e,&tr);return !tr.fStartSolid&&tr.flFraction>=.999f;
}
static void davidDecision(){
 for(auto& s:davidStates)if(valid(s.entity)&&s.entity->serialnumber==s.serial){
  if(mansionMap()&&mansionActor(s.entity)){
   // Decision con histeresis: en el borde de 70 u el umbral cruzaba de ida y
   // vuelta en cada think y davidActivity(4<->3) reiniciaba la secuencia a
   // frame 0 (el "baile"). El estado deseado debe mantenerse estable
   // david_run_dwell segundos antes de aplicarse.
   auto p=s.target;
   bool desired=aliveEnemy(s.entity)&&!s.dying&&!davidCrawling(s)&&valid(p)&&p->serialnumber==s.targetSerial&&(p->v.flags&FL_CLIENT)&&p->v.health>0&&p->v.deadflag==DEAD_NO&&length2d(p->v.origin-s.entity->v.origin)>=70.f;
   if(desired!=s.runWant){s.runWant=desired;s.runWantAt=globals->time;}
   if(desired!=s.running&&globals->time-s.runWantAt>=davidRunDwell.value)s.running=desired;
  }
  else if(maxModel(s.entity))s.running=aliveEnemy(s.entity)&&!s.dying;
 }
 if(globals->time<davidNextDecision)return;davidNextDecision=globals->time+.2f;
 std::array<bool,33> reserved{};
 // Keep a valid existing reservation before assigning newcomers by edict order.
 for(auto& s:davidStates)if(maxModel(s.entity)||(valid(s.entity)&&mansionMap()&&mansionActor(s.entity)))continue;else if(s.running){auto e=s.entity;bool keep=valid(e)&&e->serialnumber==s.serial&&davidEnable.value!=0&&davidCanRun(s)&&!davidCrowded(s,std::max(64.f,davidRadius.value));int target=keep?engine.pfnIndexOfEdict(s.target):0;
  if(target<1||target>=33||reserved[target])keep=false;
  if(keep)reserved[target]=true;else{s.running=false;s.clearSince=-1;s.cooldown=globals->time+1;}
 }
 for(auto& s:davidStates){if(maxModel(s.entity)||(valid(s.entity)&&mansionMap()&&mansionActor(s.entity))||s.running||!valid(s.entity)||s.entity->serialnumber!=s.serial)continue;
  if(davidEnable.value==0||!davidCanRun(s)||davidCrowded(s,std::max(davidRadius.value+16,davidClearRadius.value))){s.clearSince=-1;continue;}
  if(s.clearSince<0)s.clearSince=globals->time;
  int target=engine.pfnIndexOfEdict(s.target);if(target>0&&target<33&&!reserved[target]&&globals->time>=s.cooldown&&globals->time-s.clearSince>=.4f){s.running=true;reserved[target]=true;}
 }
 // Anti-traba (mansion): si un actor engagado lleva minutos sin moverse con
 // enemigo vivo, libera el latch pegajoso (silencioso, sin empujones) para
 // que su IA nativa/Latch re-enganche o retome rutas por su cuenta.
 if(davidStuckFlee.value>0){
  for(auto& s:davidStates){
   auto e=s.entity;
   if(!valid(e)||e->serialnumber!=s.serial||!mansionMap()||!mansionActor(e))continue;
   if(e->v.deadflag!=DEAD_NO||e->v.health<=0)continue;
   if(!aliveEnemy(e))continue;
   auto& latch=mansionEngageFor(e);
   if(!latch.engaged||latch.actor!=e)continue;
   if(s.stuckSince<0||globals->time-s.stuckSince<davidStuckFlee.value)continue;
   latch.engaged=false;
   s.cooldown=globals->time+1;s.clearSince=-1;
   startupTrace("DavidStuckRelease","latch liberado por inmovilidad prolongada");
  }
 }
}
static bool johnnyPlayerAlive(edict_t* p){return valid(p)&&(p->v.flags&FL_CLIENT)&&p->v.health>0&&p->v.deadflag==DEAD_NO;}
static bool johnnyCanSee(edict_t* e,edict_t* p){if(!johnnyPlayerAlive(p)||length(p->v.origin-e->v.origin)>MansionZombieNoticeRange)return false;Vector from=e->v.origin+e->v.view_ofs,to=p->v.origin+p->v.view_ofs;TraceResult tr{};engine.pfnTraceLine(&from.x,&to.x,1,e,&tr);return !tr.fStartSolid&&(tr.flFraction>=.999f||tr.pHit==p);}
static edict_t* johnnyTarget(DavidState& s){
 if(valid(s.target)&&s.target->serialnumber==s.targetSerial&&johnnyCanSee(s.entity,s.target))return s.target;
 edict_t* best=nullptr;float bestDistance=MansionZombieNoticeRange;
 for(int i=1;i<=globals->maxClients;++i){auto p=engine.pfnPEntityOfEntIndex(i);if(!johnnyCanSee(s.entity,p))continue;float d=length(p->v.origin-s.entity->v.origin);if(d<bestDistance){bestDistance=d;best=p;}}
 if(best){s.target=best;s.targetSerial=best->serialnumber;}return best;
}
static void johnnyStartThrow(DavidState& s,edict_t* target){
 if(!johnnyModel(s.entity)||s.saved.johnnyAxes==0||s.johnnyGuarding||s.johnnyThrowing||globals->time<s.johnnyNextThrow||!johnnyPlayerAlive(target))return;
 // Fix 5: en el puente el throw de 1s con velocity=0 tapona a toda la fila.
 // Se inhibe el inicio sobre la ruta del puente; fuera del puente sigue igual.
 if(mansionMap()&&mansionActor(s.entity)&&mansionOnBridgeRoute(s.entity))return;
 auto e=s.entity;float d=length(target->v.origin-e->v.origin);if(d<112.f||d>320.f||!johnnyCanSee(e,target))return;
 int side=(s.saved.johnnyAxes&1)?1:2;if((side==1&&(s.saved.severMask&((1<<3)|(1<<4)|(1<<5))))||(side==2&&(s.saved.severMask&((1<<6)|(1<<7)|(1<<8)))))side=(side==1?2:1);
 if(!(s.saved.johnnyAxes&side))return;
 s.johnnyThrowSide=side;s.johnnyThrowTarget=target;s.johnnyThrowTargetSerial=target->serialnumber;s.johnnyThrowAim=target->v.origin+target->v.view_ofs;s.johnnyThrowReleased=false;
 s.johnnyThrowing=true;s.johnnyThrowUntil=globals->time+johnny_assets::duration[johnny_assets::seqThrow];s.johnnyNextThrow=s.johnnyThrowUntil+.25f;
 Vector toward=target->v.origin-e->v.origin;e->v.angles.y=std::atan2(toward.y,toward.x)*180.f/3.14159265f;e->v.velocity.x=0;e->v.velocity.y=0;e->v.sequence=johnny_assets::seqThrow;e->v.frame=0;e->v.animtime=globals->time;e->v.framerate=slowed(e)?balance::SlowFactor:1.f;
 if(e->pvPrivateData)reinterpret_cast<void(__thiscall*)(void*)>(ravenholm_abi::address(ravenModule,ravenholm_abi::ResetSequenceInfoRva))(e->pvPrivateData);
}
static bool johnnyLaunchAxe(DavidState& s){
 auto e=s.entity;int side=s.johnnyThrowSide;
 if(s.dying||!aliveEnemy(e)||!s.johnnyThrowing)return false;
 if(s.johnnyThrowReleased)return true;
 if(!johnnyModel(e)||!side||!(s.saved.johnnyAxes&side))return false;
 int hand=side==1?5:8;Vector origin,handAngles;if(engine.pfnGetBonePosition)engine.pfnGetBonePosition(e,johnny_assets::bone[hand],&origin.x,&handAngles.x);else origin=e->v.origin+e->v.view_ofs;
 Vector aim=s.johnnyThrowAim;auto target=s.johnnyThrowTarget;
 if(valid(target)&&target->serialnumber==s.johnnyThrowTargetSerial&&johnnyPlayerAlive(target))aim=target->v.origin+target->v.view_ofs;
 Vector delta=aim-origin;float distance=length(delta);if(distance<1)return false;Vector direction=normal(delta);
 auto axe=engine.pfnCreateNamedEntity(engine.pfnAllocString("info_target"));if(!valid(axe))return false;raven.pfnSpawn(axe);if(!valid(axe))return false;
 axe->v.classname=engine.pfnAllocString("cremation_johnny_axe");axe->v.owner=e;axe->v.movetype=MOVETYPE_FLY;axe->v.solid=SOLID_BBOX;axe->v.takedamage=DAMAGE_NO;axe->v.gravity=0;float slashDamage=engine.pfnCVarGetFloat?engine.pfnCVarGetFloat("sk_zombie_dmg_one_slash"):20.f;axe->v.dmg=slashDamage>0?slashDamage:20.f;
 engine.pfnSetModel(axe,side==1?JohnnyAxe:JohnnyAxeLeft);Vector mins(-2,-2,-2),maxs(2,2,2);engine.pfnSetSize(axe,&mins.x,&maxs.x);engine.pfnSetOrigin(axe,&origin.x);
 axe->v.velocity=direction*480.f;axe->v.angles=Vector(-std::atan2(delta.z,std::sqrt(delta.x*delta.x+delta.y*delta.y))*180.f/3.14159265f,std::atan2(delta.y,delta.x)*180.f/3.14159265f,0);axe->v.avelocity=Vector(0,0,720);axe->v.fuser1=globals->time+3.f;
 s.saved.johnnyAxes&=~side;s.johnnyThrowReleased=true;davidSetBody(e,hand,3);s.poseAt=-100;return true;
}
static void johnnyFinishThrow(DavidState& s){
 if(!s.johnnyThrowReleased)johnnyLaunchAxe(s);
 s.johnnyThrowing=false;s.johnnyThrowTarget=nullptr;s.johnnyThrowTargetSerial=0;s.johnnyThrowSide=0;
}
static bool johnnyBeginGuard(DavidState& s){
 if(s.dying||!aliveEnemy(s.entity)||!johnnyModel(s.entity)||!s.saved.johnnyAxes||s.saved.johnnyDefended)return false;
 // Fix 5: el guard de 1s con velocity=0 tambien tapona el puente. Fuera del
 // puente mantiene su funcion defensiva intacta.
 if(mansionMap()&&mansionActor(s.entity)&&mansionOnBridgeRoute(s.entity))return false;
 auto e=s.entity;s.saved.johnnyDefended=1;s.saved.johnnyGuardRemaining=1.f;s.johnnyGuarding=true;s.johnnyGuardUntil=globals->time+1.f;s.johnnyThrowing=false;s.johnnyThrowTarget=nullptr;s.johnnyThrowTargetSerial=0;s.johnnyThrowSide=0;s.johnnyThrowReleased=false;s.johnnyNextThrow=s.johnnyGuardUntil;
 e->v.velocity.x=0;e->v.velocity.y=0;e->v.sequence=johnny_assets::seqGuard;e->v.frame=0;e->v.animtime=globals->time;e->v.framerate=slowed(e)?balance::SlowFactor:1.f;
 if(e->pvPrivateData)reinterpret_cast<void(__thiscall*)(void*)>(ravenholm_abi::address(ravenModule,ravenholm_abi::ResetSequenceInfoRva))(e->pvPrivateData);
 return true;
}
static void johnnyProjectileFrame(){
 for(int i=globals->maxClients+1;i<globals->maxEntities;++i){auto e=engine.pfnPEntityOfEntIndex(i);if(valid(e)&&e->v.classname&&!std::strcmp(text(e->v.classname),"cremation_johnny_axe")&&globals->time>=e->v.fuser1)engine.pfnRemoveEntity(e);}
}
static bool davidDeathActivity(int activity){return (activity>=36&&activity<=39)||(activity>=66&&activity<=69);}
static bool davidDeathSequence(edict_t* e){if(maxModel(e))return e->v.sequence>=max_assets::seqDieSimple&&e->v.sequence<=max_assets::seqDieHead;return johnnyModel(e)?e->v.sequence>=9&&e->v.sequence<=13:e->v.sequence>=8&&e->v.sequence<=12;}
static void davidCancelActions(DavidState& s){
 s.running=false;s.clearSince=-1;s.target=nullptr;s.attackEventConsumed=true;
 s.johnnyThrowing=false;s.johnnyThrowSide=0;s.johnnyThrowTarget=nullptr;s.johnnyThrowTargetSerial=0;
 s.johnnyGuarding=false;s.johnnyGuardUntil=0;s.saved.johnnyGuardRemaining=0;
}
static int __fastcall davidGetDeathActivity(void* object,void*){
 auto t=davidTable(object);auto s=davidState(davidObject(object));if(!t)return 36;
 if(s&&davidDeathActivity(s->deathActivity))return s->deathActivity;
 // Verified donor slot 114 / RVA ravenholm_abi::GetDeathActivityRva: choose from native hitgroup,
 // attack direction and available space before Killed recycles corpse health.
 int chosen=reinterpret_cast<int(__thiscall*)(void*)>(t->deathActivity)(object);
 if(!davidDeathActivity(chosen))chosen=36;
 if(s)s->deathActivity=chosen;return chosen;
}
static void __fastcall davidActivity(void* object,void*,int requested){
 auto t=davidTable(object);auto e=davidObject(object);auto s=davidState(e);if(!t)return;int attackSequence=-1;
 bool death=s&&(davidDeathActivity(requested)||s->dying||!aliveEnemy(e));
 if(death){
  s->dying=true;davidCancelActions(*s);
  // A death is a single non-looping clip. Duplicate native requests must not
  // call SetActivity again: it rewinds even an unchanged non-looping sequence.
  if(s->deathStarted&&davidDeathSequence(e))return;
  if(!davidDeathActivity(s->deathActivity))s->deathActivity=davidDeathActivity(requested)?requested:davidGetDeathActivity(object,nullptr);
  requested=s->deathActivity;
 }
 if(s){
  if(johnnyModel(e)&&s->johnnyThrowing){if(globals->time<s->johnnyThrowUntil)return;johnnyFinishThrow(*s);}
  if(johnnyModel(e)&&s->johnnyGuarding){if(globals->time<s->johnnyGuardUntil)return;s->johnnyGuarding=false;e->v.sequence=johnny_assets::seqIdle;e->v.frame=0;e->v.animtime=globals->time;}
  bool alive=aliveEnemy(s->entity),crawl=davidCrawling(*s);
  if(alive&&!death){
  if(requested==3||requested==4||requested==48||requested==49){
   int desired=maxModel(e)?(s->running?max_assets::seqRun:max_assets::seqWalk):(johnnyModel(e)?(s->running?johnny_assets::seqRun:johnny_assets::seqWalk):(crawl?david_assets::seqCrawl:(s->running?david_assets::seqRun:david_assets::seqWalk)));
   // MoveExecute is called for every route step. Reapplying SetActivity there
   // resets the animation timer and made walk/run stutter. Preserve attacks,
   // pain and death clips until they finish; only switch when locomotion changes.
   if(e->v.sequence==desired)return;
   if(davidBusy(e)&&e->v.frame<254)return;
   requested=crawl?48:(s->running?4:3);
  }
  else if(crawl&&(requested==1||requested==47||requested==61||requested==26||requested==27||requested==28||requested==29||requested==70||requested==71||requested==72||requested==73||requested==74||requested==75||requested==76))requested=18;
  // Select strikes for both native melee activities. Each selected sequence
  // owns its own contact event and native animation timing.
  if(requested==30||requested==31){
   ++s->attackRequests[requested-30];
   if(maxModel(e)){
    attackSequence=max_assets::seqAttack[engine.pfnRandomLong(0,max_assets::attackCount-1)];
   }else if(johnnyModel(e)){
    bool right=(s->saved.severMask&((1<<3)|(1<<4)|(1<<5)))==0,left=(s->saved.severMask&((1<<6)|(1<<7)|(1<<8)))==0;
    if(s->saved.johnnyAxes){right=right&&(s->saved.johnnyAxes&1);left=left&&(s->saved.johnnyAxes&2);}
    int choices[3],count=0;if(right)choices[count++]=johnny_assets::seqAttackRight;if(left)choices[count++]=johnny_assets::seqAttackLeft;if(right&&left)choices[count++]=johnny_assets::seqAttackBoth;
    if(count)attackSequence=choices[engine.pfnRandomLong(0,count-1)];
   }else{
    static constexpr int attacks[]={david_assets::seqAttackRight,david_assets::seqAttackLeft,david_assets::seqAttackBoth};
    int chosen=david_logic::uniformAttackEvent(engine.pfnRandomLong(0,2),s->saved.severMask);
    if(chosen)attackSequence=attacks[chosen-1];
   }
  }

  }
 }
 bool wasLoco=valid(e)&&(maxModel(e)?(e->v.sequence==max_assets::seqWalk||e->v.sequence==max_assets::seqRun):(johnnyModel(e)?(e->v.sequence==johnny_assets::seqWalk||e->v.sequence==johnny_assets::seqRun):(e->v.sequence==1||e->v.sequence==2||e->v.sequence==david_assets::seqCrawl)));
 float oldFrame=valid(e)?e->v.frame:0.f;
 bool slow=!death&&valid(e)&&slowed(e);float rate=slow?e->v.framerate:0;
 reinterpret_cast<void(__thiscall*)(void*,int)>(t->activity)(object,requested);
 if(death&&valid(e)){
  // Standing actors must never randomly select David's retired prone death.
  int simple=maxModel(e)?max_assets::seqDieSimple:(johnnyModel(e)?johnny_assets::seqDieSimple:8);
  int sequence=simple+(requested>=36&&requested<=39?requested-36:0);
  if(requested==66)sequence=maxModel(e)?max_assets::seqDieHead:(johnnyModel(e)?13:12);
  else if(requested==68)sequence=simple+2;
  else if(requested==69)sequence=simple+1;
  if(e->v.sequence!=sequence){e->v.sequence=sequence;e->v.frame=0;e->v.animtime=globals->time;reinterpret_cast<void(__thiscall*)(void*)>(ravenholm_abi::address(ravenModule,ravenholm_abi::ResetSequenceInfoRva))(object);}
  e->v.framerate=1;
  s->deathStarted=true;
 }
 if(attackSequence>=0&&s&&valid(e)){
  e->v.sequence=attackSequence;e->v.frame=0;e->v.animtime=globals->time;
  // Verified donor CBaseAnimating::ResetSequenceInfo. Recompute native frame
  // rate, sequence-finished/loop flags and event clock after overriding its pick.
  reinterpret_cast<void(__thiscall*)(void*)>(ravenholm_abi::address(ravenModule,ravenholm_abi::ResetSequenceInfoRva))(object);
  s->attackEventConsumed=false;
  if(johnnyModel(e)||maxModel(e))++s->attackSelections[0];else ++s->attackSelections[attackSequence-david_assets::seqAttackRight];
 }else if(valid(e)&&!death&&s&&aliveEnemy(e)){
  bool isLoco=maxModel(e)?(e->v.sequence==max_assets::seqWalk||e->v.sequence==max_assets::seqRun):(johnnyModel(e)?(e->v.sequence==johnny_assets::seqWalk||e->v.sequence==johnny_assets::seqRun):(e->v.sequence==1||e->v.sequence==2||e->v.sequence==david_assets::seqCrawl));
  if(wasLoco&&isLoco){e->v.frame=oldFrame;e->v.animtime=globals->time;}
 }
 if(slow&&valid(e))e->v.framerate=rate;
}
static void __fastcall davidMove(void* object,void*,void* target,const Vector& dir,float interval){
 auto t=davidTable(object);auto s=davidState(davidObject(object));if(!t)return;
 if(s&&s->dying)return;
 if(s){s->movedAt=globals->time;
  if(target){s->target=nullptr;
   for(int i=1;i<globals->maxEntities;++i){auto e=engine.pfnPEntityOfEntIndex(i);if(valid(e)&&e->pvPrivateData==target){s->target=e;s->targetSerial=e->serialnumber;break;}}
  }
  if(maxModel(s->entity)&&!(mansionMap()&&mansionActor(s->entity)))s->running=true;else if(s->target&&!(s->target->v.flags&FL_CLIENT))s->running=false;
  if(johnnyModel(s->entity)){
   if(s->johnnyGuarding&&globals->time<s->johnnyGuardUntil)return;
   if(s->johnnyThrowing&&globals->time<s->johnnyThrowUntil)return;
   if(s->saved.johnnyAxes){johnnyStartThrow(*s,s->target);if(s->johnnyThrowing)return;}
  }
  davidActivity(object,nullptr,s->running?4:3);
  if(mansionMap()&&mansionActor(s->entity))return;
 }
 reinterpret_cast<void(__thiscall*)(void*,void*,const Vector&,float)>(t->move)(object,target,dir,interval);
}
static void __fastcall davidEvent(void* object,void*,MonsterEvent_t* event){
 auto t=davidTable(object);auto e=davidObject(object);auto s=davidState(e);if(!t)return;MonsterEvent_t changed=*event;
 if(s&&(s->dying||!aliveEnemy(e))&&(event->event==20||(event->event>=1&&event->event<=3)))return;
 if(s&&johnnyModel(e)&&event->event==20){if(!s->johnnyThrowReleased)johnnyLaunchAxe(*s);return;}
 bool melee=event->event>=1&&event->event<=3;
 if(s&&melee){if(s->attackEventConsumed)return;changed.event=david_logic::attackEvent(event->event,s->saved.severMask);if(!changed.event)return;s->attackEventConsumed=true;++s->meleeEvents;}
 // Scope the bonus to this actor's actual native melee event. Victim damage
 // wrappers apply it before armor/resistance, retaining fractional damage.
 auto previous=steveMeleeActor;auto previousMax=maxMeleeActor;auto previousKen=kenMeleeActor;if(s&&melee&&kenModel(e))kenMeleeActor=e;if(s&&melee&&steveModel(e))steveMeleeActor=e;if(s&&melee&&maxModel(e)){maxMeleeActor=e;engine.pfnEmitSound(e,CHAN_WEAPON,MaxSawSound,1,ATTN_NORM,0,100);}
 reinterpret_cast<void(__thiscall*)(void*,MonsterEvent_t*)>(t->event)(object,&changed);
 steveMeleeActor=previous;maxMeleeActor=previousMax;kenMeleeActor=previousKen;
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
 char model[128];std::snprintf(model,sizeof model,"models/cremation/hotd2/%s/part_%02d.mdl",kenModel(e)?"ken":(maxModel(e)?"max":(johnnyModel(e)?"johnny":(steveModel(e)?"steve":"david"))),node);engine.pfnSetModel(gib,model);
 gib->v.targetname=engine.pfnAllocString(DavidGibTag);gib->v.body=std::min(davidBody(e,node),davidBodyCount(e,node)-2);gib->v.sequence=0;gib->v.frame=0;gib->v.framerate=0;gib->v.animtime=globals->time;gib->v.angles=angles;
 gib->v.movetype=MOVETYPE_NONE;gib->v.solid=SOLID_NOT;gib->v.takedamage=DAMAGE_NO;gib->v.nextthink=0;Vector zero(0,0,0);engine.pfnSetSize(gib,&zero.x,&zero.x);engine.pfnSetOrigin(gib,&at.x);
 Vector velocity=e->v.velocity+normal(impulse)*100+Vector(0,0,100);if(length(velocity)>260)velocity=normal(velocity)*260;
 AnatomicalGib g{gib,gib->serialnumber,globals->time+1,globals->time,globals->time,velocity};g.localized=true;anatomicalGibs.push_back(g);
}
static void davidSever(DavidState& s,int node,const Vector& impulse){
 if(node==1||node==9||(s.saved.severMask&(1<<node))||!davidVisiblePart(s.entity,node))return;
 auto e=s.entity;for(int n=2;n<16;++n)if(n!=9&&david_logic::descendant(n,node,david_assets::parent)&&davidVisiblePart(e,n))davidPiece(s,n,impulse);
 if(johnnyModel(e)){if(david_logic::descendant(5,node,david_assets::parent))s.saved.johnnyAxes&=~1;if(david_logic::descendant(8,node,david_assets::parent))s.saved.johnnyAxes&=~2;}
 s.saved.severMask|=1<<node;
 for(int n=2;n<16;++n)if(n!=9&&david_logic::descendant(n,node,david_assets::parent)){
  bool hasChild=false;for(int child=1;child<16;++child)if(david_assets::parent[child]==n)hasChild=true;
  if(n==node&&hasChild)davidSetBody(e,n,std::min(2,davidBodyCount(e,n)-2));else davidSetBody(e,n,davidBodyCount(e,n)-1);
 }
 s.running=false;s.clearSince=-1;s.poseAt=-100;
 if(node==2&&e->pvPrivateData){
  bool notify=e->v.deadflag==DEAD_NO&&!s.killedDuringDamage;e->v.health=0;
  if(notify){auto table=*reinterpret_cast<void***>(e->pvPrivateData);reinterpret_cast<void(__thiscall*)(void*,entvars_t*,int)>(table[ravenholm_abi::slot::Killed])(e->pvPrivateData,s.pendingAttacker,1);}
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
 if(enemyCombatBlocked(object,attacker,attacker))return;
 auto t=davidTable(object);auto s=davidState(davidObject(object));if(!t)return;
 if(s&&johnnyModel(s->entity)){
  if(s->johnnyGuarding&&globals->time<s->johnnyGuardUntil)return;
  if(attacker&&(attacker->flags&FL_CLIENT)&&amount>0&&!s->saved.johnnyDefended&&johnnyBeginGuard(*s))return;
 }
 reinterpret_cast<void(__thiscall*)(void*,entvars_t*,float,Vector,TraceResult*,int)>(t->trace)(object,attacker,amount,direction,tr,bits);
 if(davidLocalizedDamage.value==0||!s||!tr||amount<=0||bits&((1<<6)|(1<<3)|(1<<21)))return;
 int n=s->lastTrace==globals->time&&length(s->lastPoint-tr->vecEndPos)<.1f?s->lastNode:davidNodeAt(*s,tr->vecEndPos);if(!n)return;
 if(s->pendingAt!=globals->time||s->pendingAttacker!=attacker){std::fill_n(s->pending,16,0.f);s->pendingAt=globals->time;s->pendingAttacker=attacker;}
 s->pending[n]+=davidTraceWeight(amount,tr->iHitgroup);s->directAt=-100;s->directDirection=direction;
}
static int __fastcall davidDamage(void* object,void*,entvars_t* inflictor,entvars_t* attacker,float amount,int bits){
 if(enemyCombatBlocked(object,inflictor,attacker))return 0;
 auto t=davidTable(object);auto e=davidObject(object);auto s=davidState(e);if(!t)return 0;
 if(s&&johnnyModel(e)){
  if(s->johnnyGuarding&&globals->time<s->johnnyGuardUntil)return 0;
  if(attacker&&(attacker->flags&FL_CLIENT)&&amount>0&&!s->saved.johnnyDefended&&johnnyBeginGuard(*s))return 0;
 }
 if(!s||s->inDamage)return t->damage(object,inflictor,attacker,amount,bits);
 if(davidLocalizedDamage.value==0){std::fill_n(s->pending,16,0.f);s->pendingAt=-100;s->directAt=-100;s->inDamage=true;s->killedDuringDamage=false;int result=t->damage(object,inflictor,attacker,amount,bits);s->inDamage=false;return result;}
 bool wasAlive=aliveEnemy(e);float before=e->v.health;float regional[16]{};bool area=(bits&((1<<6)|(1<<3)|(1<<21)))!=0;
 if(!area&&s->pendingAt==globals->time&&s->pendingAttacker==attacker)std::copy_n(s->pending,16,regional);
 else if(!area&&s->directAt==globals->time&&s->directAttacker==attacker&&s->directNode>0)regional[s->directNode]=1;
 std::fill_n(s->pending,16,0.f);s->pendingAt=-100;s->directAt=-100;s->inDamage=true;s->killedDuringDamage=false;
 int result=t->damage(object,inflictor,attacker,amount,bits);s->inDamage=false;
 if(!wasAlive||!valid(e)||s->serial!=e->serialnumber||(e->v.effects&EF_NODRAW))return result;
 // BecomeDead can replace health with corpse durability before TakeDamage
 // returns. Use the lethal health captured at Killed, not that recycled value.
 float loss=std::max(0.f,before-(s->killedDuringDamage?s->healthAtKilled:e->v.health)),total=0;for(float value:regional)total+=value;
 if(loss<=0||total<=0)return result;s->pendingAttacker=attacker;
 for(int n=1;n<16;++n)if(regional[n]>0){s->saved.regional[n]+=loss*regional[n]/total;float fraction=n==2?davidHeadThreshold.value:(n==1?.5f:davidLimbThreshold.value);float threshold=std::max(1.f,s->saved.initialHealth*std::max(.05f,fraction));
  if(n==1){int stage=std::min(davidBodyCount(e,n)-1,int(s->saved.regional[n]/std::max(1.f,threshold*.5f)));if(stage>davidBody(e,n))davidSetBody(e,n,stage);}
  else if(n!=9&&!(s->saved.severMask&(1<<n))){
   if(s->saved.regional[n]>=threshold)davidSever(*s,n,s->directDirection);
   else if(n==2){int stage=s->saved.regional[n]>=threshold*.75f?2:(s->saved.regional[n]>=threshold*.45f?1:0);if(stage>davidBody(e,n))davidSetBody(e,n,stage);}
   else if(s->saved.regional[n]>=threshold*.45f&&davidBody(e,n)==0&&davidBodyCount(e,n)>2)davidSetBody(e,n,1);
  }
 }
 return result;
}
static void __fastcall davidKilled(void* object,void*,entvars_t* attacker,int gib){
 auto t=davidTable(object);auto s=davidState(davidObject(object));if(!t)return;if(s){davidGetDeathActivity(object,nullptr);s->dying=true;davidCancelActions(*s);if(s->inDamage){s->killedDuringDamage=true;s->healthAtKilled=s->entity->v.health;}}
 auto e=davidObject(object);int body=e?e->v.body:0;
 reinterpret_cast<void(__thiscall*)(void*,entvars_t*,int)>(t->killed)(object,attacker,gib);
 if(valid(e)&&davidModel(e)&&!(e->v.effects&EF_NODRAW)){e->v.body=body;if(!s||!s->deathStarted||!davidDeathSequence(e))davidActivity(object,nullptr,s?s->deathActivity:36);e->v.framerate=1;}
}
static bool davidInstall(edict_t* e){
 if(!davidClass(e)||!e->pvPrivateData)return false;auto source=*reinterpret_cast<void***>(e->pvPrivateData);
 for(auto& t:davidTables){if(source==t->slots.data())return true;if(source==t->source){*reinterpret_cast<void***>(e->pvPrivateData)=t->slots.data();return true;}}
 auto base=reinterpret_cast<unsigned char*>(ravenModule);
 // Fail closed on a different donor build rather than treating HLSDK layouts as proof.
 if(!base||source[ravenholm_abi::slot::TraceAttack]!=ravenholm_abi::address(ravenModule,ravenholm_abi::ZombieTraceAttackRva)||source[ravenholm_abi::slot::HandleAnimEvent]!=ravenholm_abi::address(ravenModule,ravenholm_abi::ZombieHandleAnimEventRva)||source[ravenholm_abi::slot::MoveExecute]!=ravenholm_abi::address(ravenModule,ravenholm_abi::ZombieMoveExecuteRva)||source[ravenholm_abi::slot::SetActivity]!=ravenholm_abi::address(ravenModule,ravenholm_abi::ZombieSetActivityRva)||source[ravenholm_abi::slot::GetDeathActivity]!=ravenholm_abi::address(ravenModule,ravenholm_abi::GetDeathActivityRva)||(source[ravenholm_abi::slot::Relationship]!=ravenholm_abi::address(ravenModule,ravenholm_abi::MonsterRelationshipRva)&&source[ravenholm_abi::slot::Relationship]!=reinterpret_cast<void*>(&enemyCombatRelationship))||std::memcmp(ravenholm_abi::address(ravenModule,ravenholm_abi::ResetSequenceInfoRva),"\x56\x8b\xf1\x8b\x46\x04",6)){startupTrace("David ERROR","unsupported Ravenholm zombie ABI; kept native zombie");return false;}
 auto t=std::make_unique<DavidTable>();t->source=source;std::copy_n(source,125,t->slots.begin());
 // Existing shared wrappers resolve by table identity. Unwrap then reinstall
 // around our clone so each wrapper finds its own original without recursion.
 for(auto& b:bloodDamageTables)if(b.table==source){t->slots[ravenholm_abi::slot::TakeDamage]=reinterpret_cast<void*>(b.original);t->slots[ravenholm_abi::slot::Killed]=b.killed;break;}
 for(auto& h:humanGibTables)if(h.table==source){t->slots[ravenholm_abi::slot::TakeDamage]=h.damage;t->slots[ravenholm_abi::slot::Killed]=h.killed;break;}
 t->trace=t->slots[ravenholm_abi::slot::TraceAttack];t->damage=reinterpret_cast<TakeDamageFn>(t->slots[ravenholm_abi::slot::TakeDamage]);t->killed=t->slots[ravenholm_abi::slot::Killed];t->event=t->slots[ravenholm_abi::slot::HandleAnimEvent];t->move=t->slots[ravenholm_abi::slot::MoveExecute];t->activity=t->slots[ravenholm_abi::slot::SetActivity];t->deathActivity=t->slots[ravenholm_abi::slot::GetDeathActivity];
 t->slots[ravenholm_abi::slot::TraceAttack]=reinterpret_cast<void*>(&davidTraceAttack);t->slots[ravenholm_abi::slot::TakeDamage]=reinterpret_cast<void*>(&davidDamage);t->slots[ravenholm_abi::slot::Killed]=reinterpret_cast<void*>(&davidKilled);t->slots[ravenholm_abi::slot::HandleAnimEvent]=reinterpret_cast<void*>(&davidEvent);t->slots[ravenholm_abi::slot::MoveExecute]=reinterpret_cast<void*>(&davidMove);t->slots[ravenholm_abi::slot::SetActivity]=reinterpret_cast<void*>(&davidActivity);t->slots[ravenholm_abi::slot::GetDeathActivity]=reinterpret_cast<void*>(&davidGetDeathActivity);
 *reinterpret_cast<void***>(e->pvPrivateData)=t->slots.data();davidTables.push_back(std::move(t));humanGibInstall(e);weaponBloodInstall(e);return true;
}
static void davidSpawn(edict_t* e,bool restoring=false){
 if(!davidClass(e)||(!restoring&&davidEnable.value==0)||!davidInstall(e))return;
 auto s=davidState(e,true);
 if(!restoring){s->dying=false;s->deathActivity=0;s->deathStarted=false;}
 if(!restoring){float special=std::max(0.f,std::min(1.f,johnnyChance.value));const char* targetname=text(e->v.targetname);bool fixedWave=targetname&&!std::strncmp(targetname,"mansion_enemy_fixed_",20);bool mansionJohnny=targetname&&!std::strncmp(targetname,"mansion_enemy_johnny_",21);bool maxNpc=e->v.classname&&!std::strcmp(text(e->v.classname),"monster_max");bool fixed=(e->v.classname&&!std::strcmp(text(e->v.classname),"monster_johnny"))||mansionJohnny;bool kenNpc=e->v.classname&&!std::strcmp(text(e->v.classname),"monster_ken");bool randomJohnny=!fixed&&!fixedWave&&!maxNpc&&!kenNpc&&special>0&&engine.pfnRandomFloat(0,1)<special;
  if(kenNpc){s->saved.variant=4;}
  else if(maxNpc){s->saved.variant=3;s->running=true;}
  else if(fixed||randomJohnny){s->saved.variant=2;s->saved.johnnyAxes=3;s->saved.johnnyDefended=0;s->johnnyGuarding=false;s->johnnyThrowing=false;s->johnnyNextThrow=0;}
  else if(kenChance.value>0&&engine.pfnRandomFloat(0,1)<std::min(1.f,kenChance.value)){s->saved.variant=4;}
  else {float chance=steveChance.value;s->saved.variant=(chance>=1.f||(chance>0&&engine.pfnRandomFloat(0,1)<chance))?1:0;}
  if(s->saved.variant==1){e->v.health*=1.1f;e->v.max_health=e->v.health;}
  s->saved.initialHealth=std::max(1.f,e->v.health);s->saved.prone=0;e->v.body=0;engine.pfnSetModel(e,s->saved.variant==4?KenModel:s->saved.variant==3?MaxModel:(s->saved.variant==2?JohnnyModel:(s->saved.variant==1?SteveModel:DavidModel)));e->v.sequence=s->saved.variant==3?max_assets::seqIdle:(s->saved.variant==2?johnny_assets::seqIdle:0);e->v.frame=0;e->v.animtime=globals->time;}
 if(davidLocalizedDamage.value==0){s->saved.severMask=0;std::fill_n(s->saved.regional,16,0.f);e->v.body=0;}
 Vector lo(-16,-16,0),hi(16,16,72);engine.pfnSetSize(e,&lo.x,&hi.x);e->v.view_ofs=Vector(0,0,64);
 if(maxModel(e))s->running=true;
 startupTrace(kenModel(e)?"Ken":maxModel(e)?"Max":(johnnyModel(e)?"Johnny":(steveModel(e)?"Steve":"David")),restoring?"restored original wounds/rig":(maxModel(e)?"spawn Max with two chainsaws":(johnnyModel(e)?"spawn Johnny with two throwable axes":"spawn David/Steve variant")));
}
static void davidFrame(){
 johnnyProjectileFrame();davidDecision();for(auto& s:davidStates)if(valid(s.entity)&&s.entity->serialnumber==s.serial&&davidModel(s.entity)&&!s.dying&&aliveEnemy(s.entity)){
  auto e=s.entity;
  if(maxModel(e)&&!(mansionMap()&&mansionActor(e)))s.running=true;
  if(johnnyModel(e)){
   if(s.johnnyGuarding&&globals->time>=s.johnnyGuardUntil){s.johnnyGuarding=false;s.johnnyThrowing=false;davidActivity(e->pvPrivateData,nullptr,3);}
   if(s.johnnyThrowing&&globals->time>=s.johnnyThrowUntil){johnnyFinishThrow(s);if(e->v.sequence==johnny_assets::seqThrow){davidActivity(e->pvPrivateData,nullptr,s.running?4:3);if(e->v.sequence==johnny_assets::seqThrow){e->v.sequence=s.running?johnny_assets::seqRun:johnny_assets::seqWalk;e->v.frame=0;e->v.animtime=globals->time;e->v.framerate=slowed(e)?balance::SlowFactor:1.f;if(e->pvPrivateData)reinterpret_cast<void(__thiscall*)(void*)>(ravenholm_abi::address(ravenModule,ravenholm_abi::ResetSequenceInfoRva))(e->pvPrivateData);}}}
   if(!s.johnnyGuarding&&s.saved.johnnyAxes&&!s.johnnyThrowing&&aliveEnemy(e)){auto target=johnnyTarget(s);if(target)johnnyStartThrow(s,target);}
  }
  int seq=e->v.sequence;bool locomotion=maxModel(e)?(seq==max_assets::seqWalk||seq==max_assets::seqRun):(johnnyModel(e)?(seq==johnny_assets::seqWalk||seq==johnny_assets::seqRun):(seq==1||seq==2||seq==20));
  bool moving=length2d(e->v.velocity)>1.0f||(globals->time-s.movedAt<.15f);
  // Detector de inmovilidad real (anti-traba): marca desde cuando el
  // desplazamiento es ~0 con enemigo vivo; davidDecision lo usa para soltar
  // el latch pegajoso despues de david_stuck_release segundos.
  if(aliveEnemy(e)&&e->v.deadflag==DEAD_NO){
   if(length2d(e->v.velocity)>2.0f)s.stuckSince=-1000;
   else if(s.stuckSince<0)s.stuckSince=globals->time;
  } else s.stuckSince=-1000;
  if((locomotion||(moving&&seq==0))&&(maxModel(e)||globals->time-s.movedAt<.45f)){int desired=maxModel(e)?(s.running?max_assets::seqRun:max_assets::seqWalk):(johnnyModel(e)?(s.running?johnny_assets::seqRun:johnny_assets::seqWalk):(davidCrawling(s)?david_assets::seqCrawl:(s.running?david_assets::seqRun:david_assets::seqWalk)));if(seq!=desired)davidActivity(e->pvPrivateData,nullptr,s.running?4:3);}
 }
}
static void davidSave(edict_t* e,SAVERESTOREDATA* data){auto s=davidState(e);if(s&&davidModel(e)&&raven.pfnSaveWriteFields){s->saved.johnnyGuardRemaining=johnnyModel(e)&&s->johnnyGuarding?std::max(0.f,s->johnnyGuardUntil-globals->time):0.f;raven.pfnSaveWriteFields(data,"CremationDavid1",&s->saved,davidSaveFields,ARRAYSIZE(davidSaveFields));}}
static void davidRestore(edict_t* e,SAVERESTOREDATA* data){
 if(!davidClass(e))return;bool saved=davidModel(e);
 if(!saved){davidSpawn(e);return;}
 auto s=davidState(e,true);if(raven.pfnSaveReadFields)raven.pfnSaveReadFields(data,"CremationDavid1",&s->saved,davidSaveFields,ARRAYSIZE(davidSaveFields));
 if(s->saved.version<2)s->saved.prone=0;if(s->saved.version<4){s->saved.johnnyAxes=3;s->saved.johnnyDefended=0;}if(s->saved.version<5)s->saved.johnnyGuardRemaining=0;s->saved.variant=kenModel(e)?4:maxModel(e)?3:(johnnyModel(e)?2:(steveModel(e)?1:0));s->saved.version=6;
 if(s->saved.initialHealth<=0)s->saved.initialHealth=std::max(1.f,e->v.max_health>0?e->v.max_health:e->v.health);
 bool alive=aliveEnemy(e),visibleCorpse=!alive&&!(e->v.effects&EF_NODRAW);
 s->dying=!alive;
 s->deathStarted=visibleCorpse&&davidDeathSequence(e);
 int deathBase=maxModel(e)?max_assets::seqDieSimple:(johnnyModel(e)?johnny_assets::seqDieSimple:8);
 int headDeath=maxModel(e)?max_assets::seqDieHead:(johnnyModel(e)?johnny_assets::seqDieViolent+1:12);
 s->deathActivity=s->deathStarted?(e->v.sequence==headDeath?66:36+std::min(3,std::max(0,e->v.sequence-deathBase))):0;
 bool oldPronePose=s->saved.prone||e->v.sequence==david_assets::seqCrawl||e->v.sequence==david_assets::seqCrawlIdle||e->v.sequence==david_assets::seqProneAttackRight||e->v.sequence==david_assets::seqProneAttackLeft||e->v.sequence==david_assets::seqProneAttackBoth||e->v.sequence==david_assets::seqProneDie;
 // Keep the named save field for old CremationDavid1 blocks, but discard its
 // former health-triggered state and restore every David with the standing hull.
 s->saved.prone=0;
 s->running=maxModel(e);s->target=nullptr;s->movedAt=-100;s->clearSince=-1;s->cooldown=globals->time+1;s->johnnyGuarding=false;s->johnnyThrowing=false;s->johnnyThrowReleased=false;s->johnnyThrowTarget=nullptr;s->johnnyThrowTargetSerial=0;s->johnnyThrowSide=0;davidSpawn(e,true);
 if(johnnyModel(e))for(int hand:{5,8}){int bit=hand==5?1:2;bool lost=false;for(int ancestor=1;ancestor<16;++ancestor)if((s->saved.severMask&(1<<ancestor))&&david_logic::descendant(hand,ancestor,david_assets::parent))lost=true;davidSetBody(e,hand,lost?davidBodyCount(e,hand)-1:((s->saved.johnnyAxes&bit)?0:3));}
 if(johnnyModel(e)&&s->saved.johnnyGuardRemaining>0){s->johnnyGuarding=true;s->johnnyGuardUntil=globals->time+std::min(1.f,s->saved.johnnyGuardRemaining);if(e->v.sequence!=johnny_assets::seqGuard){e->v.sequence=johnny_assets::seqGuard;e->v.frame=0;e->v.animtime=globals->time;e->v.framerate=slowed(e)?balance::SlowFactor:1.f;}}
 if(alive&&johnnyModel(e)&&e->v.sequence==johnny_assets::seqThrow){e->v.sequence=johnny_assets::seqWalk;e->v.frame=0;e->v.animtime=globals->time;e->v.framerate=slowed(e)?balance::SlowFactor:1.f;if(e->pvPrivateData)reinterpret_cast<void(__thiscall*)(void*)>(ravenholm_abi::address(ravenModule,ravenholm_abi::ResetSequenceInfoRva))(e->pvPrivateData);}
 if(oldPronePose&&alive){e->v.sequence=0;e->v.frame=0;e->v.animtime=globals->time;}
 else if(oldPronePose&&visibleCorpse){e->v.sequence=8;e->v.frame=0;e->v.animtime=globals->time;e->v.framerate=1;}
}
