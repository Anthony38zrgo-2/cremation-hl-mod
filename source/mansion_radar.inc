static int mansionRadarMessage=0;
static float mansionRadarAt=0;
static void mansionRadarPrecache(){mansionRadarMessage=engine.pfnRegUserMsg("HordeRadar",-1);mansionRadarAt=0;}
static bool mansionRadarEligible(edict_t* e){
    if(!aliveEnemy(e)||czIs(e)||!e->pvPrivateData||necroMinion(e)||bounce::isSentry(e))return false;
    auto table=*reinterpret_cast<void***>(e->pvPrivateData);int faction=reinterpret_cast<int(__thiscall*)(void*)>(table[ravenholm_abi::slot::Classify])(e->pvPrivateData);
    return faction!=0&&faction!=2&&faction!=3&&faction!=11;
}
static void mansionRadarFrame(){
    if(!mansionRadarMessage||globals->time<mansionRadarAt)return;mansionRadarAt=globals->time+.25f;
    std::vector<edict_t*> enemies;
    if(mansionMap())for(int i=1;i<globals->maxEntities;++i){auto e=engine.pfnPEntityOfEntIndex(i);if(mansionRadarEligible(e))enemies.push_back(e);}
    for(int i=1;i<=globals->maxClients;++i){auto p=engine.pfnPEntityOfEntIndex(i);if(!valid(p)||!(p->v.flags&FL_CLIENT))continue;
        int packets=std::max(1,int((enemies.size()+39)/40));
        for(int packet=0;packet<packets;++packet){int start=packet*40,count=std::min(40,int(enemies.size())-start);
            engine.pfnMessageBegin(MSG_ONE,mansionRadarMessage,nullptr,p);engine.pfnWriteByte(packet==0?1:0);engine.pfnWriteByte(mansionMap()?1:0);engine.pfnWriteByte(count);
            for(int j=0;j<count;++j){engine.pfnWriteCoord(enemies[start+j]->v.origin.x);engine.pfnWriteCoord(enemies[start+j]->v.origin.y);}engine.pfnMessageEnd();
        }
    }
}
