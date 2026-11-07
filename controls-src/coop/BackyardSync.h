// Value-only adapters for Backyard lawnmower, pool duck/pump and tree plank.
static void SyncMower(){
 if(cachedLevel!=2||!SameLevel()||!BackyardMower(0))return;uintptr_t actor=0x52c840;
 if(localOwner==0){CoopWorldActor& s=localFrame.mower;++s.revision;if(!s.revision)++s.revision;
  memcpy(&s.pose.x,&M<unsigned char>(actor),12);s.pose.pitch=M<short>(actor+12);s.pose.yaw=M<short>(actor+14);s.pose.roll=M<short>(actor+16);s.pose.health=1;
  s.pose.animation=M<short>(actor+18);s.pose.secondary=M<short>(actor+22);s.pose.frame=NativeEnemyFrame(12,s.pose.animation,M<int>(actor+24));s.pose.secondaryFrame=NativeEnemyFrame(12,s.pose.secondary,M<int>(actor+28));
  if(s.pose.frame<0){s.pose.animation=-1;s.pose.frame=0;}if(s.pose.secondaryFrame<0){s.pose.secondary=-1;s.pose.secondaryFrame=0;}
  memcpy(s.motion,&M<unsigned char>(actor+0x44),40);s.flags=M<unsigned short>(actor+0x40)&0x19c;
 }else {const CoopWorldActor& s=remoteFrame.mower;if(!s.revision)return;
  memcpy(&M<unsigned char>(actor),&s.pose.x,12);M<short>(actor+12)=s.pose.pitch;M<short>(actor+14)=s.pose.yaw;M<short>(actor+16)=s.pose.roll;
  int a=NativeEnemyFrame(12,s.pose.animation,s.pose.frame),b=NativeEnemyFrame(12,s.pose.secondary,s.pose.secondaryFrame);
  if(a>=0&&b>=0){M<short>(actor+18)=s.pose.animation;M<short>(actor+22)=s.pose.secondary;M<int>(actor+24)=a;M<int>(actor+28)=b;}
  memcpy(&M<unsigned char>(actor+0x44),s.motion,40);M<unsigned short>(actor+0x40)=(M<unsigned short>(actor+0x40)&~0x19c)|(unsigned short)s.flags;
 }
}
static const uintptr_t duckFields[]={0x52f648,0x52f64c,0x52f650,0x52f6b4,0x52f6bc,0x52f6c0,0x52f674,0x52f690,0x52f658,0x52f60c};
typedef void(__cdecl *BackyardTransformFn)(int,int,int,int);
typedef void(__cdecl *BackyardToggleFn)(int);
static BackyardTransformFn backyardScale=(BackyardTransformFn)0x4ccb20,backyardRotate=(BackyardTransformFn)0x4ccc70;
typedef void(__cdecl *BackyardPositionFn)(int,int*);
static BackyardPositionFn backyardGetPosition=(BackyardPositionFn)0x4879c0;
static BackyardTransformFn backyardMoveCollider=(BackyardTransformFn)0x487900;
static BackyardToggleFn backyardEnable=(BackyardToggleFn)0x4878a0,backyardDisable=(BackyardToggleFn)0x4878d0;
static void ReadDuck(CoopDuck& state){for(unsigned i=0;i<10;i++)state.value[i]=M<int>(duckFields[i]);}
static void WriteDuck(const CoopDuck& state){
 if(!state.revision)return;for(unsigned i=0;i<10;i++)M<int>(duckFields[i])=state.value[i];
 backyardScale(6,4096,state.value[3],4096);backyardScale(7,4096,4096-state.value[6],4096);
 if(state.value[3]==4096){moveObjectMesh(6,state.value[0]>>5,state.value[1]>>5,state.value[2]>>5);
  uintptr_t collider=0x728720+7*0x34;if(M<short>(collider+0x30)!=0){
   int previous[3]={};backyardGetPosition(7,previous);int delta[3]={state.value[0]-previous[0],state.value[1]-0x1000-previous[1],state.value[2]-previous[2]};
   if(abs(delta[0])<=32767&&abs(delta[1])<=32767&&abs(delta[2])<=32767)backyardMoveCollider(7,delta[0],delta[1],delta[2]);
   else {moveObjectCollision(7,state.value[0],state.value[1]-0x1000,state.value[2]);backyardMoveCollider(7,0,0,0);}
   // Normal reconciliation uses native pending movement, carrying any rider.
   // A first large correction uses absolute placement instead of short overflow.
  }
 }
}
static unsigned DuckOwner(){
 unsigned h=localOwner?remoteFrame.worldContext:localFrame.worldContext,g=localOwner?localFrame.worldContext:remoteFrame.worldContext;
 if(h&0x200)return 0;if(g&0x200)return 1;return localFrame.duck.revision?localFrame.duck.owner:0;
}
static void SyncDuck(){
 if(cachedLevel!=2||!SameLevel())return;CaptureWorldContext();CoopDuck& d=localFrame.duck;const CoopDuck& r=remoteFrame.duck;
 if(NewerState(r.revision,r.owner,d.revision,d.owner))d=r;
 if(!d.revision){d.revision=1;d.owner=0;ReadDuck(d);}
 unsigned owner=DuckOwner();
 if(d.owner!=owner){WriteDuck(d);d.owner=owner;d.revision=max(d.revision,r.revision)+1;if(!d.revision)d.revision=1;}
 if(owner!=localOwner)WriteDuck(d);
}
static void SyncTree(){
 if(cachedLevel!=2||!SameLevel())return;CoopTree& t=localFrame.tree;const CoopTree& r=remoteFrame.tree;
 int angle=max(0,min(M<int>(0x52f5ec),581)),velocity=max(0,min(M<int>(0x52f640),4096));
 // The tree plank is a one-way fall. Merge progress, never rewind it when
 // another player's independently loaded script still has its initial angle.
 angle=max(angle,max(t.angle,r.angle));velocity=max(velocity,max(t.velocity,r.velocity));
 if(!t.revision||angle!=t.angle||velocity!=t.velocity){++t.revision;if(!t.revision)++t.revision;t.angle=angle;t.velocity=velocity;}
 M<int>(0x52f5ec)=angle;M<int>(0x52f640)=velocity;
 // Native calls at 41a3b1/41a3c7 push the angle first: Z rotation for BOTH meshes.
 if(angle){backyardDisable(11);backyardEnable(12);backyardRotate(30,0,0,4095-angle);backyardRotate(31,0,0,4095-angle);}
}
static void BeginBackyard(){SyncMower();SyncDuck();SyncTree();}
static void EndBackyard(){
 if(cachedLevel!=2||!SameLevel())return;CoopDuck& d=localFrame.duck;
 if(d.owner==localOwner){CoopDuck next=d;ReadDuck(next);if(memcmp(next.value,d.value,sizeof(d.value))){++next.revision;d=next;}}
 else WriteDuck(d);
 SyncTree();SyncMower();
}
