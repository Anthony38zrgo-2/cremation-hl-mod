#include "mansion_waves.h"
#include <cassert>
#include <cstdio>
int main(){
    const int expected[4][5]={{20,0,0,0,0},{10,10,0,0,0},{10,10,10,10,0},{5,5,5,5,20}};
    int cursor=0;
    for(int wave=1;wave<=4;++wave){int counts[5]={},spawns[10]={};
        for(int i=0;i<mansion::total(wave);++i){++counts[mansion::kind(wave,i)];++spawns[cursor];cursor=(cursor+1)%10;}
        for(int i=0;i<5;++i)assert(counts[i]==expected[wave-1][i]);
        for(int i=0;i<10;++i)assert(spawns[i]==mansion::total(wave)/10);
    }
    assert(mansion::total(0)==0&&mansion::total(5)==0);
    std::puts("PASS: four exact compositions and round-robin CT distribution");
}
