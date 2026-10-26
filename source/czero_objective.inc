// One saved objective state per native CZ monster. Runtime path caches are
// disposable; phase, grenade inventory, bomb and timers use saved entvars.
#include "czero_navigation.h"
#include <queue>
static constexpr int CZNavCount=sizeof(czNav)/sizeof(czNav[0]);
static Vector czNavPoint(int n){return Vector(czNav[n].x,czNav[n].y,czNav[n].z);}
static int czNearest(const Vector& p){int best=0;float distance=1e9f;for(int i=0;i<CZNavCount;++i){float d=length(czNavPoint(i)-p);if(d<distance){distance=d;best=i;}}return best;}
struct CZPath{edict_t* npc;int serial;std::vector<int> nodes;size_t cursor=0;Vector goal{0,0,0};float replan=0,progressAt=0;Vector last{0,0,0};};
static std::vector<CZPath> czPaths;
static CZPath& czPath(edict_t* e){for(auto& p:czPaths)if(p.npc==e&&p.serial==e->serialnumber)return p;czPaths.push_back({e,e->serialnumber});return czPaths.back();}
static void czPlan(edict_t* e,CZPath& path,const Vector& goal){
    int start=czNearest(e->v.origin),end=czNearest(goal);std::vector<float> costs(CZNavCount,1e9f);std::vector<int> previous(CZNavCount,-1);
    using Item=std::pair<float,int>;std::priority_queue<Item,std::vector<Item>,std::greater<Item>> queue;costs[start]=0;queue.push({0,start});
    std::vector<Vector> occupied;for(int i=1;i<globals->maxEntities;++i){auto m=engine.pfnPEntityOfEntIndex(i);if(m!=e&&aliveEnemy(m))occupied.push_back(m->v.origin);}
    while(!queue.empty()){auto item=queue.top();queue.pop();int n=item.second;if(item.first>costs[n]+.01f)continue;if(n==end)break;
        for(int k=0;k<czNav[n].count;++k){int j=czNavEdges[czNav[n].first+k];float penalty=0;Vector at=czNavPoint(j);
            Vector a=czNavPoint(n),b=at;float sweep=std::max(a.z,b.z)+36.2f;a.z=b.z=sweep;TraceResult moving{};engine.pfnTraceHull(&a.x,&b.x,1,1,e,&moving);
            // Opening a rotating door moves its leaf into the room. Route
            // around that live obstacle instead of assuming the closed BSP.
            if(moving.flFraction<.999f&&valid(moving.pHit)&&std::strstr(text(moving.pHit->v.classname),"door_rotating")&&std::fabs(std::remainder(moving.pHit->v.angles.y,180.f))>10)continue;
            for(const auto& mob:occupied)if(length(at-mob)<56)penalty+=90;
            float cost=costs[n]+length(at-czNavPoint(n))+penalty;
            if(cost<costs[j]){costs[j]=cost;previous[j]=n;queue.push({cost,j});}}
    }
    path.nodes.clear();path.cursor=0;path.goal=goal;path.replan=globals->time+2;
    if(end==start)path.nodes.push_back(end);
    else if(previous[end]>=0){for(int n=end;n!=start;n=previous[n])path.nodes.push_back(n);std::reverse(path.nodes.begin(),path.nodes.end());}
}
static bool czWalkToward(edict_t* e,const Vector& to,float dt,float reach=10.f){
    if(length2d(to-e->v.origin)<reach&&std::fabs(to.z-e->v.origin.z)<20)return true;
    Vector delta=to-e->v.origin;float yaw=std::atan2(delta.y,delta.x)*180/3.14159265f;
    Vector a=e->v.origin+Vector(0,0,36),b=to+Vector(0,0,36);TraceResult tr{};engine.pfnTraceHull(&a.x,&b.x,0,1,e,&tr);
    if(valid(tr.pHit)&&tr.pHit!=e&&length(tr.vecEndPos-a)<120){const char* cls=text(tr.pHit->v.classname);
        if(std::strstr(cls,"door")){
            auto door=tr.pHit;bool rotating=std::strstr(cls,"rotating")!=nullptr;bool closed=!rotating||std::fabs(std::remainder(door->v.angles.y,180.f))<10;
            if(closed&&length(door->v.avelocity)<1&&globals->time>=e->v.fuser2){raven.pfnUse(door,e);e->v.fuser2=globals->time+3;}
            else if(!closed&&length(door->v.avelocity)<1&&door->v.nextthink>0)door->v.nextthink=std::max(door->v.nextthink,door->v.ltime+1.5f);
        }
        if(!std::strcmp(cls,"func_breakable")&&tr.pHit->pvPrivateData&&globals->time>=e->v.dmgtime){auto t=*reinterpret_cast<void***>(tr.pHit->pvPrivateData);reinterpret_cast<TakeDamageFn>(t[11])(tr.pHit->pvPrivateData,&e->v,&e->v,40,1<<7);e->v.dmgtime=globals->time+.65f;}}
    float distance=std::min(length2d(delta),110.f*dt);bool moved=hookWalkMove(e,yaw,distance,0)!=0;
    if(!moved)for(float turn:{25.f,-25.f,50.f,-50.f,90.f,-90.f,135.f,-135.f,180.f})if(hookWalkMove(e,yaw+turn,distance,0)){yaw+=turn;moved=true;break;}
    if(!moved&&e->v.iuser1!=5&&globals->time>=e->v.fuser4){e->v.fuser4=globals->time+3;char row[220];std::snprintf(row,sizeof row,"phase=%d next=%.1f,%.1f,%.1f hit=%s fraction=%.2f",e->v.iuser1,to.x,to.y,to.z,valid(tr.pHit)?text(tr.pHit->v.classname):"none",tr.flFraction);startupTrace("CZNavBlocked",row);}
    if(moved){e->v.angles.y=yaw;czSequence(e,3);e->v.fuser1=globals->time+.15f;}
    return false;
}
static bool czNavigate(edict_t* e,const Vector& goal,float dt){
    auto& p=czPath(e);if(length2d(e->v.origin-goal)<18&&std::fabs(e->v.origin.z-goal.z)<20)return true;
    if(globals->time>=p.replan||length(p.goal-goal)>4)czPlan(e,p,goal);
    while(p.cursor<p.nodes.size()&&length2d(czNavPoint(p.nodes[p.cursor])-e->v.origin)<3&&std::fabs(czNavPoint(p.nodes[p.cursor]).z-e->v.origin.z)<20)++p.cursor;
    if(p.cursor>=p.nodes.size()){if(!p.nodes.empty())return czWalkToward(e,goal,dt);p.replan=0;return false;}
    czWalkToward(e,czNavPoint(p.nodes[p.cursor]),dt,2.5f);return false;
}
static int czDoorCrowd(edict_t* e,const Vector& at){int count=0;for(int i=1;i<globals->maxEntities;++i){auto m=engine.pfnPEntityOfEntIndex(i);if(m!=e&&aliveEnemy(m)&&length(m->v.origin-at)<128)++count;}return count;}
static bool czDoorBlocked(edict_t* e,int door){
    Vector a=door?Vector(-312,-820,92):Vector(96,-256,92),b=door?Vector(-312,-704,92):Vector(-64,-256,92);TraceResult tr{};
    engine.pfnTraceHull(&a.x,&b.x,1,1,e,&tr);
    return tr.flFraction<.99f&&(!valid(tr.pHit)||!std::strstr(text(tr.pHit->v.classname),"door"));
}
static void czChooseEntrance(edict_t* e){
    const Vector outside[]={Vector(224,-256,24.1f),Vector(-328,-880,24.1f)};
    float score[2];for(int i=0;i<2;++i)score[i]=czDoorBlocked(e,i)||czDoorCrowd(e,i?Vector(-312,-768,56):Vector(0,-256,56))>3?1e8f:length2d(e->v.origin-outside[i]);
    if(score[0]>=1e8f&&score[1]>=1e8f){e->v.iuser1=6;startupTrace("CZObjective","ladder: doors obstructed/crowded");}
    else{e->v.iuser2=score[0]<=score[1]?0:1;e->v.iuser1=1;startupTrace("CZObjective",e->v.iuser2?"south door":"front door");}
    czPath(e).replan=0;e->v.fuser3=globals->time;e->v.target=0;
}
static bool czObjectiveOwns(edict_t* e){return czIs(e)&&e->v.iuser1>0;}
static bool czBombActive(){auto b=mansionFind("cz_planted_bomb");return valid(b)&&b->v.impulse==0;}
static void czPlant(edict_t* e){
    if(czBombActive())return;
    for(int i=1;i<globals->maxEntities;++i){auto old=engine.pfnPEntityOfEntIndex(i);if(valid(old)&&!std::strcmp(text(old->v.targetname),"cz_planted_bomb")&&old->v.impulse)engine.pfnRemoveEntity(old);}
    auto bomb=engine.pfnCreateNamedEntity(engine.pfnAllocString("info_target"));if(!valid(bomb))return;raven.pfnSpawn(bomb);
    bomb->v.targetname=engine.pfnAllocString("cz_planted_bomb");bomb->v.owner=e;bomb->v.message=e->v.targetname;
    bomb->v.dmgtime=globals->time+35;bomb->v.solid=SOLID_NOT;bomb->v.movetype=MOVETYPE_NONE;Vector at=e->v.origin+Vector(0,0,5);
    engine.pfnSetOrigin(bomb,&at.x);engine.pfnSetModel(bomb,"models/cremation/czero/w_c4.mdl");bomb->v.fuser1=globals->time;
    e->v.chain=bomb;e->v.iuser1=5;e->v.button=20;e->v.dmgtime=globals->time+.4f;
    engine.pfnSetModel(e,"models/cremation/czero/zombie_glock.mdl");Vector lo(-16,-16,0),hi(16,16,72);engine.pfnSetSize(e,&lo.x,&hi.x);czSequence(e,19);czGunFrame(e);
    engine.pfnEmitSound(e,CHAN_ITEM,"cremation/czero/c4_plant.wav",1,ATTN_NORM,0,100);startupTrace("CZBombPlanted","35 seconds; Glock equipped");
}
static void czPistol(edict_t* e,edict_t* target){
    if(!valid(target)||globals->time<e->v.dmgtime)return;
    if(e->v.button<=0){e->v.button=20;e->v.dmgtime=globals->time+1.8f/(slowed(e)?balance::SlowFactor:1.f);czSequence(e,21);return;}
    Vector from=e->v.origin+e->v.view_ofs,to=center(target);if(length(to-from)>1536)return;
    TraceResult tr{};engine.pfnTraceLine(&from.x,&to.x,0,e,&tr);if(tr.pHit!=target&&tr.flFraction<.999f)return;
    --e->v.button;e->v.dmgtime=globals->time+.35f/(slowed(e)?balance::SlowFactor:1.f);e->v.angles.y=std::atan2(to.y-from.y,to.x-from.x)*180/3.14159265f;
    engine.pfnEmitSound(e,CHAN_WEAPON,"cremation/czero/glock18-2.wav",1,ATTN_NORM,0,100);czSequence(e,20);e->v.frame=0;
    to=to+Vector(engine.pfnRandomFloat(-5,5),engine.pfnRandomFloat(-5,5),engine.pfnRandomFloat(-4,4));engine.pfnTraceLine(&from.x,&to.x,0,e,&tr);
    if(tr.pHit==target&&target->pvPrivateData){auto t=*reinterpret_cast<void***>(target->pvPrivateData);reinterpret_cast<TakeDamageFn>(t[11])(target->pvPrivateData,&e->v,&e->v,20,1<<1);}
}
static void czCover(edict_t* e,const Vector& threat){
    float best=-1e9f;Vector anchor=valid(e->v.chain)?e->v.chain->v.origin:e->v.origin;
    for(int i=0;i<CZNavCount;++i){Vector at=czNavPoint(i);if(length2d(at-anchor)>256||std::fabs(at.z-e->v.origin.z)>20)continue;
        Vector eye=at+Vector(0,0,56);TraceResult tr{};engine.pfnTraceLine(&eye.x,&threat.x,1,e,&tr);
        if(tr.flFraction>=.99f)continue;
        // A real adjacent obstruction plus a visible neighbouring peek point.
        for(int k=0;k<czNav[i].count;++k){int j=czNavEdges[czNav[i].first+k];Vector peek=czNavPoint(j),peye=peek+Vector(0,0,56);
            engine.pfnTraceLine(&peye.x,&threat.x,1,e,&tr);if(tr.flFraction<.99f)continue;
            float score=200-length(at-e->v.origin)-length2d(at-anchor)*.25f;if(score>best){best=score;e->v.vuser1=at;e->v.vuser2=peek;e->v.iuser3=i;}}
    }
    if(best<-1e8f){e->v.vuser1=e->v.origin;e->v.vuser2=e->v.origin;}
    e->v.fuser3=globals->time+3;e->v.fuser4=globals->time+1;czPath(e).replan=0;
}
static bool czObjectiveThink(edict_t* e,float dt){
    if(e->v.weapons<0||!mansionActor(e)||mansionCZRouteTest)return false;
    if(!e->v.iuser1){if(e->v.origin.x>=-1160&&e->v.origin.x<=456&&e->v.origin.y>=-1064&&e->v.origin.y<=552)czChooseEntrance(e);else{
        auto close=czEnemy(e);if(valid(close)&&length2d(close->v.origin-e->v.origin)<=64&&length(center(close)-(e->v.origin+Vector(0,0,40)))<=90)czFire(e,close);
        return true;}}
    if(!czSmokeCanLeave(e))return true;
    auto target=czEnemy(e);
    if(!(e->v.spawnflags&CZSmokeThrown)&&czThrowSmoke(e))return true;
    if(valid(target)&&(target->v.flags&FL_CLIENT)&&czThrowFlash(e,target))return true;
    if(e->v.iuser1!=5&&e->v.iuser1!=4&&valid(target)&&length2d(target->v.origin-e->v.origin)<=64&&length(center(target)-(e->v.origin+Vector(0,0,40)))<=90){czFire(e,target);if(czMeleeBusy(e))return true;}
    if(e->v.impulse==2){e->v.impulse=0;czSequence(e,e->v.iuser1==5?19:75);}
    if(e->v.iuser1==1){Vector goal=e->v.iuser2?Vector(-328,-880,24.1f):Vector(224,-256,24.1f);
        if(czDoorCrowd(e,e->v.iuser2?Vector(-312,-768,56):Vector(0,-256,56))>3){czChooseEntrance(e);return true;}
        if(czNavigate(e,goal,dt)){e->v.iuser1=2;e->v.iuser4=0;czPath(e).replan=0;e->v.fuser3=globals->time;}}
    else if(e->v.iuser1==2){
        const Vector front[]={Vector(176,-256,40.1f),Vector(160,-256,48.1f),Vector(64,-256,56.1f),Vector(-64,-256,56.1f)};
        const Vector south[]={Vector(-328,-824,32.1f),Vector(-328,-808,40.1f),Vector(-328,-792,48.1f),Vector(-328,-760,56.1f),Vector(-328,-704,56.1f)};
        int count=e->v.iuser2?5:4;Vector goal=e->v.iuser2?south[std::min(count-1,e->v.iuser4)]:front[std::min(count-1,e->v.iuser4)];
        if(czWalkToward(e,goal,dt)){if(++e->v.iuser4>=count){e->v.iuser1=3;czPath(e).replan=0;}}
        else if(globals->time-e->v.fuser3>12){e->v.iuser1=6;czPath(e).replan=0;}}
    else if(e->v.iuser1==3){Vector goal=e->v.origin.z>140?Vector(-624,-176,192.1f):Vector(-304,-272,56.1f);
        if(czNavigate(e,goal,dt)){e->v.iuser1=4;e->v.teleport_time=globals->time+3;e->v.vuser4=e->v.origin;czSequence(e,61);e->v.frame=0;startupTrace("CZObjective","planting at hostage zone");}}
    else if(e->v.iuser1==4){if(length(e->v.origin-e->v.vuser4)>24){e->v.iuser1=3;return true;}czSequence(e,61);if(globals->time>=e->v.teleport_time)czPlant(e);}
    else if(e->v.iuser1==5){
        if(valid(target)){e->v.vuser3=center(target);e->v.enemy=target;czPistol(e,target);}
        if(e->v.sequence==21&&globals->time<e->v.dmgtime)return true;
        if(globals->time>=e->v.fuser3)czCover(e,e->v.vuser3);
        bool peek=std::fmod(globals->time,2.8f)>1.5f;Vector goal=peek?e->v.vuser2:e->v.vuser1;
        if(!czNavigate(e,goal,dt)&&globals->time>e->v.dmgtime)czSequence(e,3);
        else if(globals->time>e->v.dmgtime)czSequence(e,19);
    }else if(e->v.iuser1==6){if(czNavigate(e,Vector(-328,288,24.1f),dt)){e->v.iuser1=7;e->v.movetype=MOVETYPE_FLY;e->v.flags&=~FL_ONGROUND;}}
    else if(e->v.iuser1==7){Vector delta=Vector(-328,280,228.1f)-e->v.origin;e->v.velocity=Vector(std::max(-60.f,std::min(60.f,delta.x*5)),std::max(-60.f,std::min(60.f,delta.y*5)),std::max(-90.f,std::min(90.f,delta.z*5)));czSequence(e,75);
        if(std::fabs(delta.z)<.25f){Vector lo(-16,-16,0),hi(16,16,36);engine.pfnSetSize(e,&lo.x,&hi.x);e->v.iuser1=8;czSequence(e,73);}}
    else if(e->v.iuser1==8){
        Vector a=e->v.origin+Vector(0,0,18),b=a+Vector(0,-40,0);TraceResult window{};engine.pfnTraceHull(&a.x,&b.x,1,3,e,&window);
        if(valid(window.pHit)&&!std::strcmp(text(window.pHit->v.classname),"func_breakable")&&window.pHit->pvPrivateData&&globals->time>=e->v.dmgtime){auto t=*reinterpret_cast<void***>(window.pHit->pvPrivateData);reinterpret_cast<TakeDamageFn>(t[11])(window.pHit->pvPrivateData,&e->v,&e->v,40,1<<7);e->v.dmgtime=globals->time+.65f;}
        e->v.velocity=Vector(0,slowed(e)?-60*balance::SlowFactor:-60,0);czSequence(e,73);if(e->v.origin.y<=196){e->v.velocity=Vector(0,0,0);e->v.movetype=MOVETYPE_STEP;e->v.iuser1=9;}}
    else if(e->v.iuser1==9){Vector p=e->v.origin+Vector(0,0,36);TraceResult tr{};engine.pfnTraceHull(&p.x,&p.x,1,1,e,&tr);
        if(!tr.fStartSolid&&!tr.fAllSolid&&(e->v.flags&FL_ONGROUND)){Vector lo(-16,-16,0),hi(16,16,72);engine.pfnSetSize(e,&lo.x,&hi.x);e->v.iuser1=3;czPath(e).replan=0;}}
    return true;
}
static void czBombFrame(){
    auto b=mansionFind("cz_planted_bomb");if(!valid(b)||b->v.impulse)return;
    float dt=std::min(.1f,std::max(0.f,globals->time-b->v.fuser1));b->v.fuser1=globals->time;
    if(globals->time>=b->v.dmgtime){b->v.impulse=2;b->v.teleport_time=globals->time;auto state=mansionState();if(valid(state))state->v.impulse=2;
        engine.pfnEmitSound(b,CHAN_ITEM,"cremation/czero/c4_explode1.wav",1,ATTN_NONE,0,100);
        for(int i=1;i<globals->maxEntities;++i){auto p=engine.pfnPEntityOfEntIndex(i);if(!valid(p))continue;
            if(p->v.flags&FL_CLIENT){reactorKill(p);}
            else if(p->v.flags&FL_MONSTER){p->v.velocity=Vector(0,0,0);p->v.flags|=FL_FROZEN;}}
        startupTrace("CZBombExploded","game over");return;
    }
    bool guarding=czIs(b->v.owner)&&aliveEnemy(b->v.owner)&&!std::strcmp(text(b->v.owner->v.targetname),text(b->v.message));edict_t* defuser=nullptr;
    for(int i=1;i<=globals->maxClients&&i<33;++i){auto p=engine.pfnPEntityOfEntIndex(i);if(!valid(p)||p->v.health<=0)continue;
        if(length(center(p)-b->v.origin)>88||!(players[i].buttons&IN_USE)||std::strcmp(engine.pfnGetPhysicsKeyValue(p,"defuse"),"1"))continue;
        Vector eye=p->v.origin+p->v.view_ofs,end=b->v.origin+Vector(0,0,6);TraceResult tr{};engine.pfnTraceLine(&eye.x,&end.x,1,p,&tr);if(tr.flFraction<.99f)continue;
        if(guarding)continue;defuser=p;break;
    }
    if(!valid(defuser)){b->v.enemy=nullptr;b->v.frags=0;}
    else if(b->v.enemy!=defuser||length(defuser->v.origin-b->v.vuser1)>4){b->v.enemy=defuser;b->v.vuser1=defuser->v.origin;b->v.frags=0;engine.pfnEmitSound(b,CHAN_ITEM,"cremation/czero/c4_disarm.wav",1,ATTN_NORM,0,100);}
    else{b->v.frags+=dt;if(b->v.frags>=5){b->v.impulse=1;b->v.teleport_time=globals->time;b->v.effects|=EF_NODRAW;engine.pfnEmitSound(b,CHAN_ITEM,"cremation/czero/c4_disarmed.wav",1,ATTN_NORM,0,100);startupTrace("CZBombDefused","five seconds with kit");}}
    if(globals->time>=b->v.pain_finished){b->v.pain_finished=globals->time+.5f;
        if(!b->v.impulse)engine.pfnEmitSound(b,CHAN_ITEM,"cremation/czero/c4_beep1.wav",1,ATTN_NORM,0,100);
    }
}
