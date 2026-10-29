// Incremental horde navigation: one shared budget, cached world traces and

// delayed retries. No whole-edict scans are allowed inside graph traversal.

#include <chrono>

using MansionNavClock=std::chrono::steady_clock;

using MansionNavItem=std::pair<float,int>;

struct MansionNavPath{

    edict_t* actor=nullptr;int serial=0,start=-1,finish=-1;std::vector<int> nodes;size_t cursor=0;

    Vector goal{0,0,0};float retryAt=0,blocked=0,recoveryAt=0;bool requested=false,planning=false;

    int node=-1,edge=0;

    int blockedFrom=-1,blockedTo=-1;float blockedUntil=0;

    int stairDirection=0;Vector stairMins{0,0,0},stairMaxs{0,0,0};

    Vector directGoal{0,0,0};float directUntil=0;bool directClear=false;

    std::vector<float> costs;std::vector<int> previous;

    std::priority_queue<MansionNavItem,std::vector<MansionNavItem>,std::greater<MansionNavItem>> pending;

};

struct MansionSharedRoute{int start=-1,finish=-1;std::vector<int> nodes;float expires=0;};

static std::vector<MansionSharedRoute> mansionSharedRoutes;

struct MansionNavEdge{float until=0;bool clear=false;};

static std::vector<MansionNavPath> mansionNavPaths;

static std::vector<MansionNavEdge> mansionNavEdges;

static constexpr int MansionNavEdgeBudget=8192,MansionNavTraceBudget=256,MansionNavQuantum=256;

static constexpr double MansionNavTimeBudgetMs=2.;

static int mansionNavEdgesLeft=0,mansionNavTracesLeft=0;

static double mansionNavWorkMs=0,mansionNavPeakMs=0;

static MansionNavClock::time_point mansionNavTickBegan;

static double mansionNavPeakMoveMs=0;

static int mansionNavPeakTraces=0,mansionNavPeakEdges=0,mansionNavPeakPaths=0;

static unsigned mansionNavStarted=0,mansionNavCompleted=0,mansionNavFailed=0;

static unsigned mansionNavUpperCompleted=0,mansionNavUpperSteps=0;

static float mansionNavCleanupAt=0;

static void mansionNavReset(){

    czNearestNavCache.clear();

    mansionNavPaths.clear();mansionNavEdges.assign(sizeof(czNavEdges)/sizeof(czNavEdges[0]),{});

    mansionSharedRoutes.clear();

    mansionNavCleanupAt=0;mansionNavWorkMs=mansionNavPeakMs=mansionNavPeakMoveMs=0;

    mansionNavPeakTraces=mansionNavPeakEdges=mansionNavPeakPaths=0;

    mansionNavStarted=mansionNavCompleted=mansionNavFailed=0;

    mansionNavUpperCompleted=mansionNavUpperSteps=0;

}

static void mansionNavBeginTick(){

    mansionNavTickBegan=MansionNavClock::now();

    mansionNavEdgesLeft=MansionNavEdgeBudget;mansionNavTracesLeft=MansionNavTraceBudget;mansionNavWorkMs=0;

    if(globals->time>=mansionNavCleanupAt){

        mansionNavCleanupAt=globals->time+1.f;

        for(auto it=czNearestNavCache.begin();it!=czNearestNavCache.end();){

            auto ent=engine.pfnPEntityOfEntIndex(it->first);

            if(!valid(ent)||!aliveEnemy(ent))it=czNearestNavCache.erase(it);

            else ++it;

        }

        mansionNavPaths.erase(std::remove_if(mansionNavPaths.begin(),mansionNavPaths.end(),[](const MansionNavPath& p){

            return !valid(p.actor)||p.actor->serialnumber!=p.serial||!aliveEnemy(p.actor);

        }),mansionNavPaths.end());

        mansionSharedRoutes.erase(std::remove_if(mansionSharedRoutes.begin(),mansionSharedRoutes.end(),[](const MansionSharedRoute& r){return r.expires<=globals->time;}),mansionSharedRoutes.end());

        mansionDoorStrikes.erase(std::remove_if(mansionDoorStrikes.begin(),mansionDoorStrikes.end(),[](const MansionDoorStrike& p){

            return !valid(p.actor)||p.actor->serialnumber!=p.serial||!aliveEnemy(p.actor);

        }),mansionDoorStrikes.end());

        mansionCultists.erase(std::remove_if(mansionCultists.begin(),mansionCultists.end(),[](const MansionCultistState& p){

            return !valid(p.entity)||p.entity->serialnumber!=p.serial||!aliveEnemy(p.entity);

        }),mansionCultists.end());

    }

}

static void mansionNavFinishTick(){

    mansionNavPeakMoveMs=std::max(mansionNavPeakMoveMs,std::chrono::duration<double,std::milli>(MansionNavClock::now()-mansionNavTickBegan).count());

    mansionNavPeakMs=std::max(mansionNavPeakMs,mansionNavWorkMs);

    mansionNavPeakTraces=std::max(mansionNavPeakTraces,MansionNavTraceBudget-mansionNavTracesLeft);

    mansionNavPeakEdges=std::max(mansionNavPeakEdges,MansionNavEdgeBudget-mansionNavEdgesLeft);

    mansionNavPeakPaths=std::max(mansionNavPeakPaths,int(mansionNavPaths.size()));

}

#ifdef CREMATION_ENABLE_VALIDATION

static void mansionNavReport(){

    char row[320];std::snprintf(row,sizeof row,"started=%u completed=%u failed=%u paths=%d peak_paths=%d peak_edges=%d peak_traces=%d peak_ms=%.3f peak_move_ms=%.3f upper_completed=%u upper_steps=%u",

        mansionNavStarted,mansionNavCompleted,mansionNavFailed,int(mansionNavPaths.size()),mansionNavPeakPaths,mansionNavPeakEdges,mansionNavPeakTraces,mansionNavPeakMs,mansionNavPeakMoveMs,mansionNavUpperCompleted,mansionNavUpperSteps);

    startupTrace("MansionNavBudget",row);

}

#endif

static MansionNavPath& mansionNavPath(edict_t* e){

    for(auto& p:mansionNavPaths)if(p.actor==e){

        if(p.serial!=e->serialnumber){p=MansionNavPath{};p.actor=e;p.serial=e->serialnumber;}

        return p;

    }

    mansionNavPaths.emplace_back();auto& p=mansionNavPaths.back();p.actor=e;p.serial=e->serialnumber;return p;

}

// mansionNearestNav is provided with spatial grid acceleration by czero_objective.inc

static void mansionNavEndPlan(MansionNavPath& path,bool found){

    path.planning=false;path.node=-1;path.cursor=0;

    if(found){

        int n=path.finish;

        for(int count=0;n>=0&&count<CZNavCount;++count){path.nodes.push_back(n);n=path.previous[n];}

        std::reverse(path.nodes.begin(),path.nodes.end());++mansionNavCompleted;if(path.goal.z>140.f)++mansionNavUpperCompleted;

        if(path.start>=0&&path.finish>=0&&path.blockedUntil<=globals->time){auto cached=std::find_if(mansionSharedRoutes.begin(),mansionSharedRoutes.end(),[&](const MansionSharedRoute& r){return r.start==path.start&&r.finish==path.finish;});

            if(cached==mansionSharedRoutes.end())mansionSharedRoutes.push_back({path.start,path.finish,path.nodes,globals->time+5.f});

            else{cached->nodes=path.nodes;cached->expires=globals->time+5.f;}}

    }else{++mansionNavFailed;path.retryAt=globals->time+2.5f+float(engine.pfnIndexOfEdict(path.actor)%17)*.025f;}

    path.costs.clear();path.previous.clear();path.pending={};

}

static void mansionPlanNav(edict_t* e,MansionNavPath& path,const Vector& goal){

    const auto began=MansionNavClock::now();

    auto budget=[&](){return mansionNavEdgesLeft>0&&mansionNavWorkMs+std::chrono::duration<double,std::milli>(MansionNavClock::now()-began).count()<MansionNavTimeBudgetMs;};

    if(!budget())return;

    if(!path.planning){

        int start=mansionNearestNav(e,e->v.origin);path.start=start;path.finish=mansionNearestNav(goal);

        path.nodes.clear();path.cursor=0;path.node=-1;path.edge=0;

        if(start<0||path.finish<0){path.retryAt=globals->time+3.f;return;}

        path.costs.assign(CZNavCount,1e30f);path.previous.assign(CZNavCount,-1);path.pending={};

        path.costs[start]=0;path.pending.push({length(czNavPoint(start)-czNavPoint(path.finish)),start});

        path.planning=true;++mansionNavStarted;

    }

    int work=0;

    while(path.planning&&work<MansionNavQuantum&&budget()){

        if(path.node<0){

            if(path.pending.empty()){mansionNavEndPlan(path,false);break;}

            auto item=path.pending.top();path.pending.pop();++work;--mansionNavEdgesLeft;

            int n=item.second;

            if(item.first>path.costs[n]+length(czNavPoint(n)-czNavPoint(path.finish))+.01f)continue;

            if(n==path.finish){mansionNavEndPlan(path,true);break;}

            path.node=n;path.edge=0;

            if(work>=MansionNavQuantum||!budget())break;

        }

        int n=path.node;

        if(path.edge>=czNav[n].count){path.node=-1;continue;}

        int index=czNav[n].first+path.edge,next=czNavEdges[index];

        if(path.blockedUntil>globals->time&&n==path.blockedFrom&&next==path.blockedTo){++path.edge;++work;--mansionNavEdgesLeft;continue;}

        Vector a=czNavPoint(n),b=czNavPoint(next);

        if(std::fabs(a.z-b.z)>18.1f){++path.edge;++work;--mansionNavEdgesLeft;continue;}

        auto& cached=mansionNavEdges[index];

        if(cached.until<=globals->time){

            if(mansionNavTracesLeft<=0)break;

            --mansionNavTracesLeft;

            float sweep=std::max(a.z,b.z)+36.2f;Vector from(a.x,a.y,sweep),to(b.x,b.y,sweep);TraceResult tr{};

            engine.pfnTraceHull(&from.x,&to.x,1,1,e,&tr);

            cached.clear=!tr.fStartSolid&&!tr.fAllSolid&&tr.flFraction>=.995f;

            // Static world edges can be shared longer. Near either rotating

            // leaf, refresh promptly so an opening/closing door changes routes.

            Vector midpoint=(a+b)*.5f;

            bool nearDoor=length2d(midpoint-mansionDoorOrigin(1))<192.f||length2d(midpoint-mansionDoorOrigin(2))<192.f;

            cached.until=globals->time+(nearDoor?.25f:(cached.clear?5.f:2.f));

        }

        ++path.edge;++work;--mansionNavEdgesLeft;

        if(!cached.clear)continue;

        float cost=path.costs[n]+length(b-a);if(cost>=path.costs[next])continue;

        path.costs[next]=cost;path.previous[next]=n;

        path.pending.push({cost+length(b-czNavPoint(path.finish)),next});

    }

    mansionNavWorkMs+=std::chrono::duration<double,std::milli>(MansionNavClock::now()-began).count();

}

static bool mansionNavEdgeClear(edict_t* e,const Vector& a,const Vector& b){

    float sweep=std::max(a.z,b.z)+36.2f;Vector from(a.x,a.y,sweep),to(b.x,b.y,sweep);TraceResult tr{};

    engine.pfnTraceHull(&from.x,&to.x,1,1,e,&tr);return !tr.fStartSolid&&!tr.fAllSolid&&tr.flFraction>=.995f;

}

static bool mansionNavSpotFree(edict_t* e,const Vector& at){

    Vector probe=at+Vector(0,0,36.2f);TraceResult tr{};

    engine.pfnTraceHull(&probe.x,&probe.x,1,1,e,&tr);if(tr.fStartSolid||tr.fAllSolid)return false;

    Vector lo=at+e->v.mins,hi=at+e->v.maxs;

    for(int i=1;i<=globals->maxClients;++i){auto other=engine.pfnPEntityOfEntIndex(i);

        if(!valid(other)||other==e||other->v.solid==SOLID_NOT||other->v.health<=0||other->v.deadflag!=DEAD_NO)continue;

        Vector otherLo=other->v.origin+other->v.mins,otherHi=other->v.origin+other->v.maxs;

        if(lo.x<otherHi.x-2&&hi.x>otherLo.x+2&&lo.y<otherHi.y-2&&hi.y>otherLo.y+2&&lo.z<otherHi.z-2&&hi.z>otherLo.z+2)return false;

    }

    for(auto other:getActiveEnemies()){

        if(other==e||other->v.solid==SOLID_NOT||!aliveEnemy(other))continue;

        Vector otherLo=other->v.origin+other->v.mins,otherHi=other->v.origin+other->v.maxs;

        if(lo.x<otherHi.x-2&&hi.x>otherLo.x+2&&lo.y<otherHi.y-2&&hi.y>otherLo.y+2&&lo.z<otherHi.z-2&&hi.z>otherLo.z+2)return false;

    }

    return true;

}

static bool mansionNavDirectClear(edict_t* e,const Vector& goal){
    Vector delta=goal-e->v.origin;float distance=length2d(delta);
    if(distance>512.f||std::fabs(delta.z)>18.f||!mansionNavEdgeClear(e,e->v.origin,goal))return false;
    int steps=std::max(1,int(std::ceil(distance/48.f)));
    for(int i=1;i<=steps;++i){Vector at=e->v.origin+delta*(float(i)/steps),top=at+Vector(0,0,18),bottom=at-Vector(0,0,24);TraceResult tr{};
        engine.pfnTraceLine(&top.x,&bottom.x,1,e,&tr);
        if(tr.fStartSolid||tr.flFraction>=1.f||tr.vecPlaneNormal.z<.65f)return false;
    }return true;
}

static bool mansionTryStuckStep(edict_t* e,MansionNavPath& path){

    // A rare recovery for the zombie horde only. Never skip a sealed door or

    // move through a wall: candidate cells and every traversed graph edge are

    // swept again, and the destination must be empty.

    if(mansionCultist(e)||path.nodes.empty()||path.cursor>=path.nodes.size())return false;

    const size_t last=std::min(path.nodes.size()-1,path.cursor+4);size_t chosen=path.cursor;Vector destination{};bool found=false;

    for(size_t i=path.cursor;i<=last;++i){Vector at=czNavPoint(path.nodes[i]);

        if(length(at-e->v.origin)>96.f||std::fabs(at.z-e->v.origin.z)>64.f)break;

        if(length(at-e->v.origin)<18.f)continue;

        bool clear=mansionNavEdgeClear(e,e->v.origin,czNavPoint(path.nodes[path.cursor]));for(size_t edge=path.cursor;clear&&edge<i;++edge){

            if(!mansionNavEdgeClear(e,czNavPoint(path.nodes[edge]),czNavPoint(path.nodes[edge+1]))){clear=false;break;}}

        if(!clear)break;

        if(!mansionNavSpotFree(e,at))continue;

        chosen=i;destination=at;found=true;

    }

    if(!found)return false;

    float distance=length(destination-e->v.origin);Vector old=e->v.origin;

    engine.pfnSetOrigin(e,&destination.x);engine.pfnDropToFloor(e);e->v.velocity=Vector(0,0,0);path.cursor=chosen+1;

    char report[192];std::snprintf(report,sizeof report,"class=%s distance=%.1f dz=%.1f from=%.0f,%.0f,%.0f to=%.0f,%.0f,%.0f",

        text(e->v.classname),distance,destination.z-old.z,old.x,old.y,old.z,destination.x,destination.y,destination.z);

    startupTrace("MansionStuckStep",report);return true;

}

#ifdef CREMATION_ENABLE_VALIDATION
static bool mansionNearRecoveryProbe(edict_t* actor,edict_t* player){
    Vector start(224,-256,24.1f),nearPlayer(224,-356,60.1f);engine.pfnSetOrigin(actor,&start.x);engine.pfnSetOrigin(player,&nearPlayer.x);
    int node=mansionNearestNav(Vector(288,-256,24.1f));MansionNavPath path{};if(node>=0)path.nodes.push_back(node);
    return node>=0&&mansionTryStuckStep(actor,path)&&length2d(actor->v.origin-start)>=18.f&&length(actor->v.origin-start)<=96.1f;
}
#endif
static bool mansionNavWalk(edict_t* e,const Vector& goal,float dt,float speed,bool running){

    if(!valid(e)||!aliveEnemy(e)||CZNavCount<=0)return false;

    auto& path=mansionNavPath(e);

    // The Mansion stair flight is narrower than a normal human hull. Follow
    // its centreline, restoring the actor's exact hull at the landing.
    bool inHouse=e->v.origin.x<0&&e->v.origin.x>-760&&e->v.origin.y>-720&&e->v.origin.y<224;
    int wanted=path.stairDirection;
    if(!wanted&&inHouse&&goal.z>160.f&&e->v.origin.z<170.f)wanted=1;
    else if(!wanted&&inHouse&&goal.z<110.f&&e->v.origin.z>180.f)wanted=-1;
    if(wanted){
        Vector landing=wanted>0?Vector(-224,-224,64.1f):Vector(-224,8,192.1f);
        if(!path.stairDirection){
            if(length2d(e->v.origin-landing)>25.f||std::fabs(e->v.origin.z-landing.z)>20.f)return mansionNavWalk(e,landing,dt,speed,running);
            path.stairMins=e->v.mins;path.stairMaxs=e->v.maxs;path.stairDirection=wanted;
            Vector lo(-8,-8,e->v.mins.z),hi(8,8,e->v.maxs.z);engine.pfnSetSize(e,&lo.x,&hi.x);
        }
        float endY=wanted>0?8.f:-224.f;
        bool arrived=wanted>0?(e->v.origin.y>=endY-2.f&&e->v.origin.z>180.f):(e->v.origin.y<=endY+2.f&&e->v.origin.z<82.f);
        if(arrived){engine.pfnSetSize(e,&path.stairMins.x,&path.stairMaxs.x);path.stairDirection=0;path.requested=false;path.nodes.clear();return false;}
        float dx=-224.f-e->v.origin.x;float remaining=std::fabs(dx)>1.f?std::fabs(dx):std::fabs(endY-e->v.origin.y);
        float yaw=std::fabs(dx)>1.f?(dx>0?0.f:180.f):(wanted>0?90.f:270.f);
        if(hookWalkMove(e,yaw,std::min(remaining,speed*dt),0)){e->v.angles.y=yaw;if(!qzIs(e)){if(running)hordeRunAnimation(e);else hordeWalkAnimation(e);}}
        return false;
    }

    if(length2d(goal-e->v.origin)<=512.f&&std::fabs(goal.z-e->v.origin.z)<=18.f){
        if(globals->time>=path.directUntil||length(path.directGoal-goal)>16.f){
            path.directGoal=goal;path.directClear=mansionNavDirectClear(e,goal);path.directUntil=globals->time+.35f;
        }
        if(path.directClear){
            Vector delta=goal-e->v.origin;float distance=length2d(delta);
            if(distance<18.f)return true;
            float yaw=std::atan2(delta.y,delta.x)*180.f/3.14159265f,step=std::min(distance,speed*dt);
            bool moved=hookWalkMove(e,yaw,step,0)!=0;float side=engine.pfnIndexOfEdict(e)%2?1.f:-1.f;
            if(!moved)for(float turn:{25.f,50.f,90.f,135.f}){
                if(hookWalkMove(e,yaw+turn*side,step,0)){yaw+=turn*side;moved=true;break;}
                if(hookWalkMove(e,yaw-turn*side,step,0)){yaw-=turn*side;moved=true;break;}
            }
            if(moved){path.blocked=0;path.requested=false;path.planning=false;path.nodes.clear();e->v.angles.y=yaw;if(!czIs(e)&&!qzIs(e)){if(running)hordeRunAnimation(e);else hordeWalkAnimation(e);}return false;}
            path.directClear=false;path.directUntil=globals->time+.35f;
        }
    }

    if(!path.requested||length(path.goal-goal)>8.f){

        path.nodes.clear();path.cursor=0;path.goal=goal;path.retryAt=0;path.blocked=0;path.recoveryAt=0;

        path.requested=true;path.planning=false;path.pending={};

        path.start=mansionNearestNav(e,e->v.origin);path.finish=mansionNearestNav(goal);

        auto cached=std::find_if(mansionSharedRoutes.begin(),mansionSharedRoutes.end(),[&](const MansionSharedRoute& r){return r.start==path.start&&r.finish==path.finish&&r.expires>globals->time;});

        if(path.blockedUntil<=globals->time&&cached!=mansionSharedRoutes.end())path.nodes=cached->nodes;

        else if(path.blockedUntil<=globals->time)for(const auto& route:mansionSharedRoutes)if(route.finish==path.finish&&route.expires>globals->time){

            auto merge=std::find(route.nodes.begin(),route.nodes.end(),path.start);

            if(merge!=route.nodes.end()){path.nodes.assign(merge,route.nodes.end());break;}

        }

        if(!path.nodes.empty()){++mansionNavCompleted;if(goal.z>140.f)++mansionNavUpperCompleted;}

    }

    if(path.planning||(path.nodes.empty()&&globals->time>=path.retryAt))mansionPlanNav(e,path,goal);

    if(path.planning||path.nodes.empty())return false;

    while(path.cursor<path.nodes.size()){

        Vector node=czNavPoint(path.nodes[path.cursor]);

        if(length2d(node-e->v.origin)<7&&std::fabs(node.z-e->v.origin.z)<18){++path.cursor;continue;}

        break;

    }

    Vector destination=path.cursor<path.nodes.size()?czNavPoint(path.nodes[path.cursor]):goal;

    Vector delta=destination-e->v.origin;

    if(path.cursor>=path.nodes.size()&&length2d(delta)<18&&std::fabs(delta.z)<20)return true;

    float yaw=std::atan2(delta.y,delta.x)*180.f/3.14159265f;

    float rate=slowed(e)?balance::SlowFactor:1.f,distance=std::min(length2d(delta),speed*dt)*rate;

    bool moved=hookWalkMove(e,yaw,distance,0)!=0;

    if(!moved)for(float turn:{25.f,-25.f,50.f,-50.f,90.f,-90.f,135.f,-135.f}){

        if(hookWalkMove(e,yaw+turn,distance,0)){yaw+=turn;moved=true;break;}

    }

    if(moved){if(goal.z>140.f)++mansionNavUpperSteps;path.blocked=0;e->v.angles.y=yaw;if(!czIs(e)&&!qzIs(e)){if(running)hordeRunAnimation(e);else hordeWalkAnimation(e);}}

    else if((path.blocked+=dt)>=1.25f){

        if(globals->time>=path.recoveryAt&&mansionTryStuckStep(e,path)){path.blocked=0;path.recoveryAt=globals->time+2.f;}

        else if(path.blocked>=2.5f){
            int from=path.cursor>0?path.nodes[path.cursor-1]:path.start;
            int to=path.cursor<path.nodes.size()?path.nodes[path.cursor]:-1;
            if(from>=0&&to>=0&&from!=to){path.blockedFrom=from;path.blockedTo=to;path.blockedUntil=globals->time+4.f;}
            path.nodes.clear();path.planning=false;path.pending={};path.retryAt=globals->time+.25f;path.blocked=0;
        }

    }

    return false;

}

