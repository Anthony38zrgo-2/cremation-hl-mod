// Versioned fixed-length snapshot, sent per player. Server owns all timers;
// the client only interpolates the displayed countdown/progress.
static float tacticalHudNext=0;
static void tacticalHudFrame(){
    if(!msgTacticalHUD||globals->time<tacticalHudNext)return;tacticalHudNext=globals->time+.2f;
    auto bomb=mansionFind("cz_planted_bomb");int status=0;bool guarding=false;float remaining=0,progress=0;
    if(valid(bomb)){
        if(bomb->v.impulse){if(globals->time-bomb->v.teleport_time<6)status=bomb->v.impulse==1?4:5;}
        else{guarding=czIs(bomb->v.owner)&&aliveEnemy(bomb->v.owner)&&!std::strcmp(text(bomb->v.owner->v.targetname),text(bomb->v.message));
            status=guarding?1:(valid(bomb->v.enemy)?3:2);remaining=std::max(0.f,bomb->v.dmgtime-globals->time);progress=std::min(1000.f,bomb->v.frags*200.f);}
    }
    for(int i=1;i<=globals->maxClients&&i<33;++i){auto p=engine.pfnPEntityOfEntIndex(i);if(!valid(p)||!(p->v.flags&FL_CLIENT))continue;
        bool alive=p->v.health>0&&p->v.deadflag==DEAD_NO;int flags=0;
        if(valid(bomb)&&status>0&&status<4&&alive){
            if(bomb->v.enemy==p)flags|=1;
            if(length(center(p)-bomb->v.origin)<=88){flags|=4;if(guarding&&(players[i].buttons&IN_USE))flags|=2;}
        }
        engine.pfnMessageBegin(MSG_ONE_UNRELIABLE,msgTacticalHUD,nullptr,p);
        engine.pfnWriteByte(1);engine.pfnWriteByte(status);engine.pfnWriteShort(int(std::ceil(remaining*10)));engine.pfnWriteShort(int(progress));
        engine.pfnWriteByte(flags);engine.pfnWriteByte(alive&&!std::strcmp(engine.pfnGetPhysicsKeyValue(p,"defuse"),"1"));
        for(int k=0;k<4;++k)engine.pfnWriteShort(alive?int(std::ceil(std::max(0.f,dmcUntil(p,k)-globals->time)*10)):0);
        engine.pfnWriteByte(alive);engine.pfnMessageEnd();
    }
}
