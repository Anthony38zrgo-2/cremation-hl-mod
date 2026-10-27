#include "david_logic.h"
#include "david_assets.h"
#include <cassert>
#include <cstdio>
int main(){
 using namespace david_logic;
 float box[6]={-1,-1,-1,1,1,1},fraction,sign;int axis;
 float start[3]={-3,0,0},end[3]={3,0,0};assert(segmentBox(start,end,box,fraction,axis,sign));assert(std::fabs(fraction-1.f/3)<1e-6f&&axis==0&&sign==-1);
 float parallelStart[3]={-3,2,0},parallelEnd[3]={3,2,0};assert(!segmentBox(parallelStart,parallelEnd,box,fraction,axis,sign));
 float shortStart[3]={-3,0,0},shortEnd[3]={-2,0,0};assert(!segmentBox(shortStart,shortEnd,box,fraction,axis,sign));
 float inside[3]={0,0,0};assert(segmentBox(inside,end,box,fraction,axis,sign)&&fraction==0);
 assert(descendant(5,3,david_assets::parent)&&!descendant(8,3,david_assets::parent));
 assert(attackEvent(3,0)==3&&attackEvent(3,1<<3)==2&&attackEvent(1,1<<4)==0&&attackEvent(3,(1<<3)|(1<<6))==0);
 assert(legGone(1<<12)&&!legGone(1<<3));
 assert(runnerEligible(true,true,true,true,true,false,400));
 assert(!runnerEligible(true,true,true,true,true,false,100));
 assert(!runnerEligible(true,true,true,true,false,false,400));
 assert(!runnerEligible(true,true,true,true,true,true,400));
 int strikes[4]{};for(int roll=0;roll<300;++roll){int david=uniformAttackEvent(roll,0),steve=uniformAttackEvent(roll,0);++strikes[david];assert(steve==david);assert(uniformAttackEvent(roll,1<<3)==2);assert(uniformAttackEvent(roll,1<<6)==1);assert(uniformAttackEvent(roll,(1<<3)|(1<<6))==0);}
 assert(strikes[1]==100&&strikes[2]==100&&strikes[3]==100);
 long long product=1;
 for(int n=1;n<16;++n){assert(david_assets::bodyBase[n]==product);product*=david_assets::bodyCount[n];assert(product<2147483647);}
 std::puts("PASS: finite hit segments, missing limb anatomy, melee eligibility, run gating and bodygroup encoding.");
}
