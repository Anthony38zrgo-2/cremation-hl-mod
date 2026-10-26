#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
namespace david_logic {
// Segment against an oriented box after transforming both endpoints to bone space.
inline bool segmentBox(const float a[3],const float b[3],const float box[6],float& fraction,int& axis,float& sign){
    float enter=0,leave=1;axis=2;sign=1;
    for(int k=0;k<3;++k){float delta=b[k]-a[k];
        if(std::fabs(delta)<1e-8f){if(a[k]<box[k]||a[k]>box[k+3])return false;continue;}
        float t0=(box[k]-a[k])/delta,t1=(box[k+3]-a[k])/delta,side=-1;
        if(t0>t1){std::swap(t0,t1);side=1;}
        if(t0>enter){enter=t0;axis=k;sign=side;}leave=std::min(leave,t1);if(enter>leave)return false;
    }
    fraction=enter;return enter<=1&&leave>=0;
}
inline bool descendant(int node,int ancestor,const int* parent){
    for(int k=node;k>0;k=parent[k])if(k==ancestor)return true;return false;
}
inline bool legGone(unsigned mask){return (mask&((1u<<10)|(1u<<11)|(1u<<12)|(1u<<13)|(1u<<14)|(1u<<15)))!=0;}
inline bool armGone(unsigned mask,bool left){return (mask&(left?((1u<<6)|(1u<<7)|(1u<<8)):((1u<<3)|(1u<<4)|(1u<<5))))!=0;}
inline int attackEvent(int requested,unsigned mask){
    bool right=!armGone(mask,false),left=!armGone(mask,true);
    if(requested==1)return right?1:0;if(requested==2)return left?2:0;
    if(requested==3)return right&&left?3:(right?1:(left?2:0));return requested;
}
inline bool runnerEligible(bool alive,bool playerTarget,bool visible,bool recentlyMoving,bool legs,bool busy,float distance){
    return alive&&playerTarget&&visible&&recentlyMoving&&legs&&!busy&&distance>=128&&distance<=768;
}
}
