#include "protocol.h"
#include <cassert>
#include <cstdio>
int main(){t246::Identity id;id.owner=0x123456789ULL;id.context=3;id.surface=4;id.lease=5;id.token=6;auto t=t246::makeTail(id);static_assert(sizeof(t)==1024);
 assert(t246::accepts(t,id,false,true));for(int i=0;i<256;i++){auto bad=t;bad.words[i]^=1;assert(!t246::accepts(bad,id,false,true));}
 assert(!t246::accepts(t,id,true,true));assert(!t246::accepts(t,id,false,false));auto old=id;for(int i=0;i<5;i++){id=old;switch(i){case 0:id.owner++;break;case 1:id.context++;break;case 2:id.surface++;break;case 3:id.lease++;break;case 4:id.token++;break;}assert(!t246::accepts(t,id,false,true));}
 assert(t246::rangeValid(256,1024,1280,256));assert(!t246::rangeValid(260,1024,1284,256));assert(!t246::rangeValid(UINT64_MAX,1024,1280,256));
 std::puts("PASS T246 256 tail-word tamper cases, high owner words, five generation changes, cancel/lease, aligned/range-overflow. WINDOW_GPU=NOT_RUN");}
