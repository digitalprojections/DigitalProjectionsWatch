#pragma once
#include <cstdint>
namespace dpwatch {
class PowerTap {
 bool ready=true,held=false;uint32_t pressedAt=0;
public:
 void reset(bool down){ready=!down;held=false;}
 bool update(bool down,uint32_t now){
  if(!ready){if(!down)ready=true;return false;}
  if(down&&!held){held=true;pressedAt=now;}
  if(!down&&held){held=false;const uint32_t duration=now-pressedAt;return duration>=40&&duration<=800;}
  return false;
 }
};
class DisplayPowerRules {
 bool asleep=false,armed=false,wakeHeld=false;
public:
 bool off()const{return asleep;}
 bool consumesBoot()const{return asleep||wakeHeld;}
 void slept(bool down){asleep=true;armed=!down;wakeHeld=false;}
 void woke(bool down){asleep=false;wakeHeld=down;}
 bool poll(bool down){if(!down){wakeHeld=false;armed=true;}return asleep&&armed&&down;}
};
}
