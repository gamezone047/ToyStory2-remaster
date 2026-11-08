// Remaining Andy's House (1) and Backyard (2) level-script mechanisms.
// Each PC publishes its OWN native values in mechanisms[i]; the receiver merges
// them using the mechanism's native direction (one-way progress, cyclic phase or
// owner snapshot). No native pointers cross the network.
//
// House    [0] garage vents 417380/417510: phases 52f588 (colliders 12/13) and 52f568.
//              Native advances them only while that player is in room 2.
//          [1] room-one trigger 52f5a0 (0 -> 8 -> 0x1000): collider 8, mesh 21 scale.
//              Native fires it only from the LOCAL player's position.
// Backyard [0] twin pressure buttons 52f6c8/52f6cc (0x1000 unpressed, decays to 0)
//              and gate stage 52f610 (0..2): colliders 5/6, meshes 2,3,4,8,9,10,21,32-35.
//          [1] toppling board 52f5e4/52f5f0, started by a LOCAL push of object 0:
//              meshes 5/11, colliders 2,3 off and 4 on below angle 0x299.
//          [2] rising platform 52f6a8/52f684 (0 idle, >0 rising, -1 done): special
//              collision entry 0, mesh 25 position, mesh 27 scale.
//          [3] challenge stage 52f664 (0..0xc8) for scripted actor slot 26.
typedef void(__cdecl *MechTransformFn)(int,int,int,int);
typedef void(__cdecl *MechToggleFn)(int);
typedef void(__cdecl *MechPairFn)(int,int);
typedef void(__cdecl *MechVoidFn)();
static MechTransformFn mechScale=(MechTransformFn)0x4ccb20,mechRotate=(MechTransformFn)0x4ccc70,mechPosition=(MechTransformFn)0x4cce30;
static MechToggleFn mechEnable=(MechToggleFn)0x4878a0,mechDisable=(MechToggleFn)0x4878d0;
static MechPairFn mechCopy=(MechPairFn)0x4ccff0;
static MechVoidFn mechSettle=(MechVoidFn)0x448080;
static uint32_t mechSeenRevision[4];static unsigned mechSeenOwner[4];
static void ResetMechanisms(){
 memset(localFrame.mechanisms,0,sizeof(localFrame.mechanisms));
 memset(mechSeenRevision,0,sizeof(mechSeenRevision));for(unsigned i=0;i<4;i++)mechSeenOwner[i]=2;
}
static int MechClamp(int v,int low,int high){return v<low?low:v>high?high:v;}
static void PublishMechanism(unsigned i,int a,int b=0,int c=0,int d=0){
 CoopMechanism& m=localFrame.mechanisms[i];const int32_t v[4]={a,b,c,d};
 if(m.revision&&m.owner==localOwner&&!memcmp(m.value,v,sizeof(v)))return;
 ++m.revision;if(!m.revision)m.revision=1;m.owner=localOwner;memcpy(m.value,v,sizeof(v));
}
// True once per newly received remote snapshot. Repeated packets are not new
// physics steps and must not re-trigger native threshold side effects.
static bool FreshMechanism(unsigned i){
 const CoopMechanism& r=remoteFrame.mechanisms[i];
 if(!r.revision||(r.revision==mechSeenRevision[i]&&r.owner==mechSeenOwner[i]))return false;
 mechSeenRevision[i]=r.revision;mechSeenOwner[i]=r.owner;return true;
}
static int PlayerRoom(unsigned context){return (context&0x10000)?(int)(context&255):-1;}
static int HostRoom(){return PlayerRoom(localOwner?remoteFrame.worldContext:localFrame.worldContext);}
static int GuestRoom(){return PlayerRoom(localOwner?localFrame.worldContext:remoteFrame.worldContext);}

// ---- Andy's House --------------------------------------------------------
static int PhaseDistance(int a,int b,int period){int d=((a-b)%period+period)%period;return d>period/2?period-d:d;}
// 417380: collider OFF on crossing 0x400, ON on crossing 0x1400 (phase & 0x1fff).
static void ApplyVentColliders(int phase){
 for(int k=0;k<2;k++){int p=(phase+k*0x1000)&0x1fff;if(p>=0x400&&p<0x1400)mechDisable(12+k);else mechEnable(12+k);}
}
static void SyncGarageVents(){
 const uintptr_t field[2]={0x52f588,0x52f568};
 unsigned owner=HostRoom()==2?0:GuestRoom()==2?1:2;
 if(owner!=2&&owner!=localOwner&&FreshMechanism(0)){
  const CoopMechanism& r=remoteFrame.mechanisms[0];
  // Keep native phase advancing locally; only correct real divergence. The
  // tolerance absorbs ordinary network lag without repeated threshold replays.
  for(unsigned k=0;k<2;k++){
   int local=M<int>(field[k])&0x1fff,remote=r.value[k]&0x1fff;
   if(PhaseDistance(local,remote,0x2000)>0x600){M<int>(field[k])=remote;if(!k)ApplyVentColliders(remote);}
  }
 }
 PublishMechanism(0,M<int>(field[0])&0x1fff,M<int>(field[1])&0x1fff);
}
static void SyncRoomTrigger(){
 int local=MechClamp(M<int>(0x52f5a0),0,0x1000),merged=local;const CoopMechanism& r=remoteFrame.mechanisms[1];
 if(r.revision)merged=max(merged,MechClamp(r.value[0],0,0x1000));
 if(merged!=local){
  // Native 4179f4 enables collider 8 once at activation, then grows mesh 21.
  if(local==0)mechEnable(8);
  M<int>(0x52f5a0)=merged;mechScale(0x15,merged,0x1000,0x1000);mechCopy(0x16,0x15);
 }
 PublishMechanism(1,merged);
}

// ---- Backyard ------------------------------------------------------------
static void SyncTwinButtons(){
 const uintptr_t timer[2]={0x52f6c8,0x52f6cc};const CoopMechanism& r=remoteFrame.mechanisms[0];
 int value[2];
 for(int k=0;k<2;k++){
  int t=MechClamp(M<int>(timer[k]),0,0x1000);
  if(r.revision){
   int remote=MechClamp(r.value[k],0,0x1000);
   // Earliest press wins: both timers decay at the same native rate.
   if(remote<t){
    if(t==0x1000){mechScale(0x20+k,0x1000,0x1000,0x1000);mechScale(0x22+k,0,0,0);} // pressed button
    M<int>(timer[k])=remote;mechScale(2+k,0x1000,remote,0x1000);t=remote;
   }
  }
  value[k]=t;
 }
 int stage=MechClamp(M<int>(0x52f610),0,2);
 if(r.revision&&r.value[2]>stage){
  int target=MechClamp(r.value[2],0,2);
  if(stage==0){mechEnable(5);mechDisable(6);mechSettle();} // 419fee: gate opens
  if(target==2){ // 41a0e6: final wobble pose, mesh 21 shown, mesh 4 hidden
   M<int>(0x52f614)=0x80;M<int>(0x52f618)=0;
   const int meshes[4]={4,8,9,10};for(int i=0;i<4;i++)mechRotate(meshes[i],0,0,0x80);
   mechScale(0x15,0x1000,0x1000,0x1000);mechScale(4,0,0,0);
  }
  M<int>(0x52f610)=target;stage=target;
 }
 PublishMechanism(0,value[0],value[1],stage);
}
static const int ToppleDone=INT32_MIN;
static int ToppleProgress(int velocity,int angle){return velocity==ToppleDone?2:angle<0x299?1:0;}
static void SyncTopple(){
 int velocity=M<int>(0x52f5e4),angle=MechClamp(M<int>(0x52f5f0),-0x1000,0x1000);
 if(velocity!=ToppleDone)velocity=MechClamp(velocity,-0x10000,0x10000);
 if(FreshMechanism(1)){
  const CoopMechanism& r=remoteFrame.mechanisms[1];
  int rv=r.value[0],ra=r.value[1],mine=ToppleProgress(velocity,angle),theirs=ToppleProgress(rv,ra);
  // Before the fall, pushing only lowers the angle: keep the most-pushed one.
  // Once falling, the first faller (host on a tie) owns the snapshot.
  bool adopt=theirs>mine||(theirs==1&&mine==1&&localOwner==1);
  if(adopt){
   if(angle>=0x299&&ra<0x299){mechDisable(2);mechDisable(3);mechEnable(4);M<int>(0x53c648)=0;} // 41a251
   velocity=rv;angle=ra;M<int>(0x52f5e4)=velocity;M<int>(0x52f5f0)=angle;
   mechRotate(5,0,0,angle);mechRotate(0xb,0,0,angle);
  }else if(theirs==0&&mine==0&&ra<angle){angle=ra;M<int>(0x52f5f0)=angle;mechRotate(5,0,0,angle);mechRotate(0xb,0,0,angle);}
 }
 PublishMechanism(1,velocity,angle);
}
// 41a846: mesh 27 height scale from platform Y, exact native integer path.
static int RiseScale(int y){
 int32_t shifted=(int32_t)((uint32_t)(y+0x5826c)<<8);
 int high=(int)(((int64_t)shifted*0x17dd22ef)>>32)>>11;high+=(int)((uint32_t)high>>31);
 return MechClamp(high,0,0x1000);
}
static int RiseProgress(int state){return state<0?2:state>0?1:0;}
static void ApplyRise(int state,int y){
 uintptr_t table=M<uintptr_t>(0x559d64);int previous=M<int>(0x52f684);
 M<int>(0x52f6a8)=state;M<int>(0x52f684)=y;
 // Entry 0 of the special collision table: +8 height, +0x10 moving flag,
 // +0x14 accumulated rider movement (native adds every height increment).
 if(Readable(table,0x18)){M<int>(table+8)=y;M<int>(table+0x10)=state>0?2:0;M<int>(table+0x14)+=y-previous;}
 mechPosition(0x19,M<int>(0x52f680)>>5,y>>5,M<int>(0x52f688)>>5);
 mechScale(0x1b,0x1000,RiseScale(y),0x1000);
}
static void SyncRise(){
 int state=MechClamp(M<int>(0x52f6a8),-1,0x800),y=MechClamp(M<int>(0x52f684),-0x10000000,0x10000000);
 if(FreshMechanism(2)){
  const CoopMechanism& r=remoteFrame.mechanisms[2];
  int mine=RiseProgress(state),theirs=RiseProgress(r.value[0]);
  if(theirs>mine||(theirs==1&&mine==1&&localOwner==1)){state=r.value[0];y=r.value[1];ApplyRise(state,y);}
 }
 PublishMechanism(2,state,y);
}
static void SyncChallenge(){
 int stage=MechClamp(M<int>(0x52f664),0,0xc8);const CoopMechanism& r=remoteFrame.mechanisms[3];
 if(r.revision&&r.value[0]>stage){
  int target=MechClamp(r.value[0],0,0xc8);
  // 41a47e: the stage-2 entry starts actor 26's command script. Replay it
  // when the other player's dialogue already passed that point.
  if(stage<2&&target>=2&&M<short>(0x52d82c)!=0){
   M<short>(0x52d890)=0;M<uintptr_t>(0x52d898)=M<uintptr_t>(0x4e02f8)+0x1c;M<int>(0x830d44)=0xb4;
  }
  M<int>(0x52f664)=target;stage=target;
 }
 PublishMechanism(3,stage);
}
static void SyncMechanisms(){
 if(!SameLevel()||(cachedLevel!=1&&cachedLevel!=2))return;
 CaptureWorldContext();
 if(cachedLevel==1){SyncGarageVents();SyncRoomTrigger();}
 else{SyncTwinButtons();SyncTopple();SyncRise();SyncChallenge();}
}
