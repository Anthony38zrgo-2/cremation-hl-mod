#include "mansion_waves.h"
#include <cassert>
#include <cstdio>
int main(){
    const int expected[4][7]={{39,0,0,0,0,1,0},{16,20,0,0,0,1,3},{19,20,20,0,0,1,20},{10,10,10,0,40,0,10}};
    int cursor=0;
    for(int wave=1;wave<=4;++wave){int counts[7]={},spawns[40]={},sectors[4][7]={};
        for(int i=0;i<mansion::hordeTotal(wave);++i){int kind=mansion::kind(wave,i);++counts[kind];++spawns[cursor];++sectors[cursor%4][kind];cursor=(cursor+1)%40;}
        for(int i=0;i<7;++i)assert(counts[i]==expected[wave-1][i]);
        assert(mansion::total(wave)==mansion::hordeTotal(wave)+(wave==4?1:0));
        assert(mansion::kind(wave,mansion::hordeTotal(wave))==5);
        for(int i=0;i<40;++i)assert(spawns[i]==mansion::hordeTotal(wave)/40);
        for(int side=0;side<4;++side){int total=0;for(int kind=0;kind<7;++kind){total+=sectors[side][kind];assert(sectors[side][kind]>=expected[wave-1][kind]/4&&sectors[side][kind]<=(expected[wave-1][kind]+3)/4);}assert(total==mansion::total(wave)/4);}
    }
    assert(mansion::total(0)==0&&mansion::total(5)==0);
    int cultistSlots[40]={};int cultists=0;
    for(int i=0;i<mansion::hordeTotal(4);++i){
        int slot=mansion::cultistSlot(4,i);
        if(slot<0)continue;
        assert(slot<40&&!cultistSlots[slot]);cultistSlots[slot]=1;++cultists;
    }
    assert(cultists==40);
    for(int i=0;i<40;++i)assert(cultistSlots[i]);
    assert(mansion::entersBackDoor(4,78)&&mansion::entersBackDoor(4,79));
    for(int i=0;i<78;++i)assert(!mansion::entersBackDoor(4,i));
    assert(mansion::doorForSector(0)==1&&mansion::doorForSector(1)==1);
    assert(mansion::doorForSector(2)==2&&mansion::doorForSector(3)==2);
    std::puts("PASS: four exact compositions and round-robin CT distribution");
}
