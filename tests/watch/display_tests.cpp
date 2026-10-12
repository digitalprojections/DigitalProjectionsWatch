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
 puts("Display power: manual off, touch wake, BOOT wake, held-button arming and wake-input consumption passed");
}
