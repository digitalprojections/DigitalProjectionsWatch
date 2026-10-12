#pragma once
#include <cstdint>
#include <cstring>
#include <cmath>
#include <algorithm>
namespace wristlink {
constexpr uint32_t Magic=0x314c5257;
constexpr uint16_t Valid=1,Paused=2;
#pragma pack(push,1)
struct Packet {
 uint32_t magic=Magic;uint8_t version=1,type=1;uint16_t flags=0;
 uint32_t session=0,sequence=0;
 int16_t ax=0,ay=0,az=0,gx=0,gy=0,gz=0;
 uint16_t throttle=500,center=0;uint32_t checksum=0;
};
#pragma pack(pop)
static_assert(sizeof(Packet)==36,"Stable wire format");
inline uint32_t crc(const void* data,size_t size){uint32_t c=~0u;auto*b=static_cast<const uint8_t*>(data);while(size--){c^=*b++;for(int i=0;i<8;++i)c=(c>>1)^(0xedb88320u & (0u-(c&1)));}return ~c;}
inline void seal(Packet& p){p.checksum=crc(&p,sizeof(p)-4);}
inline bool valid(const Packet&p){return p.magic==Magic&&p.version==1&&(p.type==1||p.type==2)&&p.session&&p.throttle<=1000&&(p.flags&~(Valid|Paused))==0&&p.checksum==crc(&p,sizeof(p)-4);}
inline int16_t quantize(float value,float scale){if(!std::isfinite(value))return 0;return static_cast<int16_t>(std::max(-32767.f,std::min(32767.f,value*scale)));}
constexpr float Rad=0.01745329252f,Deg=57.29577951f;
inline bool project(float x,float y,float z,float roll,float pitch,int& sx,int&sy){if(z<1)return false;float yy=y-pitch*z;float c=std::cos(roll),s=std::sin(roll);float xx=c*x-s*yy;yy=s*x+c*yy;float px=160+135*xx/z,py=110+135*yy/z;if(!std::isfinite(px)||!std::isfinite(py)||std::fabs(px)>2000||std::fabs(py)>2000)return false;sx=static_cast<int>(px);sy=static_cast<int>(py);return true;}
enum class ButtonAction {None,Center,Pause};
class Buttons {
 bool held=false,longPress=false;uint32_t pressAt=0;
public:
 ButtonAction update(bool down,uint32_t now){
  if(down&&!held){held=true;longPress=false;pressAt=now;}
  if(held&&down&&!longPress&&uint32_t(now-pressAt)>=1000){longPress=true;return ButtonAction::Pause;}
  if(!down&&held){held=false;if(!longPress&&uint32_t(now-pressAt)>=40)return ButtonAction::Center;}
  return ButtonAction::None;
 }
 void reset(){held=longPress=false;}
};
class Flight {
 Packet sample{};bool seen=false,started=false;uint32_t receivedAt=0,tickAt=0;
 float fusedRoll=0,fusedPitch=0,zeroRoll=0,zeroPitch=0;
public:
 float roll=0,pitch=0,yaw=0,x=0,y=0,distance=0;uint32_t packets=0;unsigned gates=0;
 bool connected(uint32_t now)const{return seen&&uint32_t(now-receivedAt)<400;}
 bool running(uint32_t now)const{return connected(now)&&(sample.flags&Valid)&&!(sample.flags&Paused);}
 const Packet& packet()const{return sample;}
 bool receive(const Packet&p,uint32_t now){
  if(!valid(p)||p.type!=1)return false;
  if(seen){if(p.session!=sample.session&&uint32_t(now-receivedAt)<2000)return false;
   if(p.session==sample.session&&int32_t(p.sequence-sample.sequence)<=0)return false;}
  bool center=!seen||p.session!=sample.session||p.center!=sample.center;
  sample=p;receivedAt=now;seen=true;++packets;
  if(center){fusedRoll=std::atan2(float(p.ay),float(p.az))*Deg;fusedPitch=std::atan2(-float(p.ax),std::hypot(float(p.ay),float(p.az)))*Deg;zeroRoll=fusedRoll;zeroPitch=fusedPitch;roll=pitch=yaw=x=y=0;}
  return true;
 }
 void tick(uint32_t now){float dt=started?std::min(.1f,float(uint32_t(now-tickAt))*.001f):.02f;tickAt=now;started=true;if(!running(now))return;
  float ar=std::atan2(float(sample.ay),float(sample.az))*Deg;
  float ap=std::atan2(-float(sample.ax),std::hypot(float(sample.ay),float(sample.az)))*Deg;
  float alpha=std::exp(-dt*4);fusedRoll=alpha*(fusedRoll+sample.gx*.01f*dt)+(1-alpha)*ar;fusedPitch=alpha*(fusedPitch+sample.gy*.01f*dt)+(1-alpha)*ap;
  roll=std::max(-50.f,std::min(50.f,fusedRoll-zeroRoll));pitch=std::max(-40.f,std::min(40.f,fusedPitch-zeroPitch));
  yaw=std::remainder(yaw+sample.gz*.01f*dt,360.f);x=std::max(-20.f,std::min(20.f,x+roll*.12f*dt));y=std::max(-15.f,std::min(15.f,y-pitch*.12f*dt));
  auto old=int(distance/40);distance+=sample.throttle*.025f*dt;if(int(distance/40)>old&&std::hypot(x,y)<6)++gates;
 }
};
}
