#pragma once
namespace mansion {
inline int total(int wave) {return wave>=1&&wave<=4?(wave<=2?20:40):0;}
// Distribute types through the wave, while CT locations rotate independently.
inline int kind(int wave,int index) {
    if(wave==1)return 0;
    if(wave==2)return index%2;
    if(wave==3)return index%4;
    return index%8<4?index%8:4;
}
inline const char* classname(int type) {
    static const char* names[]={"monster_zombie","monster_quake_zombie","monster_fastzombie","monster_poisionzombie","monster_human_grunt"};
    return names[type];
}
}
