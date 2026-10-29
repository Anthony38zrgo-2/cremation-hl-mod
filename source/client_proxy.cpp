#include <windows.h>
#include <cmath>
#include <GL/gl.h>
#include <cstring>
#include <cstdio>
#include <string>
#include <algorithm>
#include <unordered_map>
#include "wrect.h"
#include "cl_dll.h"
#include "cl_entity.h"
#include "com_model.h"
#include "beamdef.h"
#include "r_studioint.h"
#undef min
#undef max
#include "client_forwarders.inc"
static bool studioHooked=false;
static Vector renderedMuzzle(0,0,0);static float renderedMuzzleAt=-100;static double worldProjection[16];static bool haveWorldProjection=false;


static HMODULE donor;static cl_enginefunc_t eng;
#include "client_font.inc"
#include "client_judgment_hud.inc"
static bool mansionInvasionFog=false,mansionDefeat=false;
static int mansionScientistHp=0,mansionScientistMaxHp=0;
static int mansionEventMessage(const char*,int size,void* ptr){if(size!=6||!ptr)return 1;auto b=static_cast<unsigned char*>(ptr);mansionScientistHp=b[0]|b[1]<<8;mansionScientistMaxHp=b[2]|b[3]<<8;mansionInvasionFog=b[4]!=0;mansionDefeat=b[5]!=0;return 1;}
#include "client_weather.inc"
#include "client_music.inc"
#include "client_tactical_hud.inc"
#include "client_revolver.inc"
#include "client_radar.inc"
static Vector visibleMuzzle(cl_entity_t* entity){if(entity==eng.GetViewModel()&&eng.GetClientTime()-renderedMuzzleAt<.1f)return renderedMuzzle;return Vector(entity->attachment[0]);}
static void loadDonor(){if(donor)return;char p[MAX_PATH];GetModuleFileNameA(GetModuleHandleA("client.dll"),p,MAX_PATH);char* end=strrchr(p,'\\');if(end)strcpy(end+1,"client_ravenholm.dll");donor=LoadLibraryA(p);if(donor)for(int i=0;i<sizeof(clientTargets)/sizeof(clientTargets[0]);++i)clientTargets[i]=GetProcAddress(donor,clientNames[i]);}
static bool flakPending=false;
static int flakMessage(const char*,int size,void*){if(size==1)flakPending=true;return 1;}
static void drawFlak(){if(!flakPending)return;flakPending=false;auto view=eng.GetViewModel();if(!view||!view->model||!strstr(view->model->name,"v_bounce_shotgun.mdl"))return;int index=0;eng.CL_LoadModel("sprites/muzzleflash1.spr",&index);if(index){Vector zero(0,0,0);eng.pEfxAPI->R_TempSprite(visibleMuzzle(view),zero,.2f,index,kRenderTransAdd,kRenderFxNone,1,.05f,0);}}
static bool shotPending=false;static float shotAt=-100;static int shotEntities[3]{};static Vector shotVelocity(0,0,0);
static int shotMessage(const char*,int size,void* ptr){if(size!=13)return 1;auto b=static_cast<unsigned char*>(ptr);auto local=eng.GetLocalPlayer();if(!local||b[0]!=local->index)return 1;for(int i=0;i<3;++i)shotEntities[i]=b[1+i*2]|b[2+i*2]<<8;shotVelocity=Vector(short(b[7]|b[8]<<8)/8.f,short(b[9]|b[10]<<8)/8.f,short(b[11]|b[12]<<8)/8.f);shotAt=eng.GetClientTime();shotPending=true;return 1;}
static void drawShot(){if(!shotPending)return;shotPending=false;auto view=eng.GetViewModel();if(!view||!view->model||!strstr(view->model->name,"v_nailgun.mdl"))return;
 static const char* paths[]={"sprites/cremation/link_plasma_body.spr","sprites/cremation/link_plasma_core.spr","sprites/cremation/link_plasma_halo.spr"};float scales[]={.25f,.22f,.4f};for(int i=0;i<3;++i){int model=0;eng.CL_LoadModel(paths[i],&model);if(model)eng.pEfxAPI->R_TempSprite(visibleMuzzle(view),shotVelocity,scales[i],model,kRenderTransAdd,kRenderFxNone,i==2?.6f:1.f,.065f,0);}}
extern "C" int HUD_AddEntity(int type,cl_entity_t* entity,const char* model){if(eng.GetClientTime()-shotAt<.065f)for(int id:shotEntities)if(id&&entity&&entity->index==id)return 0;return reinterpret_cast<int(*)(int,cl_entity_t*,const char*)>(GetProcAddress(donor,"HUD_AddEntity"))(type,entity,model);}
struct BloodDrop{Vector origin,velocity;float born;};
static std::unordered_map<particle_t*,BloodDrop> bloodDrops;
static void bloodDropDeath(particle_t* p){bloodDrops.erase(p);}
static void bloodDropThink(particle_t* p,float){auto it=bloodDrops.find(p);if(it==bloodDrops.end()){p->die=eng.GetClientTime();return;}auto& drop=it->second;float t=eng.GetClientTime()-drop.born;if(t<0){p->die=eng.GetClientTime();return;}Vector at=drop.origin+drop.velocity*t+Vector(0.f,0.f,-325.f*t*t);for(int a=0;a<3;++a){p->org[a]=at[a];p->vel[a]=0;} }
static int gibBloodMessage(const char*,int size,void* ptr){
 if(size!=14)return 1;auto b=static_cast<unsigned char*>(ptr);auto coord=[&](int n){return short(b[n]|b[n+1]<<8)/8.f;};Vector at(coord(0),coord(2),coord(4)),direction(coord(6),coord(8),coord(10));bool red=b[12]==2;int groups=b[13]>10?10:b[13];
 // Original HL blood spray and droplets, combined with gib-style palette particles.
 int spray=0,drops=0;eng.CL_LoadModel("sprites/bloodspray.spr",&spray);eng.CL_LoadModel("sprites/blood.spr",&drops);if(spray&&drops)eng.pEfxAPI->R_BloodSprite(at,red?247:195,spray,drops,groups<6?4.f:6.f);
 // Explicit trajectories avoid the built-in tracer's forced sideways velocity.
 for(int i=0;i<groups*16&&bloodDrops.size()<4096;++i){auto p=eng.pEfxAPI->R_AllocParticle(bloodDropThink);if(!p)break;Vector origin=at+Vector(eng.pfnRandomFloat(-2,2),eng.pfnRandomFloat(-2,2),eng.pfnRandomFloat(-2,2));Vector velocity=direction*eng.pfnRandomFloat(40,85)+Vector(eng.pfnRandomFloat(-8,8),eng.pfnRandomFloat(-8,8),eng.pfnRandomFloat(70,115));float now=eng.GetClientTime();bloodDrops[p]={origin,velocity,now};p->type=pt_clientcustom;p->deathfunc=bloodDropDeath;p->color=red?67+eng.pfnRandomLong(0,7):195+eng.pfnRandomLong(0,7);p->ramp=0;p->die=now+eng.pfnRandomFloat(.8f,1.25f);for(int a=0;a<3;++a){p->org[a]=origin[a];p->vel[a]=0;}}
 return 1;
}
struct ShockClientFx{bool pending=false;int mode=0;Vector end{0,0,0};float received=0;};static ShockClientFx shockEffects[33];
static int shockMessage(const char*,int size,void* ptr){if(size!=8)return 1;auto b=static_cast<unsigned char*>(ptr);int i=b[0];if(i<1||i>32)return 1;shockEffects[i].pending=true;shockEffects[i].mode=b[1];shockEffects[i].received=eng.GetClientTime();shockEffects[i].end=Vector(short(b[2]|b[3]<<8)/8.f,short(b[4]|b[5]<<8)/8.f,short(b[6]|b[7]<<8)/8.f);return 1;}
static void drawShock(){auto local=eng.GetLocalPlayer();for(int i=1;i<33;++i){auto& fx=shockEffects[i];if(!fx.pending)continue;fx.pending=false;if(eng.GetClientTime()-fx.received>.2f)continue;auto entity=(local&&local->index==i)?eng.GetViewModel():eng.GetEntityByIndex(i);if(!entity||!entity->model)continue;if(local&&local->index==i&&!strstr(entity->model->name,"v_shockrifle.mdl"))continue;Vector from=visibleMuzzle(entity);int sprite=0;
 if(fx.mode==0){eng.CL_LoadModel("sprites/xbeam1.spr",&sprite);if(sprite){eng.pEfxAPI->R_BeamPoints(from,fx.end,sprite,.13f,2.6f,0,.55f,0,0,30,.65f,.15f,1);eng.pEfxAPI->R_BeamPoints(from,fx.end,sprite,.1f,.65f,0,1,0,0,30,.85f,.65f,1);}}
 eng.CL_LoadModel("sprites/cremation/shock_core.spr",&sprite);if(sprite){Vector zero(0,0,0);eng.pEfxAPI->R_TempSprite(from,zero,fx.mode?.22f:.12f,sprite,kRenderTransAdd,kRenderFxNone,1,.07f,0);}
}}
struct LinkFx{float until=0;Vector end{0,0,0};BEAM* beams[64]{};};static LinkFx links[33];
static int message(const char*,int size,void* ptr){if(size!=8)return 1;auto b=static_cast<unsigned char*>(ptr);int i=b[0];if(i<1||i>32)return 1;links[i].end=Vector(short(b[2]|b[3]<<8)/8.f,short(b[4]|b[5]<<8)/8.f,short(b[6]|b[7]<<8)/8.f);links[i].until=eng.GetClientTime()+.18f;return 1;}
extern "C" int Initialize(cl_enginefunc_t* e,int v){loadDonor();if(!donor)return 0;eng=*e;return reinterpret_cast<int(*)(cl_enginefunc_t*,int)>(GetProcAddress(donor,"Initialize"))(e,v);}
extern "C" void HUD_Init(){loadDonor();reinterpret_cast<void(*)()>(GetProcAddress(donor,"HUD_Init"))();weatherInit();eng.pfnHookUserMsg("HordeRadar",radarMessage);eng.pfnHookUserMsg("TacticalHUD",tacticalHudMessage);eng.pfnHookUserMsg("JudgmentHUD",judgmentHudMessage);eng.pfnHookUserMsg("MnDoors",mansionDoorsMessage);eng.pfnHookUserMsg("MnEvent",mansionEventMessage);eng.pfnHookUserMsg("CremationWaveHUD",waveHudMessage);eng.pfnHookUserMsg("LinkMuzzle",message);eng.pfnHookUserMsg("LinkShot",shotMessage);eng.pfnHookUserMsg("FlakMuz",flakMessage);eng.pfnHookUserMsg("ShockFx",shockMessage);eng.pfnHookUserMsg("GibBlood",gibBloodMessage);}
static void drawLinks(){float now=eng.GetClientTime();int sprite=0;eng.CL_LoadModel("sprites/xbeam1.spr",&sprite);if(!sprite)return;auto local=eng.GetLocalPlayer();
for(int i=1;i<33;++i){auto& s=links[i];auto entity=(local&&local->index==i)?eng.GetViewModel():eng.GetEntityByIndex(i);bool active=s.until>now&&entity&&entity->model;
if(active&&local&&local->index==i&&!strstr(entity->model->name,"v_nailgun.mdl"))active=false;
if(!active){for(auto& b:s.beams)if(b){b->die=now;b=nullptr;}continue;}
Vector from=visibleMuzzle(entity),span=s.end-from;float length=span.Length();if(length<.1f)continue;Vector dir=span*(1.f/length),side(-dir.y,dir.x,0.f);float sl=side.Length();side=sl>.01f?side*(1.f/sl):Vector(1,0,0);Vector up(side.y*dir.z-side.z*dir.y,side.z*dir.x-side.x*dir.z,side.x*dir.y-side.y*dir.x);
for(int layer=0;layer<4;++layer){Vector previous=from;for(int n=1;n<=16;++n){float t=n/16.f,env=std::sin(t*3.14159265f),phase=now*18-t*18.85f+layer*3.14159265f;Vector point=from+span*t+up*(env*6);if(layer>=2)point=point+(side*std::sin(phase)+up*std::cos(phase))*(env*5);auto& beam=s.beams[layer*16+n-1];
if(!beam||beam->die<=now)beam=eng.pEfxAPI->R_BeamPoints(previous,point,sprite,.3f,layer==0?3.6f:layer==1?.9f:.5f,0,layer==0?.3f:1.f,1,0,20,layer==1?.88f:.1f,1,layer==1?.88f:.2f);
if(beam){for(int a=0;a<3;++a){beam->source[a]=previous[a];beam->target[a]=point[a];beam->delta[a]=point[a]-previous[a];}beam->die=now+.3f;}previous=point;}}}}
extern "C" void HUD_DrawTransparentTriangles(){reinterpret_cast<void(*)()>(GetProcAddress(donor,"HUD_DrawTransparentTriangles"))();if(!studioHooked){drawLinks();drawShot();drawFlak();drawShock();}drawWeatherRain();}
extern "C" int HUD_GetStudioModelInterface(int,r_studio_interface_t**,engine_studio_api_t*);
extern "C" int HUD_VidInit();
extern "C" void HUD_Reset();
extern "C" void HUD_DrawNormalTriangles();
static HUD_CALCREF_FUNC donorCalcRefdef=nullptr;
static void hookCalcRefdef(struct ref_params_s* pparams){if(donorCalcRefdef)donorCalcRefdef(pparams);weatherOnRefdef(pparams);}
extern "C" void F(void* p){loadDonor();reinterpret_cast<void(*)(void*)>(GetProcAddress(donor,"F"))(p);auto t=static_cast<cldll_func_t*>(p);donorCalcRefdef=t->pCalcRefdef;t->pCalcRefdef=hookCalcRefdef;t->pShutdown=HUD_Shutdown;t->pHudFrame=HUD_Frame;t->pHudRedrawFunc=HUD_Redraw;t->pDrawNormalTriangles=HUD_DrawNormalTriangles;t->pAddEntity=HUD_AddEntity;t->pHudVidInitFunc=HUD_VidInit;t->pHudResetFunc=HUD_Reset;t->pInitFunc=Initialize;t->pHudInitFunc=HUD_Init;t->pDrawTransparentTriangles=HUD_DrawTransparentTriangles;t->pStudioInterface=HUD_GetStudioModelInterface;}

static engine_studio_api_t studioApi;static r_studio_interface_t studioProxy;static int (*nativeDrawModel)(int);
static int drawStudio(int flags){int result=nativeDrawModel(flags);if(studioApi.GetCurrentEntity()==eng.GetViewModel()){drawLinks();drawShot();drawFlak();drawShock();}return result;}
static engine_studio_api_t hookedStudioApi;
static void captureMuzzle(){
 auto view=eng.GetViewModel();if(!view||studioApi.GetCurrentEntity()!=view||!view->model||(!strstr(view->model->name,"v_nailgun.mdl")&&!strstr(view->model->name,"v_bounce_shotgun.mdl")&&!strstr(view->model->name,"v_shockrifle.mdl")))return;
 auto h=static_cast<unsigned char*>(studioApi.Mod_Extradata(view->model));if(!h||*reinterpret_cast<int*>(h+212)<1)return;auto attachment=h+*reinterpret_cast<int*>(h+216);int bone=*reinterpret_cast<int*>(attachment+36);if(bone<0||bone>=128)return;auto local=reinterpret_cast<float*>(attachment+40);
 auto matrices=reinterpret_cast<float(*)[3][4]>(studioApi.StudioGetBoneTransform());Vector point;for(int r=0;r<3;++r)point[r]=matrices[bone][r][3]+matrices[bone][r][0]*local[0]+matrices[bone][r][1]*local[1]+matrices[bone][r][2]*local[2];
 renderedMuzzle=point;renderedMuzzleAt=eng.GetClientTime();
 if(!wglGetCurrentContext()||!haveWorldProjection)return;
 double model[16],projection[16],eye[4]{},clip[4]{};glGetDoublev(GL_MODELVIEW_MATRIX,model);glGetDoublev(GL_PROJECTION_MATRIX,projection);double world[4]={point.x,point.y,point.z,1};
 for(int r=0;r<4;++r)for(int c=0;c<4;++c)eye[r]+=model[c*4+r]*world[c];for(int r=0;r<4;++r)for(int c=0;c<4;++c)clip[r]+=projection[c*4+r]*eye[c];
 if(clip[3]<=.001||std::fabs(worldProjection[0])<.001||std::fabs(worldProjection[5])<.001)return;
 Vector origin,up,right,forward;studioApi.GetViewInfo(origin,up,right,forward);Vector delta=point-origin;float depth=delta.x*forward.x+delta.y*forward.y+delta.z*forward.z;if(depth<1)return;
 renderedMuzzle=origin+forward*depth+right*float(clip[0]/clip[3]*depth/worldProjection[0])+up*float(clip[1]/clip[3]*depth/worldProjection[5]);
 static float reportAt=-100;if(eng.GetClientTime()-reportAt>2||eng.GetClientTime()<reportAt){reportAt=eng.GetClientTime();if(FILE* f=std::fopen("Cremation/link-muzzle-diagnostics.txt","a")){std::fprintf(f,"t=%.2f bone=%d raw=%.2f,%.2f,%.2f visual=%.2f,%.2f,%.2f ndc=%.3f,%.3f projection=%.3f,%.3f world=%.3f,%.3f\n",reportAt,bone,point.x,point.y,point.z,renderedMuzzle.x,renderedMuzzle.y,renderedMuzzle.z,clip[0]/clip[3],clip[1]/clip[3],projection[0],projection[5],worldProjection[0],worldProjection[5]);std::fclose(f);}}

}
static void drawPoints(){captureMuzzle();studioApi.StudioDrawPoints();}
extern "C" void HUD_DrawNormalTriangles(){if(wglGetCurrentContext()){glGetDoublev(GL_PROJECTION_MATRIX,worldProjection);haveWorldProjection=true;}weatherApplyFogOpenGL();reinterpret_cast<void(*)()>(GetProcAddress(donor,"HUD_DrawNormalTriangles"))();}
extern "C" int HUD_GetStudioModelInterface(int version,r_studio_interface_t** iface,engine_studio_api_t* api){loadDonor();studioApi=*api;hookedStudioApi=*api;hookedStudioApi.StudioDrawPoints=drawPoints;int result=reinterpret_cast<int(*)(int,r_studio_interface_t**,engine_studio_api_t*)>(GetProcAddress(donor,"HUD_GetStudioModelInterface"))(version,iface,&hookedStudioApi);if(result&&iface&&*iface){studioProxy=**iface;nativeDrawModel=studioProxy.StudioDrawModel;studioProxy.StudioDrawModel=drawStudio;*iface=&studioProxy;studioHooked=true;}return result;}

extern "C" int HUD_VidInit(){revolverCrosshairSprite=0;clearCremationFontCache();judgmentHudClear();tacticalHudClear();radarClear();waveHudClear();weatherClear();memset(links,0,sizeof links);bloodDrops.clear();memset(shockEffects,0,sizeof shockEffects);renderedMuzzleAt=-100;haveWorldProjection=false;shotAt=-100;shotPending=false;flakPending=false;return reinterpret_cast<int(*)()>(GetProcAddress(donor,"HUD_VidInit"))();}
extern "C" void HUD_Reset(){revolverCrosshairSprite=0;clearCremationFontCache();tacticalHudClear();radarClear();waveHudClear();weatherClear();memset(links,0,sizeof links);bloodDrops.clear();memset(shockEffects,0,sizeof shockEffects);renderedMuzzleAt=-100;haveWorldProjection=false;shotAt=-100;shotPending=false;flakPending=false;reinterpret_cast<void(*)()>(GetProcAddress(donor,"HUD_Reset"))();}
