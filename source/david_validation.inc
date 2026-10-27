// Dedicated, opt-in real-engine fixtures. Only tools/check_david_runtime.py
// enables this environment variable in its own hidden HLDS child process.
static int davidProbePhase=-1;
static float davidProbeAt=0;
static float davidProbeSavedLocalized=0;
static bool davidProbeOverridesActive=false;
static edict_t *davidProbeA=nullptr,*davidProbeB=nullptr,*davidProbePlayer=nullptr;
static edict_t* davidProbeLive=nullptr;static Vector davidProbeLiveStart;static bool davidProbeSawRun=false;
static std::array<void*,125> davidProbePlayerTable{};
static edict_t* davidProbeList=nullptr;static int davidProbeCount=0;
static TakeDamageFn davidProbeOriginalPlayerDamage=nullptr;
static float davidProbeScaledDamage=0;
static int __fastcall davidProbeNativePlayerDamage(void* object,void*,entvars_t* inflictor,entvars_t* attacker,float amount,int bits){davidProbeScaledDamage=amount;return davidProbeOriginalPlayerDamage(object,inflictor,attacker,amount,bits);}
static decltype(engine.pfnRandomLong) davidProbeOriginalRandom=nullptr;
static int davidProbeStrikeRoll=0;
static int32 davidProbeAttackRandom(int32 low,int32 high){if(low==0&&high==2){static const int rolls[]={0,1,2};return rolls[davidProbeStrikeRoll++%3];}return davidProbeOriginalRandom(low,high);}
static int __fastcall davidProbePlayerClass(void*,void*){return 2;}
static void davidProbeResult(bool ok,const char* detail){startupTrace(ok?"DavidTest PASS":"DavidTest FAIL",detail);}
static edict_t* davidProbeSpawn(const Vector& at,bool native=false,bool steve=false){
 auto e=engine.pfnCreateNamedEntity(engine.pfnAllocString("monster_zombie"));if(!valid(e))return nullptr;e->v.origin=at;e->v.angles=Vector(0,0,0);
 float enabled=davidEnable.value,chance=steveChance.value;steveChance.value=steve?1.f:0.f;if(native)davidEnable.value=0;hookSpawn(e);davidEnable.value=enabled;steveChance.value=chance;e->v.nextthink=globals->time+100;return e;
}
static void davidProbeApplyTrace(edict_t* e,int node,float amount,int bits=1<<1){
 auto s=davidState(e);if(!s)return;davidPose(*s);auto p=s->pose[node];const float* box=davidPartBounds(e,node,davidBody(e,node));Vector point=p.origin;
 for(int k=0;k<3;++k)point=point+p.axis[k]*((box[k]+box[k+3])*.5f);
 TraceResult tr{};tr.pHit=e;tr.vecEndPos=point;tr.iHitgroup=david_assets::hitgroup[node];tr.flFraction=.5f;s->lastTrace=globals->time;s->lastPoint=point;s->lastNode=node;
 auto pendingEntity=reinterpret_cast<void**>(ravenholm_abi::address(ravenModule,ravenholm_abi::MultiDamageEntityOffset));auto pendingDamage=reinterpret_cast<float*>(ravenholm_abi::address(ravenModule,ravenholm_abi::MultiDamageAmountOffset));auto pendingBits=reinterpret_cast<int*>(ravenholm_abi::address(ravenModule,ravenholm_abi::MultiDamageBitsOffset));
 *pendingEntity=nullptr;*pendingDamage=0;*pendingBits=0;
 auto table=*reinterpret_cast<void***>(e->pvPrivateData);Vector dir(1,0,0);
 reinterpret_cast<void(__thiscall*)(void*,entvars_t*,float,Vector,TraceResult*,int)>(table[ravenholm_abi::slot::TraceAttack])(e->pvPrivateData,&davidProbePlayer->v,amount,dir,&tr,bits);
 float damage=*pendingDamage;int type=*pendingBits;bool routed=*pendingEntity==e->pvPrivateData;*pendingEntity=nullptr;*pendingDamage=0;*pendingBits=0;
 davidProbeResult(routed&&damage>0,"native TraceAttack supplied positive multi-damage with original hitgroup multiplier");
 reinterpret_cast<TakeDamageFn>(table[ravenholm_abi::slot::TakeDamage])(e->pvPrivateData,&davidProbePlayer->v,&davidProbePlayer->v,damage,type);
}
static float davidProbeMovement=0;
static void davidProbeCaptureMove(edict_t*,const float*,float distance,int){davidProbeMovement+=distance;}
static void davidProbeBegin(edict_t* list,int count){
 hotd2DeathBegin(list,count);
 davidProbePhase=-1;char flag[8]{};if(!GetEnvironmentVariableA("CREMATION_DAVID_VALIDATE",flag,sizeof flag)||std::strcmp(flag,"1"))return;
 if(globals->time<1){davidProbeList=list;davidProbeCount=count;davidProbePhase=-2;davidProbeAt=globals->time+1.2f;return;}
 davidProbeSavedLocalized=davidLocalizedDamage.value;davidLocalizedDamage.value=1;davidProbeOverridesActive=true;
 for(int i=1;i<count;++i)if(aliveEnemy(&list[i])){list[i].v.health=0;list[i].v.deadflag=DEAD_DEAD;list[i].v.solid=SOLID_NOT;list[i].v.effects|=EF_NODRAW;list[i].v.nextthink=globals->time+100;}
 Vector at(-160,800,-319.9f);davidProbeA=davidProbeSpawn(at);davidProbePlayer=engine.pfnCreateFakeClient("DavidProbe");
 if(valid(davidProbePlayer)){
  auto ctor=reinterpret_cast<void(*)(entvars_t*)>(GetProcAddress(ravenModule,"player"));if(ctor)ctor(&davidProbePlayer->v);
  char reject[128]{};if(raven.pfnClientConnect)raven.pfnClientConnect(davidProbePlayer,"DavidProbe","127.0.0.1",reject);
  if(raven.pfnClientPutInServer)raven.pfnClientPutInServer(davidProbePlayer);
  Vector position=at+Vector(400,0,36);engine.pfnSetOrigin(davidProbePlayer,&position.x);
  davidProbePlayer->v.flags|=FL_FAKECLIENT|FL_FROZEN;davidProbePlayer->v.movetype=MOVETYPE_NONE;
 }
 if(!valid(davidProbeA)||!valid(davidProbePlayer)){davidProbeResult(false,"fixture allocation");return;}
 auto player=davidProbePlayer;dmcInstallPlayer(player);player->v.health=player->v.max_health=1000;
 davidProbeResult(davidModel(davidProbeA)&&davidState(davidProbeA)&&davidProbeA->v.health>0,"normal zombie receives David model and guarded native hooks");
 // The donor's ordinary health and damage are measured before/after adaptation.
 auto baseline=davidProbeSpawn(at+Vector(0,160,0),true);davidProbeResult(valid(baseline)&&baseline->v.health==davidProbeA->v.health,"normal zombie health unchanged from actual donor Spawn");
 if(valid(baseline)){auto tb=*reinterpret_cast<void***>(baseline->pvPrivateData);auto td=*reinterpret_cast<void***>(davidProbeA->pvPrivateData);float beforeB=baseline->v.health,beforeD=davidProbeA->v.health;
  reinterpret_cast<TakeDamageFn>(tb[11])(baseline->pvPrivateData,&player->v,&player->v,5,1<<1);reinterpret_cast<TakeDamageFn>(td[11])(davidProbeA->pvPrivateData,&player->v,&player->v,5,1<<1);
  davidProbeResult(std::fabs((beforeB-baseline->v.health)-(beforeD-davidProbeA->v.health))<.001f,"bullet resistance/actual health loss unchanged; no duplicate damage");
  Vector originalPosition=player->v.origin;float dealt[2][2]{};
  for(int actor=0;actor<2;++actor){auto zombie=actor?davidProbeA:baseline;Vector position=zombie->v.origin+Vector(42,0,36);engine.pfnSetOrigin(player,&position.x);
   for(int attack=0;attack<2;++attack){player->v.health=1000;player->v.armorvalue=0;player->v.velocity=Vector(0,0,0);if(actor){davidActivity(zombie->pvPrivateData,nullptr,30);}MonsterEvent_t event{};event.event=attack?3:1;auto tv=*reinterpret_cast<void***>(zombie->pvPrivateData);reinterpret_cast<void(__thiscall*)(void*,MonsterEvent_t*)>(tv[57])(zombie->pvPrivateData,&event);dealt[actor][attack]=1000-player->v.health;}
  }
  engine.pfnSetOrigin(player,&originalPosition.x);player->v.health=1000;
  char melee[160];std::snprintf(melee,sizeof melee,"melee baseline=%.2f/%.2f David=%.2f/%.2f",dealt[0][0],dealt[0][1],dealt[1][0],dealt[1][1]);startupTrace("DavidProbe",melee);
  davidProbeResult(dealt[0][0]>0&&dealt[0][1]>0&&std::fabs(dealt[0][0]-dealt[1][0])<.001f&&std::fabs(dealt[0][1]-dealt[1][1])<.001f,"single and double melee preserve actual native damage against a real player");
  bool distinct=true,nativeTiming=true;
  davidProbeOriginalRandom=engine.pfnRandomLong;engine.pfnRandomLong=davidProbeAttackRandom;davidProbeStrikeRoll=0;
  for(int attack=0;attack<3;++attack){davidActivity(davidProbeA->pvPrivateData,nullptr,31);distinct=distinct&&davidProbeA->v.sequence==david_assets::seqAttackRight+attack;
   float nativeRate=*reinterpret_cast<float*>(static_cast<char*>(davidProbeA->pvPrivateData)+ravenholm_abi::private_offset::MonsterFrameRate);nativeTiming=nativeTiming&&std::fabs(nativeRate-256.f/david_assets::duration[davidProbeA->v.sequence])<.01f&&!*reinterpret_cast<int*>(static_cast<char*>(davidProbeA->pvPrivateData)+ravenholm_abi::private_offset::MonsterAnimationEnd);}
  engine.pfnRandomLong=davidProbeOriginalRandom;
  davidProbeResult(distinct&&nativeTiming,"both native melee activities select three distinct strikes and reset their actual native timing/finished state");
  baseline->v.solid=SOLID_NOT;baseline->v.effects|=EF_NODRAW;
  auto steve=davidProbeSpawn(baseline->v.origin,false,true);auto ss=davidState(steve);
  float expectedHealth=davidState(davidProbeA)->saved.initialHealth*1.1f;
  davidProbeResult(ss&&steveModel(steve)&&ss->saved.variant==1&&std::fabs(steve->v.health-expectedHealth)<.001f&&std::fabs(steve->v.max_health-expectedHealth)<.001f,"Steve variant has exactly 110 percent of David spawn health");
  DMCPlayerTable* playerDamageEntry=nullptr;for(auto& entry:dmcPlayerTables)if(entry->base==*reinterpret_cast<void***>(player->pvPrivateData))playerDamageEntry=entry.get();
  davidProbeOriginalPlayerDamage=playerDamageEntry->damage;playerDamageEntry->damage=reinterpret_cast<TakeDamageFn>(&davidProbeNativePlayerDamage);
  float steveDamage[2]{},steveRawDamage[2]{};Vector steveTarget=steve->v.origin+Vector(42,0,36);engine.pfnSetOrigin(player,&steveTarget.x);
  for(int attack=0;attack<2;++attack){player->v.health=1000;player->v.armorvalue=0;davidActivity(steve->pvPrivateData,nullptr,30);MonsterEvent_t event{};event.event=attack?3:1;auto st=*reinterpret_cast<void***>(steve->pvPrivateData);reinterpret_cast<void(__thiscall*)(void*,MonsterEvent_t*)>(st[57])(steve->pvPrivateData,&event);steveDamage[attack]=1000-player->v.health;steveRawDamage[attack]=davidProbeScaledDamage;
   float beforeDuplicate=player->v.health;reinterpret_cast<void(__thiscall*)(void*,MonsterEvent_t*)>(st[57])(steve->pvPrivateData,&event);davidProbeResult(player->v.health==beforeDuplicate,"Steve melee event is consumed once");}
  playerDamageEntry->damage=davidProbeOriginalPlayerDamage;
  char steveDetail[240];std::snprintf(steveDetail,sizeof steveDetail,"Steve raw melee=%.2f/%.2f actual health loss=%.2f/%.2f health=%.2f baseline=%.2f/%.2f",steveRawDamage[0],steveRawDamage[1],steveDamage[0],steveDamage[1],steve->v.health,dealt[1][0],dealt[1][1]);startupTrace("SteveProbe",steveDetail);
  davidProbeResult(std::fabs(steveRawDamage[0]-dealt[1][0]*1.1f)<.001f&&std::fabs(steveRawDamage[1]-dealt[1][1]*1.1f)<.001f&&std::fabs(steveDamage[0]-std::floor(steveRawDamage[0]))<.001f&&std::fabs(steveDamage[1]-std::floor(steveRawDamage[1]))<.001f&&davidMeleeScale(&steve->v)==1.f,"Steve single and two-arm hits deal exactly 110 percent before armor; bonus scope clears");
  int strikeCounts[2][3]{};davidProbeOriginalRandom=engine.pfnRandomLong;engine.pfnRandomLong=davidProbeAttackRandom;davidProbeStrikeRoll=0;
  for(int actor=0;actor<2;++actor){auto zombie=actor?steve:davidProbeA;for(int i=0;i<300;++i){davidActivity(zombie->pvPrivateData,nullptr,(i&1)?31:30);int selected=zombie->v.sequence-david_assets::seqAttackRight;if(selected>=0&&selected<3)++strikeCounts[actor][selected];}}
  engine.pfnRandomLong=davidProbeOriginalRandom;
  davidProbeResult(strikeCounts[0][0]==100&&strikeCounts[0][1]==100&&strikeCounts[0][2]==100&&strikeCounts[1][0]==100&&strikeCounts[1][1]==100&&strikeCounts[1][2]==100,"David and Steve select right, left and two-arm strikes with identical one-third probabilities");
  steve->v.health=expectedHealth*.4f;float hurtHealth=steve->v.health;steve->v.sequence=0;
  char sbuffer[4096]{};char* stokens[1024]{};SAVERESTOREDATA steveSave{};steveSave.pBaseData=sbuffer;steveSave.pCurrentData=sbuffer;steveSave.bufferSize=sizeof sbuffer;steveSave.tokenCount=1024;steveSave.pTokens=stokens;steveSave.time=globals->time;
  davidSave(steve,&steveSave);ss->saved.variant=0;ss->saved.initialHealth=1;steveSave.pCurrentData=sbuffer;steveSave.size=0;davidRestore(steve,&steveSave);
  davidProbeResult(steveModel(steve)&&ss->saved.variant==1&&std::fabs(ss->saved.initialHealth-expectedHealth)<.001f&&steve->v.health==hurtHealth&&!ss->saved.prone,"real save/restore keeps Steve identity, initial health and damage without multiplying health twice or enabling prone");
  engine.pfnSetModel(steve,SteveCharred);davidProbeResult(davidModel(steve)&&steveModel(steve),"charred Steve retains shared David identity");
  engine.pfnRemoveEntity(steve);engine.pfnSetOrigin(player,&originalPosition.x);player->v.health=1000;
  baseline->v.health=0;baseline->v.deadflag=DEAD_DEAD;baseline->v.solid=SOLID_NOT;baseline->v.effects|=EF_NODRAW;
 }
 auto s=davidState(davidProbeA);s->target=player;s->targetSerial=player->serialnumber;s->movedAt=globals->time;s->clearSince=globals->time-.5f;s->entity->v.sequence=1;
 davidNextDecision=0;davidDecision();
 char detail[160];std::snprintf(detail,sizeof detail,"crowded=%d canrun=%d running=%d health=%.1f",int(davidCrowded(*s,320)),int(davidCanRun(*s)),int(s->running),s->entity->v.health);startupTrace("DavidProbe",detail);
 davidProbeResult(s->running,"player with no nearby enemies grants one runner");
 auto rawMove=engine.pfnMoveToOrigin;engine.pfnMoveToOrigin=davidProbeCaptureMove;auto table=*reinterpret_cast<void***>(davidProbeA->pvPrivateData);Vector direction(1,0,0);
 s->running=false;davidProbeA->v.sequence=0;davidProbeA->v.framerate=1;davidProbeMovement=0;reinterpret_cast<void(__thiscall*)(void*,void*,const Vector&,float)>(table[ravenholm_abi::slot::MoveExecute])(davidProbeA->pvPrivateData,player->pvPrivateData,direction,.1f);float walk=davidProbeMovement;
 s->running=true;davidProbeMovement=0;reinterpret_cast<void(__thiscall*)(void*,void*,const Vector&,float)>(table[ravenholm_abi::slot::MoveExecute])(davidProbeA->pvPrivateData,player->pvPrivateData,direction,.1f);float run=davidProbeMovement;engine.pfnMoveToOrigin=rawMove;
 std::snprintf(detail,sizeof detail,"native movement walk=%.3f run=%.3f",walk,run);startupTrace("DavidProbe",detail);davidProbeResult(run>walk*2&&walk>0&&davidProbeA->v.sequence==2,"running changes actual native movement and original run sequence together");
 davidProbeB=davidProbeSpawn(at+Vector(0,64,0));auto sb=davidState(davidProbeB);sb->target=player;sb->targetSerial=player->serialnumber;sb->movedAt=globals->time;sb->clearSince=globals->time-.5f;davidProbeB->v.sequence=1;
 davidNextDecision=0;davidDecision();davidProbeResult(s->running&&!sb->running,"simultaneous eligible Davids retain exactly one reservation per player");
 // Put an enemy (different class label, same zombie faction) beside the player.
 Vector nearbyPoint=player->v.origin+Vector(0,48,0);engine.pfnSetOrigin(davidProbeB,&nearbyPoint.x);sb->running=false;
 davidNextDecision=0;davidDecision();davidProbeResult(!s->running&&davidCrowded(*s,256),"enemy beside player cancels approach sprint");
 // Make this nearby NPC a true ally by classification, not by proximity/name.
 static std::array<void*,125> allyTable;std::copy_n(*reinterpret_cast<void***>(davidProbeB->pvPrivateData),125,allyTable.begin());allyTable[8]=reinterpret_cast<void*>(&davidProbePlayerClass);*reinterpret_cast<void***>(davidProbeB->pvPrivateData)=allyTable.data();
 davidProbeResult(!davidCrowded(*s,256),"nearby ally does not block enemy-density condition");
 auto sentry=engine.pfnCreateNamedEntity(engine.pfnAllocString("info_target"));raven.pfnSpawn(sentry);sentry->v.classname=engine.pfnAllocString("cremation_sentry");sentry->v.flags|=FL_MONSTER;sentry->v.health=100;sentry->v.deadflag=DEAD_NO;sentry->v.owner=player;engine.pfnSetOrigin(sentry,&nearbyPoint.x);
 davidProbeResult(!davidCrowded(*s,256),"scripted allied sentry is excluded without invoking a nonexistent monster vtable slot");engine.pfnRemoveEntity(sentry);
 davidProbeB->v.health=0;davidProbeB->v.deadflag=DEAD_DEAD;davidProbeB->v.solid=SOLID_NOT;davidProbeB->v.effects|=EF_NODRAW;
 // Freeze fixtures in the original waiting pose for precise anatomical traces.
 davidProbeA->v.sequence=0;davidProbeA->v.frame=0;davidProbeA->v.framerate=0;s->poseAt=-100;davidPose(*s);
 auto head=s->pose[2];const float* hb=david_assets::hitbox[2];Vector headCenter=head.origin;for(int k=0;k<3;++k)headCenter=headCenter+head.axis[k]*((hb[k]+hb[k+3])*.5f);
 Vector a=headCenter+Vector(40,0,0),b=headCenter-Vector(40,0,0);TraceResult tr{};engine.pfnTraceLine(&a.x,&b.x,0,player,&tr);davidProbeResult(tr.pHit==davidProbeA&&tr.iHitgroup==1,"real GoldSrc trace identifies animated David head hitbox");
 // Force an arm's regional threshold through actual native TraceAttack + damage.
 s->saved.initialHealth=50;davidProbeA->v.health=200;davidProbeA->v.max_health=200;size_t before=anatomicalGibs.size();davidProbeApplyTrace(davidProbeA,3,60);
 davidProbeResult((s->saved.severMask&(1<<3))&&davidBody(davidProbeA,4)==3&&anatomicalGibs.size()==before+3,"upper arm sever removes descendants and spawns actual posed regional gibs");
 davidProbeResult(david_logic::attackEvent(1,s->saved.severMask)==0&&david_logic::attackEvent(3,s->saved.severMask)==2,"missing arm cannot deal phantom melee damage");
 // Directly through the missing hand must no longer collide with its old box.
 s->poseAt=-100;davidPose(*s);Vector hand=s->pose[5].origin;a=hand+Vector(18,0,0);b=hand-Vector(18,0,0);engine.pfnTraceLine(&a.x,&b.x,0,player,&tr);davidProbeResult(tr.pHit!=davidProbeA||tr.iHitgroup!=5,"removed arm hitboxes no longer obstruct actual traces");
 // Pain poses can cross the removed hand through the still-present torso.
 // Select an unobstructed line through its former box before adding a target.
 bool clearRay=false;Vector behindAt;auto hp=s->pose[5];const float* oldBox=david_assets::hitbox[5];
 for(int corner=0;corner<9&&!clearRay;++corner)for(int axis=0;axis<4&&!clearRay;++axis){
  Vector point=hp.origin;for(int k=0;k<3;++k){float factor=corner==8?.5f:((corner&(1<<k))?.9f:.1f);point=point+hp.axis[k]*(oldBox[k]+(oldBox[k+3]-oldBox[k])*factor);}
  Vector dir=axis<2?Vector(axis? -1.f:1.f,0,0):Vector(0,axis==2?1.f:-1.f,0);a=point+dir*40;b=point-dir*80;engine.pfnTraceLine(&a.x,&b.x,0,player,&tr);
  if(tr.flFraction>=.999f){behindAt=point-dir*40;clearRay=true;}
 }
 auto behind=engine.pfnCreateNamedEntity(engine.pfnAllocString("info_target"));raven.pfnSpawn(behind);Vector small(-2,-2,-2),large(2,2,2);behind->v.solid=SOLID_BBOX;behind->v.movetype=MOVETYPE_NONE;engine.pfnSetSize(behind,&small.x,&large.x);engine.pfnSetOrigin(behind,&behindAt.x);
 engine.pfnTraceLine(&a.x,&b.x,0,player,&tr);davidProbeResult(clearRay&&tr.pHit==behind,"trace through missing hand reaches the real entity behind it");engine.pfnRemoveEntity(behind);
 size_t beforeHand=anatomicalGibs.size();davidProbeApplyTrace(davidProbeA,8,60);davidProbeResult(davidBody(davidProbeA,8)==3&&anatomicalGibs.size()==beforeHand+1,"isolated hand sever hides its mesh and emits exactly one hand fragment");
 // Real donor named-field serialization, with isolated save buffer/token table.
 char buffer[4096]{};char* tokens[1024]{};SAVERESTOREDATA save{};save.pBaseData=buffer;save.pCurrentData=buffer;save.bufferSize=sizeof buffer;save.tokenCount=1024;save.pTokens=tokens;save.time=globals->time;
 DavidSaved original=s->saved,roundtrip{};original.prone=0;raven.pfnSaveWriteFields(&save,"CremationDavid1",&original,davidSaveFields,ARRAYSIZE(davidSaveFields));save.pCurrentData=buffer;save.size=0;raven.pfnSaveReadFields(&save,"CremationDavid1",&roundtrip,davidSaveFields,ARRAYSIZE(davidSaveFields));
 davidProbeResult(roundtrip.version==5&&roundtrip.variant==original.variant&&roundtrip.severMask==original.severMask&&roundtrip.regional[3]==original.regional[3]&&roundtrip.prone==0,"real donor serializer preserves version-5 anatomy and clears the disabled prone state");
 davidProbePhase=0;davidProbeAt=globals->time+.8f;
}
static void davidProbeFrame(){
 hotd2DeathFrame();
 if(davidProbePhase==-2&&globals->time>=davidProbeAt){davidProbeBegin(davidProbeList,davidProbeCount);return;}
 if(davidProbePhase==1&&valid(davidProbeLive)){auto live=davidState(davidProbeLive);if(live&&live->running)davidProbeSawRun=true;}
 if(davidProbePhase<0||globals->time<davidProbeAt)return;auto e=davidProbeA;auto s=davidState(e);if(!s)return;
 if(davidProbePhase==0){
  int fading=0;for(auto& g:anatomicalGibs)if(g.localized&&valid(g.entity)&&g.entity->v.rendermode==kRenderTransTexture&&g.entity->v.renderamt>0&&g.entity->v.renderamt<255)++fading;
  davidProbeResult(fading==4,"local fragments remain physical and fade before their one-second deadline");davidProbePhase=2;davidProbeAt=globals->time+.3f;return;
 }else if(davidProbePhase==2){
  bool remains=false;for(auto& g:anatomicalGibs)if(g.localized&&valid(g.entity))remains=true;davidProbeResult(!remains,"local fragments removed after one second by shared gib simulation");
  davidProbeApplyTrace(e,11,60);davidActivity(e->pvPrivateData,nullptr,4);davidProbeResult(david_logic::legGone(s->saved.severMask)&&e->v.sequence==20&&!s->running,"leg loss switches native locomotion to original HOTD2 crawl");
  e->v.health=200;davidProbeApplyTrace(e,2,60);char detail[200];std::snprintf(detail,sizeof detail,"head result health=%.2f dead=%d body=%d mask=%d regional=%.2f",e->v.health,e->v.deadflag,davidBody(e,2),s->saved.severMask,s->saved.regional[2]);startupTrace("DavidProbe",detail);davidProbeResult(e->v.health<=0&&e->v.deadflag!=DEAD_NO&&davidBody(e,2)==3,"head sever is lethal even with remaining body health");
  auto lethal=davidProbeSpawn(e->v.origin+Vector(0,-120,0));davidProbeApplyTrace(lethal,2,60);auto ls=davidState(lethal);davidProbeResult(ls&&(ls->saved.severMask&(1<<2))&&lethal->v.health<=0&&lethal->v.deadflag!=DEAD_NO,"headshot that also exhausts normal health still decapitates without a second native death");
  auto gibbed=davidProbeSpawn(e->v.origin+Vector(0,120,0));size_t before=anatomicalGibs.size();auto table=*reinterpret_cast<void***>(gibbed->pvPrivateData);reinterpret_cast<TakeDamageFn>(table[ravenholm_abi::slot::TakeDamage])(gibbed->pvPrivateData,&davidProbePlayer->v,&davidProbePlayer->v,500,1<<6);
  int general=0;for(size_t i=before;i<anatomicalGibs.size();++i)if(!anatomicalGibs[i].localized)++general;
  davidProbeResult((gibbed->v.effects&EF_NODRAW)&&general==6,"extreme damage preserves one ordinary six-piece humanoid gibbing");
  davidProbeLiveStart=Vector(-160,800,-319.9f);davidProbeOriginalRandom=engine.pfnRandomLong;engine.pfnRandomLong=davidProbeAttackRandom;davidProbeStrikeRoll=0;davidProbeLive=davidProbeSpawn(davidProbeLiveStart);davidProbeLive->v.nextthink=globals->time+.1f;davidProbeSawRun=false;davidProbePhase=1;davidProbeAt=globals->time+11;
 }else if(davidProbePhase==1){
  engine.pfnRandomLong=davidProbeOriginalRandom;auto live=davidState(davidProbeLive);float moved=valid(davidProbeLive)?length2d(davidProbeLive->v.origin-davidProbeLiveStart):0;char detail[200];std::snprintf(detail,sizeof detail,"native chase moved=%.2f everRun=%d target=%d seq=%d",moved,int(davidProbeSawRun),live&&valid(live->target)?engine.pfnIndexOfEdict(live->target):-1,valid(davidProbeLive)?davidProbeLive->v.sequence:-1);startupTrace("DavidProbe",detail);
  davidProbeResult(moved>30&&davidProbeSawRun&&live&&live->target==davidProbePlayer,"unmodified native Think acquires the real player, navigates and uses contextual run");
  char strikes[220];std::snprintf(strikes,sizeof strikes,"natural David strikes=%d/%d/%d events=%d requests=%d/%d",live->attackSelections[0],live->attackSelections[1],live->attackSelections[2],live->meleeEvents,live->attackRequests[0],live->attackRequests[1]);startupTrace("DavidProbe",strikes);
  davidProbeResult(live->attackSelections[0]>0&&live->attackSelections[1]>0&&live->attackSelections[2]>0&&live->meleeEvents>=3,"native chase AI actually plays all three David strikes and dispatches timed contacts");
  Vector closeAt=davidProbeLive->v.origin;engine.pfnRemoveEntity(davidProbeLive);davidProbeLive=davidProbeSpawn(closeAt,false,true);davidProbeLive->v.nextthink=globals->time+.1f;
  Vector target=closeAt+Vector(42,0,36);engine.pfnSetOrigin(davidProbePlayer,&target.x);davidProbePlayer->v.health=1000;davidProbeStrikeRoll=0;davidProbeOriginalRandom=engine.pfnRandomLong;engine.pfnRandomLong=davidProbeAttackRandom;
  davidProbePhase=3;davidProbeAt=globals->time+8;
 }else if(davidProbePhase==3){
  auto live=davidState(davidProbeLive);engine.pfnRandomLong=davidProbeOriginalRandom;
  char strikes[220];std::snprintf(strikes,sizeof strikes,"natural Steve strikes=%d/%d/%d events=%d requests=%d/%d health=%.2f",live->attackSelections[0],live->attackSelections[1],live->attackSelections[2],live->meleeEvents,live->attackRequests[0],live->attackRequests[1],davidProbePlayer->v.health);startupTrace("SteveProbe",strikes);
  davidProbeResult(live->attackSelections[0]>0&&live->attackSelections[1]>0&&live->attackSelections[2]>0&&live->meleeEvents>=3&&davidProbePlayer->v.health<1000,"native Steve AI plays each strike with controlled selection and real timed melee damage");
  startupTrace("DavidTest COMPLETE");davidProbePhase=-1;if(davidProbeOverridesActive){davidLocalizedDamage.value=davidProbeSavedLocalized;davidProbeOverridesActive=false;}
 }
}
