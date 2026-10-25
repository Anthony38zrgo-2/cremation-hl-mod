// Scope native gib model selection to the monster currently taking damage/dying.
static bool humanoidRemains(edict_t* e){if(!e||e->free)return false;const char* n=text(e->v.classname);return std::strstr(n,"zombie")||!std::strcmp(n,"monster_human_grunt");}
static bool humanGibContext=false;
static std::vector<std::pair<edict_t*,int>> humanNativeGibs;
struct AnatomicalGib{edict_t* entity;int serial;float expires,born,last;Vector velocity;int bounces=0;bool asleep=false;int bloodDecals=5;float nextBlood=0;};
static std::vector<AnatomicalGib> anatomicalGibs;
static void anatomicalBloodImpact(AnatomicalGib& g,const TraceResult& tr){
 if(g.bloodDecals<=0||globals->time<g.nextBlood||tr.flFraction>=1||engine.pfnCVarGetFloat("violence_hblood")==0)return;
 int entity=tr.pHit?engine.pfnIndexOfEdict(tr.pHit):0;if(entity&&tr.pHit->v.solid!=SOLID_BSP)return;
 static const char* names[]={"{blood1","{blood2","{blood3","{blood4","{blood5","{blood6"};int index=engine.pfnDecalIndex(names[engine.pfnRandomLong(0,5)]);if(index<0)return;
 bool high=index>255;int message=entity?(high?TE_DECALHIGH:TE_DECAL):(high?TE_WORLDDECALHIGH:TE_WORLDDECAL);
 engine.pfnMessageBegin(MSG_BROADCAST,SvcTemp,nullptr,nullptr);engine.pfnWriteByte(message);coords(tr.vecEndPos);engine.pfnWriteByte(high?index-256:index);if(entity)engine.pfnWriteShort(entity);engine.pfnMessageEnd();--g.bloodDecals;g.nextBlood=globals->time+.12f;
}
static void anatomicalRest(AnatomicalGib& g,const Vector& ground){
    auto e=g.entity;
    // Lay elongated pieces on their side, then place the actual mesh on the floor.
    e->v.angles.x=(e->v.body>=2)?90.f:0.f;e->v.angles.z=0;
    float minZ=-3.f;auto h=static_cast<unsigned char*>(engine.pfnGetModelPtr(e));
    if(h){int parts=*reinterpret_cast<int*>(h+204),offset=*reinterpret_cast<int*>(h+208);
        if(parts>0){auto part=h+offset;int count=*reinterpret_cast<int*>(part+64),models=*reinterpret_cast<int*>(part+72);if(e->v.body<count){auto model=h+models+e->v.body*112;int vertices=*reinterpret_cast<int*>(model+80),index=*reinterpret_cast<int*>(model+88);float pitch=e->v.angles.x*3.14159265f/180;minZ=0;
            for(int i=0;i<vertices;++i){auto v=reinterpret_cast<float*>(h+index+i*12);minZ=std::min(minZ,-std::sin(pitch)*v[0]+std::cos(pitch)*v[2]);}}}}
    Vector at=ground+Vector(0,0,-minZ+.3f);engine.pfnSetOrigin(e,&at.x);g.velocity=Vector(0,0,0);e->v.velocity=Vector(0,0,0);e->v.avelocity=Vector(0,0,0);g.asleep=true;
}
static void humanGibSpawn(Vector origin,Vector impulse){
    if(length(impulse)>260)impulse=normal(impulse)*260;
    for(int body=0;body<6;++body){auto e=engine.pfnCreateNamedEntity(engine.pfnAllocString("info_target"));if(!e)continue;raven.pfnSpawn(e);e->v.targetname=engine.pfnAllocString("cremation_anatomical_gib");engine.pfnSetModel(e,"models/hgibs.mdl");e->v.body=body;
        // One simulation owns movement. Decorative pieces cannot collide with one another.
        e->v.movetype=MOVETYPE_NONE;e->v.solid=SOLID_NOT;e->v.nextthink=0;e->v.takedamage=0;e->v.velocity=Vector(0,0,0);
        float phase=body*1.04719755f+engine.pfnRandomFloat(-.12f,.12f);Vector radial(std::cos(phase),std::sin(phase),0);
        Vector at=origin+radial*16+Vector(0,0,6+body*2);TraceResult tr;engine.pfnTraceLine(&origin.x,&at.x,1,nullptr,&tr);if(tr.flFraction<1)at=tr.vecEndPos+tr.vecPlaneNormal*4;engine.pfnSetOrigin(e,&at.x);
        Vector velocity=impulse*.3f+radial*engine.pfnRandomFloat(85,150)+Vector(0,0,engine.pfnRandomFloat(100,180));e->v.angles=Vector(engine.pfnRandomFloat(0,360),engine.pfnRandomFloat(0,360),engine.pfnRandomFloat(0,360));
        anatomicalGibs.push_back({e,e->serialnumber,globals->time+25,globals->time,globals->time,velocity});}
}
static void humanGibFinish(){
    if(humanNativeGibs.empty())return;
    Vector origin(0,0,0),impulse(0,0,0);int count=0;
    for(auto& g:humanNativeGibs)if(valid(g.first)&&g.first->serialnumber==g.second){origin=origin+g.first->v.origin;impulse=impulse+g.first->v.velocity;++count;g.first->v.flags|=FL_KILLME;g.first->v.effects|=EF_NODRAW;g.first->v.solid=SOLID_NOT;}
    humanNativeGibs.clear();if(count)humanGibSpawn(origin*(1.f/count),impulse*(1.f/count));
}
static void anatomicalGibFrame(){
    for(auto it=anatomicalGibs.begin();it!=anatomicalGibs.end();){auto& g=*it;auto e=g.entity;if(!valid(e)||e->serialnumber!=g.serial){it=anatomicalGibs.erase(it);continue;}
        if(globals->time>=g.expires){engine.pfnRemoveEntity(e);it=anatomicalGibs.erase(it);continue;}
        float dt=std::min(.25f,std::max(0.f,globals->time-g.last));g.last=globals->time;
        if(!g.asleep){int steps=std::max(1,int(std::ceil(dt/.01f)));float step=dt/steps;
            for(int i=0;i<steps&&!g.asleep;++i){g.velocity.z-=800*step;Vector from=e->v.origin,to=from+g.velocity*step;TraceResult tr;engine.pfnTraceLine(&from.x,&to.x,1,nullptr,&tr);
                if(tr.fStartSolid||tr.fAllSolid){Vector up=from+Vector(0,0,4);engine.pfnSetOrigin(e,&up.x);g.velocity=Vector(0,0,20);continue;}
                if(tr.flFraction<1){anatomicalBloodImpact(g,tr);float incoming=dot(g.velocity,tr.vecPlaneNormal);Vector at=tr.vecEndPos+tr.vecPlaneNormal*.5f;engine.pfnSetOrigin(e,&at.x);++g.bounces;
                    if(tr.vecPlaneNormal.z>.65f&&(length(g.velocity)<85||g.bounces>=3||globals->time-g.born>2)){anatomicalRest(g,tr.vecEndPos);break;}
                    if(incoming<0)g.velocity=(g.velocity-tr.vecPlaneNormal*incoming)*.55f-tr.vecPlaneNormal*(incoming*.25f);
                    if(globals->time>=e->v.fuser1&&std::fabs(incoming)>35){static const char* clips[]={"debris/flesh1.wav","debris/flesh2.wav","debris/flesh3.wav"};engine.pfnEmitSound(e,CHAN_BODY,clips[e->v.body%3],.5f,ATTN_NORM,0,100);e->v.fuser1=globals->time+.2f;}
                }else engine.pfnSetOrigin(e,&to.x);
                if(!g.asleep)e->v.angles=e->v.angles+Vector(95,130,70)*step;
            }
        }++it;
    }
}

struct HumanGibTable{void** table;void* damage;void* killed;};
static std::vector<HumanGibTable> humanGibTables;
static int __fastcall humanGibDamage(void* object,void*,entvars_t* inflictor,entvars_t* attacker,float amount,int bits){
    auto table=*reinterpret_cast<void***>(object);void* original=nullptr;for(auto& t:humanGibTables)if(t.table==table)original=t.damage;
    bool previous=humanGibContext;humanGibContext=true;int result=original?reinterpret_cast<int(__thiscall*)(void*,entvars_t*,entvars_t*,float,int)>(original)(object,inflictor,attacker,amount,bits):0;humanGibContext=previous;if(!previous)humanGibFinish();return result;
}
static void __fastcall humanGibKilled(void* object,void*,entvars_t* attacker,int gib){
    auto table=*reinterpret_cast<void***>(object);void* original=nullptr;for(auto& t:humanGibTables)if(t.table==table)original=t.killed;
    bool previous=humanGibContext;humanGibContext=true;if(original)reinterpret_cast<void(__thiscall*)(void*,entvars_t*,int)>(original)(object,attacker,gib);humanGibContext=previous;if(!previous)humanGibFinish();
}
static void humanGibInstall(edict_t* e){
    if(!humanoidRemains(e)||!e->pvPrivateData)return;auto table=*reinterpret_cast<void***>(e->pvPrivateData);for(auto& t:humanGibTables)if(t.table==table)return;
    DWORD old;if(!VirtualProtect(table+11,sizeof(void*)*3,PAGE_READWRITE,&old))return;humanGibTables.push_back({table,table[11],table[13]});table[11]=reinterpret_cast<void*>(&humanGibDamage);table[13]=reinterpret_cast<void*>(&humanGibKilled);DWORD unused;VirtualProtect(table+11,sizeof(void*)*3,old,&unused);
}
static void hookGibSetModel(edict_t* e,const char* path){
 bool gib=humanGibContext&&(!std::strcmp(path,"models/agibs.mdl")||!std::strcmp(path,"models/hgibs.mdl"));
 engine.pfnSetModel(e,gib?"models/hgibs.mdl":path);if(gib)humanNativeGibs.push_back({e,e->serialnumber});
}
