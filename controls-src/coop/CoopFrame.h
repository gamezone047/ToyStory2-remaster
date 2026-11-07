#pragma once
#include <stdint.h>
#include <string.h>

// Value-only wire/IPC ABI. Never send native pointers or native actor memory.
#pragma pack(push,1)
struct CoopPose {
 int32_t x,y,z;
 int16_t pitch,yaw,roll,animation,secondary;
 int32_t frame,secondaryFrame;
 int16_t health;
};
struct CoopShot {uint32_t sequence;int32_t start[3],end[3],step[3];int16_t life,length;uint32_t color;};
struct CoopObject {uint32_t revision,owner;int32_t position[3],direction[2];int16_t fall,push,progress,duration,heading,node,minimum,path;};
struct CoopBoss {
 uint32_t revision,owner,phase;
 CoopPose pose;
 int32_t motion[10],commandOffset;
 uint32_t damage[2];
 int16_t timer,flags;
 uint32_t hurtTimer,previousHealth,flashTick,collisionFlags;
};
struct CoopEnemy {uint32_t revision;int32_t position[3];int16_t pitch,yaw,roll,model,health;uint16_t flags;uint32_t damage;};
struct CoopPlatform {uint32_t revision,owner;int32_t velocity,angle,delta;};
struct CoopAnimation {int16_t animation,secondary;int32_t frame,secondaryFrame;};
struct CoopEvent {uint32_t revision,owner;int32_t value;};
struct CoopGarage {uint32_t revision,owner;CoopPose pose;int32_t motion[10];uint32_t phase,node,laps,playerLaps,region,direction;int32_t health;uint32_t flags;};
struct CoopShovel {uint32_t revision,owner;int32_t velocity,height;};
struct CoopBurrow {uint32_t revision;CoopPose pose;int32_t motion[10];uint32_t flags,hole,previous,timer,closed;};
struct CoopWorldActor {uint32_t revision;CoopPose pose;int32_t motion[10];uint32_t flags;};
struct CoopDuck {uint32_t revision,owner;int32_t value[10];};
struct CoopTree {uint32_t revision;int32_t angle,velocity;};
struct CoopFrame {
 uint32_t sequence,level,layout,pickupCount;
 CoopPose pose;
 uint32_t coins[16],deadActors[2],tokens,reserved;
 uint32_t drops[2];
 CoopShot shots[8];
 CoopObject objects[10];
 uint16_t collections[512];
 uint32_t scriptFlags;
 uint32_t actorCollections[2];
 CoopBoss boss;
 CoopEnemy enemies[64];
 CoopPlatform platform;
 CoopGarage garage;CoopShovel shovel;CoopEvent pause,music;uint32_t damageFlash;uint32_t spawnedTokens;CoopAnimation enemyAnimations[64];uint32_t worldContext;
 uint32_t rcPushSequence,rcPushAngle;CoopBurrow burrow;
 CoopWorldActor mower;CoopDuck duck;CoopTree tree;CoopEvent travel;CoopPlatform backyardPlatforms[2];
};
struct CoopShared {
 uint32_t magic,version,helperTick,gameTick;
 CoopFrame local,remote;
};
#pragma pack(pop)
static_assert(sizeof(CoopPose)==32,"Pose ABI");
static_assert(sizeof(CoopShot)==48,"Shot ABI");
static_assert(sizeof(CoopObject)==44,"Object ABI");
static_assert(sizeof(CoopBoss)==116,"Boss ABI");
static_assert(sizeof(CoopEnemy)==32,"Enemy ABI");
static_assert(sizeof(CoopFrame)==5408,"Frame ABI");
static_assert(sizeof(CoopShared)==10832,"Bridge ABI");
static const uint32_t CoopMagic=0x32504354, CoopVersion=14;
static bool CoopValidPose(const CoopPose& p) {
 const int bound=0x10000000;
 return p.x>=-bound&&p.x<=bound&&p.y>=-bound&&p.y<=bound&&p.z>=-bound&&p.z<=bound&&
  p.pitch>=-4096&&p.pitch<=4096&&p.yaw>=-4096&&p.yaw<=4096&&p.roll>=-4096&&p.roll<=4096&&
  p.animation>=-1&&p.animation<35&&p.secondary>=-1&&p.secondary<35&&
  p.frame>=0&&p.frame<=0x10000000&&p.secondaryFrame>=0&&p.secondaryFrame<=0x10000000&&
  p.health>=-1&&p.health<=100;
}
static bool CoopValidFrame(const CoopFrame& f) {
 if(!(f.level<=16&&f.pickupCount<=512&&CoopValidPose(f.pose)&&f.reserved==0&&f.tokens<=255&&f.scriptFlags<=3))return false;
 for(unsigned i=0;i<64;i++){
  const CoopEnemy& e=f.enemies[i];if(!e.revision)continue;
  if(e.model<=0||e.model>255||e.health<0||e.health>=80||e.damage>1000000||(e.flags&~0x10c)!=0||
   e.pitch< -4096||e.pitch>4096||e.yaw< -4096||e.yaw>4096||e.roll< -4096||e.roll>4096)return false;
  for(unsigned a=0;a<3;a++)if(e.position[a]<-0x10000000||e.position[a]>0x10000000)return false;
 }
 for(unsigned i=0;i<64;i++){const CoopAnimation& a=f.enemyAnimations[i];
  if(a.animation< -1||a.animation>=100||a.secondary< -1||a.secondary>=100||a.frame<0||a.frame>0x10000000||a.secondaryFrame<0||a.secondaryFrame>0x10000000)return false;
 }
 for(unsigned i=0;i<2;i++){const CoopPlatform& b=f.backyardPlatforms[i];if(b.revision&&(b.owner>1||b.velocity< -65536||b.velocity>65536||b.angle< -1792||b.angle>(i?896:1792)||b.delta< -4096||b.delta>4096))return false;}
 if(f.worldContext&&(!(f.worldContext&0x10000)||(f.worldContext&~0x10fffu)||(f.worldContext&255)>31||((f.worldContext&0x100)&&(f.worldContext&255)!=5)))return false;
 if(f.mower.revision){if(!CoopValidPose(f.mower.pose)||(f.mower.flags&~0x19cu))return false;for(unsigned i=0;i<10;i++)if(f.mower.motion[i]<-0x10000000||f.mower.motion[i]>0x10000000)return false;}
 if(f.duck.revision){if(f.duck.owner>1)return false;for(unsigned i=0;i<10;i++)if(f.duck.value[i]<-0x10000000||f.duck.value[i]>0x10000000)return false;
  if(f.duck.value[3]<0||f.duck.value[3]>4096||f.duck.value[6]<0||f.duck.value[6]>4096||f.duck.value[7]<0||f.duck.value[7]>4096||f.duck.value[8]<0||f.duck.value[8]>30||f.duck.value[9]<0||f.duck.value[9]>1)return false;}
 if(f.tree.revision&&(f.tree.angle<0||f.tree.angle>581||f.tree.velocity<0||f.tree.velocity>4096))return false;
 if(f.travel.revision&&(f.travel.owner>1||f.travel.value<1||f.travel.value>15))return false;
 if(f.rcPushAngle>4095)return false;
 if(f.burrow.revision){const CoopBurrow& b=f.burrow;
  if(!CoopValidPose(b.pose)||b.flags&~0x1acu||b.hole>=32||b.previous>=32||b.timer>100)return false;
  for(unsigned i=0;i<10;i++)if(b.motion[i]<-0x10000000||b.motion[i]>0x10000000)return false;
 }
 if(f.spawnedTokens>31)return false;
 if(f.damageFlash>180)return false;
 if(f.pause.revision&&(f.pause.owner>1||f.pause.value<0||f.pause.value>1))return false;
 if(f.music.revision&&(f.music.owner>1||f.music.value<0||f.music.value>=22))return false;
 if(f.shovel.revision&&(f.shovel.owner>1||f.shovel.velocity< -1||f.shovel.velocity>0x1000000||f.shovel.height< -0x10000000||f.shovel.height>0x10000000))return false;
 if(f.garage.revision){const CoopGarage& g=f.garage;
  if(g.owner>1||!CoopValidPose(g.pose)||g.pose.animation>=5||g.pose.secondary>=5||g.phase>3||g.node>(f.level==2?64u:44u)||g.laps>3||g.playerLaps>3||g.region>15||g.direction>1||(g.health!=1&&g.health!=202)||(g.flags&~0x18c))return false;
  for(unsigned i=0;i<10;i++)if(g.motion[i]< -0x10000000||g.motion[i]>0x10000000)return false;
 }
 if(f.platform.revision&&(f.platform.owner>1||f.platform.velocity< -65536||f.platform.velocity>65536||f.platform.angle< -1792||f.platform.angle>1792||f.platform.delta< -4096||f.platform.delta>4096))return false;
 if(f.boss.revision){
  const CoopBoss& b=f.boss;
  if(b.owner>1||b.phase>3||!CoopValidPose(b.pose)||b.pose.animation>=8||b.pose.secondary>=8||b.commandOffset<0||b.commandOffset>2048||
     b.hurtTimer>3600||b.previousHealth>100||b.flashTick>1||b.collisionFlags>255||b.damage[0]>1000000||b.damage[1]>1000000)return false;
  for(unsigned i=0;i<10;i++)if(b.motion[i]<-0x10000000||b.motion[i]>0x10000000)return false;
 }
 for(unsigned i=0;i<8;i++){
  const CoopShot& s=f.shots[i];if(!s.sequence)continue;
  if(s.life<0||s.life>64||s.length<0||s.length>32767)return false;
  for(int axis=0;axis<3;axis++)if(s.start[axis]<-0x10000000||s.start[axis]>0x10000000||
   s.end[axis]<-0x10000000||s.end[axis]>0x10000000||s.step[axis]<-0x1000000||s.step[axis]>0x1000000)return false;
 }
 for(unsigned i=0;i<10;i++){
  const CoopObject& o=f.objects[i];if(!o.revision)continue;
  if(o.owner>1||o.path<0||o.node<0||o.minimum<0||o.minimum>o.node)return false;
  for(int a=0;a<3;a++)if(o.position[a]<-0x10000000||o.position[a]>0x10000000)return false;
  for(int a=0;a<2;a++)if(o.direction[a]<-0x1000000||o.direction[a]>0x1000000)return false;
 }
 return true;
}
static bool CoopObjectNewer(const CoopObject& a,const CoopObject& b){
 return a.revision&&(!b.revision||(int32_t)(a.revision-b.revision)>0||(a.revision==b.revision&&a.owner<b.owner));
}
static bool CoopBit(const uint32_t* bits,unsigned n){return (bits[n/32]&(1u<<(n%32)))!=0;}
static void CoopSetBit(uint32_t* bits,unsigned n){bits[n/32]|=1u<<(n%32);}
static int CoopLerp(int from,int to,unsigned elapsed,unsigned duration){
 if(!duration||elapsed>=duration)return to;
 return from+(int)(((int64_t)to-from)*elapsed/duration);
}
static short CoopLerpAngle(short from,short to,unsigned elapsed,unsigned duration){
 int delta=((int)to-from)&4095;if(delta>=2048)delta-=4096;
 return (short)(CoopLerp(from,from+delta,elapsed,duration)&4095);
}
// The native renderer identifies its local player as the LAST list element.
// Its matrix storage holds 64 objects. Keep that limit and a zero terminator.
static unsigned CoopBuildRenderList(uintptr_t* out,const uintptr_t* native,
                                   uintptr_t local,uintptr_t remote) {
 unsigned n=0;
 for(unsigned i=0;i<64&&native[i];++i) {
  if(native[i]!=local&&n<(remote?62u:63u))out[n++]=native[i];
 }
 if(remote)out[n++]=remote;
 out[n++]=local;out[n]=0;return n;
}
