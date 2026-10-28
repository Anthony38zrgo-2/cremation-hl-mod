// Incremental horde navigation: one shared budget, cached world traces and
// delayed retries. No whole-edict scans are allowed inside graph traversal.
#include <chrono>
using MansionNavClock=std::chrono::steady_clock;
using MansionNavItem=std::pair<float,int>;
struct MansionNavPath{
    edict_t* actor=nullptr;int serial=0;std::vector<int> nodes;size_t cursor=0;
    Vector goal{0,0,0};float retryAt=0,blocked=0;bool requested=false,planning=false;
    int finish=-1,node=-1,edge=0;
    std::vector<float> costs;std::vector<int> previous;
    std::priority_queue<MansionNavItem,std::vector<MansionNavItem>,std::greater<MansionNavItem>> pending;
};
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
    mansionNavPaths.clear();mansionNavEdges.assign(sizeof(czNavEdges)/sizeof(czNavEdges[0]),{});
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
        mansionNavPaths.erase(std::remove_if(mansionNavPaths.begin(),mansionNavPaths.end(),[](const MansionNavPath& p){
            return !valid(p.actor)||p.actor->serialnumber!=p.serial||!aliveEnemy(p.actor);
        }),mansionNavPaths.end());
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
static int mansionNearestNav(const Vector& p){
    int best=-1;float distance=1e30f;
    for(int i=0;i<CZNavCount;++i){Vector d=czNavPoint(i)-p;float squared=d.x*d.x+d.y*d.y+d.z*d.z;if(squared<distance){distance=squared;best=i;}}
    return best;
}
static void mansionNavEndPlan(MansionNavPath& path,bool found){
    path.planning=false;path.node=-1;path.cursor=0;
    if(found){
        int n=path.finish;
        for(int count=0;n>=0&&count<CZNavCount;++count){path.nodes.push_back(n);n=path.previous[n];}
        std::reverse(path.nodes.begin(),path.nodes.end());++mansionNavCompleted;if(path.goal.z>140.f)++mansionNavUpperCompleted;
    }else{++mansionNavFailed;path.retryAt=globals->time+2.5f+float(engine.pfnIndexOfEdict(path.actor)%17)*.025f;}
    path.costs.clear();path.previous.clear();path.pending={};
}
static void mansionPlanNav(edict_t* e,MansionNavPath& path,const Vector& goal){
    const auto began=MansionNavClock::now();
    auto budget=[&](){return mansionNavEdgesLeft>0&&mansionNavWorkMs+std::chrono::duration<double,std::milli>(MansionNavClock::now()-began).count()<MansionNavTimeBudgetMs;};
    if(!budget())return;
    if(!path.planning){
        int start=mansionNearestNav(e->v.origin);path.finish=mansionNearestNav(goal);
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
static bool mansionNavWalk(edict_t* e,const Vector& goal,float dt,float speed){
    if(!valid(e)||!aliveEnemy(e)||CZNavCount<=0)return false;
    auto& path=mansionNavPath(e);
    if(!path.requested||length(path.goal-goal)>8.f){
        path.nodes.clear();path.cursor=0;path.goal=goal;path.retryAt=0;path.blocked=0;
        path.requested=true;path.planning=false;path.pending={};
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
    if(moved){if(goal.z>140.f)++mansionNavUpperSteps;path.blocked=0;e->v.angles.y=yaw;if(!czIs(e)&&!qzIs(e))hordeWalkAnimation(e);}
    else if((path.blocked+=dt)>=1.f){path.nodes.clear();path.retryAt=globals->time+.75f;path.blocked=0;}
    return false;
}
