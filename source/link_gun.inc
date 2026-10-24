// Continuous Link Gun secondary. All render effects are temporary entities;
// living targets keep their original render state, including freeze effects.
static int linkBeamSprite=0;
static constexpr const char* LinkSkeletonModel="models/cremation/link_skeleton.mdl";
static constexpr float LinkRange=768.f,LinkAmmoInterval=.5f,LinkTickInterval=.1f;
struct LinkLock {
    edict_t* target=nullptr;int serial=0;bool acquired=false,finished=false;
    float nextTick=0,nextAmmo=0,lastTick=0,connectedTime=0,nextSound=0;
    Vector lastOrigin{0,0,0},lastAngles{0,0,0};float lastHeight=72,lastDamageAt=-100;
    bool damaged=false,deathShown=false;
    std::array<edict_t*,4> glows{};std::array<int,4> glowSerials{};
};
static std::array<LinkLock,33> linkLocks{};
struct LinkSkeleton {
    edict_t* entity;int serial;float born;
    edict_t* victim=nullptr;int victimSerial=0,sourceSequence=-1;
    float animationStarted=0,deathDuration=1.f;bool matched=false,creature=false;
};
static std::vector<LinkSkeleton> linkSkeletons;
static bool linkEnemy(edict_t* e) {
    if(!aliveEnemy(e)||!e->v.takedamage||!e->pvPrivateData)return false;
    auto table=*reinterpret_cast<void***>(e->pvPrivateData);
    using ClassifyFn=int(__thiscall*)(void*);
    int faction=reinterpret_cast<ClassifyFn>(table[8])(e->pvPrivateData);
    return faction!=3&&faction!=11; // Native human passives and player allies.
}
static Vector linkDirection(edict_t* p) {
    Vector aim=p->v.v_angle+p->v.punchangle;
    float yaw=aim.y*3.14159265359f/180.f,pitch=aim.x*3.14159265359f/180.f;
    return Vector(std::cos(pitch)*std::cos(yaw),std::cos(pitch)*std::sin(yaw),-std::sin(pitch));
}
static edict_t* acquireLink(edict_t* p) {
    Vector eye=p->v.origin+p->v.view_ofs,direction=linkDirection(p),end=eye+direction*LinkRange;
    TraceResult direct{};engine.pfnTraceLine(&eye.x,&end.x,0,p,&direct);
    if(linkEnemy(direct.pHit))return direct.pHit;
    edict_t* best=nullptr;float bestScore=1e9f;
    for(int i=globals->maxClients+1;i<globals->maxEntities;++i) {
        auto e=engine.pfnPEntityOfEntIndex(i);if(!linkEnemy(e))continue;
        Vector delta=center(e)-eye;float distance=length(delta);
        if(distance<1.f||distance>LinkRange)continue;
        float cosine=dot(direction,delta*(1.f/distance));
        if(cosine<.98480775f||!canSee(eye,e,p))continue; // 10-degree acquisition cone.
        float score=(1.f-cosine)*1024.f+distance/LinkRange;
        if(score<bestScore) {best=e;bestScore=score;}
    }
    return best;
}
static constexpr const char* LinkGlowSprite="sprites/cremation/link_plasma_beam_glow.spr";
static constexpr const char* LinkHotSprite="sprites/cremation/link_plasma_beam_hot.spr";
static void linkClearGlows(LinkLock& state) {
    for(int i=0;i<4;++i) {
        auto e=state.glows[i];
        if(valid(e)&&e->serialnumber==state.glowSerials[i])engine.pfnRemoveEntity(e);
        state.glows[i]=nullptr;
    }
}
static void linkGlowUpdate(LinkLock& state,edict_t* player,const Vector& from,const Vector& to,bool impact) {
    for(int i=0;i<4;++i) {
        auto e=state.glows[i];bool hot=(i&1)!=0,atTarget=i>=2;
        if(atTarget&&!impact) {
            if(valid(e)&&e->serialnumber==state.glowSerials[i])e->v.effects|=EF_NODRAW;
            continue;
        }
        Vector position=atTarget?to:from;
        float scale=atTarget?(hot?.28f:.65f):(hot?.1f:.18f);
        if(!valid(e)||e->serialnumber!=state.glowSerials[i]) {
            e=plasmaLayer(player,position,hot?LinkHotSprite:LinkGlowSprite,scale,hot?255:160);
            state.glows[i]=e;if(!e)continue;state.glowSerials[i]=e->serialnumber;
        }
        e->v.effects&=~EF_NODRAW;engine.pfnSetOrigin(e,&position.x);
        e->v.scale=scale*(1.f+.08f*std::sin(globals->time*14.f+i));
        e->v.frame=std::fmod(globals->time*12.f+i*3.f,16.f);e->v.animtime=globals->time;
    }
}
static void linkBeam(const Vector& from,const Vector& to,bool impactLight,const Vector& aim,LinkLock& state,edict_t* player) {
    Vector span=to-from;float distance=length(span);
    Vector toward=normal(span),side=normal(Vector(-toward.y,toward.x,0));
    if(length(side)<.1f)side=Vector(1,0,0);
    Vector up=normal(Vector(toward.y*side.z-toward.z*side.y,
                            toward.z*side.x-toward.x*side.z,
                            toward.x*side.y-toward.y*side.x));
    Vector control1=from+normal(aim)*std::min(180.f,distance*.45f);
    Vector control2=to-toward*std::min(96.f,distance*.2f)+up*std::min(24.f,distance*.05f);
    float pulse=.5f+.5f*std::sin(globals->time*14.f);
    auto pointAt=[&](float t,int layer) {
        if(t<=0)return from;if(t>=1)return to;
        float u=1.f-t,envelope=std::sin(t*3.14159265359f);
        Vector point=from*(u*u*u)+control1*(3*u*u*t)+control2*(3*u*t*t)+to*(t*t*t);
        float phase=globals->time*18.f-t*18.8495559215f;
        // Nearly steady white core; two independent green helices wrap around it.
        point=point+side*(std::sin(phase*.45f)*2.f*envelope);
        if(layer>=2) {
            float radius=std::min(12.f,std::max(4.f,distance*.02f))*(.7f+.3f*(1.f-t))*envelope;
            phase+=(layer-2)*3.14159265359f;
            point=point+side*(std::sin(phase)*radius)+up*(std::cos(phase)*radius*.7f);
        }
        return point;
    };
    for(int layer=0;layer<4;++layer) {
        int segments=layer<2?10:16;
        Vector previous=from;
        for(int segment=1;segment<=segments;++segment) {
            Vector point=pointAt(segment/float(segments),layer);
            engine.pfnMessageBegin(MSG_PVS,SvcTemp,&from.x,nullptr);
            engine.pfnWriteByte(TE_BEAMPOINTS);coords(previous);coords(point);engine.pfnWriteShort(linkBeamSprite);
            engine.pfnWriteByte(0);engine.pfnWriteByte(20);engine.pfnWriteByte(2);
            engine.pfnWriteByte(layer==0?int(36+10*pulse):layer==1?int(9+3*pulse):5);
            engine.pfnWriteByte(0);
            engine.pfnWriteByte(layer==1?225:layer==0?15:75);
            engine.pfnWriteByte(255);
            engine.pfnWriteByte(layer==1?225:layer==0?25:85);
            engine.pfnWriteByte(layer==0?75:layer==1?255:int(180+50*pulse));
            engine.pfnWriteByte(layer==0?25:60);engine.pfnMessageEnd();
            previous=point;
        }
    }
    linkGlowUpdate(state,player,from,to,impactLight);
    engine.pfnMessageBegin(MSG_PVS,SvcTemp,&from.x,nullptr);
    engine.pfnWriteByte(TE_DLIGHT);coords(from);engine.pfnWriteByte(7);
    engine.pfnWriteByte(25);engine.pfnWriteByte(255);engine.pfnWriteByte(45);
    engine.pfnWriteByte(1);engine.pfnWriteByte(0);engine.pfnMessageEnd();
    if(!impactLight)return;
    engine.pfnMessageBegin(MSG_PVS,SvcTemp,&to.x,nullptr);
    engine.pfnWriteByte(TE_DLIGHT);coords(to);engine.pfnWriteByte(16);
    engine.pfnWriteByte(35);engine.pfnWriteByte(255);engine.pfnWriteByte(65);
    engine.pfnWriteByte(2);engine.pfnWriteByte(0);engine.pfnMessageEnd();
}
static bool linkSequenceInfo(edict_t* e,int sequence,std::string& name,int& activity,float& duration) {
    auto header=static_cast<unsigned char*>(engine.pfnGetModelPtr(e));if(!header)return false;
    if(*reinterpret_cast<int*>(header)!=0x54534449)return false; // IDST.
    int length=*reinterpret_cast<int*>(header+72),count=*reinterpret_cast<int*>(header+164);
    int offset=*reinterpret_cast<int*>(header+168);
    if(sequence<0||sequence>=count||offset<244||offset+size_t(sequence+1)*176>size_t(length))return false;
    auto desc=header+offset+sequence*176;size_t n=0;while(n<32&&desc[n])++n;
    name.assign(reinterpret_cast<char*>(desc),n);
    for(auto& c:name)if(c>='A'&&c<='Z')c=char(c-'A'+'a');
    activity=*reinterpret_cast<int*>(desc+40);
    float fps=*reinterpret_cast<float*>(desc+32);int frames=*reinterpret_cast<int*>(desc+56);
    duration=fps>0?std::max(.1f,(frames-1)/fps):1.f;return true;
}
static bool linkMatchDeath(edict_t* victim,edict_t* skeleton,int& sequence,float& duration) {
    std::string source;int activity=0;
    if(!linkSequenceInfo(victim,victim->v.sequence,source,activity,duration))return false;
    // Prefer matching native death activities. Bone tracks cannot be copied
    // between unrelated rigs, so select the corresponding supplied skeleton pose.
    int best=-1;
    for(int candidate=31;candidate<=37;++candidate) {
        std::string label;int candidateActivity;float unused;
        if(linkSequenceInfo(skeleton,candidate,label,candidateActivity,unused)&&activity>0&&candidateActivity==activity) {best=candidate;break;}
    }
    bool named=source.find("die")!=std::string::npos||source.find("death")!=std::string::npos||source.find("shot")!=std::string::npos;
    if(best<0&&!named)return false;
    if(source.find("head")!=std::string::npos)best=36;
    else if(source.find("gut")!=std::string::npos||source.find("chest")!=std::string::npos)best=37;
    else if(source.find("back")!=std::string::npos)best=35;
    else if(source.find("forward")!=std::string::npos||source.find("front")!=std::string::npos)best=33;
    if(best<0)best=31;
    sequence=best;return true;
}
static void linkSkeleton(const Vector& origin,const Vector& angles,float height,edict_t* victim) {
    auto e=engine.pfnCreateNamedEntity(engine.pfnAllocString("info_target"));
    if(!e) {startupTrace("LinkSkeletonFX","allocation failed");return;}
    raven.pfnSpawn(e);
    e->v.targetname=engine.pfnAllocString("cremation_link_skeleton_fx");
    bool creature=victim&&!victim->free&&!humanoidRemains(victim);
    engine.pfnSetModel(e,creature?text(victim->v.model):LinkSkeletonModel);
    e->v.effects&=~EF_NODRAW;e->v.nextthink=0;
    Vector mins(-24,-24,0),maxs(24,24,std::max(64.f,height));
    engine.pfnSetSize(e,&mins.x,&maxs.x);
    Vector visibleOrigin=origin+Vector(0,0,3);
    engine.pfnSetOrigin(e,&visibleOrigin.x);
    e->v.angles=Vector(0,angles.y,0);e->v.movetype=MOVETYPE_NONE;e->v.solid=SOLID_NOT;
    e->v.takedamage=0;e->v.sequence=31;e->v.frame=0;e->v.animtime=globals->time;e->v.framerate=0;
    e->v.scale=std::max(.65f,std::min(1.8f,height/72.f));
    e->v.rendermode=kRenderTransTexture;e->v.renderamt=210;e->v.renderfx=kRenderFxGlowShell;
    e->v.rendercolor=Vector(35,255,65);
    LinkSkeleton state{e,e->serialnumber,globals->time};state.animationStarted=globals->time;
    if(victim&&!victim->free) {state.victim=victim;state.victimSerial=victim->serialnumber;}
    int chosen=31;float duration=1;
    if(state.victim&&linkMatchDeath(state.victim,e,chosen,duration)) {
        e->v.sequence=chosen;state.matched=true;state.sourceSequence=state.victim->v.sequence;state.deathDuration=duration;
        state.animationStarted=globals->time-std::max(0.f,std::min(1.f,state.victim->v.frame/255.f))*duration;
    } else e->v.sequence=13; // Brief neutral pose while native AI chooses its death.
    if(creature){state.creature=true;state.matched=true;state.sourceSequence=victim->v.sequence;e->v.sequence=victim->v.sequence;e->v.body=victim->v.body;e->v.skin=victim->v.skin;e->v.scale=victim->v.scale;e->v.angles=victim->v.angles;engine.pfnSetOrigin(e,&victim->v.origin.x);std::string label;int activity;linkSequenceInfo(victim,e->v.sequence,label,activity,state.deathDuration);state.animationStarted=globals->time-std::max(0.f,std::min(1.f,victim->v.frame/255.f))*state.deathDuration;}
    linkSkeletons.push_back(state);
    char report[192];std::snprintf(report,sizeof report,"entity=%d model_index=%d effects=%d origin=%.1f,%.1f,%.1f height=%.1f",engine.pfnIndexOfEdict(e),e->v.modelindex,e->v.effects,visibleOrigin.x,visibleOrigin.y,visibleOrigin.z,height);
    startupTrace("LinkSkeletonFX",report);
}
static void linkFrame() {
    for(int i=1;i<33;++i)if(linkLocks[i].acquired) {
        auto player=engine.pfnPEntityOfEntIndex(i);
        if(!valid(player)||player->v.health<=0||!nailgun(player)) {linkClearGlows(linkLocks[i]);linkLocks[i]=LinkLock{};}
    }
    for(auto& skeleton:linkSkeletons) {
        auto e=skeleton.entity;if(!valid(e)||e->serialnumber!=skeleton.serial) {skeleton.entity=nullptr;continue;}
        float elapsed=globals->time-skeleton.born;
        if(elapsed>=(skeleton.creature?5.f:6.f)) {engine.pfnRemoveEntity(e);skeleton.entity=nullptr;continue;}
        auto victim=skeleton.victim;
        bool hasVictim=victim&&!victim->free&&victim->serialnumber==skeleton.victimSerial;
        if(skeleton.creature){
            if(hasVictim){
                if(e->v.sequence!=victim->v.sequence){e->v.sequence=victim->v.sequence;std::string label;int activity;linkSequenceInfo(victim,e->v.sequence,label,activity,skeleton.deathDuration);skeleton.animationStarted=globals->time-std::max(0.f,std::min(1.f,victim->v.frame/255.f))*skeleton.deathDuration;}
                engine.pfnSetOrigin(e,&victim->v.origin.x);e->v.angles=victim->v.angles;
            }
            float progress=std::max(0.f,std::min(1.f,(globals->time-skeleton.animationStarted)/std::max(.1f,skeleton.deathDuration)));
            e->v.frame=progress*255;e->v.framerate=progress<1?1:0;e->v.animtime=globals->time;e->v.effects&=~EF_NODRAW;
            e->v.rendermode=kRenderTransColor;e->v.renderfx=kRenderFxGlowShell;e->v.rendercolor=Vector(35,255,65);e->v.renderamt=190*std::min(1.f,(5.f-elapsed)*2);
            continue;
        }
        int chosen=31;float duration=1;
        bool sourceDeath=hasVictim&&linkMatchDeath(victim,e,chosen,duration);
        if(sourceDeath) {
            if(!skeleton.matched||skeleton.sourceSequence!=victim->v.sequence) {
                e->v.sequence=chosen;skeleton.matched=true;skeleton.sourceSequence=victim->v.sequence;
                skeleton.deathDuration=duration;
                skeleton.animationStarted=globals->time-std::max(0.f,std::min(1.f,victim->v.frame/255.f))*duration;
                char report[96];std::snprintf(report,sizeof report,"source_sequence=%d skeleton_sequence=%d duration=%.3f",victim->v.sequence,chosen,duration);
                startupTrace("LinkSkeletonAnimation",report);
            }
            Vector position=victim->v.origin+Vector(0,0,3);engine.pfnSetOrigin(e,&position.x);
            e->v.angles.y=victim->v.angles.y;
        } else {
            if(!skeleton.matched&&elapsed>=.3f) {
                e->v.sequence=31;skeleton.matched=true;skeleton.animationStarted=globals->time;
                std::string label;int activity;float nativeDuration;
                if(linkSequenceInfo(e,31,label,activity,nativeDuration))skeleton.deathDuration=nativeDuration;
            }
        }
        // Corpse frames advance at AI Think frequency. Drive the matching
        // animation by elapsed time and allow client-side studio interpolation.
        float progress=skeleton.matched?(globals->time-skeleton.animationStarted)/skeleton.deathDuration:0;
        e->v.frame=std::max(0.f,std::min(255.f,progress*255.f));
        e->v.framerate=0;
        if(skeleton.matched&&progress<1.f) {
            std::string label;int activity;float nativeDuration;
            if(linkSequenceInfo(e,e->v.sequence,label,activity,nativeDuration))
                e->v.framerate=nativeDuration/skeleton.deathDuration;
        }
        e->v.animtime=globals->time;
        e->v.effects&=~EF_NODRAW;
        // Green glow/transparency lasts one second; the skeleton stays behind.
        if(elapsed<1.f) {
            float glow=1.f-elapsed;
            e->v.renderfx=kRenderFxGlowShell;e->v.rendermode=kRenderTransTexture;
            e->v.rendercolor=Vector(35,255,65)*glow;
            e->v.renderamt=210.f+45.f*elapsed;
        } else {
            e->v.renderfx=kRenderFxNone;e->v.rendercolor=Vector(255,255,255);
            e->v.rendermode=elapsed<5.75f?kRenderNormal:kRenderTransTexture;
            e->v.renderamt=elapsed<5.75f?255.f:255.f*(6.f-elapsed)/.25f;
        }
    }
    linkSkeletons.erase(std::remove_if(linkSkeletons.begin(),linkSkeletons.end(),[](const LinkSkeleton& s){return !s.entity;}),linkSkeletons.end());
}
static void linkDeathEffect(LinkLock& state,edict_t* target) {
    if(state.deathShown)return;
    state.deathShown=true;linkClearGlows(state);
    linkSkeleton(state.lastOrigin,state.lastAngles,state.lastHeight,
        target&&!target->free&&target->serialnumber==state.serial?target:nullptr);
    if(target&&!target->free&&target->serialnumber==state.serial) {
        target->v.effects|=EF_NODRAW;target->v.solid=SOLID_NOT;
    }
}
static void linkHud(edict_t* p,int remaining) {
    if(!msgAmmoX||nailAmmoIndex<0)return;
    engine.pfnMessageBegin(MSG_ONE,msgAmmoX,nullptr,p);
    engine.pfnWriteByte(nailAmmoIndex);engine.pfnWriteByte(remaining);engine.pfnMessageEnd();
}
static void linkPostThink(edict_t* p,bool active,bool held) {
    int i=engine.pfnIndexOfEdict(p);if(i<1||i>32)return;
    auto& state=linkLocks[i];
    if(!active||!held||p->v.health<=0) {linkClearGlows(state);state=LinkLock{};return;}
    if(state.finished) {linkClearGlows(state);return;}
    if(!state.acquired) {
        auto e=acquireLink(p);if(!e)return;
        state.target=e;state.serial=e->serialnumber;state.acquired=true;
        state.lastTick=globals->time-LinkTickInterval;
    }
    auto target=state.target;
    if(!linkEnemy(target)||target->serialnumber!=state.serial) {
        // Some native monsters finish Killed/Think after TakeDamage returns.
        if(state.damaged&&globals->time-state.lastDamageAt<=.35f&&
           (target->free||target->serialnumber!=state.serial||target->v.health<=0||target->v.deadflag!=DEAD_NO))
            linkDeathEffect(state,target);
        linkClearGlows(state);state.finished=true;return;
    }
    if(globals->time<state.nextTick)return;
    state.nextTick=globals->time+LinkTickInterval;
    float dt=std::max(0.f,std::min(.2f,globals->time-state.lastTick));state.lastTick=globals->time;
    Vector eye=p->v.origin+p->v.view_ofs,to=center(target);
    if(length(to-eye)>LinkRange) {linkClearGlows(state);return;} // Keep the lock; no damage through distance/walls.
    TraceResult tr{};engine.pfnTraceLine(&eye.x,&to.x,0,p,&tr);
    bool visible=!tr.fStartSolid&&!tr.fAllSolid&&(tr.flFraction>=.999f||tr.pHit==target);
    Vector beamEnd=visible?to:Vector(tr.vecEndPos);
    Vector direction=linkDirection(p);
    float yaw=(p->v.v_angle.y+p->v.punchangle.y)*3.14159265359f/180.f;
    // GoldSrc view-right is (sin(yaw), -cos(yaw), 0). The opposite
    // vector placed the beam left of the right-handed viewmodel.
    Vector right(std::sin(yaw),-std::cos(yaw),0);
    Vector up=normal(Vector(right.y*direction.z-right.z*direction.y,
                            right.z*direction.x-right.x*direction.z,
                            right.x*direction.y-right.y*direction.x));
    Vector muzzle=eye+direction*18.f+right*6.f-up*6.f;
    if(!visible) {linkBeam(muzzle,beamEnd,false,direction,state,p);return;}
    if(globals->time>=state.nextAmmo) {
        if(sharedAmmo(p,nailAmmoIndex,0)<1) {linkClearGlows(state);state.finished=true;return;}
        int remaining=sharedAmmo(p,nailAmmoIndex,1);
        if(remaining<0) {linkClearGlows(state);state.finished=true;return;}
        state.nextAmmo=globals->time+LinkAmmoInterval;linkHud(p,remaining);
    }
    linkBeam(muzzle,to,true,direction,state,p);
    if(globals->time>=state.nextSound) {
        sound(p,"weapons/ut2004/link_altfire.wav");state.nextSound=globals->time+.35f;
        animation(p,5);
    }
    players[i].animUntil=globals->time+.2f;
    float dps=std::min(48.f,12.f+state.connectedTime*6.f);
    state.lastOrigin=target->v.origin;state.lastAngles=target->v.angles;
    state.lastHeight=target->v.maxs.z-target->v.mins.z;
    float healthBefore=target->v.health;
    auto table=*reinterpret_cast<void***>(target->pvPrivateData);
    auto takeDamage=reinterpret_cast<TakeDamageFn>(table[11]);
    int result=takeDamage(target->pvPrivateData,&p->v,&p->v,dps*dt,(1<<10)|(1<<12)); // ENERGYBEAM | NEVERGIB.
    state.connectedTime+=dt;
    if(result>0||target->free||target->serialnumber!=state.serial||target->v.health<healthBefore) {
        state.damaged=true;state.lastDamageAt=globals->time;
    }
    if(target->free||target->serialnumber!=state.serial||target->v.health<=0||target->v.deadflag!=DEAD_NO) {
        linkDeathEffect(state,target);state.finished=true;
    }
}
