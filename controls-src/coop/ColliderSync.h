// Level mechanism groups (levels 4-14): moving colliders (pulleys, lifts, path
// movers, balances, bridges, rotating platforms) and the switch/door/puzzle
// state globals that drive them.
// Ownership: a player standing on one of the group's colliders (native 559e80)
// owns it, the host on a tie. Groups with a radius are also owned by the
// nearest player within that radius of the anchor (the first collider, or the
// anchor mesh), with hysteresis; otherwise the last owner keeps the group
// (host first). The owner's native script runs the mechanism; it publishes
// value-only collider transforms (geometry position, rotation, pending
// velocity/rotation delta, motion and enable flags) and the group's globals.
// Observers write them before and after their native level tick; meshes and
// other visuals are recomputed natively from them every tick. A new owner first
// adopts the last published state, then continues natively from it.
// Pointer globals are bytecode programs in toy2.exe static data and are only
// written when they point into that image. Player-carrying, camera, HUD and
// hazard globals are never listed (local by design).
// trigger: a state global the observer's own player can change (button, target,
// step-on). When the observer's native tick changes it from the value it was
// given, the observer sends one compare-and-swap request (var, from, to); the
// owner applies it once, only if its own value still equals 'from'.
// apply: the observer/owner reaches the value through this native transition instead of a raw
// write (it redraws meshes and moves colliders). mask: bits that are shared (0 = all).
typedef void (*GroupApplyFn)(int value);
// mesh/axis: instead of a global, the value is axis 0/1/2 of that mesh's position (4ccef0 units);
// observers place the mesh (4cce30, >>5) when all axes arrive. For meshes the native script only
// moves while the LOCAL player is near (level 4 crumbling platforms).
// read: the value is derived from several globals (packed); apply must then write them back.
typedef int (*GroupReadFn)();
struct ColliderVarDesc {uintptr_t address;bool pointer,trigger;GroupApplyFn apply;int mask;int mesh,axis;GroupReadFn read;};
static int GroupVarValue(const ColliderVarDesc& v){int x=v.read?v.read():M<int>(v.address);return v.mask?x&v.mask:x;}
static void GroupVarSet(const ColliderVarDesc& v,int value){
 if(v.apply){if(GroupVarValue(v)!=value)v.apply(value);}
 else if(v.mask)M<int>(v.address)=(M<int>(v.address)&~v.mask)|(value&v.mask);
 else M<int>(v.address)=value;
}
// colliderFrom[k]: the collider index is read from that global at run time (level 13 platform
// records); colliders[k] must then be -2.
// refresh: observer hook after the owner's values changed native state (visuals the native
// script only redraws on its own events). request: owner hook after applying a peer request.
typedef void (*GroupRefreshFn)();
typedef void (*GroupRequestFn)(unsigned var,int from,int to);
struct ColliderGroupDesc {unsigned level;int colliders[2];ColliderVarDesc vars[8];int anchorMesh;int radius;uintptr_t colliderFrom[2];GroupRefreshFn refresh;GroupRequestFn request;};
static int GroupCollider(const ColliderGroupDesc& g,unsigned k){
 if(g.colliders[k]!=-2)return g.colliders[k];
 int c=g.colliderFrom[k]?M<int>(g.colliderFrom[k]):-1;return c>=0&&c<64?c:-1;
}
static void Level14Refresh();static void Level14Request(unsigned,int,int);
// ---- Level 10 floor-switch puzzle (tick 425f60, 426f5e..4270c7) ---------------------------
// A landing on surface 0x20/0x21/0x22 (re-armed by 52fc24 between landings) moves one step
// between the counters 52fc50/52fc30/52fcd8 (0..2 each); meshes 0x12/0x13/0x14 follow them every
// tick (425680). All three at 1 -> 52fc14 bit 4: collider 0x13 off, lamps, path movers start
// (shared one-way through the script engine). The counters are one packed group value, so an
// observer's landing (which changes two counters) is one compare-and-swap request.
static int Level10SwitchRead(){return (M<int>(0x52fc50)&3)|(M<int>(0x52fc30)&3)<<2|(M<int>(0x52fcd8)&3)<<4;}
static void Level10Switches(int value){M<int>(0x52fc50)=value&3;M<int>(0x52fc30)=value>>2&3;M<int>(0x52fcd8)=value>>4&3;}
// ---- Level 11 paint can (tick 42a130) ------------------------------------------------------
// Three paint buttons (table 4f3fdc, landing on the collider) change the colour bits 52fd14 and
// 428890(new) performs the transition (shows/hides the colour meshes 4f3f48 table, rotates them,
// rewrites route entries 4f3e00..4f3e56). Bit 0x800 is the local blink (428ba0) and is not shared.
// Four level buttons (table 4f3fcc) set button bits 52fd3c through 4292c0(new) (each pressed
// collider moves +-0xe10 in y) and the target fill 52fd30; the fill 52fdfc moves to it every tick
// and 428c80 redraws the can contents from it. Shared as one proximity group anchored on the can.
typedef void(__cdecl *PaintStateFn)(int);
static PaintStateFn paintColour=(PaintStateFn)0x428890,paintLevelButtons=(PaintStateFn)0x4292c0;
static void Level11Colour(int value){paintColour(value);}
static void Level11Buttons(int value){paintLevelButtons(value);}
// A level-button press also sets the target fill (42a353: -1600 * n from table 4f3fcc); a request
// carries one value, so the owner derives the target from the pressed button bit.
static void Level11Request(unsigned var,int,int to){
 static const int bits[4]={0x10,0x20,0x40,0x80},levels[4]={0,0x14,0x28,0x48};
 if(var!=1)return;
 for(int k=3;k>=0;k--)if(to&bits[k]){M<int>(0x52fd30)=-1600*levels[k];return;}
}
#include "ColliderGroups.h"
static const unsigned colliderGroupCount=sizeof(colliderGroups)/sizeof(colliderGroups[0]);
typedef void(__cdecl *ColliderPlaceFn)(int,int,int,int);
typedef void(__cdecl *MeshPositionFn)(int,int*);
static ColliderPlaceFn colliderPlace=(ColliderPlaceFn)0x488510;
static MeshPositionFn meshPosition=(MeshPositionFn)0x4ccef0;
static const ColliderGroupDesc* ColliderGroup(unsigned index){
 unsigned n=0;
 for(unsigned i=0;i<colliderGroupCount;i++)if(colliderGroups[i].level==cachedLevel){if(n==index)return &colliderGroups[i];++n;}
 return 0;
}
static uintptr_t ColliderGeometry(int collider){
 if(collider<0||collider>=64)return 0;
 short g=M<short>(0x728720+collider*0x34+0x30);return g>0&&g<256?0x7295a8+g*0x34:0;
}
static void ReadColliderGroup(const ColliderGroupDesc& g,CoopMover& m){
 int meshPos[3]={0,0,0},meshId=-1;
 for(unsigned k=0;k<8;k++)if(g.vars[k].mesh>0&&g.vars[k].mesh!=meshId){meshId=g.vars[k].mesh;meshPos[0]=meshPos[1]=meshPos[2]=0;meshPosition(meshId,meshPos);}
 for(unsigned k=0;k<8;k++){m.vars[k]=g.vars[k].mesh>0?meshPos[g.vars[k].axis&3]:g.vars[k].address?GroupVarValue(g.vars[k]):0;
  if(m.vars[k]<-0x10000000||m.vars[k]>0x10000000)m.vars[k]=0;}
 for(unsigned k=0;k<2;k++){
  CoopColliderState& s=m.colliders[k];s=CoopColliderState();if(GroupCollider(g,k)<0)continue;
  uintptr_t c=0x728720+GroupCollider(g,k)*0x34,geometry=ColliderGeometry(GroupCollider(g,k));if(!geometry)continue;
  for(unsigned a=0;a<3;a++){int v=M<int>(geometry+a*4);s.pos[a]=v<-0x10000000?-0x10000000:v>0x10000000?0x10000000:v;}
  for(unsigned a=0;a<3;a++){s.rot[a]=M<short>(c+0xc+a*2);s.vel[a]=M<short>(c+0x14+a*2);}
  for(unsigned a=0;a<3;a++)s.delta[a]=M<short>(c+0x20+a*2);
  s.flags=(uint8_t)((M<unsigned short>(c+0x32)&0x88)|(M<short>(geometry+0x2c)?1:0));s.present=1;
 }
}
// Observer smoothing: positions/rotations move from the shown pose to each new owner snapshot
// over the measured packet interval (20..150 ms); jumps over 0x20000 snap. Observers get no
// velocity or pending rotation (no local prediction); the rotation-dirty flag rebuilds collision.
typedef DWORD (*ColliderClockFn)();
static DWORD ColliderTickCount(){return GetTickCount();}
static ColliderClockFn colliderClock=ColliderTickCount;
struct ColliderSmoothing {uint32_t revision;bool ready;int from[3],to[3],shown[3];short fromRot[3],toRot[3],shownRot[3];DWORD start,duration,arrival;};
static ColliderSmoothing colliderSmoothing[24][2];
static void ResetColliderSmoothing(){memset(colliderSmoothing,0,sizeof(colliderSmoothing));}
static void SmoothCollider(ColliderSmoothing& z,const CoopColliderState& s,uint32_t revision,int* pos,short* rot){
 DWORD now=colliderClock();
 if(!z.ready||revision!=z.revision){
  bool snap=!z.ready;
  for(unsigned a=0;a<3;a++)if(abs((int64_t)s.pos[a]-z.shown[a])>0x20000)snap=true;
  DWORD interval=z.ready?now-z.arrival:0;z.duration=interval<20?20:interval>150?150:interval;
  for(unsigned a=0;a<3;a++){z.from[a]=snap?s.pos[a]:z.shown[a];z.to[a]=s.pos[a];z.fromRot[a]=snap?s.rot[a]:z.shownRot[a];z.toRot[a]=s.rot[a];}
  z.start=now;z.arrival=now;z.revision=revision;z.ready=true;
 }
 unsigned elapsed=now-z.start;
 for(unsigned a=0;a<3;a++){
  z.shown[a]=CoopLerp(z.from[a],z.to[a],elapsed,z.duration);
  short delta=(short)(z.toRot[a]-z.fromRot[a]);z.shownRot[a]=(short)(z.fromRot[a]+CoopLerp(0,delta,elapsed,z.duration));
  pos[a]=z.shown[a];rot[a]=z.shownRot[a];
 }
}
static void WriteColliderGroup(const ColliderGroupDesc& g,const CoopMover& m,int smoothGroup=-1){
 if(!m.revision)return;
 bool changed=false;int meshPos[3];int meshId=-1,axes=0;
 for(unsigned k=0;k<8;k++){const ColliderVarDesc& v=g.vars[k];if(v.mesh>0&&v.axis>=0&&v.axis<3){meshId=v.mesh;meshPos[v.axis]=m.vars[k];axes|=1<<v.axis;}}
 if(meshId>0&&axes==7)mechPosition(meshId,meshPos[0]>>5,meshPos[1]>>5,meshPos[2]>>5);
 for(unsigned k=0;k<8;k++){const ColliderVarDesc& v=g.vars[k];if(!v.address)continue;
  if(v.pointer&&(m.vars[k]<0x4e0000||m.vars[k]>=0x520000))continue;
  if(GroupVarValue(v)!=m.vars[k]){changed=true;GroupVarSet(v,m.vars[k]);}}
 if(changed&&g.refresh)g.refresh();
 for(unsigned k=0;k<2;k++){
  const CoopColliderState& s=m.colliders[k];if(GroupCollider(g,k)<0||!s.present||!ColliderGeometry(GroupCollider(g,k)))continue;
  uintptr_t c=0x728720+GroupCollider(g,k)*0x34;
  if(smoothGroup>=0&&smoothGroup<24){
   int pos[3];short rot[3];SmoothCollider(colliderSmoothing[smoothGroup][k],s,m.revision,pos,rot);
   colliderPlace(GroupCollider(g,k),pos[0],pos[1],pos[2]);
   for(unsigned a=0;a<3;a++){M<short>(c+0xc+a*2)=rot[a];M<short>(c+0x14+a*2)=0;M<short>(c+0x20+a*2)=0;}
   M<unsigned short>(c+0x32)=(unsigned short)((M<unsigned short>(c+0x32)&~0x88)|8);
  }else{
   colliderPlace(GroupCollider(g,k),s.pos[0],s.pos[1],s.pos[2]);
   for(unsigned a=0;a<3;a++){M<short>(c+0xc+a*2)=s.rot[a];M<short>(c+0x14+a*2)=s.vel[a];}
   for(unsigned a=0;a<3;a++)M<short>(c+0x20+a*2)=s.delta[a];
   M<unsigned short>(c+0x32)=(unsigned short)((M<unsigned short>(c+0x32)&~0x88)|(s.flags&0x88));
  }
  M<short>(ColliderGeometry(GroupCollider(g,k))+0x2c)=(short)(s.flags&1?8:0);   // 4878a0/4878d0 enable state
 }
}
static bool MoverNewer(const CoopMover& a,const CoopMover& b){
 return a.revision&&(!b.revision||(int32_t)(a.revision-b.revision)>0||(a.revision==b.revision&&a.owner<b.owner));
}
static bool GroupAnchor(const ColliderGroupDesc& g,int* p){
 uintptr_t geometry=ColliderGeometry(GroupCollider(g,0));
 if(geometry){for(unsigned a=0;a<3;a++)p[a]=M<int>(geometry+a*4);return true;}
 if(g.anchorMesh<0)return false;
 p[0]=p[1]=p[2]=INT32_MIN;meshPosition(g.anchorMesh,p);return p[0]!=INT32_MIN;
}
static int64_t GroupDistance(const int* a,int x,int y,int z){
 int64_t dx=((int64_t)x-a[0])>>4,dy=((int64_t)y-a[1])>>4,dz=((int64_t)z-a[2])>>4;return dx*dx+dy*dy+dz*dz;
}
// Proximity owner (0 host, 1 guest) or 2 when nobody is within the radius.
static unsigned ProximityOwner(const ColliderGroupDesc& g,unsigned current){
 int anchor[3];if(g.radius<=0||!GroupAnchor(g,anchor))return 2;
 int64_t local=GroupDistance(anchor,M<int>(0x52f300),M<int>(0x52f304),M<int>(0x52f308));
 int64_t remote=GroupDistance(anchor,remoteFrame.pose.x,remoteFrame.pose.y,remoteFrame.pose.z);
 int64_t d[2]={localOwner?remote:local,localOwner?local:remote},r=((int64_t)g.radius>>4)*((int64_t)g.radius>>4);
 bool in[2]={d[0]<=r,d[1]<=r};
 if(!in[0]&&!in[1])return 2;if(in[0]!=in[1])return in[0]?0:1;
 unsigned other=current<=1?1-current:1;if(current>1)return d[1]*4<d[0]*3?1:0;
 return d[other]*4<d[current]*3?other:current;   // hysteresis: the other must be clearly nearer
}
static uint8_t colliderRequestsApplied[24];
static void ResetColliderGroups(){memset(localFrame.movers,0,sizeof(localFrame.movers));localFrame.standing=-1;memset(colliderRequestsApplied,0,sizeof(colliderRequestsApplied));ResetColliderSmoothing();}
// Owner: apply the peer's newest trigger request once (compare-and-swap).
static void ApplyColliderRequest(const ColliderGroupDesc& g,unsigned i,const CoopMover& r){
 if(!r.revision||r.requestCount==colliderRequestsApplied[i])return;
 colliderRequestsApplied[i]=r.requestCount;
 if(r.requestVar>=8)return;const ColliderVarDesc& v=g.vars[r.requestVar];
 if(v.address&&v.trigger&&!v.pointer&&GroupVarValue(v)==r.requestFrom){GroupVarSet(v,r.requestTo);++syncStats.requestsApplied;
  SyncLog("group %u request applied %x %d -> %d",i,(unsigned)v.address,r.requestFrom,r.requestTo);if(g.request)g.request(r.requestVar,r.requestFrom,r.requestTo);}
 else{++syncStats.requestsRefused;SyncLog("group %u request refused %x from %d (now %d)",i,(unsigned)v.address,r.requestFrom,v.address?GroupVarValue(v):0);}
}
// Observer: after its native tick, a trigger var differs from the value written before it.
static void DetectColliderRequest(const ColliderGroupDesc& g,CoopMover& p){
 if(p.requestPending)return;
 for(unsigned k=0;k<8;k++){const ColliderVarDesc& v=g.vars[k];if(!v.address||!v.trigger||v.pointer)continue;
  int local=GroupVarValue(v);
  if(local!=p.vars[k]&&local>=-0x10000000&&local<=0x10000000){
   p.requestVar=(uint8_t)k;p.requestFrom=p.vars[k];p.requestTo=local;++p.requestCount;p.requestPending=1;
   ++syncStats.requestsSent;SyncLog("request sent %x %d -> %d",(unsigned)v.address,p.vars[k],local);return;}
 }
}
static void SyncColliderGroups(bool after){
 if(!SameLevel())return;
 int standing=M<int>(0x559e80);
 if(cachedLevel==4&&M<int>(0x52b81c)==0x24)standing=0x1a;   // bridge deck surface (41cb97 trigger)
 localFrame.standing=standing<-1||standing>255?-1:standing;
 int host=localOwner?remoteFrame.standing:localFrame.standing,guest=localOwner?localFrame.standing:remoteFrame.standing;
 for(unsigned i=0;i<24;i++){
  const ColliderGroupDesc* g=ColliderGroup(i);if(!g)break;
  CoopMover& p=localFrame.movers[i];const CoopMover& r=remoteFrame.movers[i];
  bool resumed=false;   // this PC reloaded the level and the other PC echoes our own last record
  if(r.group==i&&MoverNewer(r,p)){   // adopt the owner's state, keep our own request fields
   resumed=!p.revision&&r.owner==localOwner;
   uint8_t count=p.requestCount,var=p.requestVar,pending=p.requestPending;int32_t from=p.requestFrom,to=p.requestTo;
   p=r;p.requestCount=count;p.requestVar=var;p.requestFrom=from;p.requestTo=to;p.requestPending=pending;
   if(pending&&var<8&&r.vars[var]!=from)p.requestPending=0;   // the owner moved on (applied or diverged)
  }
  bool h=false,q=false;for(unsigned k=0;k<2;k++){int c=GroupCollider(*g,k);if(c>=0){h|=host==c;q|=guest==c;}}
  unsigned current=p.revision?p.owner:0,nearest=ProximityOwner(*g,current);
  unsigned owner=h?0:q?1:nearest<=1?nearest:current;
  if(!p.revision){p.revision=1;p.owner=(uint8_t)owner;p.group=(uint8_t)i;ReadColliderGroup(*g,p);}
  if(p.owner!=owner){
   ++syncStats.groupOwnerChanges;SyncLog("group %u owner %s -> %s (%s)",i,p.owner?"guest":"host",owner?"guest":"host",h||q?"standing":nearest<=1?"nearest":"kept");
   if(owner==localOwner){WriteColliderGroup(*g,p);p.requestPending=0;}   // continue from the last published state
   p.owner=(uint8_t)owner;p.revision=max(p.revision,r.revision)+1;if(!p.revision)p.revision=1;
  }
  // Without this the fresh native state would be published over the record just adopted (a rollback).
  if(resumed&&owner==localOwner)WriteColliderGroup(*g,p);
  if(owner!=localOwner){if(after)DetectColliderRequest(*g,p);WriteColliderGroup(*g,p,(int)i);continue;}
  colliderSmoothing[i][0].ready=colliderSmoothing[i][1].ready=false;   // owner: next observation starts fresh
  if(r.group==i)ApplyColliderRequest(*g,i,r);
  if(after){CoopMover next=p;ReadColliderGroup(*g,next);
   if(memcmp(next.vars,p.vars,sizeof(p.vars))||memcmp(next.colliders,p.colliders,sizeof(p.colliders))){++next.revision;if(!next.revision)next.revision=1;p=next;}}
 }
}

// ---- Level 14 lamp puzzle (tick 42e790, helpers 42d710/42d8b0/42d9c0/42db10) -------------
// 52ffd0: low nibble = lamps, high nibble = target; solved when equal (then it blinks 0xff/0).
// The first visit (52ffb4) scrambles from the random stream, so each PC used to get its own
// puzzle. A landing (53c64c == -0x28) on surface 0x20+k toggles lamp k and a neighbour (42d710),
// spends a move (52ff70, counter meshes 0x36..0x39) and flashes mesh 52ff48 for 52ff90 ticks.
// Out of moves -> rescramble (42db10). Solved -> door 52ff7c rises (42e1d0) and the local
// cutscene 52ffc8 plays on each PC. Shared as one proximity group; the lamp meshes are only
// redrawn natively on events, so observers redraw them when the owner's values arrive, and an
// observer's press becomes a compare-and-swap request on 52ffd0 that also spends the move.
typedef void(__cdecl *PuzzleDrawFn)();
static PuzzleDrawFn puzzleLamps=(PuzzleDrawFn)0x42d8b0,puzzleLampsAlt=(PuzzleDrawFn)0x42d9c0;
static void Level14Refresh(){
 puzzleLamps();puzzleLampsAlt();
 int moves=M<int>(0x52ff70);
 for(int mesh=0x36;mesh<0x3a;mesh++){int v=mesh==0x36+moves?0x1000:0;mechScale(mesh,v,v,v);}   // 42e9d1 counter
}
static void Level14Request(unsigned var,int from,int to){
 if(var!=0)return;
 bool press=(from&0xf0)!=((from&0xf)<<4)&&(to&0xf0)==(from&0xf0);   // a press on an unsolved puzzle
 if(press&&M<int>(0x52ff70)>0)--M<int>(0x52ff70);
 Level14Refresh();
}
