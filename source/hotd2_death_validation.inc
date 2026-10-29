// Opt-in fixtures in an isolated HLDS child; ordinary games never enable these.
static float hotd2DeathProbeAt=0;
static int hotd2DeathProbePhase=-1;
static edict_t* hotd2DeathActors[6]{};
static bool hotd2DeathProbeFailed=false;
static bool hotd2DeathSeen[6]{};
static float hotd2DeathFirstFrame[6]{};
static int hotd2DeathLastSequence[6]{};
static float hotd2DeathLastFrame[6]{};
static bool hotd2DeathContinuityFailed[6]{};
static void hotd2DeathResult(bool ok,const char* detail){
 if(!ok)hotd2DeathProbeFailed=true;
 startupTrace(ok?"Hotd2Death PASS":"Hotd2Death FAIL",detail);
}
static bool hotd2DeathSequence(edict_t* e){
 return johnnyModel(e)?e->v.sequence>=9&&e->v.sequence<=13:e->v.sequence>=8&&e->v.sequence<=12;
}
static void hotd2DeathBegin(edict_t* list,int count){
 char flag[8]{};if(!GetEnvironmentVariableA("CREMATION_DAVID_VALIDATE",flag,sizeof flag)||std::strcmp(flag,"2"))return;
 char run[64]{};GetEnvironmentVariableA("CREMATION_DEATH_RUN",run,sizeof run);startupTrace("Hotd2Death BEGIN",run);
 hotd2DeathProbePhase=0;hotd2DeathProbeAt=globals->time+1.2f;
 for(int i=1;i<count;++i)if(aliveEnemy(&list[i])){list[i].v.health=0;list[i].v.deadflag=DEAD_DEAD;list[i].v.solid=SOLID_NOT;list[i].v.effects|=EF_NODRAW;list[i].v.nextthink=globals->time+100;}
 auto player=engine.pfnCreateFakeClient("DeathProbe");
 if(valid(player)){auto ctor=reinterpret_cast<void(*)(entvars_t*)>(GetProcAddress(ravenModule,"player"));if(ctor)ctor(&player->v);
  char reject[128]{};raven.pfnClientConnect(player,"DeathProbe","127.0.0.1",reject);raven.pfnClientPutInServer(player);player->v.flags|=FL_NOTARGET|FL_FROZEN;}
 for(int i=0;i<6;++i){
  auto e=engine.pfnCreateNamedEntity(engine.pfnAllocString("monster_zombie"));hotd2DeathActors[i]=e;if(!valid(e)){hotd2DeathResult(false,"fixture allocation");continue;}
  e->v.origin=Vector(-160+(i%3)*96,800+(i/3)*96,-319.9f);
  e->v.targetname=engine.pfnAllocString(i>=2?"mansion_enemy_johnny_deathprobe":"mansion_enemy_fixed_deathprobe");
  float chance=steveChance.value;steveChance.value=i==1?1.f:0.f;hookSpawn(e);steveChance.value=chance;
  e->v.nextthink=globals->time+100;e->v.framerate=1;
 }
}
static void hotd2DeathFrame(){
 if(hotd2DeathProbePhase>=1){for(int i=0;i<6;++i){auto e=hotd2DeathActors[i];if(!valid(e))continue;
  if(!hotd2DeathContinuityFailed[i]&&(e->v.sequence!=hotd2DeathLastSequence[i]||e->v.frame+.01f<hotd2DeathLastFrame[i])){char msg[150];std::snprintf(msg,sizeof msg,"actor=%d death continuity changed sequence %d->%d or rewound frame %.2f->%.2f",i,hotd2DeathLastSequence[i],e->v.sequence,hotd2DeathLastFrame[i],e->v.frame);hotd2DeathResult(false,msg);hotd2DeathContinuityFailed[i]=true;}
  hotd2DeathLastSequence[i]=e->v.sequence;hotd2DeathLastFrame[i]=e->v.frame;
 }}
 if(hotd2DeathProbePhase<0||globals->time<hotd2DeathProbeAt)return;
 if(hotd2DeathProbePhase==0){
  for(int i=0;i<6;++i){auto e=hotd2DeathActors[i];if(!valid(e)||!e->pvPrivateData)continue;auto s=davidState(e);if(!s)continue;
   s->saved.johnnyDefended=1;
   if(i==2||i==4){s->johnnyThrowing=true;s->johnnyThrowUntil=globals->time+10;s->johnnyThrowSide=1;e->v.sequence=johnny_assets::seqThrow;}
   else if(i==3){s->johnnyGuarding=true;s->johnnyGuardUntil=globals->time+10;s->saved.johnnyGuardRemaining=10;e->v.sequence=johnny_assets::seqGuard;}
   else e->v.sequence=i==1?david_assets::seqAttackBoth:1;
   e->v.frame=0;
   if(i==4){davidActivity(e->pvPrivateData,nullptr,66);hotd2DeathResult(e->v.sequence==13&&!s->johnnyThrowing,"headshot death selects its final clip immediately and preempts a live throw lock");}
   if(i==5)e->v.framerate=0;
   auto world=engine.pfnPEntityOfEntIndex(0);
   if(i==3){e->v.health=0;davidKilled(e->pvPrivateData,nullptr,&world->v,0);}
   else davidDamage(e->pvPrivateData,nullptr,&world->v,&world->v,10000,1<<12);
   hotd2DeathResult(!s->johnnyThrowing&&!s->johnnyGuarding&&s->saved.johnnyGuardRemaining==0,"Killed cancels attack/guard state before native death scheduling");
   hotd2DeathLastSequence[i]=e->v.sequence;hotd2DeathLastFrame[i]=e->v.frame;
   e->v.nextthink=globals->time+.05f;
  }
  hotd2DeathProbePhase=1;hotd2DeathProbeAt=globals->time+.3f;return;
 }
 if(hotd2DeathProbePhase==1){
  for(int i=0;i<6;++i){auto e=hotd2DeathActors[i];bool ok=valid(e)&&hotd2DeathSequence(e)&&!(e->v.effects&EF_NODRAW)&&e->v.framerate>0;
   hotd2DeathSeen[i]=ok;hotd2DeathFirstFrame[i]=valid(e)?e->v.frame:0;char msg[150];std::snprintf(msg,sizeof msg,"actor=%d death selected=%d sequence=%d frame=%.2f rate=%.2f",i,int(ok),valid(e)?e->v.sequence:-1,valid(e)?e->v.frame:0,valid(e)?e->v.framerate:0);hotd2DeathResult(ok,msg);
   if(valid(e)){int sequence=e->v.sequence;float frame=e->v.frame,animtime=e->v.animtime;auto table=*reinterpret_cast<void***>(e->pvPrivateData);
    int nativeDeath=reinterpret_cast<int(__thiscall*)(void*)>(table[ravenholm_abi::slot::GetDeathActivity])(e->pvPrivateData);
    davidActivity(e->pvPrivateData,nullptr,nativeDeath);davidActivity(e->pvPrivateData,nullptr,nativeDeath==37?38:37);
    std::snprintf(msg,sizeof msg,"actor=%d repeated/native death requests preserve sequence=%d and frame=%.2f",i,sequence,frame);hotd2DeathResult(e->v.sequence==sequence&&e->v.frame==frame&&e->v.animtime==animtime,msg);
    if(i==2){float health=e->v.health;int deadflag=e->v.deadflag;bool invaded=mansionInvaded;auto raider=mansionRaiders[0];auto attack=mansionRaidAttacks[0];
     e->v.health=100;e->v.deadflag=DEAD_NO;mansionInvaded=true;mansionRaiders[0]={e,e->serialnumber};mansionRaidAttacks[0].active=true;
     bool moved=mansionEventMove(e,.05f);mansionMove(e,.05f);bool melee=mansionRaidMelee(e,e,0,.05f);
     hotd2DeathResult(!moved&&!melee&&!mansionRaidAttacks[0].active&&e->v.sequence==sequence&&e->v.frame==frame&&e->v.animtime==animtime,"dying Johnny cannot be animated or moved by scientist attacker logic");
     e->v.health=health;e->v.deadflag=deadflag;mansionInvaded=invaded;mansionRaiders[0]=raider;mansionRaidAttacks[0]=attack;
    }
   }
  }
  hotd2DeathProbePhase=2;hotd2DeathProbeAt=globals->time+.5f;return;
 }
 if(hotd2DeathProbePhase==2){
  for(int i=0;i<6;++i){auto e=hotd2DeathActors[i];bool ok=hotd2DeathSeen[i]&&valid(e)&&hotd2DeathSequence(e)&&e->v.frame>hotd2DeathFirstFrame[i];char msg[140];std::snprintf(msg,sizeof msg,"actor=%d real native death animation progresses %.2f -> %.2f",i,hotd2DeathFirstFrame[i],valid(e)?e->v.frame:0);hotd2DeathResult(ok,msg);}
  hotd2DeathProbePhase=3;hotd2DeathProbeAt=globals->time+3;return;
 }
 for(int i=0;i<6;++i){auto e=hotd2DeathActors[i];bool ok=valid(e)&&hotd2DeathSequence(e)&&e->v.frame>=254;
  if(valid(e)){auto s=davidState(e);float frame=e->v.frame;int sequence=e->v.sequence,axes=s->saved.johnnyAxes;davidActivity(e->pvPrivateData,nullptr,3);MonsterEvent_t event{};event.event=20;davidEvent(e->pvPrivateData,nullptr,&event);
   ok=ok&&e->v.frame==frame&&e->v.sequence==sequence&&s->saved.johnnyAxes==axes&&!johnnyBeginGuard(*s);}
  char msg[150];std::snprintf(msg,sizeof msg,"actor=%d death completes and corpse cannot walk, guard or throw; final frame=%.2f",i,valid(e)?e->v.frame:0);hotd2DeathResult(ok,msg);
  if(valid(e)){Vector head,hips,angles;engine.pfnGetBonePosition(e,2,&head.x,&angles.x);engine.pfnGetBonePosition(e,9,&hips.x,&angles.x);
   std::snprintf(msg,sizeof msg,"actor=%d corpse physically lies down: head=%.2f hips=%.2f above origin",i,head.z-e->v.origin.z,hips.z-e->v.origin.z);hotd2DeathResult(head.z-e->v.origin.z<30&&hips.z-e->v.origin.z<25,msg);
   if(i==2){int sequence=e->v.sequence;float frame=e->v.frame;
    for(int death=9;death<=13;++death){e->v.sequence=death;e->v.frame=255;engine.pfnGetBonePosition(e,2,&head.x,&angles.x);engine.pfnGetBonePosition(e,9,&hips.x,&angles.x);
     std::snprintf(msg,sizeof msg,"Johnny death sequence=%d final native pose head=%.2f hips=%.2f",death,head.z-e->v.origin.z,hips.z-e->v.origin.z);hotd2DeathResult(head.z-e->v.origin.z<30&&hips.z-e->v.origin.z<25,msg);
    }e->v.sequence=sequence;e->v.frame=frame;
   }
  }
 }
 startupTrace(hotd2DeathProbeFailed?"Hotd2Death INCOMPLETE":"Hotd2Death COMPLETE","David, Steve, Johnny idle/throw/guard and zero-rate deaths");hotd2DeathProbePhase=-1;
}
