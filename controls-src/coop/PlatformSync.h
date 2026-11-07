// Verified Andy's House attic balance plank: collider 15, mesh 15.
// Rotation words are quarter-angle units; pending rotation delta is native state.
static const uintptr_t Plank=0x728720+15*0x34;
static bool plankAdvanced;
typedef int (__cdecl *BalanceFn)(int,int,int,int,int,int,int);
static BalanceFn nativeBalance=(BalanceFn)0x49ec00;
typedef void (__cdecl *PlatformMeshFn)(int,int);
static PlatformMeshFn platformMesh=(PlatformMeshFn)0x488bd0;
static bool PlatformNewer(const CoopPlatform& a,const CoopPlatform& b){
 return a.revision&&(!b.revision||(int32_t)(a.revision-b.revision)>0||(a.revision==b.revision&&a.owner<b.owner));
}
static void ApplyPlatform(const CoopPlatform& p){
 if(!p.revision||cachedLevel!=1||!Readable(Plank,0x34)||M<short>(Plank+0x30)==0)return;
 M<short>(Plank+0x10)=(short)p.angle; // roll; pitch/yaw remain level-local
 M<short>(Plank+0x24)=(short)p.delta;
 M<unsigned short>(Plank+0x32)|=8; // native collision transform dirty
 M<int>(0x52f58c)=p.velocity;
 platformMesh(15,15);
}
// Publish room presence independently of standing on the platform. Contact-only
// ownership cannot wake a tilted platform that the guest cannot yet reach.
static void CaptureWorldContext(){
 int room=M<int>(0x54dea0);localFrame.worldContext=0;
 if(room<0||room>31)return;
 localFrame.worldContext=0x10000u|(unsigned)room;
 if(cachedLevel==2&&M<short>(0x52f396)>0&&M<short>(0x52f38e))for(unsigned i=0;i<2;i++)if((M<unsigned short>(0x728720+i*0x34+0x32)&3)==2&&M<int>(0x559e80)!=(int)i)localFrame.worldContext|=0x400u<<i;
 if(cachedLevel==2&&(M<int>(0x52b81c)==11||(M<unsigned short>(0x728720+7*0x34+0x32)&3)==2))localFrame.worldContext|=0x200;
 if(cachedLevel==1&&room==5&&M<short>(0x52f396)>0&&M<short>(0x52f38e)!=0&&
    (M<unsigned short>(Plank+0x32)&3)==2&&M<int>(0x559e80)!=15)localFrame.worldContext|=0x100;
}
static unsigned PlatformOwner(unsigned host,unsigned guest){
 bool h=(host&0x10000)&&(host&255)==5,g=(guest&0x10000)&&(guest&255)==5;
 if(h&&(host&0x100))return 0;
 if(g&&(guest&0x100))return 1;
 if(h)return 0;if(g)return 1;return 0;
}
static unsigned DesiredPlatformOwner(){return localOwner==0?PlatformOwner(localFrame.worldContext,remoteFrame.worldContext):PlatformOwner(remoteFrame.worldContext,localFrame.worldContext);}
static void SyncPlatform(){
 if(cachedLevel!=1||!SameLevel())return;CaptureWorldContext();
 CoopPlatform& p=localFrame.platform;const CoopPlatform& r=remoteFrame.platform;
 unsigned owner=DesiredPlatformOwner();
 if(owner==localOwner){
  if(!p.revision){p.revision=1;p.owner=owner;p.velocity=M<int>(0x52f58c);p.angle=M<short>(Plank+0x10);p.delta=M<short>(Plank+0x24);}
  else if(p.owner!=owner){ApplyPlatform(p);p.owner=owner;p.revision=max(p.revision,r.revision)+1;if(!p.revision)p.revision=1;}
 }else{
  if(r.revision&&r.owner==owner&&(p.owner!=owner||PlatformNewer(r,p)))p=r;
  if(p.revision)ApplyPlatform(p);
 }
}
static uint32_t backyardAppliedRevision[2];static unsigned backyardAppliedOwner[2];
static void ApplyBackyardPlatform(unsigned i,const CoopPlatform& p){
 uintptr_t collider=0x728720+i*0x34;if(!p.revision||!Readable(collider,0x34)||!M<short>(collider+0x30))return;
 // A repeated packet is not another physics step. Do not rewind a committed
 // angle or replay a delta already consumed by the native collision update.
 if(backyardAppliedRevision[i]!=p.revision||backyardAppliedOwner[i]!=p.owner){
  M<short>(collider+0x10)=(short)p.angle;M<short>(collider+0x24)=(short)p.delta;M<unsigned short>(collider+0x32)|=8;
  M<int>(0x52f6dc+i*4)=p.velocity;backyardAppliedRevision[i]=p.revision;backyardAppliedOwner[i]=p.owner;
 }
 platformMesh(i,i);
}
static int BackyardBalance(int collision,int mesh,int velocity,int mode,int minimum,int maximum,int divisor){
 unsigned i=(unsigned)collision;CaptureWorldContext();CoopPlatform& p=localFrame.backyardPlatforms[i];const CoopPlatform& r=remoteFrame.backyardPlatforms[i];
 if(PlatformNewer(r,p))p=r;
 unsigned bit=0x400u<<i,h=localOwner?remoteFrame.worldContext:localFrame.worldContext,g=localOwner?localFrame.worldContext:remoteFrame.worldContext;
 unsigned owner=(h&bit)?0:(g&bit)?1:p.revision?p.owner:0;
 if(!p.revision){p.revision=1;p.owner=owner;p.velocity=velocity;p.angle=M<short>(0x728720+i*0x34+0x10);p.delta=0;}
 if(p.owner!=owner){ApplyBackyardPlatform(i,p);p.owner=owner;p.revision=max(p.revision,r.revision)+1;if(!p.revision)p.revision=1;}
 if(owner!=localOwner){ApplyBackyardPlatform(i,p);return p.velocity;}
 // The owner already has the live, committed collider rotation. Native
 // balance queues +0x24 for a LATER collision step; restoring p.angle here
 // would erase the preceding step on every frame and freeze the visible mesh.
 p.velocity=nativeBalance(collision,mesh,p.velocity,mode,minimum,maximum,divisor);
 p.angle=M<short>(0x728720+i*0x34+0x10);p.delta=M<short>(0x728720+i*0x34+0x24);++p.revision;return p.velocity;
}
static int __cdecl BalancePlatform(int collision,int mesh,int velocity,int mode,int minimum,int maximum,int divisor){
 if(cachedLevel==2&&SameLevel()&&collision>=0&&collision<2&&mesh==collision)return BackyardBalance(collision,mesh,velocity,mode,minimum,maximum,divisor);
 if(cachedLevel!=1||!SameLevel()||collision!=15||mesh!=15)return nativeBalance(collision,mesh,velocity,mode,minimum,maximum,divisor);
 plankAdvanced=true;SyncPlatform();CoopPlatform& p=localFrame.platform;
 if(DesiredPlatformOwner()!=localOwner){ApplyPlatform(p);return p.revision?p.velocity:velocity;}
 int result=nativeBalance(collision,mesh,p.velocity,mode,minimum,maximum,divisor);
 p.velocity=result;p.angle=M<short>(Plank+0x10);p.delta=M<short>(Plank+0x24);++p.revision;
 return result;
}
static void BeginPlatformTick(){plankAdvanced=false;SyncPlatform();}
static void EndPlatformTick(){
 if(cachedLevel!=1||!SameLevel())return;SyncPlatform();
 if(plankAdvanced||DesiredPlatformOwner()!=localOwner||!Readable(Plank,0x34)||M<short>(Plank+0x30)==0)return;
 // The room-five native call was skipped. Continue the unloaded native tilt
 // without pretending the local player is in the attic or triggering its boss.
 unsigned short flags=M<unsigned short>(Plank+0x32);int excluded=M<int>(0x559e80);
 M<unsigned short>(Plank+0x32)&=~3;M<int>(0x559e80)=-1;
 int velocity=BalancePlatform(15,15,M<int>(0x52f58c),1,-448,448,8);
 M<int>(0x52f58c)=velocity;M<int>(0x559e80)=excluded;
 M<unsigned short>(Plank+0x32)=(M<unsigned short>(Plank+0x32)&~3)|(flags&3);
}

static void RefreshBackyardPlatformMeshes(){
 if(cachedLevel!=2||!SameLevel())return;
 for(unsigned i=0;i<2;i++)if(localFrame.backyardPlatforms[i].revision&&M<short>(0x728720+i*0x34+0x30))platformMesh(i,i);
}
