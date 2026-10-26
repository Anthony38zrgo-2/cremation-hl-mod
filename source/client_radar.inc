struct RadarPoint{float x,y;};
static RadarPoint radarEnemies[512];static int radarCount=0;static bool radarEnabled=false;static float radarReceived=-100;
#include "radar_geometry.h"
static int radarMessage(const char*,int size,void* ptr){
    if(size<3)return 1;auto b=static_cast<unsigned char*>(ptr);int count=b[2];if(count>40||size!=3+count*4)return 1;
    if(b[0])radarCount=0;radarEnabled=b[1]!=0;radarReceived=eng.GetClientTime();
    for(int i=0;i<count&&radarCount<512;++i){int at=3+i*4;radarEnemies[radarCount++]={short(b[at]|b[at+1]<<8)/8.f,short(b[at+2]|b[at+3]<<8)/8.f};}
    return 1;
}
static void radarClear(){radarCount=0;radarEnabled=false;radarReceived=-100;}
static void drawRadar(){
    if(!radarEnabled||eng.GetClientTime()-radarReceived>1.5f)return;
    SCREENINFO screen{};screen.iSize=sizeof screen;if(!eng.pfnGetScreenInfo(&screen))return;
    int size=screen.iWidth/7;if(size<150)size=150;if(size>230)size=230;
    int left=18,top=18,cx=left+size/2,cy=top+size/2,radius=size/2;float scale=(radius-5)/2560.f;
    for(int y=-radius;y<=radius;++y){int width=int(std::sqrt(float(radius*radius-y*y)));eng.pfnFillRGBA(cx-width,cy+y,2*width+1,1,5,25,12,205);}
    auto px=[&](float x){return cx+int((x+352)*scale);};auto py=[&](float y){return cy-int((y+256)*scale);};
    for(const auto& rect:radarTerrain){int x0=px(rect[0]),x1=px(rect[2]),y0=py(rect[3]),y1=py(rect[1]);
        int dx=std::max(std::abs(x0-cx),std::abs(x1-cx)),dy=std::max(std::abs(y0-cy),std::abs(y1-cy));
        if(dx*dx+dy*dy<radius*radius){eng.pfnFillRGBA(x0,y0,x1-x0+1,y1-y0+1,35,80,48,190);continue;}
        for(int y=y0;y<=y1;++y){int dy=y-cy;if(dy<-radius||dy>radius)continue;int width=int(std::sqrt(float(radius*radius-dy*dy)));
            int a=x0>cx-width?x0:cx-width,b=x1<cx+width?x1:cx+width;if(b>=a)eng.pfnFillRGBA(a,y,b-a+1,1,35,80,48,190);}
    }
    // The central building's footprint distinguishes the house from its yard.
    eng.pfnFillRGBA(px(-680),py(248),px(0)-px(-680),py(-768)-py(248),65,110,73,200);
    for(int i=0;i<radarCount;++i){int x=px(radarEnemies[i].x),y=py(radarEnemies[i].y);int dx=x-cx,dy=y-cy;
        if(dx*dx+dy*dy<(radius-3)*(radius-3))eng.pfnFillRGBA(x-2,y-2,4,4,255,65,50,245);}
    auto player=eng.GetLocalPlayer();if(player){int x=px(player->origin[0]),y=py(player->origin[1]);
        eng.pfnFillRGBA(x-2,y-2,5,5,255,230,90,255);float yaw=player->angles[1]*3.14159265f/180;
        for(int i=3;i<10;++i)eng.pfnFillRGBA(x+int(std::cos(yaw)*i),y-int(std::sin(yaw)*i),2,2,255,230,90,240);
    }
    eng.pfnDrawCharacter(cx-4,top+3,'N',130,225,145);
}
extern "C" int HUD_Redraw(float time,int intermission){int result=reinterpret_cast<int(*)(float,int)>(GetProcAddress(donor,"HUD_Redraw"))(time,intermission);if(!intermission)drawRadar();return result;}
