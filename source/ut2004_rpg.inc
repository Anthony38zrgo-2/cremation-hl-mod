// UT2004-style loaded RPG. Native Ravenholm rockets retain explosion/damage
// handling; custom launch state owns charging, ammunition and straight flight.
static constexpr const char* RpgFire="weapons/ut2004/fire.wav";
static constexpr const char* RpgAlt="weapons/ut2004/altfire.wav";
static constexpr const char* RpgLoad="weapons/ut2004/load.wav";
static constexpr const char* RpgPlunger="weapons/ut2004/plunger.wav";
static constexpr const char* RpgFlight="weapons/ut2004/flight.wav";
static constexpr const char* RpgEquip="weapons/ut2004/equip.wav";
struct RpgCharge {bool active=false,charging=false,waitRelease=false,startPending=false;int loaded=0;float started=0,nextFire=0;};
static std::array<RpgCharge,33> rpgCharges{};
struct RpgMissile {edict_t* entity;int serial;float expires;};
static std::vector<RpgMissile> rpgMissiles;
static int rpgAmmoIndex=-1,rpgClipOffset=-1,rpgSmoke=0;
using RpgTouch=void(__thiscall*)(void*,void*);
static RpgTouch nativeRpgTouch=nullptr;
static bool rpgPlayer(edict_t* p) {
    return valid(p)&&p->v.health>0&&p->v.viewmodel&&!std::strcmp(text(p->v.viewmodel),"models/v_rpg.mdl");
}
static int rpgFindClipOffset() {
    auto base=reinterpret_cast<unsigned char*>(ravenModule);
    auto nt=reinterpret_cast<IMAGE_NT_HEADERS*>(base+reinterpret_cast<IMAGE_DOS_HEADER*>(base)->e_lfanew);
    auto section=IMAGE_FIRST_SECTION(nt);size_t size=nt->OptionalHeader.SizeOfImage;
    for(int j=0;j<nt->FileHeader.NumberOfSections;++j) {
        if(!(section[j].Characteristics&IMAGE_SCN_MEM_WRITE))continue;
        size_t end=std::min(size,size_t(section[j].VirtualAddress+section[j].Misc.VirtualSize));
        for(size_t i=section[j].VirtualAddress;i+16<=end;i+=4) {
            auto f=reinterpret_cast<const uint32_t*>(base+i);
            uintptr_t low=reinterpret_cast<uintptr_t>(base),name=f[1];
            if(f[0]==10 && f[3]==1 && f[2]<4096 && name>=low && name+8<low+size &&
               !std::memcmp(reinterpret_cast<void*>(name),"m_iClip",8))return int(f[2]);
        }
    }
    return -1;
}
static edict_t* rpgInventory(edict_t* p) {
    if(!engine.pfnFindEntityByString)return nullptr;
    edict_t* e=nullptr;
    for(int n=0;n<globals->maxEntities;++n) {
        e=engine.pfnFindEntityByString(e,"classname","weapon_rpg");
        if(!e||engine.pfnIndexOfEdict(e)==0)break;
        if(valid(e)&&e->v.owner==p&&e->pvPrivateData)return e;
    }
    return nullptr;
}
static int* rpgClip(edict_t* p) {
    auto e=rpgInventory(p);if(!e||rpgClipOffset<0)return nullptr;
    int* clip=reinterpret_cast<int*>(static_cast<char*>(e->pvPrivateData)+rpgClipOffset);
    return *clip>=-1&&*clip<=1?clip:nullptr;
}
static int rpgAmmo(edict_t* p) {
    auto clip=rpgClip(p);return clip?std::min(30,std::max(0,*clip)+sharedAmmo(p,rpgAmmoIndex,0)):0;
}
static void rpgHud(edict_t* p) {
    int i=engine.pfnIndexOfEdict(p);auto clip=rpgClip(p);
    if(i<1||i>32||!clip)return;
    if(msgCurWeapon&&rpgPlayer(p)) {
        engine.pfnMessageBegin(MSG_ONE,msgCurWeapon,nullptr,p);
        engine.pfnWriteByte(1);engine.pfnWriteByte(8);
        engine.pfnWriteByte(255);engine.pfnMessageEnd();
    }
    if(msgAmmoX&&rpgAmmoIndex>=0) {
        engine.pfnMessageBegin(MSG_ONE,msgAmmoX,nullptr,p);
        engine.pfnWriteByte(rpgAmmoIndex);engine.pfnWriteByte(sharedAmmo(p,rpgAmmoIndex,0));engine.pfnMessageEnd();
    }
}
// Use the native no-clip marker (-1) and keep the entire pool in the saved
// player ammo array. This disables automatic magazine reloads and saves cleanly.
static int* rpgReserve(edict_t* p) {
    if(!p||!p->pvPrivateData||ammoOffset<0||rpgAmmoIndex<0||rpgAmmoIndex>=32)return nullptr;
    auto reserve=reinterpret_cast<int*>(static_cast<char*>(p->pvPrivateData)+ammoOffset)+rpgAmmoIndex;
    return *reserve>=0&&*reserve<=250?reserve:nullptr;
}
static void rpgNormalize(edict_t* p) {
    int i=engine.pfnIndexOfEdict(p);if(i<1||i>32)return;
    auto clip=rpgClip(p);auto reserve=rpgReserve(p);if(!clip||!reserve)return;
    int total=std::min(30,*reserve+std::max(0,*clip));
    if(rpgCharges[i].startPending) {total=5;rpgCharges[i].startPending=false;}
    bool changed=*clip!=-1||*reserve!=total;
    *clip=-1;*reserve=total;
    if(changed)rpgHud(p);
}
static bool rpgPickup(edict_t* item,edict_t* player) {
    if(!valid(item)||!valid(player)||!(player->v.flags&FL_CLIENT)||player->v.health<=0)return false;
    const char* name=text(item->v.classname);
    bool ammo=name&&!std::strcmp(name,"ammo_rpgclip");
    bool duplicate=name&&!std::strcmp(name,"weapon_rpg")&&!item->v.owner&&rpgInventory(player);
    if(!ammo&&!duplicate)return false;
    rpgNormalize(player);auto reserve=rpgReserve(player);
    if(!reserve||!rpgInventory(player))return false;
    if(*reserve>=30)return true; // Leave a full-pool pickup in the map.
    ++*reserve;
    engine.pfnEmitSound(player,CHAN_ITEM,"items/9mmclip1.wav",1,ATTN_NORM,0,100);
    if(msgItemPickup) {
        engine.pfnMessageBegin(MSG_ONE,msgItemPickup,nullptr,player);
        engine.pfnWriteString(name);engine.pfnMessageEnd();
    }
    rpgHud(player);engine.pfnRemoveEntity(item);return true;
}
static void rpgPrecache() {
    for(auto path:{RpgFire,RpgAlt,RpgLoad,RpgPlunger,RpgFlight,RpgEquip})engine.pfnPrecacheSound(const_cast<char*>(path));
    engine.pfnPrecacheModel(const_cast<char*>("models/rpgrocket.mdl"));
    rpgSmoke=engine.pfnPrecacheModel(const_cast<char*>("sprites/smoke.spr"));
    rpgClipOffset=rpgFindClipOffset();
    nativeRpgTouch=reinterpret_cast<RpgTouch>(GetProcAddress(ravenModule,"?RocketTouch@CRpgRocket@@QAEXPAVCBaseEntity@@@Z"));
    char report[96];std::snprintf(report,sizeof report,"clip_offset=%d native_touch=%d",rpgClipOffset,int(nativeRpgTouch!=nullptr));startupTrace("UT2004Rpg",report);
}
static bool rpgTouch(edict_t* a,edict_t* b) {
    for(auto& missile:rpgMissiles)if(missile.entity==a&&a->serialnumber==missile.serial) {
        if(!valid(b)||b==a->v.owner)return true;
        for(auto& sibling:rpgMissiles)if(sibling.entity==b&&b->serialnumber==sibling.serial&&b->v.owner==a->v.owner)return true;
        // Call the native touch only once. It schedules the native explosion.
        missile.entity=nullptr;
        engine.pfnEmitSound(a,CHAN_VOICE,RpgFlight,0,ATTN_NORM,(1<<5),100);
        if(nativeRpgTouch&&a->pvPrivateData&&b->pvPrivateData)nativeRpgTouch(a->pvPrivateData,b->pvPrivateData);
        return true;
    }
    return false;
}
static bool rpgVolley(edict_t* p,int count,bool secondary) {
    count=std::min(count,rpgAmmo(p));auto clip=rpgClip(p);
    if(count<1||!clip||!nativeRpgTouch)return false;
    Vector aim=p->v.v_angle+p->v.punchangle;
    Vector eye=p->v.origin+p->v.view_ofs;
    std::vector<edict_t*> spawned;
    for(int n=0;n<count;++n) {
        float spread=(n-(count-1)*.5f)*8.f;
        float yaw=(aim.y+spread)*3.14159265359f/180.f,pitch=aim.x*3.14159265359f/180.f;
        Vector direction(std::cos(pitch)*std::cos(yaw),std::cos(pitch)*std::sin(yaw),-std::sin(pitch));
        Vector right(-std::sin(yaw),std::cos(yaw),0);
        Vector origin=eye+direction*20.f+right*((n-(count-1)*.5f)*6.f)-Vector(0,0,4);
        TraceResult trace{};engine.pfnTraceLine(&eye.x,&origin.x,0,p,&trace);
        if(trace.flFraction<1.f)origin=Vector(trace.vecEndPos)-direction*2.f;
        auto e=engine.pfnCreateNamedEntity(engine.pfnAllocString("rpg_rocket"));
        if(!e) {for(auto made:spawned)engine.pfnRemoveEntity(made);return false;}
        e->v.angles=Vector(aim.x,aim.y+spread,0);
        raven.pfnSpawn(e);
        e->v.targetname=engine.pfnAllocString("ut2004_rpg_missile");
        e->v.owner=p;e->v.movetype=MOVETYPE_FLY;e->v.gravity=0;e->v.velocity=direction*1500.f;
        e->v.angles=Vector(-aim.x,aim.y+spread,0);e->v.nextthink=0;e->v.effects|=EF_LIGHT;
        engine.pfnSetOrigin(e,&origin.x);spawned.push_back(e);
    }
    sharedAmmo(p,rpgAmmoIndex,count);
    for(auto e:spawned) {
        rpgMissiles.push_back({e,e->serialnumber,globals->time+10.f});
        engine.pfnEmitSound(e,CHAN_VOICE,RpgFlight,.65f,ATTN_NORM,0,100);
        engine.pfnMessageBegin(MSG_BROADCAST,SvcTemp,nullptr,nullptr);
        engine.pfnWriteByte(TE_BEAMFOLLOW);engine.pfnWriteShort(engine.pfnIndexOfEdict(e));engine.pfnWriteShort(rpgSmoke);
        engine.pfnWriteByte(20);engine.pfnWriteByte(5);
        engine.pfnWriteByte(224);engine.pfnWriteByte(224);engine.pfnWriteByte(255);engine.pfnWriteByte(220);engine.pfnMessageEnd();
    }
    sound(p,secondary&&count>1?RpgAlt:RpgFire);animation(p,3);p->v.punchangle.x-=2.f;
    char report[128];std::snprintf(report,sizeof report,"missiles=%d spacing=8 ammo_remaining=%d secondary=%d",count,rpgAmmo(p),int(secondary));startupTrace("RpgVolley",report);
    return true;
}
static void rpgFrame() {
    for(auto& missile:rpgMissiles)if(missile.entity) {
        auto e=missile.entity;
        if(!valid(e)||e->serialnumber!=missile.serial)missile.entity=nullptr;
        else if(globals->time>=missile.expires) {engine.pfnRemoveEntity(e);missile.entity=nullptr;}
    }
    rpgMissiles.erase(std::remove_if(rpgMissiles.begin(),rpgMissiles.end(),[](const RpgMissile& m){return !m.entity;}),rpgMissiles.end());
}
static void rpgPostThink(edict_t* p,int buttons) {
    int i=engine.pfnIndexOfEdict(p);if(i<1||i>32)return;
    auto& state=rpgCharges[i];bool active=rpgPlayer(p),held=(buttons&Attack2)!=0;
    if(!active) {state.active=false;state.charging=false;state.loaded=0;state.waitRelease=false;return;}
    if(!state.active) {state.nextFire=std::max(state.nextFire,globals->time+.5f);sound(p,RpgEquip);}
    state.active=true;
    if(!held)state.waitRelease=false;
    if(!state.charging&&held&&!state.waitRelease&&globals->time>=state.nextFire&&rpgAmmo(p)>0) {
        state.charging=true;state.started=globals->time;state.loaded=0;
        engine.pfnEmitSound(p,CHAN_ITEM,RpgPlunger,.8f,ATTN_NORM,0,100);rpgHud(p);
    }
    if(state.charging) {
        int loaded=std::min(3,std::min(rpgAmmo(p),int((globals->time-state.started+.0001f)/1.f)));
        if(loaded>state.loaded) {
            state.loaded=loaded;engine.pfnEmitSound(p,CHAN_ITEM,RpgLoad,1,ATTN_NORM,0,100);rpgHud(p);
        }
        bool automatic=state.loaded==3&&globals->time>=state.started+4.5f;
        if(!held||automatic) {
            rpgVolley(p,state.loaded,true);state.charging=false;state.loaded=0;
            state.nextFire=globals->time+.8f;state.waitRelease=held;rpgHud(p);
        }
    } else if(!held&&(buttons&Attack)&&globals->time>=state.nextFire) {
        if(rpgVolley(p,1,false)) {state.nextFire=globals->time+.8f;rpgHud(p);}
    }
}

