// RC race adapters for Andy's House and Backyard; shovel remains House-only. No script pointers cross the network.
static const uintptr_t RcActor=0x52d9ec;
static bool RcLevel(){return cachedLevel==1||cachedLevel==2;}
static uintptr_t RcNode(){return cachedLevel==2?0x52f668:0x52f528;}
static uintptr_t RcLaps(){return cachedLevel==2?0x52f620:0x52f4f8;}
static uintptr_t RcRegion(){return cachedLevel==2?0x52f6b0:0x52f584;}
static uintptr_t RcDirection(){return cachedLevel==2?0x52f5e0:0x52f580;}
static bool raceSpectator,shovelContact;
static int rcBackgroundTrack=-1;static unsigned rcPreviousPhase;static bool rcEndMusicPending;
static uint32_t rcPushApplied;
static bool NewerState(uint32_t rev,uint32_t owner,uint32_t oldRev,uint32_t oldOwner){return rev&&(!oldRev||(int32_t)(rev-oldRev)>0||(rev==oldRev&&owner<oldOwner));}
static int RcFrame(short animation,int frame){const int count[5]={12,16,12,12,24};return animation<0||animation>=5?0:max(0,min(frame,(count[animation]-1)<<16));}
static void ReadRc(CoopGarage& g){
 // Native cleanup can remove RC after the race. Keep the last valid snapshot.
 if(M<short>(RcActor+0x14)!=8)return;
 CoopPose& p=g.pose;memcpy(&p.x,&M<unsigned char>(RcActor),12);
 p.pitch=M<short>(RcActor+12);p.yaw=M<short>(RcActor+14);p.roll=M<short>(RcActor+16);p.animation=M<short>(RcActor+18);p.secondary=M<short>(RcActor+22);
 p.frame=RcFrame(p.animation,M<int>(RcActor+24));p.secondaryFrame=RcFrame(p.secondary,M<int>(RcActor+28));p.health=1;
 memcpy(g.motion,&M<unsigned char>(RcActor+0x44),40);
 g.phase=M<unsigned>(0x52f2f8);g.node=M<unsigned>(RcNode());g.laps=M<unsigned>(RcLaps());g.playerLaps=M<unsigned>(0x52ad64);g.region=M<unsigned>(RcRegion());g.direction=M<unsigned>(RcDirection());
 g.health=M<short>(RcActor+0x7e);g.flags=M<unsigned short>(RcActor+0x40)&0x18c;
}
static void WriteRc(const CoopGarage& g){
 if(!g.revision||M<short>(RcActor+0x14)!=8)return;
 const CoopPose& p=g.pose;memcpy(&M<unsigned char>(RcActor),&p.x,12);
 M<short>(RcActor+12)=p.pitch;M<short>(RcActor+14)=p.yaw;M<short>(RcActor+16)=p.roll;M<short>(RcActor+18)=p.animation;M<short>(RcActor+22)=p.secondary;
 M<int>(RcActor+24)=RcFrame(p.animation,p.frame);M<int>(RcActor+28)=RcFrame(p.secondary,p.secondaryFrame);
 memcpy(&M<unsigned char>(RcActor+0x44),g.motion,40);M<short>(RcActor+0x7e)=(short)g.health;
 M<unsigned short>(RcActor+0x40)=(M<unsigned short>(RcActor+0x40)&~0x38c)|(g.flags&0x18c)|1;
 M<unsigned>(0x52f2f8)=g.phase;M<unsigned>(RcNode())=g.node;M<unsigned>(RcLaps())=g.laps;M<unsigned>(0x52ad64)=g.playerLaps;M<unsigned>(RcRegion())=g.region;M<unsigned>(RcDirection())=g.direction;
}
static void SyncRc(){
 if(!RcLevel()||!SameLevel())return;CoopGarage& g=localFrame.garage;const CoopGarage& r=remoteFrame.garage;
 if(NewerState(r.revision,r.owner,g.revision,g.owner))g=r;
 if(g.revision&&g.owner!=localOwner)WriteRc(g);
}
static void BeginGarage(){
 raceSpectator=false;if(!RcLevel()||!SameLevel())return;
 CoopGarage& g=localFrame.garage;
 if(!g.phase&&!rcPreviousPhase){int track=M<int>(0x5282cc);if(track>=0&&track<22)rcBackgroundTrack=track;}
 bool starts=(M<unsigned short>(RcActor+0x40)&0x200)!=0&&M<unsigned>(0x52f2f8)==0;
 SyncRc();
 if((starts&&!g.phase)||(!g.revision&&(localOwner==0||M<unsigned>(0x52f2f8)>0))){g.revision=max(g.revision,remoteFrame.garage.revision)+1;if(!g.revision)g.revision=1;g.owner=localOwner;ReadRc(g);if(starts)M<unsigned short>(RcActor+0x40)|=0x200;}
 raceSpectator=g.revision&&g.phase&&g.owner!=localOwner;
 if(raceSpectator){M<unsigned>(0x52f2f8)=0;M<unsigned short>(RcActor+0x40)&=~0x200;}
}
static void EndGarage(){
 if(!RcLevel()||!SameLevel())return;CoopGarage& g=localFrame.garage;
 if(g.revision){if(g.owner==localOwner){CoopGarage next=g;ReadRc(next);if(memcmp((const char*)&next+8,(const char*)&g+8,sizeof(g)-8)){++next.revision;if(!next.revision)next.revision=1;g=next;}}else WriteRc(g);}
 if(rcPreviousPhase>0&&rcPreviousPhase<3&&(g.phase==0||g.phase>=3))rcEndMusicPending=true;
 rcPreviousPhase=g.phase;raceSpectator=false;
}
typedef void (__cdecl *GarageMeshFn)(int,int,int,int);
static GarageMeshFn garageMesh=(GarageMeshFn)0x4cce30;
static void WriteShovel(const CoopShovel& s){
 if(!s.revision)return;
 M<int>(0x52f504)=s.velocity;M<int>(0x52f5bc)=s.height;
 uintptr_t collision=M<uintptr_t>(0x559d64);if(Readable(collision,0xcc))M<int>(collision+0xc8)=M<int>(0x52f530)+s.height;
 garageMesh(6,M<int>(0x52f5b8)>>5,s.height>>5,M<int>(0x52f5c0)>>5);
}
static void BeginShovel(){
 if(cachedLevel!=1||!SameLevel())return;CoopShovel& s=localFrame.shovel;const CoopShovel& r=remoteFrame.shovel;
 if(NewerState(r.revision,r.owner,s.revision,s.owner))s=r;
 if(!s.revision){s.revision=1;s.owner=0;s.velocity=M<int>(0x52f504);s.height=M<int>(0x52f5bc);}
 bool contact=M<int>(0x53c5e4)==48;
 if(contact&&!shovelContact&&s.velocity==0){++s.revision;s.owner=localOwner;}
 shovelContact=contact;if(s.owner!=localOwner)WriteShovel(s);
}
static void EndShovel(){
 if(cachedLevel!=1||!SameLevel())return;CoopShovel& s=localFrame.shovel;
 if(s.owner==localOwner){int velocity=M<int>(0x52f504),height=M<int>(0x52f5bc);
  if(velocity!=s.velocity||height!=s.height){s.velocity=velocity;s.height=height;++s.revision;}
 }else WriteShovel(s);
}
static PoseSample rcPoseSamples[32];static unsigned rcPoseCount;static bool rcPoseReady,rcPresenting;
static CoopPose rcDisplayPose;static unsigned rcPresentationOwner=2;
static unsigned char rcSavedRender[32];
static void ResetRcPresentation(){rcPoseReady=false;rcPoseCount=0;rcPresenting=false;rcPresentationOwner=2;}
static const CoopPose& SmoothRcPoseAt(const CoopPose& received,uint32_t sequence,DWORD now){
 if(!rcPoseReady){rcPoseCount=0;rcPoseReady=true;}
 if(!rcPoseCount||(int32_t)(sequence-rcPoseSamples[rcPoseCount-1].sequence)>0){
  if(rcPoseCount){
   const PoseSample& last=rcPoseSamples[rcPoseCount-1];
   int64_t dx=(int64_t)received.x-last.pose.x,dy=(int64_t)received.y-last.pose.y,dz=(int64_t)received.z-last.pose.z;
   if(now-last.time>500||dx>0x40000||dx< -0x40000||dy>0x40000||dy< -0x40000||dz>0x40000||dz< -0x40000)rcPoseCount=0;
  }
  if(rcPoseCount==32){memmove(rcPoseSamples,rcPoseSamples+1,31*sizeof(PoseSample));--rcPoseCount;}
  PoseSample& sample=rcPoseSamples[rcPoseCount++];sample.pose=received;sample.time=now;sample.sequence=sequence;
 }
 DWORD renderTime=now-100;
 while(rcPoseCount>2&&(int32_t)(renderTime-rcPoseSamples[1].time)>=0){
  memmove(rcPoseSamples,rcPoseSamples+1,(rcPoseCount-1)*sizeof(PoseSample));--rcPoseCount;
 }
 const PoseSample& a=rcPoseSamples[0];
 if(rcPoseCount==1||(int32_t)(renderTime-a.time)<=0){rcDisplayPose=a.pose;return rcDisplayPose;}
 const PoseSample& b=rcPoseSamples[1];
 unsigned duration=b.time-a.time,elapsed=renderTime-a.time;
 // Hold the last authoritative position on a stall, rather than predicting
 // through walls. Reconnection/teleport discards the old history above.
 if(!duration||elapsed>=duration){rcDisplayPose=b.pose;return rcDisplayPose;}
 rcDisplayPose=a.pose;
 rcDisplayPose.x=CoopLerp(a.pose.x,b.pose.x,elapsed,duration);
 rcDisplayPose.y=CoopLerp(a.pose.y,b.pose.y,elapsed,duration);
 rcDisplayPose.z=CoopLerp(a.pose.z,b.pose.z,elapsed,duration);
 rcDisplayPose.yaw=CoopLerpAngle(a.pose.yaw,b.pose.yaw,elapsed,duration);
 rcDisplayPose.pitch=CoopLerpAngle(a.pose.pitch,b.pose.pitch,elapsed,duration);
 rcDisplayPose.roll=CoopLerpAngle(a.pose.roll,b.pose.roll,elapsed,duration);
 if(a.pose.animation==b.pose.animation&&b.pose.frame>=a.pose.frame)
  rcDisplayPose.frame=CoopLerp(a.pose.frame,b.pose.frame,elapsed,duration);
 if(a.pose.secondary==b.pose.secondary&&b.pose.secondaryFrame>=a.pose.secondaryFrame)
  rcDisplayPose.secondaryFrame=CoopLerp(a.pose.secondaryFrame,b.pose.secondaryFrame,elapsed,duration);
 return rcDisplayPose;
}

static void BeginRcPresentation(){
 if(!RcLevel()||!SameLevel()||!localFrame.garage.revision||localFrame.garage.owner==localOwner||M<short>(RcActor+0x14)!=8){ResetRcPresentation();return;}
 const CoopGarage& state=localFrame.garage;
 if(rcPresentationOwner!=state.owner){ResetRcPresentation();rcPresentationOwner=state.owner;}
 const CoopPose& smooth=SmoothRcPoseAt(state.pose,state.revision,GetTickCount());
 int64_t distance=0;const int position[3]={state.pose.x,state.pose.y,state.pose.z};
 for(unsigned axis=0;axis<3;axis++){int64_t d=((int64_t)M<int>(0x52f300+axis*4)-position[axis])/256;distance+=d*d;}
 // Up close, draw at the collision pose; the delayed race presentation would
 // otherwise make Buzz appear to clip through the car he is actually touching.
 const CoopPose& p=distance<256*256?state.pose:smooth;
 memcpy(rcSavedRender,&M<unsigned char>(RcActor),32);rcPresenting=true;
 memcpy(&M<unsigned char>(RcActor),&p.x,12);
 M<short>(RcActor+12)=p.pitch;M<short>(RcActor+14)=p.yaw;M<short>(RcActor+16)=p.roll;
 M<short>(RcActor+18)=p.animation;M<short>(RcActor+22)=p.secondary;
 M<int>(RcActor+24)=RcFrame(p.animation,p.frame);M<int>(RcActor+28)=RcFrame(p.secondary,p.secondaryFrame);
}
static void EndRcPresentation(){if(rcPresenting){memcpy(&M<unsigned char>(RcActor),rcSavedRender,32);rcPresenting=false;}}

static void ResetGarage(){rcBackgroundTrack=-1;rcPreviousPhase=0;rcEndMusicPending=false;rcPushApplied=0;localFrame.rcPushSequence=localFrame.rcPushAngle=0;ResetRcPresentation();localFrame.garage={};localFrame.shovel={};raceSpectator=shovelContact=false;}

typedef void (__cdecl *RcImpulseFn)(uintptr_t,int,int);
static RcImpulseFn nativeRcImpulse=(RcImpulseFn)0x408a60;
typedef void (__cdecl *ActorContactFn)();
static ActorContactFn nativeActorContact=(ActorContactFn)0x407440;
static void __cdecl SharedContactImpulse(uintptr_t actor,int angle,int damage){
 nativeRcImpulse(actor,angle,damage);
 if(actor==RcActor&&RcLevel()&&SameLevel()&&M<short>(actor+0x14)==8){
  ++localFrame.rcPushSequence;if(!localFrame.rcPushSequence)++localFrame.rcPushSequence;
  localFrame.rcPushAngle=(unsigned)angle&4095;
 }
}
static void ApplyRcPush(){
 if(!RcLevel()||!SameLevel()||M<short>(RcActor+0x14)!=8)return;
 unsigned owner=localFrame.garage.revision?localFrame.garage.owner:0;
 if(owner!=localOwner)return;
 uint32_t sequence=remoteFrame.rcPushSequence;
 if(!sequence||(int32_t)(sequence-rcPushApplied)<=0)return;
 rcPushApplied=sequence;
 // Native contact already performed the guest's player reaction. Replay only
 // RC's impulse on its simulation owner, once, with no fabricated laser damage.
 int64_t dx=((int64_t)remoteFrame.pose.x-M<int>(RcActor))/256;
 int64_t dy=((int64_t)remoteFrame.pose.y-M<int>(RcActor+4))/256;
 int64_t dz=((int64_t)remoteFrame.pose.z-M<int>(RcActor+8))/256;
 if(dx*dx+dy*dy+dz*dz<=256*256)nativeRcImpulse(RcActor,remoteFrame.rcPushAngle,0);
}
static void __cdecl SharedActorContact(){
 if(!RcLevel()||!SameLevel()||M<short>(RcActor+0x14)!=8){nativeActorContact();return;}
 SyncRc();ApplyRcPush();
 // Frustum/host activation must not remove a locally collidable RC. Restore
 // the entire bounded list after the contact pass, even when it was full.
 uintptr_t saved[65];for(unsigned i=0;i<65;i++)saved[i]=M<uintptr_t>(0x52f1d0+i*4);
 bool found=false;unsigned n=0;while(n<64&&saved[n]){if(saved[n]==RcActor)found=true;++n;}
 if(!found&&n<64){M<uintptr_t>(0x52f1d0+n*4)=RcActor;M<uintptr_t>(0x52f1d0+(n+1)*4)=0;}
 nativeActorContact();
 if(!found&&n==64){M<uintptr_t>(0x52f1d0)=RcActor;M<uintptr_t>(0x52f1d4)=0;nativeActorContact();}
 for(unsigned i=0;i<65;i++)M<uintptr_t>(0x52f1d0+i*4)=saved[i];
}
