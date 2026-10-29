// Use Valve's original .357 sprite through the engine crosshair renderer.
// The donor HUD may clear it while handling its own inventory updates.
static HSPRITE revolverCrosshairSprite=0;
static void restoreRevolverCrosshair(){
    auto view=eng.GetViewModel();
    if(!view||!view->model||std::strcmp(view->model->name,"models/v_357.mdl"))return;
    if(!revolverCrosshairSprite)revolverCrosshairSprite=eng.pfnSPR_Load("sprites/cremation/hl_revolver_crosshairs.spr");
    if(!revolverCrosshairSprite)return;
    wrect_t rect{};rect.left=48;rect.right=72;rect.top=0;rect.bottom=24;
    eng.pfnSetCrosshair(revolverCrosshairSprite,rect,255,255,255);
}
