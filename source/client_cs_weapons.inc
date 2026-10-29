// Native HL prediction still owns Python/Egon. Keep the CS model presentation
// independent so its animation and FOV cannot be replaced by those carriers.
struct CSWeaponClientState {int kind=0,sequence=0,fov=90,serial=-1;float animationStart=0;};
static CSWeaponClientState csWeaponClient;
struct NativeCrosshair {HSPRITE sprite=0;wrect_t rect{};int r=255,g=255,b=255;};
static NativeCrosshair csNativeCrosshair;
static void csSetCrosshair(HSPRITE sprite,wrect_t rect,int r,int g,int b){
    csNativeCrosshair={sprite,rect,r,g,b};
    // The donor's autoaim sprite has its own moving aim offset. CS weapons
    // use the screen-centred HUD sight below, including during native updates.
    if(csWeaponClient.kind){wrect_t empty{};eng.pfnSetCrosshair(0,empty,0,0,0);}
    else eng.pfnSetCrosshair(sprite,rect,r,g,b);
}
static int csWeaponMessage(const char*,int size,void* data){
    if(size!=4)return 1;
    auto b=static_cast<unsigned char*>(data);
    if(b[0]>2||(b[0]&&b[1]>5)||(b[2]!=90&&b[2]!=40&&b[2]!=10))return 1;
    int previousKind=csWeaponClient.kind;
    bool restart=b[0]!=csWeaponClient.kind||b[1]!=csWeaponClient.sequence||b[3]!=csWeaponClient.serial;
    bool changed=restart||b[2]!=csWeaponClient.fov;
    csWeaponClient.kind=b[0];csWeaponClient.sequence=b[1];csWeaponClient.fov=b[2];csWeaponClient.serial=b[3];
    if(b[0]){wrect_t empty{};eng.pfnSetCrosshair(0,empty,0,0,0);}
    else if(previousKind)eng.pfnSetCrosshair(csNativeCrosshair.sprite,csNativeCrosshair.rect,csNativeCrosshair.r,csNativeCrosshair.g,csNativeCrosshair.b);
    if(restart){csWeaponClient.animationStart=eng.GetClientTime();if(b[0])eng.pfnWeaponAnim(b[1],0);}
    if(changed&&b[0])if(FILE* f=std::fopen("Cremation/cs-weapon-diagnostics.txt","a")){
        std::fprintf(f,"client kind=%d sequence=%d fov=%d serial=%d time=%.3f\n",b[0],b[1],b[2],b[3],eng.GetClientTime());std::fclose(f);
    }
    return 1;
}
static bool csWeaponPrepareViewModel(cl_entity_t* view){
    if(!csWeaponClient.kind||!view||!view->model)return false;
    const char* expected=csWeaponClient.kind==1?"/awp/v_awp.mdl":"/deagle/v_deagle.mdl";
    if(!strstr(view->model->name,expected))return false;
    view->curstate.sequence=csWeaponClient.sequence;
    view->curstate.animtime=csWeaponClient.animationStart;
    view->curstate.frame=0;view->curstate.framerate=1;
    view->latched.sequencetime=0;
    view->latched.prevsequence=csWeaponClient.sequence;
    return csWeaponClient.kind==1&&csWeaponClient.fov<90;
}
extern "C" int HUD_UpdateClientData(client_data_t* data,float time){
    int result=reinterpret_cast<int(*)(client_data_t*,float)>(GetProcAddress(donor,"HUD_UpdateClientData"))(data,time);
    if(csWeaponClient.kind)data->fov=float(csWeaponClient.fov);
    return result;
}
static void drawCSWeaponSight(){
    if(!csWeaponClient.kind)return;
    SCREENINFO screen{};screen.iSize=sizeof screen;if(!eng.pfnGetScreenInfo(&screen))return;
    int w=screen.iWidth,h=screen.iHeight,cx=w/2,cy=h/2;
    wrect_t empty{};eng.pfnSetCrosshair(0,empty,0,0,0);
    if(csWeaponClient.kind==1&&csWeaponClient.fov<90){
        int radius=int(h*.46f);
        for(int y=0;y<h;y+=3){
            float dy=float(y+1-cy),square=float(radius*radius)-dy*dy;
            int extent=square>0?int(std::sqrt(square)):0;
            if(!extent)eng.pfnFillRGBA(0,y,w,std::min(3,h-y),0,0,0,255);
            else{
                eng.pfnFillRGBA(0,y,cx-extent,std::min(3,h-y),0,0,0,255);
                eng.pfnFillRGBA(cx+extent,y,w-cx-extent,std::min(3,h-y),0,0,0,255);
            }
        }
        eng.pfnFillRGBA(cx-radius,cy,radius*2,1,0,0,0,255);
        eng.pfnFillRGBA(cx,cy-radius,1,radius*2,0,0,0,255);
        eng.pfnFillRGBA(cx-1,cy-1,3,3,160,10,10,255);
    }else{
        int length=std::max(5,h/100),gap=std::max(3,h/180);
        eng.pfnFillRGBA(cx-gap-length,cy,length,1,40,240,50,255);
        eng.pfnFillRGBA(cx+gap+1,cy,length,1,40,240,50,255);
        eng.pfnFillRGBA(cx,cy-gap-length,1,length,40,240,50,255);
        eng.pfnFillRGBA(cx,cy+gap+1,1,length,40,240,50,255);
    }
}
