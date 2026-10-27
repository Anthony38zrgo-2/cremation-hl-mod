// Necromancer: native zombie AI, one summon per owner, ground-only invocation.
#include "necro_charred.inc"
static constexpr const char* NecroView="models/cremation/v_necromancer.mdl";
static constexpr const char* NecroPlayer="models/cremation/p_necromancer.mdl";
struct NecroState {edict_t* orb=nullptr;int serial=0;Vector velocity{0,0,0};float expires=0,next=0;int buttons=0,sent=-1;bool active=false;};
static std::array<NecroState,33> necroStates{};
struct NecroFire {Vector origin;edict_t* owner;int serial;float expires,tick;std::vector<std::pair<edict_t*,int>> flames;};
static std::vector<NecroFire> necroFires;
struct NecroVtable {void** table;void* original;};
static std::vector<NecroVtable> necroVtables;
static bool necroMinion(edict_t* e){return valid(e)&&!std::strcmp(text(e->v.classname),"monster_minion");}
static int __fastcall necroClassify(void* object,void*) {
    for(int i=1;i<globals->maxEntities;++i){auto e=engine.pfnPEntityOfEntIndex(i);if(necroMinion(e)&&e->pvPrivateData==object)return 11;}
    auto table=*reinterpret_cast<void***>(object);
    for(auto& v:necroVtables)if(v.table==table)return reinterpret_cast<int(__thiscall*)(void*)>(v.original)(object);
    return 7;
}
static void necroInstall(edict_t* e) {
    if(!e->pvPrivateData)return;auto table=*reinterpret_cast<void***>(e->pvPrivateData);
    for(auto& v:necroVtables)if(v.table==table)return;
    DWORD old;if(VirtualProtect(table+ravenholm_abi::slot::Classify,sizeof(void*),PAGE_READWRITE,&old)){
        necroVtables.push_back({table,table[ravenholm_abi::slot::Classify]});table[ravenholm_abi::slot::Classify]=reinterpret_cast<void*>(&necroClassify);DWORD unused;VirtualProtect(table+ravenholm_abi::slot::Classify,sizeof(void*),old,&unused);
    }
}
static bool necromancer(edict_t* p){return valid(p)&&p->v.health>0&&p->v.viewmodel&&(!std::strcmp(text(p->v.viewmodel),"models/v_hgun.mdl")||!std::strcmp(text(p->v.viewmodel),NecroView));}
static edict_t* necroOwned(edict_t* p){for(int i=1;i<globals->maxEntities;++i){auto e=engine.pfnPEntityOfEntIndex(i);if(necroMinion(e)&&e->v.owner==p&&aliveEnemy(e))return e;}return nullptr;}
static int necroFireFrames=1;
static void necroPrecache(){
    for(auto p:{NecroView,NecroPlayer,"models/cremation/w_necromancer.mdl","models/cremation/minion.mdl","sprites/cremation/necro_orb.spr","sprites/cremation/necro_fire.spr"})engine.pfnPrecacheModel(const_cast<char*>(p));
    for(auto& pair:necroCharred)engine.pfnPrecacheModel(const_cast<char*>(pair[1]));
    int size=0;auto data=engine.pfnLoadFileForMe(const_cast<char*>("sprites/cremation/necro_fire.spr"),&size);if(data){if(size>=32)necroFireFrames=*reinterpret_cast<int*>(data+28);engine.pfnFreeFile(data);}
}
static void necroHurt(edict_t* e,edict_t* owner,float amount,bool burn){
    if(!aliveEnemy(e)||necroMinion(e)||!e->pvPrivateData)return;
    auto table=*reinterpret_cast<void***>(e->pvPrivateData);int classification=reinterpret_cast<int(__thiscall*)(void*)>(table[ravenholm_abi::slot::Classify])(e->pvPrivateData);
    if(classification==11||classification==3||classification==1)return;
    const char* oldModel=text(e->v.model);
    reinterpret_cast<int(__thiscall*)(void*,entvars_t*,entvars_t*,float,int)>(table[ravenholm_abi::slot::TakeDamage])(e->pvPrivateData,&owner->v,&owner->v,amount,(burn?(1<<3):(1<<6))|(1<<12));
    if(burn&&valid(e)&&!aliveEnemy(e))for(auto& pair:necroCharred)if(!std::strcmp(oldModel,pair[0])){engine.pfnSetModel(e,pair[1]);break;}
}
static void necroIgnite(edict_t* p,const Vector& origin){
    if(!valid(p)||engine.pfnPointContents(&origin.x)==CONTENTS_WATER)return;
    NecroFire fire{origin,p,p->serialnumber,globals->time+5.f,globals->time,{}};
    for(int j=0;j<13;++j){float angle=j*2.399963f,r=j?45.f+float(j%3)*48.f:0;Vector at=origin+Vector(std::cos(angle)*r,std::sin(angle)*r,48);TraceResult tr;Vector end=at-Vector(0,0,128);engine.pfnTraceLine(&at.x,&end.x,1,p,&tr);if(tr.flFraction>=1||tr.vecPlaneNormal.z<.6f)continue;
        Vector point=tr.vecEndPos+Vector(0,0,12);auto flame=plasmaLayer(p,point,"sprites/cremation/necro_fire.spr",.65f,210);if(flame){flame->v.targetname=engine.pfnAllocString("cremation_necro_fx");fire.flames.push_back({flame,flame->serialnumber});}}
    necroFires.push_back(fire);
}
static void necroDetonate(edict_t* p,edict_t* minion){
    Vector origin=minion->v.origin;minion->v.flags|=FL_KILLME;minion->v.effects|=EF_NODRAW;minion->v.solid=SOLID_NOT;
    engine.pfnMessageBegin(MSG_PVS,SvcTemp,&origin.x,nullptr);engine.pfnWriteByte(TE_EXPLOSION);coords(origin+Vector(0,0,24));engine.pfnWriteShort(flashSprite);engine.pfnWriteByte(24);engine.pfnWriteByte(16);engine.pfnWriteByte(0);engine.pfnMessageEnd();
    necroIgnite(p,origin);
    for(int i=1;i<globals->maxEntities;++i){auto e=engine.pfnPEntityOfEntIndex(i);if(!aliveEnemy(e)||length(center(e)-origin)>180)continue;TraceResult tr;Vector to=center(e),from=origin+Vector(0,0,24);engine.pfnTraceLine(&from.x,&to.x,0,p,&tr);if(tr.pHit==e||tr.flFraction==1)necroHurt(e,p,45,false);}
}
static bool necroSummon(edict_t* owner,const TraceResult& hit){
    for(int j=0;j<9;++j){float angle=j*.785398f;Vector at=hit.vecEndPos+Vector(j?std::cos(angle)*40:0,j?std::sin(angle)*40:0,38);TraceResult tr;Vector end=at-Vector(0,0,64);engine.pfnTraceHull(&at.x,&end.x,0,1,owner,&tr);if(tr.fStartSolid||tr.fAllSolid||tr.flFraction==1||tr.vecPlaneNormal.z<.6f)continue;
        auto e=engine.pfnCreateNamedEntity(engine.pfnAllocString("monster_minion"));if(!e)return false;e->v.origin=tr.vecEndPos-Vector(0,0,35);e->v.angles=Vector(0,owner->v.v_angle.y,0);e->v.owner=owner;raven.pfnSpawn(e);e->v.classname=engine.pfnAllocString("monster_minion");e->v.owner=owner;engine.pfnSetModel(e,"models/cremation/minion.mdl");necroInstall(e);return true;
    }return false;
}
static void necroPostThink(edict_t* p,int buttons){
    int i=engine.pfnIndexOfEdict(p);auto& s=necroStates[i];bool active=necromancer(p);auto minion=necroOwned(p);bool pending=valid(s.orb)&&s.orb->serialnumber==s.serial;int charge=(!pending&&!minion)?1:0;
    if(msgAmmoX&&(s.sent!=charge||(active&&!s.active))){engine.pfnMessageBegin(MSG_ONE,msgAmmoX,nullptr,p);engine.pfnWriteByte(23);engine.pfnWriteByte(charge);engine.pfnMessageEnd();s.sent=charge;}
    if(active){p->v.viewmodel=engine.pfnAllocString(NecroView);p->v.weaponmodel=engine.pfnAllocString(NecroPlayer);
        if(!s.active){animation(p,1);s.next=globals->time+.5f;}
        if(globals->time>=s.next&&(buttons&Attack2)&&!(s.buttons&Attack2)&&minion){necroDetonate(p,minion);animation(p,5);s.next=globals->time+.5f;}
        if(globals->time>=s.next&&(buttons&Attack)&&!(s.buttons&Attack)&&charge){engine.pfnMakeVectors(&p->v.v_angle.x);Vector at=p->v.origin+p->v.view_ofs+globals->v_forward*24+globals->v_right*8;auto orb=plasmaLayer(p,at,"sprites/cremation/necro_orb.spr",.3f,255);if(orb){orb->v.targetname=engine.pfnAllocString("cremation_necro_fx");s.orb=orb;s.serial=orb->serialnumber;s.velocity=globals->v_forward*450+Vector(0,0,120);s.expires=globals->time+6;animation(p,3);s.next=globals->time+.6f;
            engine.pfnMessageBegin(MSG_BROADCAST,SvcTemp,nullptr,nullptr);engine.pfnWriteByte(TE_BEAMFOLLOW);engine.pfnWriteShort(engine.pfnIndexOfEdict(orb));engine.pfnWriteShort(trailSprite);engine.pfnWriteByte(8);engine.pfnWriteByte(6);engine.pfnWriteByte(255);engine.pfnWriteByte(20);engine.pfnWriteByte(12);engine.pfnWriteByte(180);engine.pfnMessageEnd();}}
        if(globals->time>s.next&&p->v.weaponanim!=0)animation(p,0);
    }s.buttons=buttons;s.active=active;
}
static void necroStep(float dt){
    for(int i=1;i<33;++i){auto& s=necroStates[i];if(!valid(s.orb)||s.orb->serialnumber!=s.serial){s.orb=nullptr;continue;}auto p=engine.pfnPEntityOfEntIndex(i);bool done=!valid(p)||p->v.health<=0||globals->time>s.expires;
        if(!done){Vector from=s.orb->v.origin;s.velocity.z-=640*dt;Vector end=from+s.velocity*dt;TraceResult tr;engine.pfnTraceLine(&from.x,&end.x,0,p,&tr);engine.pfnSetOrigin(s.orb,&tr.vecEndPos.x);s.orb->v.frame=std::fmod(globals->time*16,16.f);
            if(tr.flFraction<1){if(tr.vecPlaneNormal.z>=.65f){necroSummon(p,tr);done=true;}else {s.velocity=(s.velocity-tr.vecPlaneNormal*(2*dot(s.velocity,tr.vecPlaneNormal)))*.65f;Vector nudge=tr.vecEndPos+tr.vecPlaneNormal*3;engine.pfnSetOrigin(s.orb,&nudge.x);}}}
        if(done){s.orb->v.flags|=FL_KILLME;s.orb=nullptr;}
    }
}
static void necroFrame(){
    for(auto it=necroFires.begin();it!=necroFires.end();){auto& f=*it;bool expired=globals->time>=f.expires||!valid(f.owner)||f.owner->serialnumber!=f.serial;
        for(auto& pair:f.flames)if(valid(pair.first)&&pair.first->serialnumber==pair.second){if(expired)pair.first->v.flags|=FL_KILLME;else{pair.first->v.frame=std::fmod(globals->time*15,float(std::max(1,necroFireFrames)));pair.first->v.renderamt=210*std::min(1.f,(f.expires-globals->time)*2);}}
        if(expired){it=necroFires.erase(it);continue;}
        if(globals->time>=f.tick){f.tick=globals->time+.25f;for(int i=1;i<globals->maxEntities;++i){auto e=engine.pfnPEntityOfEntIndex(i);if(!aliveEnemy(e)||length2d(center(e)-f.origin)>160||std::fabs(e->v.origin.z-f.origin.z)>80)continue;Vector a=f.origin+Vector(0,0,24),b=center(e);TraceResult tr;engine.pfnTraceLine(&a.x,&b.x,0,f.owner,&tr);if(tr.pHit==e||tr.flFraction==1)necroHurt(e,f.owner,3,true);}}++it;
    }
}

// Native timed hand grenades keep their fuse/physics and use the minion fire zone.
struct IncendiaryGrenade {edict_t* entity;int serial;float originalDamage;};
static std::vector<IncendiaryGrenade> incendiaryGrenades;
static bool incendiaryBeforeThink(edict_t* e){
    if(!valid(e)||std::strcmp(text(e->v.classname),"grenade")||e->v.dmgtime<=0||(e->v.effects&EF_NODRAW))return false;
    for(auto& g:incendiaryGrenades)if(g.entity==e&&g.serial==e->serialnumber)return true;
    incendiaryGrenades.erase(std::remove_if(incendiaryGrenades.begin(),incendiaryGrenades.end(),[](const IncendiaryGrenade& g){return !valid(g.entity)||g.entity->serialnumber!=g.serial;}),incendiaryGrenades.end());
    incendiaryGrenades.push_back({e,e->serialnumber,e->v.dmg});e->v.dmg*=.5f;return true;
}
static void incendiaryAfterThink(edict_t* e,bool tracked,edict_t* owner,int ownerSerial){
    if(tracked&&valid(e)&&(e->v.effects&EF_NODRAW)&&valid(owner)&&owner->serialnumber==ownerSerial)necroIgnite(owner,e->v.origin);
}
