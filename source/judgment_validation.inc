// Real server fixture: exercise locomotion and real virtual damage, rather
// than force-removing the pair as the general wave-completion smoke does.
#include "judgment_probe_poses.h"
static bool judgmentProbeBullet(edict_t* e,edict_t* player,int bone,bool apply=true,const Vector* modelPoint=nullptr){
    Vector point,ignored(0,0,0);
    if(modelPoint){Vector forward,right,up;engine.pfnAngleVectors(&e->v.angles.x,&forward.x,&right.x,&up.x);point=e->v.origin+forward*modelPoint->x-right*modelPoint->y+up*modelPoint->z;}
    else engine.pfnGetBonePosition(e,bone,&point.x,&ignored.x);
    Vector from=point+Vector(200,0,0),to=point-Vector(40,0,0);TraceResult tr{};engine.pfnTraceLine(&from.x,&to.x,0,player,&tr);
    if(!apply){if(tr.pHit!=e){char miss[280];std::snprintf(miss,sizeof miss,"seq=%d frame=%.0f yaw=%.0f bone=%d hit=%s fraction=%.2f startsolid=%d aim=%.1f,%.1f,%.1f",e->v.sequence,e->v.frame,e->v.angles.y,bone,valid(tr.pHit)?text(tr.pHit->v.classname):"none",tr.flFraction,tr.fStartSolid,point.x,point.y,point.z);startupTrace("JudgmentProbeRayMiss",miss);}return tr.pHit==e;}
    char report[320];std::snprintf(report,sizeof report,"class=%s bone=%d hit=%s fraction=%.3f box=%.0f,%.0f,%.0f/%.0f,%.0f,%.0f aim=%.0f,%.0f,%.0f",text(e->v.classname),bone,valid(tr.pHit)?text(tr.pHit->v.classname):"none",tr.flFraction,e->v.absmin.x,e->v.absmin.y,e->v.absmin.z,e->v.absmax.x,e->v.absmax.y,e->v.absmax.z,point.x,point.y,point.z);startupTrace("JudgmentProbeBullet",report);
    if(tr.pHit!=e)return false;
    auto pendingEntity=reinterpret_cast<void**>(ravenholm_abi::address(ravenModule,ravenholm_abi::MultiDamageEntityOffset));auto pendingDamage=reinterpret_cast<float*>(ravenholm_abi::address(ravenModule,ravenholm_abi::MultiDamageAmountOffset));auto pendingBits=reinterpret_cast<int*>(ravenholm_abi::address(ravenModule,ravenholm_abi::MultiDamageBitsOffset));
    *pendingEntity=nullptr;*pendingDamage=0;*pendingBits=0;float before=e->v.fuser4;auto table=*reinterpret_cast<void***>(e->pvPrivateData);
    reinterpret_cast<void(__thiscall*)(void*,entvars_t*,float,Vector,TraceResult*,int)>(table[ravenholm_abi::slot::TraceAttack])(e->pvPrivateData,&player->v,10,normal(to-from),&tr,1<<1);
    float amount=*pendingDamage;int bits=*pendingBits;void* target=*pendingEntity;*pendingEntity=nullptr;*pendingDamage=0;*pendingBits=0;
    if(target==e->pvPrivateData&&amount>0)reinterpret_cast<TakeDamageFn>(table[ravenholm_abi::slot::TakeDamage])(e->pvPrivateData,&player->v,&player->v,amount,bits);
    std::snprintf(report,sizeof report,"pending_damage=%.1f shared_before=%.1f shared_after=%.1f",amount,before,e->v.fuser4);startupTrace("JudgmentProbeBulletDamage",report);
    return e->v.fuser4<before;
}
static bool judgmentProbeObstacles(edict_t* zeal){
    Vector saved=zeal->v.origin,heading=zeal->v.vuser1,angles=zeal->v.angles,wp1=zeal->v.vuser2,wp2=zeal->v.vuser3;float nav=zeal->v.fuser3,until=zeal->v.pain_finished;Vector start(400,-500,100),across(-900,-500,100),inside;engine.pfnSetOrigin(zeal,&start.x);zeal->v.vuser1=Vector(0,0,0);zeal->v.fuser3=0;
    TraceResult tr{};bool barrier=!judgmentFlightTrace(zeal,start,across,tr),clear=judgmentFlightOpen(zeal,start);int blocked=0;
    inside=Vector(tr.vecEndPos)+normal(across-start)*32.f;Vector resolved=judgmentFlightGoal(zeal,inside);bool resolvedOpen=judgmentFlightOpen(zeal,resolved)&&length(resolved-inside)>8;
    for(int i=0;i<800;++i){judgmentFly(zeal,across,180.f,.05f);if(!judgmentFlightOpen(zeal,zeal->v.origin))++blocked;}
    float traveled=length(zeal->v.origin-start),remaining=length(zeal->v.origin-across);Vector end=zeal->v.origin;TraceResult navTrace{};bool navClear=judgmentFlightTrace(zeal,end,zeal->v.vuser3,navTrace);char navLine[260];std::snprintf(navLine,sizeof navLine,"stage=%.0f first=%.0f,%.0f,%.0f second=%.0f,%.0f,%.0f goal_open=%d segment_clear=%d fraction=%.3f",zeal->v.fuser3,zeal->v.vuser2.x,zeal->v.vuser2.y,zeal->v.vuser2.z,zeal->v.vuser3.x,zeal->v.vuser3.y,zeal->v.vuser3.z,judgmentFlightOpen(zeal,across),navClear,navTrace.flFraction);startupTrace("JudgmentProbeNav",navLine);
    Vector ceilingStart(1800,-256,780),ceilingTarget(1800,-256,940);engine.pfnSetOrigin(zeal,&ceilingStart.x);zeal->v.fuser3=0;
    bool ceiling=!judgmentFlightTrace(zeal,ceilingStart,ceilingTarget,tr);
    for(int i=0;i<60;++i){judgmentFly(zeal,ceilingTarget,180.f,.05f);if(!judgmentFlightOpen(zeal,zeal->v.origin))++blocked;}
    char line[240];std::snprintf(line,sizeof line,"wall=%d ceiling=%d blocked_samples=%d traveled=%.1f remaining=%.1f end=%.1f,%.1f,%.1f resolved_solid_goal=%d",barrier,ceiling,blocked,traveled,remaining,end.x,end.y,end.z,resolvedOpen);startupTrace("JudgmentProbeObstacles",line);
    engine.pfnSetOrigin(zeal,&saved.x);zeal->v.vuser1=heading;zeal->v.angles=angles;zeal->v.vuser2=wp1;zeal->v.vuser3=wp2;zeal->v.fuser3=nav;zeal->v.pain_finished=until;
    return clear&&barrier&&ceiling&&resolvedOpen&&blocked==0&&traveled>60.f&&remaining<64.f;
}
static void judgmentProbeFrame(float dt){
    static int stage=0,failures=0;static float began=0,minAir=1.e9f,maxAir=-1.e9f;static edict_t *k=nullptr,*z=nullptr,*p=nullptr;static Vector start,soloStart;
    auto check=[&](bool pass,const char* detail){startupTrace(pass?"JudgmentProbePASS":"JudgmentProbeFAIL",detail);if(!pass)++failures;};
    auto hit=[&](edict_t* e){auto table=*reinterpret_cast<void***>(e->pvPrivateData);reinterpret_cast<TakeDamageFn>(table[ravenholm_abi::slot::TakeDamage])(e->pvPrivateData,&p->v,&p->v,10,1<<1);};
    if(stage==0){
        p=engine.pfnCreateFakeClient("JudgmentProbe");if(!valid(p)){check(false,"fake_client");stage=99;return;}
        char reject[128]{};if(raven.pfnClientConnect)raven.pfnClientConnect(p,"JudgmentProbe","127.0.0.1",reject);raven.pfnClientPutInServer(p);
        p->v.health=10000;p->v.takedamage=DAMAGE_NO;Vector pos(-304,-272,92);engine.pfnSetOrigin(p,&pos.x);
        auto state=mansionState();state->v.frags=4;state->v.armorvalue=80;
        auto blocker=engine.pfnCreateNamedEntity(engine.pfnAllocString("monster_zombie"));blocker->v.origin=Vector(1824,100,24);hookSpawn(blocker);blocker->v.targetname=engine.pfnAllocString("mansion_enemy_boss_gate_probe");
        check(!mansionEmit(state)&&judgmentCurrentHealth()==0,"boss_waits_for_last_horde_enemy");engine.pfnRemoveEntity(blocker);
        if(!judgmentSpawnPair(Vector(1824,-256,24.1f),Vector(0,180,0),4,80,29)){check(false,"spawn_pair");stage=99;return;}
        for(int i=1;i<globals->maxEntities;++i){auto e=engine.pfnPEntityOfEntIndex(i);if(judgmentKuarl(e))k=e;if(judgmentZeal(e))z=e;}
        check(valid(k)&&valid(z),"two_custom_models");if(!valid(k)||!valid(z)){stage=99;return;}
        check(k->v.fuser4==1500&&z->v.fuser4==1500,"shared_hp_1500");check(k->v.maxs.z==280.f,"kuarl_scaled_model_and_hull_2_5");check(mansionRouteActive(k),"hidden_player_uses_approach_route");
        start=k->v.origin;began=globals->time;stage=1;
    }
    if(stage==99)return;
    mansionMove(k,std::min(dt,.1f));judgmentFrame(std::min(dt,.1f));
    if(stage<=2){float air=z->v.origin.z-k->v.origin.z;minAir=std::min(minAir,air);maxAir=std::max(maxAir,air);}
    if(stage==1&&globals->time-began>=3.f){
        Vector right, left, angles;engine.pfnGetBonePosition(k,11,&right.x,&angles.x);engine.pfnGetBonePosition(k,14,&left.x,&angles.x);
        float feet=std::min(right.z,left.z)-k->v.origin.z;
        check(length2d(k->v.origin-start)>100.f,"kuarl_physically_leaves_spawn_along_route");
        check(std::fabs(k->v.origin.z-24.f)<2.f&&std::fabs(feet)<35.f,"animated_feet_match_grounded_hull");
        check(minAir>210.f&&maxAir-minAir>8.f&&z->v.frame>0,"zeal_flies_above_kuarl_and_animates");
        char report[220];std::snprintf(report,sizeof report,"route_distance=%.1f kuarl_z=%.1f feet=%.1f zeal_air=%.1f..%.1f flags=%d",length2d(k->v.origin-start),k->v.origin.z,feet,minAir,maxAir,int(k->v.flags));startupTrace("JudgmentProbeMovement",report);
        check(judgmentProbeBullet(z,p,1),"real_bullet_trace_hits_zeal_body_and_damages_shared_hp");judgmentSetHp(k,1500);
        check(judgmentProbeBullet(z,p,17),"real_bullet_trace_hits_zeal_wing_and_damages_shared_hp");judgmentSetHp(k,1500);
        check(judgmentProbeBullet(k,p,1),"real_bullet_trace_hits_kuarl_and_damages_armor");judgmentSetHp(k,1500);
        int samples=0,hits=0;int seq=z->v.sequence;float frame=z->v.frame;Vector ang=z->v.angles;
        for(const auto& pose:judgmentProbePoses)for(int yaw:{0,90,180,270}){z->v.sequence=pose.sequence;z->v.frame=pose.frame;z->v.animtime=globals->time;z->v.angles.y=float(yaw);engine.pfnSetOrigin(z,&z->v.origin.x);
            const int bones[]={1,2,19,22};for(int part=0;part<4;++part){++samples;Vector point(pose.points[part][0],pose.points[part][1],pose.points[part][2]);if(judgmentProbeBullet(z,p,bones[part],false,&point))++hits;}}
        z->v.sequence=seq;z->v.frame=frame;z->v.angles=ang;engine.pfnSetOrigin(z,&z->v.origin.x);
        std::snprintf(report,sizeof report,"animated_zeal_rays=%d/%d",hits,samples);startupTrace("JudgmentProbeHitCoverage",report);check(hits==samples,"body_head_wings_are_hittable_across_flight_poses");
        Vector pos=k->v.origin+Vector(-350,0,36);engine.pfnSetOrigin(p,&pos.x);k->v.target=0;start=k->v.origin;began=globals->time;stage=2;
    }
    if(stage==2&&globals->time-began>=2.f){
        check(judgmentProbeObstacles(z),"flight_sweeps_whole_body_avoids_wall_ceiling_and_solid_goals");
        check(length2d(k->v.origin-start)>70.f,"kuarl_chases_visible_player");hit(k);check(k->v.fuser4==1499.f&&z->v.fuser4==1499.f,"armored_hit_costs_one_shared_hp");judgmentSetHp(k,1500);
        hit(z);check(z->v.fuser4==1470.f&&k->v.fuser4==1470.f,"zeal_hit_costs_thirty_shared_hp");
        for(int i=1;i<35;++i)hit(z);
        check(k->v.deadflag!=DEAD_NO&&z->v.deadflag==DEAD_NO&&z->v.fuser4==450.f&&z->v.iuser1==1,"kuarl_falls_at_450_and_zeal_is_alive");
        soloStart=z->v.origin;began=globals->time;stage=3;
    }
    if(stage==3&&globals->time-began>=3.f){
        check(length(z->v.origin-soloStart)>50.f&&z->v.origin.z>50.f,"solo_zeal_moves_and_stays_airborne");
        int lethalHits=0;for(int i=0;i<15;++i)if(judgmentProbeBullet(z,p,1))++lethalHits;
        check(lethalHits==15,"actual_bullets_kill_solo_zeal");check(!aliveEnemy(k)&&!aliveEnemy(z)&&z->v.fuser4==0,"both_defeated_without_blocking_wave");
        began=globals->time;stage=4;
    }
    if(stage==4&&globals->time-began>=1.f){check(z->v.frame>20.f,"zeal_death_animation_advances");char report[64];std::snprintf(report,sizeof report,"failures=%d",failures);startupTrace("JudgmentProbeComplete",report);stage=99;}
}
