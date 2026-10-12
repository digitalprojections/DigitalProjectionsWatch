#include "DisplayPowerRules.h"
#include <cassert>
#include <cstdio>
int main(){
 dpwatch::DisplayPowerRules s;assert(!s.off()&&!s.consumesBoot());
 s.slept(false);assert(s.off()&&s.consumesBoot());assert(!s.poll(false));assert(s.poll(true));
 s.woke(true);assert(!s.off()&&s.consumesBoot());s.poll(true);assert(s.consumesBoot());s.poll(false);assert(!s.consumesBoot());
 s.slept(true);assert(!s.poll(true));assert(!s.poll(false));assert(s.poll(true));
 s.woke(false);assert(!s.off()&&!s.consumesBoot());assert(!s.poll(true));
 for(int i=0;i<100;i++){s.slept(false);assert(s.off());assert(s.poll(true));s.woke(true);s.poll(false);assert(!s.off()&&!s.consumesBoot());}
 dpwatch::PowerTap tap;assert(!tap.update(true,10));assert(!tap.update(false,30));assert(!tap.update(true,100));assert(tap.update(false,180));assert(!tap.update(false,200));assert(!tap.update(true,300));assert(!tap.update(false,1500));tap.reset(true);assert(!tap.update(true,1600));assert(!tap.update(false,1700));assert(!tap.update(true,1800));assert(tap.update(false,1860));tap.reset(false);assert(!tap.update(true,0xfffffff0));assert(tap.update(false,64));
 puts("Display power: manual off, touch wake, BOOT wake, held-button arming and wake-input consumption passed");
}
