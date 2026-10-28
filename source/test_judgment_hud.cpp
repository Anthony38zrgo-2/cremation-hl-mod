#include <algorithm>
#include <cassert>
#include <cstring>
#include <cstdio>
#include <vector>
struct SCREENINFO{int iSize,iWidth,iHeight;};
struct Fill{int x,y,w,h,r,g,b,a;};
static std::vector<Fill> fills;
static int labelX=0,labelY=0;
struct HudEngine{
    float (*pfnGetCvarFloat)(const char*);
    int (*pfnGetScreenInfo)(SCREENINFO*);
    void (*pfnFillRGBA)(int,int,int,int,int,int,int,int);
    void (*pfnDrawConsoleStringLen)(const char*,int*,int*);
    void (*pfnDrawSetTextColor)(float,float,float);
    int (*pfnDrawConsoleString)(int,int,char*);
};
static HudEngine eng{
    [](const char*){return 1.f;},
    [](SCREENINFO* s){s->iWidth=1920;s->iHeight=1080;return 1;},
    [](int x,int y,int w,int h,int r,int g,int b,int a){fills.push_back({x,y,w,h,r,g,b,a});},
    [](const char* text,int* w,int* h){*w=int(std::strlen(text))*8;*h=16;},
    [](float,float,float){},
    [](int x,int y,char* text){assert(!std::strcmp(text,"Judgment"));labelX=x;labelY=y;return x;}
};
#include "client_judgment_hud.inc"
int main(){
    unsigned char full[]={1,1,0xdc,5,0xdc,5};
    judgmentHudMessage(nullptr,6,full);assert(judgmentHud.active&&judgmentHud.hp==1500);
    drawJudgmentHud();assert(fills.size()==3&&fills[2].r==195&&fills[2].g==10);
    int fullWidth=fills[2].w;assert(labelX+int(std::strlen("Judgment"))*4==960);assert(labelY>=fills[0].y&&labelY<fills[0].y+fills[0].h);
    assert(!std::strcmp(judgmentMusicPath(true),"/sound/music/judgment_leprosy.wav"));
    unsigned char half[]={1,1,0xee,2,0xdc,5};fills.clear();judgmentHudMessage(nullptr,6,half);drawJudgmentHud();assert(fills[2].w==fullWidth/2);
    unsigned char bad[]={9,0,0,0,0,0};judgmentHudMessage(nullptr,6,bad);assert(judgmentHud.active&&judgmentHud.hp==750);
    unsigned char dead[]={1,0,0,0,0xdc,5};judgmentHudMessage(nullptr,6,dead);fills.clear();drawJudgmentHud();assert(fills.empty());
    assert(!std::strcmp(judgmentMusicPath(true),"/sound/music/myfuneral_loop.wav"));
    assert(!std::strcmp(judgmentMusicPath(false),"/sound/music/menu_introduction.wav"));
    std::puts("PASS: red proportional boss bar, centered Judgment label, encounter music selection and defeat cleanup.");
}
