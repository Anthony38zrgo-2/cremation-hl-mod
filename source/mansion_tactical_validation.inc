// Explicit HLDS fixture for Mansion sensing, siege, combat and navigation.
static void mansionTacticalProbe(float dt){
 static int stage=0;static edict_t* actor=nullptr;static edict_t* player=nullptr;static edict_t* blockers[2]{};
 static float since=0;static Vector seen{0,0,0},groupStart{0,0,0};static bool sight=false,memory=false,expired=false,doorHit=false,combat=false,autoCombat=false,closedDoor=false,stairs=false,playerPriority=false,bridge=false;
 auto state=mansionState();if(!valid(state)||stage==99)return;
 auto fail=[](const char* why){startupTrace("MansionTacticsProbe FAIL",why);stage=99;};
 if(stage==0){
  player=engine.pfnCreateFakeClient("MansionTacticsProbe");if(!valid(player)){fail("fake client");return;}
  char reject[128]{};if(raven.pfnClientConnect)raven.pfnClientConnect(player,"MansionTacticsProbe","127.0.0.1",reject);
  raven.pfnClientPutInServer(player);player->v.health=1000;player->v.armorvalue=0;Vector initialPlayer(1400,0,60.1f);engine.pfnSetOrigin(player,&initialPlayer.x);
  state->v.frags=1;state->v.armorvalue=1;state->v.health=0;
  if(!mansionEmit(state)){fail("spawn zombie");return;}
  actor=mansionFind("mansion_enemy_johnny_1_01");if(!aliveEnemy(actor)){fail("zombie invalid");return;}
  actor->v.flags|=FL_ONGROUND;bool startsForward=mansionTargetStarts(actor,"mansion_route_00_01");actor->v.target=engine.pfnAllocString("mansion_route_00_00");inMansionMove=true;mansionRouteMarkerMove(actor,.05f);inMansionMove=false;
  bool skipsSpawn=mansionTargetStarts(actor,"mansion_route_00_01");Vector bridgePlayer=actor->v.origin+Vector(100,-16,36);engine.pfnSetOrigin(player,&bridgePlayer.x);
  float before=length2d(player->v.origin-actor->v.origin);inMansionMove=true;mansionPursuePlayer(actor,player,.1f);inMansionMove=false;
  bridge=startsForward&&skipsSpawn&&length2d(player->v.origin-actor->v.origin)<before;
  {char row[160];std::snprintf(row,sizeof row,"start=%d skip=%d see=%d before=%.1f after=%.1f flags=%d",int(startsForward),int(skipsSpawn),int(johnnyCanSee(actor,player)),before,length2d(player->v.origin-actor->v.origin),int(actor->v.flags));startupTrace("MansionBridgeProbe",row);}
  Vector obstructedStart(-752,1920,24.1f),obstructedPlayer(-496,1408,60.1f);engine.pfnSetOrigin(actor,&obstructedStart.x);engine.pfnSetOrigin(player,&obstructedPlayer.x);actor->v.target=engine.pfnAllocString("mansion_route_24_01");
  auto routePoint=mansionFind("mansion_route_24_01");float routeBefore=valid(routePoint)?length2d(actor->v.origin-routePoint->v.origin):0;
  bool blocked=!mansionBridgePursuitClear(actor,player);inMansionMove=true;mansionPursuePlayer(actor,player,.1f);inMansionMove=false;
  bool safeRoute=valid(routePoint)&&length2d(actor->v.origin-routePoint->v.origin)<routeBefore;
  bridge=bridge&&blocked&&safeRoute;startupTrace(blocked&&safeRoute?"MansionBridgeObstacle PASS":"MansionBridgeObstacle FAIL","blocked shortcut follows authored route");
  Vector outsideRange=actor->v.origin+Vector(121,0,36);engine.pfnSetOrigin(player,&outsideRange.x);
  startupTrace(!johnnyCanSee(actor,player)?"MansionVisionRange PASS":"MansionVisionRange FAIL","player beyond 120 units is not detected");
  auto npc=engine.pfnCreateNamedEntity(engine.pfnAllocString(CZClass));auto scientist=engine.pfnCreateNamedEntity(engine.pfnAllocString("monster_scientist"));bool npcPolicy=false;
  if(valid(npc)&&valid(scientist)){npc->v.origin=Vector(-550,-300,192.1f);hookSpawn(npc);npc->v.targetname=engine.pfnAllocString("mansion_enemy_cz_room_probe");scientist->v.origin=Vector(-624,-176,192.1f);hookSpawn(scientist);
   auto oldScientist=mansionScientist;bool oldInvaded=mansionInvaded;mansionScientist={scientist,scientist->serialnumber,1,true};mansionInvaded=true;
   Vector npcAt(-550,-300,192.1f),playerAt(-600,-250,228.1f);engine.pfnSetOrigin(npc,&npcAt.x);engine.pfnSetOrigin(player,&playerAt.x);
   bool detected=mansionCZRoomPlayer(npc)==player&&mansionPriorityPlayer(npc)==nullptr;mansionEventMove(npc,.05f);bool pistol=npc->v.iuser1==5&&npc->v.button<20;
   Vector outside(-300,-250,228.1f);engine.pfnSetOrigin(player,&outside.x);bool cleared=mansionCZRoomPlayer(npc)==nullptr;mansionEventMove(npc,.05f);npcPolicy=detected&&pistol&&cleared&&npc->v.iuser1==0;
   mansionScientist=oldScientist;mansionInvaded=oldInvaded;}
  if(valid(npc))engine.pfnRemoveEntity(npc);if(valid(scientist))engine.pfnRemoveEntity(scientist);
  startupTrace(npcPolicy?"MansionCZRoomPolicy PASS":"MansionCZRoomPolicy FAIL","pistol in room, knife outside, no player pursuit");
  Vector at(224,-256,24.1f),p(320,-256,60.1f);engine.pfnSetOrigin(actor,&at.x);engine.pfnSetOrigin(player,&p.x);
  actor->v.angles.y=0;since=globals->time;stage=1;
 }
 if(stage==1){
  inMansionMove=true;mansionNavBeginTick();mansionTacticalMove(actor,dt);mansionNavFinishTick();inMansionMove=false;
  int i=engine.pfnIndexOfEdict(actor);auto& sense=mansionTacticalSenses[i];
  if(sense.target==player){sight=true;seen=sense.lastSeen;bool wasInvaded=mansionInvaded;mansionInvaded=true;mansionRaiders[0]={actor,actor->serialnumber};mansionRaidAttacks[0].active=true;
   bool assigned=mansionPriorityPlayer(actor)==player;inMansionMove=true;mansionNavBeginTick();mansionPursuePlayer(actor,player,dt);mansionNavFinishTick();inMansionMove=false;
   auto runner=davidState(actor);bool runningFar=runner&&runner->running;Vector savedPlayer=player->v.origin,closePoint=actor->v.origin+Vector(69,0,36);engine.pfnSetOrigin(player,&closePoint.x);mansionPursuePlayer(actor,player,dt);bool walkingNear=runner&&!runner->running;engine.pfnSetOrigin(player,&savedPlayer.x);
   startupTrace(runningFar&&walkingNear?"MansionRunRange PASS":"MansionRunRange FAIL","running starts at 70 units");
   playerPriority=assigned&&!mansionRaidAttacks[0].active&&mansionEventThink(actor);mansionRaiders[0]={};mansionInvaded=wasInvaded;
   Vector behindWall(-128,-400,60.1f);engine.pfnSetOrigin(player,&behindWall.x);playerPriority=playerPriority&&mansionPriorityPlayer(actor)==player;since=globals->time;stage=2;}
  else if(globals->time-since>3.f){fail("player not detected");return;}
 }
 if(stage==2){
  inMansionMove=true;mansionNavBeginTick();mansionTacticalMove(actor,dt);mansionNavFinishTick();inMansionMove=false;
  auto& sense=mansionTacticalSenses[engine.pfnIndexOfEdict(actor)];float elapsed=globals->time-since;
  if(elapsed>.3f&&elapsed<1.5f&&!valid(czEnemy(actor))&&sense.target==player&&length(sense.lastSeen-seen)<1.f)memory=true;
  if(elapsed>2.4f){expired=sense.target==nullptr;Vector doorStart(64,-256,56.1f);engine.pfnSetOrigin(actor,&doorStart.x);since=globals->time;stage=30;}
 }
 if(stage==30){
  inMansionMove=true;mansionNavBeginTick();mansionNavWalk(actor,Vector(-64,-256,56.1f),dt,55.f,false);mansionNavFinishTick();inMansionMove=false;
  if(globals->time-since<1.2f)return;
  closedDoor=actor->v.origin.x>0.f&&mansionDoorHealth(1)>0;stage=3;
 }
 if(stage==3){
  Vector at(64,-256,56.1f);engine.pfnSetOrigin(actor,&at.x);float before=mansionDoorHealth(1);mansionStrikeDoor(actor,1);doorHit=mansionDoorHealth(1)<before;
  Vector from(224,-256,24.1f),victim(266,-256,60.1f);engine.pfnSetOrigin(actor,&from.x);engine.pfnSetOrigin(player,&victim.x);actor->v.angles.y=0;player->v.health=1000;player->v.armorvalue=0;
  davidActivity(actor->pvPrivateData,nullptr,30);MonsterEvent_t ev{};ev.event=1;auto table=*reinterpret_cast<void***>(actor->pvPrivateData);reinterpret_cast<void(__thiscall*)(void*,MonsterEvent_t*)>(table[ravenholm_abi::slot::HandleAnimEvent])(actor->pvPrivateData,&ev);combat=player->v.health<1000;
  player->v.health=1000;player->v.velocity=Vector(0,0,0);player->v.movetype=MOVETYPE_NONE;
  davidActivity(actor->pvPrivateData,nullptr,1);actor->v.nextthink=globals->time+.05f;mansionRaiders[0]={actor,actor->serialnumber};mansionRaidAttacks[0]={};since=globals->time;stage=31;
 }
 if(stage==31){
  inMansionMove=true;mansionNavBeginTick();mansionPursuePlayer(actor,player,dt);mansionNavFinishTick();inMansionMove=false;
  if(player->v.health<1000.f)autoCombat=true;
  if(!autoCombat&&globals->time-since<8.f)return;
  {char row[180];std::snprintf(row,sizeof row,"auto actor=%.0f,%.0f,%.0f player=%.0f,%.0f,%.0f sequence=%d frame=%.0f enemy=%d next=%.1f",actor->v.origin.x,actor->v.origin.y,actor->v.origin.z,player->v.origin.x,player->v.origin.y,player->v.origin.z,actor->v.sequence,actor->v.frame,valid(actor->v.enemy),actor->v.nextthink-globals->time);startupTrace("MansionTacticsPosition",row);}
  Vector farPoint(1400,0,60.1f),foot(-224,-224,64.1f);engine.pfnSetOrigin(player,&farPoint.x);engine.pfnSetOrigin(actor,&foot.x);actor->v.sequence=1;actor->v.frame=0;since=globals->time;stage=4;
 }
 if(stage==4){
  inMansionMove=true;mansionNavBeginTick();mansionNavWalk(actor,Vector(-224,8,192.1f),dt,55.f,false);mansionNavFinishTick();inMansionMove=false;
  if(actor->v.origin.z>190.f&&actor->v.origin.y>=6.f&&actor->v.mins.x<-12.f){stairs=true;stage=5;}
  else if(globals->time-since>30.f)stage=5;
  if(stage==5){
   char row[160];std::snprintf(row,sizeof row,"stairs at %.1f %.1f %.1f goal=%.1f",actor->v.origin.x,actor->v.origin.y,actor->v.origin.z,length(actor->v.origin-Vector(-224,8,192.1f)));startupTrace("MansionTacticsPosition",row);mansionNavReport();
   for(int i=0;i<2;++i){if(!mansionEmit(state)){fail("group spawn");return;}char name[64];std::snprintf(name,sizeof name,"mansion_enemy_johnny_1_%02d",i+2);blockers[i]=mansionFind(name);if(!aliveEnemy(blockers[i])){fail("group invalid");return;}}
   Vector a(800,-288,24.1f),b(840,-288,24.1f),c(880,-288,24.1f);engine.pfnSetOrigin(actor,&a.x);engine.pfnSetOrigin(blockers[0],&b.x);engine.pfnSetOrigin(blockers[1],&c.x);
   actor->v.sequence=1;actor->v.frame=0;for(auto z:blockers){z->v.sequence=1;z->v.frame=0;}groupStart=actor->v.origin;since=globals->time;
  }
 }
 if(stage==5){
  inMansionMove=true;mansionNavBeginTick();for(auto z:{actor,blockers[0],blockers[1]})mansionNavWalk(z,Vector(1100,-288,24.1f),dt,55.f,false);mansionNavFinishTick();inMansionMove=false;
  if(globals->time-since<8.f)return;
  bool group=actor->v.origin.x-groupStart.x>32.f;
  bool nearRecovery=mansionNearRecoveryProbe(actor,player);
  startupTrace(nearRecovery?"MansionNearRecovery PASS":"MansionNearRecovery FAIL","player within 640; checked route and free destination");
  char row[160];std::snprintf(row,sizeof row,"group at %.1f %.1f %.1f moved=%.1f",actor->v.origin.x,actor->v.origin.y,actor->v.origin.z,length2d(actor->v.origin-groupStart));startupTrace("MansionTacticsPosition",row);for(auto z:blockers){std::snprintf(row,sizeof row,"blocker %.1f %.1f %.1f flags=%d",z->v.origin.x,z->v.origin.y,z->v.origin.z,int(z->v.flags));startupTrace("MansionTacticsPosition",row);}mansionNavReport();
  char detail[240];std::snprintf(detail,sizeof detail,"%s sight=%d memory=%d expired=%d door=%d closed=%d combat=%d auto=%d stairs=%d group=%d priority=%d bridge=%d",sight&&memory&&expired&&doorHit&&closedDoor&&combat&&autoCombat&&stairs&&group&&playerPriority&&bridge?"PASS":"FAIL",int(sight),int(memory),int(expired),int(doorHit),int(closedDoor),int(combat),int(autoCombat),int(stairs),int(group),int(playerPriority),int(bridge));startupTrace("MansionTacticsProbe",detail);stage=99;
 }
}
