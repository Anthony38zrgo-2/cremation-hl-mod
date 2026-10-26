// Radioactive pit, ballistic launch pad and ammo-only deathmatch rewards.
#include "reactor_layout.h"
static bool reactorActive=false;
static int reactorGoldSprite=0;
static int reactorPortalSprite=0;
static std::array<float,2> reactorPortalLightAt{};
static std::array<float,33> reactorPortalCooldown{};
static std::array<float,33> reactorJumpCooldown{};
static std::array<bool,33> reactorInFlight{};
static void reactorTestFrame();
static void reactorPreviewFrame();
static const char* reactorWeapons[]={"weapon_bounce_shotgun","weapon_nailgun","weapon_shockrifle","weapon_rpg","weapon_handgrenade"};
static const char* reactorWorldModels[]={"models/cremation/w_bounce_shotgun.mdl","models/cremation/w_nailgun.mdl","models/cremation/w_shockrifle.mdl","models/w_rpg.mdl","models/w_grenade.mdl"};
static const char** reactorPowerups=dmcNames;
static const char** reactorPowerupModels=dmcModels;
static bool reactorInside(float x,float y){
    bool inside=false;const int n=sizeof(reactorOutline)/sizeof(reactorOutline[0]);
    for(int i=0,j=n-1;i<n;j=i++){
        float ax=reactorOutline[i][0],ay=reactorOutline[i][1],bx=reactorOutline[j][0],by=reactorOutline[j][1];
        if((ay>y)!=(by>y)&&x<(bx-ax)*(y-ay)/(by-ay)+ax)inside=!inside;
    }
    if(!inside)return false;
    for(const auto& r:reactorSafeRects)if(x>=r[0]&&x<=r[2]&&y>=r[1]&&y<=r[3])return false;
    return true;
}
static void reactorPrecache(){
    if(!mansionMap())return;
    reactorGoldSprite=engine.pfnPrecacheModel(const_cast<char*>("sprites/cremation/pickup_gold_square.spr"));
    reactorPortalSprite=engine.pfnPrecacheModel(const_cast<char*>("sprites/cremation/return_portal.spr"));
    engine.pfnPrecacheModel(const_cast<char*>("sprites/cremation/powerup_blue_square.spr"));
    for(auto model:reactorWorldModels)engine.pfnPrecacheModel(const_cast<char*>(model));
    dmcPrecache();
    for(auto path:{"buttons/button3.wav","items/gunpickup2.wav","ambience/port_suckin1.wav","ambience/port_suckout1.wav"})engine.pfnPrecacheSound(const_cast<char*>(path));
}
static void reactorKill(edict_t* victim){
    if(!valid(victim)||victim->v.health<=0||victim->v.deadflag!=DEAD_NO||!victim->pvPrivateData)return;
    auto world=engine.pfnPEntityOfEntIndex(0);auto table=*reinterpret_cast<void***>(victim->pvPrivateData);
    victim->v.takedamage=DAMAGE_YES;
    reinterpret_cast<TakeDamageFn>(table[11])(victim->pvPrivateData,&world->v,&world->v,1000000.f,(1<<20)|(1<<13));
    if(valid(victim)&&victim->v.deadflag==DEAD_NO){victim->v.health=0;reinterpret_cast<void(__thiscall*)(void*,entvars_t*,int)>(table[13])(victim->pvPrivateData,&world->v,2);}
    startupTrace("ReactorKill",text(victim->v.classname));
}
static bool reactorGive(edict_t* player,int kind){
    if(kind<0||kind>=5||!valid(player)||!player->pvPrivateData||ammoOffset<0)return false;
    int* ammo=reinterpret_cast<int*>(static_cast<char*>(player->pvPrivateData)+ammoOffset);
    const int indices[]={shotgunAmmoIndex,nailAmmoIndex,shockAmmoIndex,rpgAmmoIndex,grenadeAmmoIndex};
    const int caps[]={125,250,50,30,10},packs[]={12,50,15,3,3};
    int index=indices[kind];bool owned=false;
    for(int i=1;i<globals->maxEntities;++i){auto e=engine.pfnPEntityOfEntIndex(i);if(valid(e)&&e->v.aiment==player&&!std::strcmp(text(e->v.classname),reactorWeapons[kind])){owned=true;break;}}
    if(owned&&index>=0&&index<32&&ammo[index]>=caps[kind])return false;
    if(!owned){
        auto item=engine.pfnCreateNamedEntity(engine.pfnAllocString(reactorWeapons[kind]));if(!valid(item))return false;
        item->v.origin=player->v.origin;item->v.spawnflags|=1<<30;hookSpawn(item);item->v.effects|=EF_NODRAW;
        if(raven.pfnTouch)raven.pfnTouch(item,player);
        if(!valid(item)||item->v.aiment!=player){if(valid(item))engine.pfnRemoveEntity(item);return false;}
        // Message hooks can discover an index while adding the first weapon.
        int updated[]={shotgunAmmoIndex,nailAmmoIndex,shockAmmoIndex,rpgAmmoIndex,grenadeAmmoIndex};index=updated[kind];
    }
    if(index>=0&&index<32){ammo[index]=std::min(caps[kind],std::max(0,ammo[index])+packs[kind]);
        if(msgAmmoX){engine.pfnMessageBegin(MSG_ONE,msgAmmoX,nullptr,player);engine.pfnWriteByte(index);engine.pfnWriteByte(ammo[index]);engine.pfnMessageEnd();}
    }else if(owned)return false;
    engine.pfnEmitSound(player,CHAN_ITEM,"items/gunpickup2.wav",1,ATTN_NORM,0,100);
    if(msgItemPickup){engine.pfnMessageBegin(MSG_ONE,msgItemPickup,nullptr,player);engine.pfnWriteString(reactorWeapons[kind]);engine.pfnMessageEnd();}
    startupTrace("ReactorPickup",reactorWeapons[kind]);return true;
}
static bool reactorGivePowerup(edict_t* player,int kind){return dmcGive(player,kind);}
static bool reactorReturn(edict_t* player){
    auto exit=mansionFind("reactor_return_exit");if(!valid(exit))return false;
    static const Vector offsets[]={Vector(0,0,0),Vector(0,48,0),Vector(0,-48,0),Vector(-48,0,0),Vector(0,96,0),Vector(0,-96,0)};
    for(const auto& offset:offsets){Vector destination=exit->v.origin+offset;TraceResult tr{};
        engine.pfnTraceHull(&destination.x,&destination.x,0,1,player,&tr);if(tr.fStartSolid||tr.fAllSolid)continue;
        Vector down=destination-Vector(0,0,80);engine.pfnTraceLine(&destination.x,&down.x,1,player,&tr);if(tr.flFraction==1||tr.vecPlaneNormal.z<.65f)continue;
        engine.pfnEmitSound(player,CHAN_ITEM,"ambience/port_suckin1.wav",1,ATTN_NORM,0,100);
        engine.pfnSetOrigin(player,&destination.x);player->v.velocity=player->v.basevelocity=Vector(0,0,0);player->v.flags&=~FL_ONGROUND;
        player->v.angles=player->v.v_angle=Vector(0,0,0);player->v.punchangle=Vector(0,0,0);player->v.fixangle=1;
        int index=engine.pfnIndexOfEdict(player);if(index>0&&index<33){reactorPortalCooldown[index]=globals->time+2;reactorInFlight[index]=false;}
        engine.pfnEmitSound(player,CHAN_BODY,"ambience/port_suckout1.wav",1,ATTN_NORM,0,100);
        char report[140];std::snprintf(report,sizeof report,"at=%.1f,%.1f,%.1f",destination.x,destination.y,destination.z);startupTrace("ReactorReturn",report);return true;
    }return false;
}
static void reactorPortalFrame(){
    for(int portalIndex=0;portalIndex<2;++portalIndex){
    auto portal=mansionFind(portalIndex?"reactor_powerup_portal":"reactor_return_portal");if(!valid(portal))continue;
    const char* visualName=portalIndex?"reactor_powerup_portal_visual":"reactor_return_visual";
    auto visual=mansionFind(visualName);
    if(!valid(visual)){visual=engine.pfnCreateNamedEntity(engine.pfnAllocString("info_target"));if(!valid(visual))return;raven.pfnSpawn(visual);
        visual->v.targetname=engine.pfnAllocString(visualName);visual->v.owner=portal;visual->v.movetype=MOVETYPE_NOCLIP;visual->v.solid=SOLID_NOT;
        engine.pfnSetModel(visual,"sprites/cremation/return_portal.spr");visual->v.rendermode=kRenderTransAdd;visual->v.renderamt=230;visual->v.scale=.625f;
        Vector lo(-22,-22,-40),hi(22,22,40);engine.pfnSetSize(visual,&lo.x,&hi.x);
    }
    engine.pfnSetOrigin(visual,&portal->v.origin.x);visual->v.frame=std::fmod(globals->time*12,16.f);
    if(globals->time>=reactorPortalLightAt[portalIndex]){reactorPortalLightAt[portalIndex]=globals->time+.2f;
        engine.pfnMessageBegin(MSG_PVS,SvcTemp,&portal->v.origin.x,nullptr);engine.pfnWriteByte(TE_DLIGHT);coords(portal->v.origin);
        for(int value:{8,35,120,255,3,0})engine.pfnWriteByte(value);engine.pfnMessageEnd();
    }
    for(int i=1;i<=globals->maxClients&&i<33;++i){auto player=engine.pfnPEntityOfEntIndex(i);
        if(!valid(player)||!(player->v.flags&FL_CLIENT)||player->v.health<=0||player->v.deadflag!=DEAD_NO||globals->time<reactorPortalCooldown[i])continue;
        if(length2d(player->v.origin-portal->v.origin)<24&&std::fabs(player->v.origin.z-portal->v.origin.z)<42)reactorReturn(player);
    }
    }
}
static void reactorActivate(){
    reactorActive=mansionMap()&&valid(mansionFind("reactor_jump_pad"));
    reactorJumpCooldown.fill(0);reactorInFlight.fill(false);reactorPortalCooldown.fill(0);reactorPortalLightAt.fill(0);
    if(reactorActive)startupTrace("ReactorActivate","depth=448 bridge=central island=small_house rewards=1 portal=rear_yard pickup_pool=5");
}
static void reactorFrame(){
    if(!reactorActive)return;
    reactorPreviewFrame();
    reactorTestFrame();
    reactorPortalFrame();
    for(int i=1;i<globals->maxEntities;++i){auto e=engine.pfnPEntityOfEntIndex(i);if(!valid(e)||e->v.health<=0||e->v.deadflag!=DEAD_NO)continue;
        if(!(e->v.flags&(FL_CLIENT|FL_MONSTER)))continue;
        float feet=e->v.origin.z+e->v.mins.z;
        if(feet<=-399&&reactorInside(e->v.origin.x,e->v.origin.y))reactorKill(e);
    }
    for(int jumpZone=0;jumpZone<2;++jumpZone){
    auto pad=mansionFind(jumpZone?"reactor_powerup_jump_pad":"reactor_jump_pad"),destination=mansionFind(jumpZone?"reactor_powerup_jump_destination":"reactor_jump_destination");
    if(valid(pad)&&valid(destination))for(int i=1;i<=globals->maxClients&&i<33;++i){auto player=engine.pfnPEntityOfEntIndex(i);
        if(!valid(player)||player->v.health<=0||!(player->v.flags&FL_CLIENT))continue;
        if(reactorInFlight[i]&&(player->v.flags&FL_ONGROUND)&&globals->time>reactorJumpCooldown[i]-1.35f){
            bool onIsland=false;for(const auto& r:reactorIslands)if(player->v.origin.x>r[0]&&player->v.origin.x<r[2]&&player->v.origin.y>r[1]&&player->v.origin.y<r[3])onIsland=true;
            if(onIsland&&player->v.origin.z+player->v.mins.z>=112){
                player->v.velocity.x=player->v.velocity.y=0;
                char report[140];std::snprintf(report,sizeof report,"at=%.1f,%.1f,%.1f",player->v.origin.x,player->v.origin.y,player->v.origin.z);startupTrace("ReactorLanding",report);
            }reactorInFlight[i]=false;
        }
        if(globals->time<reactorJumpCooldown[i])continue;
        Vector delta=player->v.origin-pad->v.origin;float feet=player->v.origin.z+player->v.mins.z;
        if(std::fabs(delta.x)>32||std::fabs(delta.y)>32||feet<26||feet>34||!(player->v.flags&FL_ONGROUND))continue;
        float gravity=engine.pfnCVarGetFloat("sv_gravity");if(gravity<=0)gravity=800;
        gravity*=player->v.gravity>0?player->v.gravity:1;
        constexpr float flight=1.35f;Vector velocity=(destination->v.origin-player->v.origin)*(1.f/flight);velocity.z+=.5f*gravity*flight;
        player->v.velocity=velocity;player->v.basevelocity=Vector(0,0,0);player->v.flags&=~FL_ONGROUND;
        reactorJumpCooldown[i]=globals->time+1.5f;reactorInFlight[i]=true;engine.pfnEmitSound(player,CHAN_ITEM,"buttons/button3.wav",1,ATTN_NORM,0,100);
        char report[160];std::snprintf(report,sizeof report,"player=%d velocity=%.1f,%.1f,%.1f flight=1.35",i,velocity.x,velocity.y,velocity.z);startupTrace("ReactorJump",report);
    }
    }
    for(int stationIndex=0;stationIndex<2;++stationIndex){bool powerup=stationIndex==1;char marker[48];std::snprintf(marker,sizeof marker,powerup?"reactor_powerup_0":"reactor_pickup_0");auto station=mansionFind(marker);if(!valid(station))continue;
        std::snprintf(marker,sizeof marker,"reactor_display_%d",stationIndex);auto display=mansionFind(marker);
        if(!valid(display)){
            display=engine.pfnCreateNamedEntity(engine.pfnAllocString("info_target"));if(!valid(display))continue;raven.pfnSpawn(display);
            display->v.targetname=engine.pfnAllocString(marker);display->v.owner=station;display->v.solid=SOLID_NOT;display->v.movetype=MOVETYPE_NOCLIP;
            display->v.impulse=0;display->v.effects|=EF_NODRAW;
        }
        if(globals->time>=station->v.dmgtime&&(display->v.impulse==0||(display->v.effects&EF_NODRAW))){
            int kind=engine.pfnRandomLong(0,powerup?3:4);display->v.impulse=kind+1;engine.pfnSetModel(display,powerup?reactorPowerupModels[kind]:reactorWorldModels[kind]);
            display->v.effects&=~EF_NODRAW;display->v.rendermode=kRenderNormal;display->v.renderfx=kRenderFxGlowShell;display->v.rendercolor=powerup?Vector(55,170,255):Vector(255,190,35);display->v.renderamt=4;
            if(powerup){display->v.rendercolor=kind==0?Vector(128,128,255):kind==1?Vector(255,128,0):Vector(128,128,128);
                display->v.rendermode=kind==2?kRenderTransColor:kRenderNormal;display->v.renderamt=kind==2?30:kind==3?255:100;display->v.renderfx=kind==3?kRenderFxNone:kRenderFxGlowShell;}
            startupTrace(powerup?"ReactorPowerupSpawn":"ReactorRewardSpawn",powerup?reactorPowerups[kind]:reactorWeapons[kind]);
        }
        if(powerup){auto h=static_cast<unsigned char*>(engine.pfnGetModelPtr(display));if(h){int seq=*reinterpret_cast<int*>(h+168);float fps=*reinterpret_cast<float*>(h+seq+32);int frames=*reinterpret_cast<int*>(h+seq+56);display->v.frame=frames>1?std::fmod(globals->time*fps*256.f/(frames-1),256.f):0;display->v.animtime=globals->time;}}
        Vector at=station->v.origin+Vector(0,0,std::sin(globals->time*2+stationIndex)*5);engine.pfnSetOrigin(display,&at.x);display->v.angles.y=std::fmod(globals->time*35+stationIndex*90,360.f);
        for(int j=0;j<6;++j){
            char name[48];std::snprintf(name,sizeof name,"reactor_square_%d_%d",stationIndex,j);auto square=mansionFind(name);
            if(!valid(square)){square=engine.pfnCreateNamedEntity(engine.pfnAllocString("info_target"));if(!valid(square))continue;raven.pfnSpawn(square);
                square->v.targetname=engine.pfnAllocString(name);square->v.owner=station;square->v.solid=SOLID_NOT;square->v.movetype=MOVETYPE_NOCLIP;
                engine.pfnSetModel(square,powerup?"sprites/cremation/powerup_blue_square.spr":"sprites/cremation/pickup_gold_square.spr");square->v.rendermode=kRenderTransAdd;square->v.renderamt=230;square->v.rendercolor=Vector(255,255,255);square->v.scale=.12f;
                Vector lo(-3,-3,-3),hi(3,3,3);engine.pfnSetSize(square,&lo.x,&hi.x);
            }
            float angle=globals->time*1.5f+j*1.047198f;
            Vector particle=at+Vector(std::cos(angle)*25,std::sin(angle)*25,std::sin(globals->time*2+j)*16);
            engine.pfnSetOrigin(square,&particle.x);
            if(display->v.effects&EF_NODRAW)square->v.effects|=EF_NODRAW;else square->v.effects&=~EF_NODRAW;
        }
        if(display->v.effects&EF_NODRAW)continue;
        for(int i=1;i<=globals->maxClients;++i){auto player=engine.pfnPEntityOfEntIndex(i);if(!valid(player)||!(player->v.flags&FL_CLIENT)||player->v.health<=0)continue;
            if(length2d(player->v.origin-at)<36&&std::fabs(player->v.origin.z-at.z)<52&&(powerup?reactorGivePowerup(player,display->v.impulse-1):reactorGive(player,display->v.impulse-1))){
                display->v.effects|=EF_NODRAW;station->v.dmgtime=globals->time+(powerup?((display->v.impulse==2||display->v.impulse==3)?300.f:60.f):15.f);break;
            }
        }
    }
}
