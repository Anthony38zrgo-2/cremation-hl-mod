#pragma once
namespace mansion {
static constexpr int SpawnCount=40, SectorCount=4;
inline int hordeTotal(int wave) {return wave>=1&&wave<=4?(wave<=2?40:80):0;}
inline int total(int wave) {return hordeTotal(wave)+(wave==4?1:0);}
inline int johnnyTotal(int wave) {return wave==1?10:(wave==2?15:0);}
inline bool isJohnny(int wave,int index) {return index>=0&&index<johnnyTotal(wave)&&index<hordeTotal(wave);}
inline int kind(int wave,int index);
inline int cultistSlot(int wave,int index) {
    if(wave!=4||index<0||index>=hordeTotal(wave)||kind(wave,index)!=4)return -1;
    return (index/8)*4+(index%8)-4;
}
inline bool entersBackDoor(int wave,int index) {return wave==4&&(index==78||index==79);}
inline int doorForSector(int sector) {return sector%4>=2?2:1;}
// Validated walkable points on the outside perimeter of the large house.
// They are ordered clockwise so the cultists can visibly circle the building.
static constexpr float cultistPerimeter[40][3]={
    {-800,448,24.1f},{-672,448,24.1f},{-544,448,24.1f},{-416,448,24.1f},{-288,448,24.1f},
    {-160,448,24.1f},{-32,448,24.1f},{96,448,24.1f},{224,448,24.1f},{320,448,24.1f},
    {288,288,24.1f},{320,160,24.1f},{320,32,24.1f},{320,-96,24.1f},{320,-224,24.1f},
    {320,-352,24.1f},{320,-480,24.1f},{320,-608,24.1f},{320,-736,24.1f},{320,-864,24.1f},
    {256,-928,24.1f},{128,-928,24.1f},{0,-928,24.1f},{-128,-928,24.1f},{-256,-928,24.1f},
    {-384,-928,24.1f},{-512,-928,24.1f},{-640,-928,24.1f},{-768,-928,24.1f},{-896,-928,24.1f},
    {-896,-768,24.1f},{-896,-640,24.1f},{-896,-512,24.1f},{-896,-384,24.1f},{-896,-256,24.1f},
    {-864,-128,24.1f},{-928,0,24.1f},{-928,128,24.1f},{-896,256,24.1f},{-896,384,24.1f}
};
// Distribute types through the wave, while CT locations rotate independently.
inline int kind(int wave,int index) {
    if(index==hordeTotal(wave))return 5;
    if(wave>=1&&wave<=3&&index==0)return 5;
    if(wave==1)return 0;
    int sector=index%4,round=index/4;
    if(wave==2)return (round+sector)%2;
    if(wave==3)return (round+sector)%4;
    return round%2?4:(round/2+sector)%4;
}
inline const char* classname(int type) {
    static const char* names[]={"monster_zombie","monster_max","monster_randy","monster_poisionzombie","monster_human_grunt","monster_czero_zombie"};
    return names[type];
}
}
