// Revive only the tactical NPC, preserving native entity construction and AI.
// The queue uses saved entvars so a save/load cannot lose or duplicate it.
static MansionEntityRef mansionCZRespawnController;
static edict_t* mansionCZRespawnState(bool create=false){
    auto& ref=mansionCZRespawnController;
    if(valid(ref.entity)&&ref.entity->serialnumber==ref.serial)return ref.entity;
    ref.entity=mansionFind("mansion_cz_respawn_state");
    if(!valid(ref.entity)&&create){
        ref.entity=engine.pfnCreateNamedEntity(engine.pfnAllocString("info_target"));
        if(valid(ref.entity)){hookSpawn(ref.entity);ref.entity->v.targetname=engine.pfnAllocString("mansion_cz_respawn_state");}
    }
    ref.serial=valid(ref.entity)?ref.entity->serialnumber:0;
    return ref.entity;
}
static void mansionCZRespawnActivate(){mansionCZRespawnController={};mansionCZRespawnState();}
static int mansionEnemiesRemaining(edict_t* state){
    return mansionAlive()+std::max(0,mansion::total(int(state->v.frags))-int(state->v.armorvalue));
}
static void mansionCZDeath(edict_t* npc){
    if(!mansionMap()||!mansionActor(npc))return;
    auto wave=mansionState();if(!valid(wave)||wave->v.impulse||wave->v.frags<1||wave->v.frags>3)return;
    auto queue=mansionCZRespawnState(true);if(!valid(queue))return;
    if(queue->v.frags!=wave->v.frags){queue->v.frags=wave->v.frags;queue->v.impulse=0;}
    if(queue->v.impulse==2)return;
    mansionAliveAt=-1;
    int remaining=mansionEnemiesRemaining(wave);
    queue->v.impulse=remaining>5?1:2;
    queue->v.dmgtime=remaining>5?globals->time+3.f:0;
    char report[96];std::snprintf(report,sizeof report,"%s remaining=%d wave=%d",remaining>5?"queued":"stopped",remaining,int(wave->v.frags));startupTrace("MansionCZRespawn",report);
}
static void mansionCZRespawnFrame(){
    return; // Special tactical NPC temporarily disabled.
    auto& ref=mansionCZRespawnController;
    if(!valid(ref.entity)||ref.entity->serialnumber!=ref.serial||ref.entity->v.impulse!=1)return;
    auto queue=ref.entity;auto state=mansionState();
    if(!valid(state)||state->v.impulse||queue->v.frags!=state->v.frags){queue->v.impulse=0;return;}
    int remaining=mansionEnemiesRemaining(state);
    if(remaining<=5){queue->v.impulse=2;startupTrace("MansionCZRespawn","stopped remaining<=5");return;}
    if(globals->time<queue->v.dmgtime)return;
    for(int i=1;i<globals->maxEntities;++i){auto e=engine.pfnPEntityOfEntIndex(i);if(mansionActor(e)&&czIs(e)&&aliveEnemy(e)){queue->v.impulse=0;return;}}
    // Reuse the validated farthest-free spawn selection, without advancing
    // the normal wave's spawn counter or changing its composition.
    float count=state->v.armorvalue,cursor=state->v.health,selected=state->v.dmg_save;
    state->v.armorvalue=float(mansion::hordeTotal(int(state->v.frags)));
    bool emitted=mansionEmit(state);
    state->v.armorvalue=count;state->v.health=cursor;state->v.dmg_save=selected;
    if(emitted){queue->v.impulse=0;mansionAliveAt=-1;startupTrace("MansionCZRespawn","revived");}
    else queue->v.dmgtime=globals->time+.5f;
}
