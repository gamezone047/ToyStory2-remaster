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
// One level-script mechanism as published by its sender (sender-local state).
struct CoopMechanism {uint32_t revision,owner;int32_t value[4];};
// Levels 3-15: scripted actor (boss/special actor) snapshot published by the
// encounter owner. Observers publish the same record with their own damage
// requests in damage[localOwner]. render holds actor bytes 0x24..0x37 (scale
// and flash values the level scripts write); never native pointers.
struct CoopScriptActor {uint32_t revision;uint8_t slot,owner,collision,live;CoopPose pose;int32_t motion[10];uint8_t render[20];int32_t commandOffset;int16_t timer;uint16_t flags;uint32_t damage[2];};
// Levels 3-15: whitelisted level-script globals (see CoopScriptBounds).
struct CoopScriptVars {uint32_t revision,owner;int32_t value[32];};
// Level mechanism group (ColliderSync.h) published by its owner: native
// geometry position, rotation, pending velocity, pending rotation, motion flags
// (0x88) and enable state (bit 0) of up to two colliders, plus up to eight
// state globals. request*: the sender's own compare-and-swap trigger request.
struct CoopColliderState {int32_t pos[3];int16_t rot[3],vel[3],delta[3];uint8_t flags,present;};
struct CoopMover {uint32_t revision;uint8_t owner,group,requestCount,requestVar;int32_t requestFrom,requestTo;uint8_t requestPending,pad[3];int32_t vars[8];CoopColliderState colliders[2];};
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
 CoopMechanism mechanisms[4];
 CoopScriptActor scriptActors[4];CoopScriptVars script;
 int32_t standing;CoopMover movers[24];
 // 2026.11.7.14 (appended at 8892): life generation per actor slot (the host's native respawn
 // advances it; deadActors/removedActors/drops bits refer to the CURRENT life), drops collected
 // in the current life, script removals (no death animation, no coin) and token reveal events
 // (bits 0..15 revision, bit 16 shown, bit 17 owner).
 uint8_t enemyLife[64];uint32_t removedActors[2];uint32_t tokenEvents[5];
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
static_assert(sizeof(CoopMechanism)==24,"Mechanism ABI");
static_assert(sizeof(CoopScriptActor)==116,"Script actor ABI");
static_assert(sizeof(CoopScriptVars)==136,"Script vars ABI");
static_assert(sizeof(CoopColliderState)==32,"Collider ABI");
static_assert(sizeof(CoopMover)==116,"Mover ABI");
static_assert(sizeof(CoopFrame)==8984,"Frame ABI");
static_assert(sizeof(CoopShared)==17984,"Bridge ABI");
static const uint32_t CoopMagic=0x32504354, CoopVersion=23;
static bool CoopValidPose(const CoopPose& p) {
 const int bound=0x10000000;
 return p.x>=-bound&&p.x<=bound&&p.y>=-bound&&p.y<=bound&&p.z>=-bound&&p.z<=bound&&
  p.pitch>=-4096&&p.pitch<=4096&&p.yaw>=-4096&&p.yaw<=4096&&p.roll>=-4096&&p.roll<=4096&&
  p.animation>=-1&&p.animation<35&&p.secondary>=-1&&p.secondary<35&&
  p.frame>=0&&p.frame<=0x10000000&&p.secondaryFrame>=0&&p.secondaryFrame<=0x10000000&&
  p.health>=-1&&p.health<=100;
}
// Level-specific bounds. Slot meaning: House [0] garage vent phases, [1] ramp
// trigger; Backyard [0] twin buttons/gate, [1] toppling board, [2] rising
// platform, [3] challenge stage. Every other level/slot must stay empty.
static bool CoopValidMechanisms(const CoopFrame& f){
 for(unsigned i=0;i<4;i++){
  const CoopMechanism& m=f.mechanisms[i];if(!m.revision)continue;
  const int32_t* v=m.value;if(m.owner>1)return false;
  if(f.level==1){
   if(i==0){for(unsigned k=0;k<4;k++)if(v[k]<0||v[k]>0x1fff)return false;}
   else if(i==1){if(v[0]<0||v[0]>0x1000||v[1]||v[2]||v[3])return false;}
   else return false;
  }else if(f.level==2){
   if(i==0&&(v[0]<0||v[0]>0x1000||v[1]<0||v[1]>0x1000||v[2]<0||v[2]>2||v[3]))return false;
   if(i==1&&((v[0]!=INT32_MIN&&(v[0]< -0x10000||v[0]>0x10000))||v[1]< -0x1000||v[1]>0x1000||v[2]||v[3]))return false;
   if(i==2&&(v[0]< -1||v[0]>0x800||v[1]< -0x10000000||v[1]>0x10000000||v[2]||v[3]))return false;
   if(i==3&&(v[0]<0||v[0]>0xc8||v[1]||v[2]||v[3]))return false;
  }else return false;
 }
 return true;
}
// Per-level script whitelist. Mirrored by GameFrame.cs (CoopTests compares the
// tables through native-rules.bin). Any index/slot not listed must stay empty.
struct CoopScriptBound {uint8_t level,index;int32_t low,high;};
// slot..slotHigh: a record that follows a native slot variable (level 9 minions).
struct CoopActorRule {uint8_t level,index,slot,slotHigh;int16_t healthLow,healthHigh;int32_t commandHigh;};
static const CoopScriptBound CoopScriptBounds[]={
 // level 3 boss: phase, damage, height target, height, expected health, flash, cutscene length
 {3,0,0,0x500},{3,1,0x1000,0x3800},{3,2,0,0x3800},{3,3,-0x100,0x4000},{3,4,-1,1000},{3,5,0,4},{3,6,0,0x400},
 // level 4 progress: target bits, sequence rounds, actor 24 stage
 {4,0,0,7},{4,1,0,7},{4,2,0,0xc8},
 // rescued-friend reward stages
 {5,0,0,0xc8}, {7,0,0,0xc8}, {8,0,0,0xc8}, {10,0,0,0xc8}, {11,0,0,0xc8}, {13,0,0,0xc8}, {14,0,0,0xc8},
 // level 7 NPC door target; level 10 target/puzzle bits; level 11 bridges, pour progress (0 idle, 1..0x3c timer, 0x3d done)
 {5,1,0,1}, {7,1,0,0x4000}, {10,1,0,7}, {11,1,0,0x10000}, {11,2,0,0x10000}, {11,3,0,0x3d}, {11,4,0,0x3d}, {11,5,0,0x3d}, {11,6,0,0x3d}, {11,7,0,0x3d}, {11,8,0,0x3d},
 // level 6 boss: phase, expected health, hurt timer, attack cycle, waypoint x, oscillation, outside flag
 {6,0,0,4},{6,1,-1,1000},{6,2,-0x100,0x12c},{6,3,-0x100,0x258},{6,4,-0x40000,0x40000},{6,5,0,0x800},{6,6,0,1},
 // level 12 boss: phase, expected health, hurt timer, rise velocity
 {12,0,0,4},{12,1,-1,1000},{12,2,-0x100,0x3c},{12,3,0,0x100000},
 // level 9 boss: phase, hit count, pair index, pair slots A/B, expected health, release timer,
 // release flag, orbit angle, bob angle, orbit frame, boss height
 {9,0,0,5},{9,1,0,7},{9,2,0,12},{9,3,7,11},{9,4,7,11},{9,5,-1,1000},{9,6,-0x100,0x258},{9,7,0,1},
 {9,8,0,0xfff},{9,9,0,0xfff},{9,10,0,0x180000},{9,11,-0x10000000,0x10000000},
 // level 15 boss: defeats, fight flags 0/1/2, expected health 0/1/2, hurt timers 0/1/2, last defeated
 {15,0,0,5},{15,1,0,3},{15,2,0,3},{15,3,0,3},{15,4,-1,1000},{15,5,-1,1000},{15,6,-1,1000},
 {15,7,-0x100,0x3c},{15,8,-0x100,0x3c},{15,9,-0x100,0x3c},{15,10,0,2},
};
static const CoopActorRule CoopActorRules[]={
 {3,0,0,0,-1,1000,4096},{6,0,0,0,-1,1000,4096},{12,0,0,0,-1,1000,4096},
 {9,0,0,0,-1,1000,4096},{9,1,7,11,-1,1000,4096},{9,2,7,11,-1,1000,4096},
 {15,0,0,0,-1,1000,4096},{15,1,1,1,-1,1000,4096},{15,2,2,2,-1,1000,4096},
};
static const unsigned CoopScriptBoundCount=sizeof(CoopScriptBounds)/sizeof(CoopScriptBounds[0]);
static const unsigned CoopActorRuleCount=sizeof(CoopActorRules)/sizeof(CoopActorRules[0]);
static bool CoopValidScript(const CoopFrame& f){
 const CoopScriptVars& v=f.script;
 if(v.revision){
  if(v.owner>2)return false;
  for(unsigned i=0;i<32;i++){
   bool listed=false;
   for(unsigned k=0;k<CoopScriptBoundCount;k++){const CoopScriptBound& b=CoopScriptBounds[k];
    if(b.level==f.level&&b.index==i){listed=true;if(v.value[i]<b.low||v.value[i]>b.high)return false;}}
   if(!listed&&v.value[i])return false;
  }
 }else if(v.owner)return false;
 for(unsigned i=0;i<4;i++){
  const CoopScriptActor& a=f.scriptActors[i];if(!a.revision)continue;
  const CoopActorRule* rule=0;
  for(unsigned k=0;k<CoopActorRuleCount;k++)if(CoopActorRules[k].level==f.level&&CoopActorRules[k].index==i)rule=&CoopActorRules[k];
  if(!rule||a.slot<rule->slot||a.slot>rule->slotHigh||a.owner>1||a.live>1)return false;
  const CoopPose& p=a.pose;const int bound=0x10000000;
  if(p.x<-bound||p.x>bound||p.y<-bound||p.y>bound||p.z<-bound||p.z>bound||p.pitch<-4096||p.pitch>4096||p.yaw<-4096||p.yaw>4096||p.roll<-4096||p.roll>4096||
     p.animation< -1||p.animation>=100||p.secondary< -1||p.secondary>=100||p.frame<0||p.frame>bound||p.secondaryFrame<0||p.secondaryFrame>bound||
     p.health<rule->healthLow||p.health>rule->healthHigh)return false;
  for(unsigned k=0;k<10;k++)if(a.motion[k]<-bound||a.motion[k]>bound)return false;
  if(a.commandOffset< -1||a.commandOffset>rule->commandHigh||a.timer< -1||a.damage[0]>1000000||a.damage[1]>1000000)return false;
 }
 return true;
}
// Mechanism groups per level (ColliderGroups.h); record i is group i. Mirrored by GameFrame.cs.
static const uint8_t CoopMoverGroupCounts[16]={0,0,0,0,12,8,0,10,2,0,3,6,0,7,2,0};
static unsigned CoopMoverGroups(uint32_t level){return level<16?CoopMoverGroupCounts[level]:0u;}
static bool CoopValidMovers(const CoopFrame& f){
 if(f.standing< -1||f.standing>255)return false;
 for(unsigned i=0;i<24;i++){const CoopMover& m=f.movers[i];if(!m.revision)continue;
  if(i>=CoopMoverGroups(f.level)||m.group!=i||m.owner>1||m.requestVar>7||m.requestPending>1||m.pad[0]||m.pad[1]||m.pad[2])return false;
  if(m.requestFrom<-0x10000000||m.requestFrom>0x10000000||m.requestTo<-0x10000000||m.requestTo>0x10000000)return false;
  for(unsigned k=0;k<8;k++)if(m.vars[k]<-0x10000000||m.vars[k]>0x10000000)return false;
  for(unsigned k=0;k<2;k++){const CoopColliderState& c=m.colliders[k];
   if(c.present>1||(c.flags&~0x89))return false;
   for(unsigned a=0;a<3;a++)if(c.pos[a]<-0x10000000||c.pos[a]>0x10000000)return false;}
 }
 return true;
}
static bool CoopValidFrame(const CoopFrame& f) {
 if(!(f.level<=16&&f.pickupCount<=512&&CoopValidPose(f.pose)&&f.reserved==0&&f.tokens<=255&&f.scriptFlags<=3))return false;
 if(!CoopValidMechanisms(f)||!CoopValidScript(f)||!CoopValidMovers(f))return false;
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
 for(unsigned i=0;i<5;i++)if(f.tokenEvents[i]&~0x3ffffu)return false;
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
