#pragma once
namespace dpwatch {
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
