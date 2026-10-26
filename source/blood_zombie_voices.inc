#include "blood_zombie_assets.h"
struct ZombieVoiceState{int serial=-1,last=-1;bool dead=false;float busy=0,step=0;};
static std::vector<ZombieVoiceState> zombieVoices;
static bool normalZombie(edict_t* e){return valid(e)&&e->v.classname&&!std::strcmp(text(e->v.classname),"monster_zombie");}
static void zombieVoicePlay(edict_t* e,ZombieVoiceEvent event){
 if(!normalZombie(e))return;int index=engine.pfnIndexOfEdict(e);if(index<1||index>=globals->maxEntities)return;
 if(zombieVoices.size()!=globals->maxEntities)zombieVoices.resize(globals->maxEntities);
 auto& s=zombieVoices[index];if(s.serial!=e->serialnumber){s=ZombieVoiceState{};s.serial=e->serialnumber;}
 // The removed butcher puke was the only Attack clip. Use the retained axe swipe.
 if(event==ZombieVoiceEvent::Attack)event=ZombieVoiceEvent::Swipe;
 bool death=event==ZombieVoiceEvent::Death;bool effect=event==ZombieVoiceEvent::Swipe||event==ZombieVoiceEvent::Step;
 if(s.dead||(!death&&!effect&&globals->time<s.busy))return;
 constexpr int clipCount=sizeof(zombieVoiceClips)/sizeof(zombieVoiceClips[0]);
 int choices[clipCount],count=0;for(int i=0;i<clipCount;++i)if(zombieVoiceClips[i].event==event&&i!=s.last)choices[count++]=i;
 if(!count)for(int i=0;i<clipCount;++i)if(zombieVoiceClips[i].event==event)choices[count++]=i;
 if(!count)return;int selected=choices[engine.pfnRandomLong(0,count-1)];auto& clip=zombieVoiceClips[selected];s.last=selected;
 if(!effect)s.busy=globals->time+clip.duration+.15f;if(death)s.dead=true;
 engine.pfnEmitSound(e,effect?CHAN_BODY:CHAN_VOICE,clip.path,event==ZombieVoiceEvent::Step?.35f:1.f,ATTN_NORM,0,100);
 if(GetEnvironmentVariableA("CREMATION_AUDIO_VALIDATE",nullptr,0))startupTrace("ZombieVoice",clip.path);
}
static bool zombieVoiceSound(edict_t* e,const char* sample,int flags){
 if(!normalZombie(e)||!sample||flags&(1<<5))return false;
 ZombieVoiceEvent event;
 if(std::strstr(sample,"zo_idle"))event=ZombieVoiceEvent::Idle;
 else if(std::strstr(sample,"zo_alert"))event=ZombieVoiceEvent::Alert;
 else if(std::strstr(sample,"zo_pain"))event=ZombieVoiceEvent::Pain;
 else if(std::strstr(sample,"zo_attack"))event=ZombieVoiceEvent::Attack;
 else if(std::strstr(sample,"claw_"))event=ZombieVoiceEvent::Swipe;
 else return false;
 zombieVoicePlay(e,event);return true;
}
static void zombieVoicePrecache(){zombieVoices.clear();for(auto& clip:zombieVoiceClips)engine.pfnPrecacheSound(const_cast<char*>(clip.path));char count[16];std::snprintf(count,sizeof(count),"%u",unsigned(sizeof(zombieVoiceClips)/sizeof(zombieVoiceClips[0])));startupTrace("ZombieVoicePrecache",count);}
static void zombieVoiceFrame(){
 static float next=0;if(globals->time<next&&next-globals->time<1)return;next=globals->time+.2f;
 for(int i=1;i<globals->maxEntities;++i){auto e=engine.pfnPEntityOfEntIndex(i);if(!normalZombie(e)||!aliveEnemy(e))continue;
  bool fresh=i>=int(zombieVoices.size())||zombieVoices[i].serial!=e->serialnumber;
  if(fresh)zombieVoicePlay(e,ZombieVoiceEvent::Rise);
  auto& s=zombieVoices[i];if((e->v.flags&FL_ONGROUND)&&length2d(e->v.velocity)>30&&globals->time>s.step){s.step=globals->time+.75f;zombieVoicePlay(e,ZombieVoiceEvent::Step);}
 }
}
