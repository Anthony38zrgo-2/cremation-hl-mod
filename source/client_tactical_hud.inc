// Uses original sprite atlases. No changes to Ravenholm's client exports,
// status messages or HUD atlas: this overlay has its own versioned message.
struct TacticalHudState{int bomb=0,flags=0,doorHp[2]{1000,1000},zombies=0,wave=0;bool kit=false,alive=false;float received=-100,doorsReceived=-100,bombEnd=0,progress=0,powerEnds[4]{},toastEnd=0,plantToastEnd=0,doorToastEnd=0;};
static TacticalHudState tacticalHud;
static HSPRITE tacticalCs=0,tacticalDmcNumbers=0,tacticalPowerIcons[3]{};
static void tacticalHudTrace(const char* line){static int enabled=-1;if(enabled<0){char env[8]{};GetEnvironmentVariableA("CREMATION_HUD_VALIDATE",env,sizeof env);enabled=env[0]=='1';if(!enabled){GetEnvironmentVariableA("CREMATION_HUD_TRACE",env,sizeof env);enabled=env[0]=='1';}}if(enabled){if(FILE* f=std::fopen("Cremation/diagnostics/hud-client-trace.log","a")){std::fprintf(f,"%s\n",line);std::fclose(f);}}}
static void tacticalHudClear(){tacticalHud=TacticalHudState{};tacticalCs=tacticalDmcNumbers=0;for(auto& icon:tacticalPowerIcons)icon=0;tacticalHudTrace("HUDClear");}
static int tacticalHudMessage(const char*,int size,void* ptr){
    if(size!=17||!ptr)return 1;auto b=static_cast<unsigned char*>(ptr);if(b[0]!=1||b[1]>5)return 1;
    auto word=[&](int n){return unsigned(b[n])|(unsigned(b[n+1])<<8);};float now=eng.GetClientTime();int before=tacticalHud.bomb;
    tacticalHud.received=now;tacticalHud.bomb=b[1];tacticalHud.bombEnd=now+word(2)*.1f;tacticalHud.progress=std::min(1000.f,float(word(4)));
    tacticalHud.flags=b[6];tacticalHud.kit=b[7]!=0;tacticalHud.alive=b[16]!=0;
    for(int i=0;i<4;++i)tacticalHud.powerEnds[i]=now+word(8+i*2)*.1f;
    if(before==0&&b[1]>0)tacticalHud.plantToastEnd=now+3.f;
    if(b[1]>=4&&before!=b[1])tacticalHud.toastEnd=now+6.f;
    return 1;
}
static int mansionDoorsMessage(const char*,int size,void* ptr){
    if(size!=8||!ptr)return 1;auto b=static_cast<unsigned char*>(ptr);if(b[0]!=2)return 1;
    auto word=[&](int n){return unsigned(b[n])|(unsigned(b[n+1])<<8);};
    int hp[2]={std::min(1000,int(word(1))),std::min(1000,int(word(3)))};
    tacticalHud.zombies=int(word(5));tacticalHud.wave=b[7];
    float now=eng.GetClientTime();for(int i=0;i<2;++i){if(tacticalHud.doorHp[i]>0&&hp[i]==0)tacticalHud.doorToastEnd=now+3.5f;tacticalHud.doorHp[i]=hp[i];}
    tacticalHud.doorsReceived=now;return 1;
}
static void tacticalSprite(HSPRITE sprite,int x,int y,int left,int top,int width,int height,int r,int g,int b){
    if(!sprite)return;wrect_t rect{left,left+width,top,top+height};eng.pfnSPR_Set(sprite,r,g,b);eng.pfnSPR_DrawAdditive(0,x,y,&rect);
}
static void tacticalText(const char* text,int cx,int y,int r,int g,int b){int w=0,h=0;eng.pfnDrawConsoleStringLen(text,&w,&h);eng.pfnDrawSetTextColor(r/255.f,g/255.f,b/255.f);eng.pfnDrawConsoleString(cx-w/2,y,const_cast<char*>(text));}
static void tacticalDigits(int value,int x,int y,bool cs,int r,int g,int b,int count=2){
    static const int csX[]={0,23,47,70,95,119,144,169,192,216};int divisor=1;for(int i=1;i<count;++i)divisor*=10;
    for(int i=0;i<count;++i){int digit=(value/divisor)%10;divisor=std::max(1,divisor/10);
        tacticalSprite(cs?tacticalCs:tacticalDmcNumbers,x+i*22,y,cs?csX[digit]:digit*24,0,20,cs?25:24,r,g,b);}
}
static void drawTacticalHud(){
    float now=eng.GetClientTime();if(now-tacticalHud.received>1.2f||now<tacticalHud.received||eng.pfnGetCvarFloat("hud_draw")==0)return;
    SCREENINFO screen{};screen.iSize=sizeof screen;if(!eng.pfnGetScreenInfo(&screen))return;int w=screen.iWidth,h=screen.iHeight;
    if(!tacticalCs)tacticalCs=eng.pfnSPR_Load("sprites/cremation/hud/cz/640hud7.spr");
    if(!tacticalDmcNumbers)tacticalDmcNumbers=eng.pfnSPR_Load("sprites/cremation/hud/dmc/640hud7.spr");
    static const char* icons[]={"sprites/cremation/hud/dmc/icnquad.spr","sprites/cremation/hud/dmc/icninvin.spr","sprites/cremation/hud/dmc/icninvis.spr"};
    for(int i=0;i<3;++i)if(!tacticalPowerIcons[i])tacticalPowerIcons[i]=eng.pfnSPR_Load(icons[i]);
    int status=tacticalHud.bomb;float left=std::max(0.f,tacticalHud.bombEnd-now);
    if(status>0){int r=status==4?80:255,g=status==4?220:(status==5||left<=10?40:160),b=status==4?100:0;
        int y=h-46,x=w/2-44;bool pulse=std::fmod(now*(left<=10?4.f:2.f),1.f)<.5f;
        if(status<4){tacticalSprite(tacticalCs,x-40,y-4,pulse?192:224,148,32,32,r,g,b);
            tacticalSprite(tacticalCs,x,y,144,72,24,24,r,g,b);int seconds=int(std::ceil(left));seconds=std::min(5999,seconds);
            tacticalDigits(seconds/60,x+30,y,true,r,g,b);eng.pfnDrawCharacter(x+76,y+3,':',r,g,b);tacticalDigits(seconds%60,x+88,y,true,r,g,b);
        }
        if(now<tacticalHud.toastEnd){int size=std::max(24,std::min(48,h*32/1080));
            if(status==5)drawCremationFontText("BOMBA EXPLOTADA","Nosifer",size,w/2,int(h*.24f),255,55,40);
            else if(status==4)drawCremationFontText("BOMBA DESACTIVADA","Butcherman",size,w/2,int(h*.24f),255,55,40);
        }
        if(now<tacticalHud.plantToastEnd)drawCremationFontText("BOMBA PLANTADA","Nosifer",std::max(24,std::min(48,h*32/1080)),w/2,int(h*.56f),255,55,40);
        if(status>=4&&now<tacticalHud.toastEnd)tacticalSprite(tacticalCs,w/2-16,int(h*.24f)+25,status==4?32:0,148,32,32,r,g,b);
        if(status==3&&(tacticalHud.flags&1)&&tacticalHud.alive){int width=std::min(600,std::max(200,w/2)),x0=(w-width)/2,y0=int(h*.64f),height=12;
            float progress=std::min(1000.f,tacticalHud.progress+(now-tacticalHud.received)*200);
            tacticalText("DESACTIVANDO LA BOMBA",w/2,y0-25,255,160,0);
            if(eng.pfnFillRGBABlend)eng.pfnFillRGBABlend(x0,y0,width,height,0,0,0,180);
            eng.pfnFillRGBA(x0-2,y0-2,width+4,2,255,160,0,210);eng.pfnFillRGBA(x0-2,y0+height,width+4,2,255,160,0,210);
            eng.pfnFillRGBA(x0-2,y0,2,height,255,160,0,210);eng.pfnFillRGBA(x0+width,y0,2,height,255,160,0,210);
            eng.pfnFillRGBA(x0+2,y0+2,int((width-4)*progress/1000),height-4,255,160,0,230);
        }else if((tacticalHud.flags&4)&&tacticalHud.alive&&status<4){
            tacticalText(status==1?"ELIMINA AL NPC PARA DESACTIVAR":"MANTEN USAR PARA DESACTIVAR",w/2,int(h*.64f)-25,255,160,0);
        }
    }
    if(tacticalHud.kit&&tacticalHud.alive)tacticalSprite(tacticalCs,18,h-116,32,148,32,32,80,180,90);
    // DMC status icons stack on the left, with native DMC numeric glyphs.
    // Biosuit has no dedicated DMC status sprite: use its own suit HUD glyph.
    if(tacticalHud.alive){int total=0;for(int i=0;i<4;++i)if(tacticalHud.powerEnds[i]>now)total+=i==3?45:37;
        int radarSize=std::min(230,std::max(150,w/7));int y=std::max(18+radarSize+14,h/2-total);
        const int colors[4][3]={{128,128,255},{255,128,0},{180,180,180},{100,220,100}};
        for(int i=0;i<4;++i){float remaining=tacticalHud.powerEnds[i]-now;if(remaining<=0)continue;
            int height=i==3?40:32;auto c=colors[i];bool on=remaining>3||std::fmod(now*5,1.f)<.5f;
            if(on){if(i<3)tacticalSprite(tacticalPowerIcons[i],6,y,0,0,32,32,c[0],c[1],c[2]);else tacticalSprite(tacticalDmcNumbers,6,y,0,24,40,40,c[0],c[1],c[2]);
                tacticalDigits(std::min(99,int(std::ceil(remaining))),54,y+(height-24)/2,false,c[0],c[1],c[2]);}
            y+=height+5;
        }
    }
    if(now-tacticalHud.doorsReceived<=1.2f&&tacticalHud.doorsReceived<=now){
        const int width=176,height=14,x0=w-width-28,y0=int(h*.34f);
        for(int i=0;i<2;++i){int y=y0+i*48,hp=std::max(0,std::min(1000,tacticalHud.doorHp[i]));char label[32];
            std::snprintf(label,sizeof label,hp?"Door %d":"Door %d  ROTA",i+1);eng.pfnDrawSetTextColor(1.f,.82f,.72f);eng.pfnDrawConsoleString(x0,y-22,label);
            if(eng.pfnFillRGBABlend)eng.pfnFillRGBABlend(x0,y,width,height,0,0,0,190);
            eng.pfnFillRGBA(x0-1,y-1,width+2,2,170,35,30,230);eng.pfnFillRGBA(x0-1,y+height-1,width+2,2,170,35,30,230);
            eng.pfnFillRGBA(x0-1,y,2,height,170,35,30,230);eng.pfnFillRGBA(x0+width-1,y,2,height,170,35,30,230);
            if(hp)eng.pfnFillRGBA(x0+2,y+2,(width-4)*hp/1000,height-4,225,35,24,240);
        }
    }
    if(now-tacticalHud.doorsReceived<=1.2f&&tacticalHud.doorsReceived<=now&&tacticalHud.wave>0){char zombies[48],wave[32];
            std::snprintf(zombies,sizeof zombies,"Zombies: %d",tacticalHud.zombies);
            std::snprintf(wave,sizeof wave,"Wave: %d",tacticalHud.wave);
            const int pixels=std::max(14,std::min(18,h*18/1080));
            const int y=int(h*.80f),cx=w/2;
            drawCremationFontText(zombies,"Nosifer",pixels,cx,y,255,45,45);
            drawCremationFontText(wave,"Nosifer",pixels,cx,y+pixels+5,255,45,45);
        }
    if(now<tacticalHud.doorToastEnd)drawCremationFontText("PUERTA DESTRUIDA","Nosifer",std::max(24,std::min(48,h*32/1080)),w/2,int(h*.32f),255,55,40);
    int mask=0;for(int i=0;i<4;++i)if(tacticalHud.alive&&tacticalHud.powerEnds[i]>now)mask|=1<<i;
    bool bar=status==3&&(tacticalHud.flags&1)&&tacticalHud.alive;int signature=status|(mask<<3)|(bar?128:0)|(tacticalHud.kit?256:0)|(tacticalHud.alive?512:0);
    static int last=-1;if(signature!=last){last=signature;char row[160];std::snprintf(row,sizeof row,"HUDDraw state=%d bar=%d power=%d kit=%d alive=%d sprites=%d",status,int(bar),mask,int(tacticalHud.kit),int(tacticalHud.alive),int(tacticalCs&&tacticalDmcNumbers&&tacticalPowerIcons[0]&&tacticalPowerIcons[1]&&tacticalPowerIcons[2]));tacticalHudTrace(row);}
}
