#pragma once
namespace mansion {
static constexpr int SpawnCount=40, SectorCount=4;
inline int hordeTotal(int wave) {return wave>=1&&wave<=4?(wave<=2?40:80):0;}
inline int total(int wave) {return hordeTotal(wave)+(wave>=1&&wave<=4?1:0);}
// Distribute types through the wave, while CT locations rotate independently.
inline int kind(int wave,int index) {
    if(index==hordeTotal(wave))return 5;
    if(wave==1)return 0;
    int sector=index%4,round=index/4;
    if(wave==2)return (round+sector)%2;
    if(wave==3)return (round+sector)%4;
    return round%2?4:(round/2+sector)%4;
}
inline const char* classname(int type) {
    static const char* names[]={"monster_zombie","monster_quake_zombie","monster_fastzombie","monster_poisionzombie","monster_human_grunt","monster_czero_zombie"};
    return names[type];
}
}
