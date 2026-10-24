// TFC single shotgun and engineer level-one sentry adaptation for Ravenholm.
// Included inside bounce's private namespace to reuse tested trace/damage helpers.
struct TfcState { int clip=8; float nextFire=0,nextReload=0; bool reloading=false,active=false,shellPhase=false; };
std::array<TfcState,33> tfcPlayers{};
struct EngineerState {
    int metal=200; edict_t* gun=nullptr; int serial=0; edict_t* base=nullptr; int baseSerial=0;
    edict_t* owner=nullptr; int ownerSerial=0; float completeAt=0,nextShot=0; int ammo=100; bool froze=false;
};
std::array<EngineerState,33> engineers{};
bool isTfcActive(edict_t* p) {return valid(p) && p->v.viewmodel && !std::strcmp(text(p->v.viewmodel),"models/v_shotgun.mdl");}
void engineerNotice(edict_t* p,const char* text) {if(eng->pfnClientPrintf)eng->pfnClientPrintf(p,print_center,text);}
void tfcHud(edict_t* p,TfcState& s) {
    if(!msgCurWeapon||!eng->pfnMessageBegin)return;
    eng->pfnMessageBegin(MSG_ONE,msgCurWeapon,nullptr,p);eng->pfnWriteByte(1);eng->pfnWriteByte(7);eng->pfnWriteByte(s.clip);eng->pfnMessageEnd();
    if(host.shells)sendAmmo(p,shellAmmoIndex,host.shells(p,0));
}
void tfcReload(edict_t* p,TfcState& s) {
    if(s.reloading||globals->time<s.nextFire||s.clip>=8||!host.shells||host.shells(p,0)<=0)return;
    s.reloading=true;s.shellPhase=false;s.nextReload=globals->time+.1f;s.nextFire=std::max(s.nextFire,s.nextReload);host.animation(p,5);
}
void tfcPostThinkImpl(edict_t* p,int buttons) {
    int i=playerIndex(p);if(i<0)return;auto& s=tfcPlayers[i];
    bool active=isTfcActive(p);
    if(!active){s.active=false;s.reloading=false;return;}
    if(!s.active){s.active=true;s.nextFire=globals->time+.5f;host.animation(p,6);tfcHud(p,s);}
    bool fire=(buttons&ButtonAttack)!=0;
    if(s.reloading && fire && s.clip>0)s.reloading=false;
    for(int step=0;step<16 && s.reloading && globals->time>=s.nextReload;++step) {
        if(!s.shellPhase){s.shellPhase=true;s.nextReload+=.25f;host.animation(p,3);continue;}
        if(s.clip<8 && host.shells && host.shells(p,0)>0){host.shells(p,1);++s.clip;tfcHud(p,s);}
        if(s.clip>=8||!host.shells||host.shells(p,0)<=0)s.reloading=false;
        else {s.nextReload+=.25f;host.animation(p,3);}
    }
    if(buttons&(1<<13))tfcReload(p,s);
    if(!fire||globals->time<s.nextFire||p->v.health<=0)return;
    if(s.clip<=0){tfcReload(p,s);return;}
    if(s.reloading)return;
    Vector forward,right,up;angleVectors(p->v.v_angle+p->v.punchangle,forward,right,up);
    Vector eye=p->v.origin+p->v.view_ofs;
    struct Damage {edict_t* entity;float amount;};std::vector<Damage> damage;
    unsigned seed=players[i].seed;
    // Local tfc.dll single shotgun: six 4-damage pellets, 0.04 X/Y cone,
    // 2048-unit traces, eight-shell clip, 0.5 s primary interval. No alt attack.
    for(int n=0;n<6;++n) {
        float x=sharedRandomFloat(seed+n*4,-.5f,.5f)+sharedRandomFloat(seed+n*4+1,-.5f,.5f);
        float y=sharedRandomFloat(seed+n*4+2,-.5f,.5f)+sharedRandomFloat(seed+n*4+3,-.5f,.5f);
        Vector end=eye+normal(forward+right*(x*.04f)+up*(y*.04f))*2048;
        TraceResult tr{};eng->pfnTraceLine(&eye.x,&end.x,0,p,&tr);
        if(valid(tr.pHit)&&tr.pHit->v.takedamage) {
            auto it=std::find_if(damage.begin(),damage.end(),[&](const Damage& d){return d.entity==tr.pHit;});
            if(it==damage.end())damage.push_back({tr.pHit,4});else it->amount+=4;
        }
    }
    for(auto& d:damage)takeDamage(d.entity,p,p,d.amount,DamageBullet);
    --s.clip;s.nextFire=globals->time+.5f;host.animation(p,1);host.sound(p,"cremation/tfc_shotgun.wav");tfcHud(p,s);
    p->v.effects|=EF_MUZZLEFLASH;
}
void unfreezeEngineer(EngineerState& s) {
    if(s.froze && valid(s.owner)&&s.owner->serialnumber==s.ownerSerial)s.owner->v.flags&=~FL_FROZEN;
    s.froze=false;
}
void removeSentry(EngineerState& s) {
    unfreezeEngineer(s);destroy(s.gun,s.serial);destroy(s.base,s.baseSerial);
    s.completeAt=0;s.owner=nullptr;
}
bool isSentryImpl(edict_t* e) {return valid(e)&&e->v.classname && !std::strcmp(text(e->v.classname),"cremation_sentry");}
bool engineerBuildingImpl(edict_t* p) {
    int i=playerIndex(p);return i>=0&&valid(engineers[i].gun)&&engineers[i].completeAt>globals->time;
}
void engineerFrameImpl(float dt) {
    for(auto& s:engineers) {
        if(!s.gun)continue;
        if(!valid(s.gun)||s.gun->serialnumber!=s.serial||s.gun->v.health<=0 ||
           !valid(s.owner)||s.owner->serialnumber!=s.ownerSerial) {removeSentry(s);continue;}
        // Native sentry AI is bypassed; Ravenholm still owns its health/death ABI.
        s.gun->v.nextthink=globals->time+.1f;
        if(s.completeAt>globals->time) {
            if(s.owner->v.health<=0){removeSentry(s);continue;}
            continue;
        }
        if(s.completeAt) {s.completeAt=0;unfreezeEngineer(s);engineerNotice(s.owner,"Sentry lista.\n");}
        Vector origin=s.gun->v.origin+Vector(0,0,32);edict_t* enemy=nullptr;float nearest=1024;
        for(int i=globals->maxClients+1;i<globals->maxEntities;++i) {
            auto e=eng->pfnPEntityOfEntIndex(i);if(!aliveEnemy(e)||isSentryImpl(e))continue;
            float distance=length(center(e)-origin);if(distance>=nearest)continue;
            TraceResult tr{};Vector to=center(e);eng->pfnTraceLine(&origin.x,&to.x,0,s.gun,&tr);
            if(tr.flFraction>=1.f||tr.pHit==e){nearest=distance;enemy=e;}
        }
        if(!enemy || s.ammo<=0) {s.gun->v.sequence=0;continue;}
        Vector to=center(enemy);float targetYaw=vecToYaw(to-origin),delta=targetYaw-s.gun->v.angles.y;
        while(delta>180)delta-=360;while(delta< -180)delta+=360;
        float turn=std::max(-180.f*dt,std::min(180.f*dt,delta));s.gun->v.angles.y+=turn;
        if(std::fabs(delta)>12 || globals->time<s.nextShot)continue;
        TraceResult shot{};eng->pfnTraceLine(&origin.x,&to.x,0,s.gun,&shot);
        if(shot.pHit!=enemy && shot.flFraction<1.f)continue;
        takeDamage(enemy,s.gun,s.owner,16,DamageBullet);
        --s.ammo;s.nextShot=globals->time+.2f;s.gun->v.sequence=1;s.gun->v.frame=0;s.gun->v.animtime=globals->time;
        host.sound(s.gun,"cremation/tfc_sentry_fire.wav");
    }
}
bool engineerCommandImpl(edict_t* p,const char* command) {
    bool build=!std::strcmp(command,"cremation_build")||!std::strcmp(command,"build");
    bool dismantle=!std::strcmp(command,"cremation_dismantle");
    bool repair=!std::strcmp(command,"cremation_repair");
    if(!build&&!dismantle&&!repair)return false;
    int i=playerIndex(p);if(i<0||p->v.health<=0)return true;auto& s=engineers[i];
    if(dismantle||repair) {
        if(!valid(s.gun)||length(p->v.origin-s.gun->v.origin)>128){engineerNotice(p,"Acercate a tu sentry.\n");return true;}
        if(dismantle){removeSentry(s);s.metal=std::min(200,s.metal+65);engineerNotice(p,"Sentry desmontada: +65 metal.\n");return true;}
        int need=int(std::ceil((150.f-s.gun->v.health)/5.f));int cost=std::max(0,std::min(s.metal,need));
        s.gun->v.health=std::min(150.f,s.gun->v.health+cost*5);s.metal-=cost;
        if(host.shells){int refill=std::min(100-s.ammo,host.shells(p,0));if(refill>0){host.shells(p,refill);s.ammo+=refill;}}
        char text[100];std::snprintf(text,sizeof text,"Sentry: %.0f HP, %d balas. Metal: %d\n",s.gun->v.health,s.ammo,s.metal);engineerNotice(p,text);return true;
    }
    if(valid(s.gun)){engineerNotice(p,"Ya tienes una sentry. N: desmontar; J: reparar.\n");return true;}
    if(s.metal<130){engineerNotice(p,"Necesitas 130 de metal. Recoge las baterias de metal.\n");return true;}
    Vector f,r,u;angleVectors(Vector(0,p->v.v_angle.y,0),f,r,u);
    Vector eye=p->v.origin+p->v.view_ofs,desired=eye+f*80;
    TraceResult tr{};eng->pfnTraceLine(&eye.x,&desired.x,0,p,&tr);
    if(tr.flFraction<1.f){engineerNotice(p,"No hay espacio para construir.\n");return true;}
    Vector down=desired-Vector(0,0,192);eng->pfnTraceLine(&desired.x,&down.x,0,p,&tr);
    if(tr.flFraction>=1.f||tr.vecPlaneNormal.z<.7f){engineerNotice(p,"Busca suelo plano.\n");return true;}
    Vector at=Vector(tr.vecEndPos)+Vector(0,0,1),hull=at+Vector(0,0,36);
    if(eng->pfnTraceHull){TraceResult fit{};eng->pfnTraceHull(&hull.x,&hull.x,0,1,p,&fit);if(fit.fStartSolid||fit.fAllSolid){engineerNotice(p,"No hay espacio para construir.\n");return true;}}
    auto gun=eng->pfnCreateNamedEntity(eng->pfnAllocString("monster_sentry"));
    if(!valid(gun))return true;
    host.spawn(gun);
    auto base=createRuntime("cremation_sentry_base",p);
    if(!valid(base)){eng->pfnRemoveEntity(gun);return true;}
    gun->v.classname=eng->pfnAllocString("cremation_sentry");gun->v.health=150;gun->v.max_health=150;
    gun->v.takedamage=1;gun->v.movetype=MOVETYPE_NONE;gun->v.solid=SOLID_BBOX;gun->v.flags|=FL_MONSTER;
    gun->v.owner=p;gun->v.sequence=0;gun->v.frame=0;gun->v.framerate=1;gun->v.angles=Vector(0,p->v.v_angle.y,0);
    eng->pfnSetModel(gun,"models/cremation/tfc_sentry.mdl");eng->pfnSetModel(base,"models/cremation/tfc_sentry_base.mdl");
    Vector lo(-16,-16,0),hi(16,16,48);eng->pfnSetSize(gun,&lo.x,&hi.x);
    eng->pfnSetOrigin(gun,&at.x);eng->pfnSetOrigin(base,&at.x);
    s.gun=gun;s.serial=gun->serialnumber;s.base=base;s.baseSerial=base->serialnumber;s.owner=p;s.ownerSerial=p->serialnumber;
    s.metal-=130;s.ammo=100;s.completeAt=globals->time+4;s.nextShot=s.completeAt;
    s.froze=(p->v.flags&FL_FROZEN)==0;p->v.flags|=FL_FROZEN;
    host.sound(gun,"cremation/tfc_build.wav");engineerNotice(p,"Construyendo sentry (4 segundos).\n");return true;
}
bool engineerMetalImpl(edict_t* pickup,edict_t* p) {
    if(!valid(pickup)||!valid(p)||!(p->v.flags&FL_CLIENT)||!pickup->v.targetname)return false;
    if(std::strncmp(text(pickup->v.targetname),"cremation_metal_",16))return false;
    int i=playerIndex(p);if(i<0||engineers[i].metal>=200)return true;
    engineers[i].metal=std::min(200,engineers[i].metal+50);eng->pfnRemoveEntity(pickup);
    char message[64];std::snprintf(message,sizeof message,"Metal: %d / 200\n",engineers[i].metal);engineerNotice(p,message);return true;
}
void tfcPrecacheImpl() {
    for(auto model:{"models/v_shotgun.mdl","models/p_shotgun.mdl","models/cremation/tfc_sentry.mdl","models/cremation/tfc_sentry_base.mdl"})eng->pfnPrecacheModel(const_cast<char*>(model));
    for(auto sound:{"cremation/tfc_shotgun.wav","cremation/tfc_sentry_fire.wav","cremation/tfc_build.wav"})eng->pfnPrecacheSound(const_cast<char*>(sound));
    // Precache the Ravenholm sentry damage carrier during legal world precache.
    if(eng->pfnCreateNamedEntity&&host.spawn){auto e=eng->pfnCreateNamedEntity(eng->pfnAllocString("monster_sentry"));if(e){host.spawn(e);eng->pfnRemoveEntity(e);}}
}
void tfcResetImpl(){for(auto& s:engineers)if(s.gun)removeSentry(s);engineers.fill(EngineerState{});tfcPlayers.fill(TfcState{});}
