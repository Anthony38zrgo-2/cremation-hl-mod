// A reliable encounter snapshot drives both the health overlay and music.
struct JudgmentHudState{bool active=false;int hp=0,maximum=1500;};
static JudgmentHudState judgmentHud;
static const char* judgmentMusicPath(bool inLevel){return inLevel?(judgmentHud.active?"/sound/music/judgment_leprosy.wav":"/sound/music/myfuneral_loop.wav"):"/sound/music/menu_introduction.wav";}
static void judgmentHudClear(){judgmentHud=JudgmentHudState{};}
static int judgmentHudMessage(const char*,int size,void* ptr){
    if(size!=6||!ptr)return 1;auto b=static_cast<unsigned char*>(ptr);if(b[0]!=1||b[1]>1)return 1;
    int hp=b[2]|(b[3]<<8),maximum=b[4]|(b[5]<<8);if(maximum<1||hp>maximum)return 1;
    judgmentHud.active=b[1]!=0&&hp>0;judgmentHud.hp=hp;judgmentHud.maximum=maximum;return 1;
}
static void drawJudgmentHud(){
    if(!judgmentHud.active||eng.pfnGetCvarFloat("hud_draw")==0)return;
    SCREENINFO screen{};screen.iSize=sizeof screen;if(!eng.pfnGetScreenInfo(&screen))return;
    int width=std::min(screen.iWidth-40,std::min(720,std::max(260,int(screen.iWidth*.55f))));
    int x=(screen.iWidth-width)/2,y=std::max(24,int(screen.iHeight*.045f)),height=30;
    eng.pfnFillRGBA(x-2,y-2,width+4,height+4,220,30,30,245);
    eng.pfnFillRGBA(x,y,width,height,45,5,8,245);
    int fill=int((width-4)*float(judgmentHud.hp)/judgmentHud.maximum);
    if(fill>0)eng.pfnFillRGBA(x+2,y+2,fill,height-4,195,10,20,245);
    drawCremationFontText("Judgment","Butcherman",std::max(20,std::min(30,screen.iHeight*24/1080)),screen.iWidth/2,y+1,255,255,245);
}
