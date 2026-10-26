static constexpr int CZSmokeThrown=1<<28;
static constexpr const char* CZSmokeModel="models/cremation/czero/w_smokegrenade.mdl";
static constexpr const char* CZSmokeSprite="sprites/cremation/czero/smokepuff.spr";
static constexpr int CZFlashThrown=1<<27;
static int czFadeMessage=0;
static bool czSmokeCanLeave(edict_t* e){return !(e->v.spawnflags&(CZSmokeThrown|CZFlashThrown))||globals->time>=e->v.pain_finished;}
static edict_t* czCreateGrenade(edict_t* e,const Vector& destination,bool flash){
    Vector origin=e->v.origin+Vector(0,0,52);auto grenade=engine.pfnCreateNamedEntity(engine.pfnAllocString("info_target"));if(!valid(grenade))return nullptr;
    raven.pfnSpawn(grenade);grenade->v.targetname=engine.pfnAllocString(flash?"cz_flash_grenade":"cz_smoke_grenade");grenade->v.owner=e;
    engine.pfnSetModel(grenade,flash?"models/cremation/czero/w_flashbang.mdl":CZSmokeModel);grenade->v.movetype=MOVETYPE_NONE;grenade->v.solid=SOLID_NOT;
    Vector lo(-3,-3,-3),hi(3,3,3);engine.pfnSetSize(grenade,&lo.x,&hi.x);engine.pfnSetOrigin(grenade,&origin.x);
    float gravity=engine.pfnCVarGetFloat("sv_gravity");if(gravity<=0)gravity=800;
    grenade->v.velocity=(destination-origin)*(1.f/1.2f)+Vector(0,0,.5f*gravity*1.2f);
    grenade->v.dmgtime=globals->time+1.25f;grenade->v.radsuit_finished=globals->time;grenade->v.takedamage=DAMAGE_NO;
    e->v.pain_finished=globals->time+.7f;e->v.impulse=2;e->v.sequence=57;e->v.frame=0;e->v.animtime=globals->time;
    return grenade;
}
static bool czCluster(edict_t* npc,Vector& destination){
    int largest=3;bool found=false;
    for(int i=1;i<globals->maxEntities;++i){auto seed=engine.pfnPEntityOfEntIndex(i);
        if(!aliveEnemy(seed)||seed==npc||!std::strstr(text(seed->v.classname),"zombie"))continue;
        auto p=seed->v.origin;if(p.x< -800||p.x>320||p.y< -864||p.y>352||length(p-npc->v.origin)>768)continue;
        int count=0;Vector sum(0,0,0);
        for(int j=1;j<globals->maxEntities;++j){auto z=engine.pfnPEntityOfEntIndex(j);
            if(z==npc||!aliveEnemy(z)||!std::strstr(text(z->v.classname),"zombie")||length(z->v.origin-p)>144)continue;
            ++count;sum=sum+z->v.origin;}
        if(count>largest){largest=count;destination=sum*(1.f/count)+Vector(0,0,12);found=true;}
    }return found;
}
static bool czThrowSmoke(edict_t* e){
    if(e->v.spawnflags&CZSmokeThrown)return false;Vector destination;
    if(!czCluster(e,destination)||!valid(czCreateGrenade(e,destination,false)))return false;
    e->v.spawnflags|=CZSmokeThrown;startupTrace("CZSmokeThrow","cluster>=4 near mansion");return true;
}
static bool czThrowFlash(edict_t* e,edict_t* player){
    if(!(e->v.spawnflags&CZSmokeThrown)||(e->v.spawnflags&CZFlashThrown)||!valid(player)||!(player->v.flags&FL_CLIENT)||dmcInvisible(player))return false;
    Vector eye=e->v.origin+e->v.view_ofs,to=center(player);TraceResult visible{};engine.pfnTraceLine(&eye.x,&to.x,0,e,&visible);if(visible.flFraction<.999f&&visible.pHit!=player)return false;
    if(!valid(czCreateGrenade(e,player->v.origin+Vector(0,0,24),true)))return false;
    e->v.spawnflags|=CZFlashThrown;startupTrace("CZFlashThrow","visible player after smoke");return true;
}
static void czFlashDetonate(edict_t* grenade){
    engine.pfnEmitSound(grenade,CHAN_ITEM,"cremation/czero/flashbang-1.wav",1,ATTN_NORM,0,100);
    for(int i=1;i<=globals->maxClients;++i){auto p=engine.pfnPEntityOfEntIndex(i);if(!valid(p)||p->v.health<=0||!czFadeMessage)continue;
        Vector eye=p->v.origin+p->v.view_ofs,delta=grenade->v.origin-eye;float distance=length(delta);if(distance>1024)continue;
        TraceResult tr{};engine.pfnTraceLine(&eye.x,&grenade->v.origin.x,1,p,&tr);if(tr.flFraction<.99f)continue;
        Vector look=linkDirection(p);float front=dot(look,normal(delta));float strength=(front>.5f?1.f:front>-.3f?.6f:.25f)*(1.f-distance/1400.f);
        engine.pfnMessageBegin(MSG_ONE,czFadeMessage,nullptr,p);engine.pfnWriteShort(int(4.f*strength*4096));engine.pfnWriteShort(int(strength*4096));engine.pfnWriteShort(0);
        for(int c:{255,255,255,int(255*strength)})engine.pfnWriteByte(c);engine.pfnMessageEnd();startupTrace("CZFlashBlinded","line of sight player");
    }
}
static bool czSmokeBlocks(const Vector& a,const Vector& b){
    Vector delta=b-a;float lengthSquared=dot(delta,delta);if(lengthSquared<1)return false;
    for(int i=1;i<globals->maxEntities;++i){auto e=engine.pfnPEntityOfEntIndex(i);
        if(!valid(e)||std::strcmp(text(e->v.targetname),"cz_smoke_cloud")||globals->time>=e->v.dmgtime)continue;
        Vector centre=e->v.origin+Vector(0,0,64);float t=std::max(0.f,std::min(1.f,dot(centre-a,delta)/lengthSquared));
        if(length(a+delta*t-centre)<150)return true;
    }return false;
}
static void czSmokeFrame(){
    for(int i=1;i<globals->maxEntities;++i){auto e=engine.pfnPEntityOfEntIndex(i);if(!valid(e))continue;const char* name=text(e->v.targetname);
        if(!std::strcmp(name,"cz_smoke_grenade")||!std::strcmp(name,"cz_flash_grenade")){
            // Sweep against map geometry, not allied bodies: a crowded spawn
            // must not bounce the single cover grenade back onto its owner.
            float elapsed=std::min(.15f,std::max(0.f,globals->time-e->v.radsuit_finished));e->v.radsuit_finished=globals->time;
            int steps=std::max(1,int(std::ceil(elapsed/.01f)));float dt=elapsed/steps;
            float gravity=engine.pfnCVarGetFloat("sv_gravity");if(gravity<=0)gravity=800;
            for(int step=0;step<steps;++step){e->v.velocity.z-=gravity*dt;Vector next=e->v.origin+e->v.velocity*dt;TraceResult tr{};
                engine.pfnTraceLine(&e->v.origin.x,&next.x,1,e,&tr);
                if(tr.flFraction<1){next=tr.vecEndPos+tr.vecPlaneNormal*3;float into=dot(e->v.velocity,tr.vecPlaneNormal);
                    e->v.velocity=(e->v.velocity-tr.vecPlaneNormal*into)*.7f-tr.vecPlaneNormal*(into*.35f);
                }engine.pfnSetOrigin(e,&next.x);e->v.angles=e->v.angles+Vector(180,240,80)*dt;
            }
            if(globals->time<e->v.dmgtime)continue;
            if(!std::strcmp(name,"cz_flash_grenade")){czFlashDetonate(e);engine.pfnRemoveEntity(e);continue;}
            auto cloud=engine.pfnCreateNamedEntity(engine.pfnAllocString("info_target"));if(valid(cloud)){raven.pfnSpawn(cloud);cloud->v.targetname=engine.pfnAllocString("cz_smoke_cloud");
                cloud->v.solid=SOLID_NOT;cloud->v.movetype=MOVETYPE_NONE;cloud->v.dmgtime=globals->time+18;cloud->v.teleport_time=globals->time;engine.pfnSetOrigin(cloud,&e->v.origin.x);
                engine.pfnEmitSound(cloud,CHAN_ITEM,"cremation/czero/sg_explode.wav",1,ATTN_NORM,0,100);
                for(int j=0;j<16;++j){auto puff=engine.pfnCreateNamedEntity(engine.pfnAllocString("info_target"));if(!valid(puff))continue;raven.pfnSpawn(puff);
                    puff->v.targetname=engine.pfnAllocString("cz_smoke_puff");puff->v.owner=cloud;puff->v.skin=j;puff->v.dmgtime=cloud->v.dmgtime;
                    puff->v.solid=SOLID_NOT;puff->v.movetype=MOVETYPE_NONE;engine.pfnSetModel(puff,CZSmokeSprite);
                    puff->v.rendermode=kRenderTransTexture;puff->v.rendercolor=Vector(185,185,185);puff->v.scale=5.5f+(j%3)*.5f;
                }
                char row[100];std::snprintf(row,sizeof row,"at=%.1f,%.1f,%.1f",cloud->v.origin.x,cloud->v.origin.y,cloud->v.origin.z);startupTrace("CZSmokeDetonate",row);
            }engine.pfnRemoveEntity(e);
        }else if(!std::strcmp(name,"cz_smoke_cloud")){if(globals->time>=e->v.dmgtime)engine.pfnRemoveEntity(e);
        }else if(!std::strcmp(name,"cz_smoke_puff")){
            if(!valid(e->v.owner)||globals->time>=e->v.dmgtime){engine.pfnRemoveEntity(e);continue;}
            auto cloud=e->v.owner;float age=globals->time-cloud->v.teleport_time,angle=e->v.skin*2.39996f;
            Vector at=cloud->v.origin+Vector(std::cos(angle)*(e->v.skin%3)*43,std::sin(angle)*(e->v.skin%3)*43,20+(e->v.skin%4)*33+std::sin(age+e->v.skin)*8);
            engine.pfnSetOrigin(e,&at.x);e->v.frame=std::fmod(age*6+e->v.skin,10.f);
            e->v.renderamt=std::min(245.f,std::min(age*450,(cloud->v.dmgtime-globals->time)*180));
        }
    }
}
