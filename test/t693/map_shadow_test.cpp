#include "../t496/map_shadow.h"
#include <cassert>
#include <thread>

int main(){
 using namespace t496map;
 constexpr uint32_t target=0x8f36;
 constexpr uintptr_t display=1;
 assert(!observe(target).known());
 created(display,1,0);
 madeCurrent(display,1);
 generated(5);
 const uint32_t index=2;
 bound(target,5,&index,0,1024);
 allocated(target,1024);
 const auto first=observe(target);
 assert(first.known()&&first.size==1024);
 assert(!mapped(first,-1,1024)&&!mapped(first,0,1025));
 assert(mapped(first,0,1024)>0);
 assert(!mapped(first,0,1024));
 const auto pending=observe(target);
 madeCurrent(display,0);
 std::thread transfer([&]{
  assert(!observe(target).known());
  madeCurrent(display,1);
  assert(unmapped(pending));
  madeCurrent(display,0);
 });
 transfer.join();
 madeCurrent(display,1);
 assert(!unmapped(pending));
 allocated(target,2048);
 assert(observe(target).storage!=first.storage&&!mapped(first,0,1024));
 const auto second=observe(target);
 assert(mapped(second,0,1024));
 bound(target,0);
 assert(!unmapped(second));
 bound(target,5);
 assert(!unmapped(observe(target)));
 deleted(5);
 generated(5);
 bound(target,5);
 allocated(target,1024);
 const auto renewed=observe(target);
 assert(renewed.life!=first.life);
 created(display,2,1);
 madeCurrent(display,2);
 bound(target,5);
 assert(observe(target).life==renewed.life&&observe(target).context!=renewed.context);
 created(display,3,0);
 madeCurrent(display,3);
 bound(target,5);
 assert(!observe(target).known());
 generated(5);
 bound(target,5);
 allocated(target,1024);
 assert(observe(target).life!=renewed.life&&observe(target).group!=renewed.group);
 retired(display,3);
 created(display,3,0);
 madeCurrent(display,3);
 assert(!observe(target).known()&&observe(target).context!=renewed.context);
}
