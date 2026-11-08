// Verified adapter for Tin Robot. Other encounters require their own native
// script mapping; never copy unknown per-level globals or native pointers.
static const uintptr_t TinRobot=0x52cd20;
static bool BossReplicated(){return cachedLevel==1&&SameLevel()&&localFrame.boss.revision!=0;}
static int BossFrame(short animation,int frame){
 const int frames[8]={24,24,24,24,24,6,8,16};
 return animation<0||animation>=8?0:max(0,min(frame,(frames[animation]-1)<<16));
}
static bool ReadBoss(CoopBoss& state){
 if(M<short>(TinRobot+0x14)!=5)return false;
 CoopPose& p=state.pose;
 memcpy(&p.x,&M<unsigned char>(TinRobot),12);
 p.pitch=M<short>(TinRobot+0xc);p.yaw=M<short>(TinRobot+0xe);p.roll=M<short>(TinRobot+0x10);
 p.animation=M<short>(TinRobot+0x12);p.secondary=M<short>(TinRobot+0x16);
 p.frame=BossFrame(p.animation,M<int>(TinRobot+0x18));p.secondaryFrame=BossFrame(p.secondary,M<int>(TinRobot+0x1c));p.health=M<short>(TinRobot+0x7e);
 memcpy(state.motion,&M<unsigned char>(TinRobot+0x44),40);
 uintptr_t commands=M<uintptr_t>(0x4e02d8),command=M<uintptr_t>(TinRobot+0x80);
 if(!commands||command<commands||command-commands>2048)return false;
 state.commandOffset=(int)(command-commands);state.timer=M<short>(TinRobot+0x78);
 state.flags=M<short>(TinRobot+0x40)&0x10c;
 state.phase=M<unsigned>(0x52f578);state.hurtTimer=M<unsigned>(0x52f590);
 state.previousHealth=M<unsigned>(0x52f5a8);state.flashTick=M<unsigned>(0x52f5c8);
 uintptr_t descriptor=M<uintptr_t>(TinRobot+0x98);
 if(!Readable(descriptor,0x1b))return false;
 state.collisionFlags=M<unsigned char>(descriptor+0x1a);
 return CoopValidPose(p)&&state.phase<=3&&state.hurtTimer<=3600&&state.previousHealth<=100&&state.flashTick<=1;
}
static void WriteBoss(const CoopBoss& state){
 if(M<short>(TinRobot+0x14)!=5)return;
 if(state.pose.health==0){SafeActorDeath(TinRobot,2);M<short>(TinRobot+0x14)=0;return;}
 uintptr_t commands=M<uintptr_t>(0x4e02d8),descriptor=M<uintptr_t>(TinRobot+0x98);
 if(!Readable(commands+state.commandOffset,2)||!Readable(descriptor,0x1b))return;
 const CoopPose& p=state.pose;
 memcpy(&M<unsigned char>(TinRobot),&p.x,12);
 M<short>(TinRobot+0xc)=p.pitch;M<short>(TinRobot+0xe)=p.yaw;M<short>(TinRobot+0x10)=p.roll;
 M<short>(TinRobot+0x12)=p.animation;M<short>(TinRobot+0x16)=p.secondary;
 M<int>(TinRobot+0x18)=BossFrame(p.animation,p.frame);M<int>(TinRobot+0x1c)=BossFrame(p.secondary,p.secondaryFrame);
 M<short>(TinRobot+0x7e)=p.health;memcpy(&M<unsigned char>(TinRobot+0x44),state.motion,40);
 M<uintptr_t>(TinRobot+0x80)=commands+state.commandOffset;M<short>(TinRobot+0x78)=state.timer;
 M<unsigned short>(TinRobot+0x40)=(M<unsigned short>(TinRobot+0x40)&~0x10c)|(state.flags&0x10c);
 M<unsigned>(0x52f578)=state.phase;M<unsigned>(0x52f590)=state.hurtTimer;
 M<unsigned>(0x52f5a8)=state.previousHealth;M<unsigned>(0x52f5c8)=state.flashTick;
 M<unsigned char>(descriptor+0x1a)=(unsigned char)state.collisionFlags;
}
static void ObserveBossDamage(){
 int health=M<short>(TinRobot+0x7e);
 if(bossLastHealth>=10&&health>=0&&health<bossLastHealth&&bossLastHealth-health<=20)
  bossDamage[localOwner]+=(unsigned)(bossLastHealth-health);
 bossLastHealth=health;
}
static void BeginBossTick(){
 if(cachedLevel!=1||!SameLevel())return;
 ObserveBossDamage();
 CoopBoss& state=localFrame.boss;const CoopBoss& remote=remoteFrame.boss;
 bossDamage[1-localOwner]=max(bossDamage[1-localOwner],remote.damage[1-localOwner]);
 // The owner reloaded the level (no record): the fight continues from this PC's native state.
 if(state.revision&&state.owner!=localOwner&&!remote.revision){state.owner=localOwner;++state.revision;SyncLog("Tin Robot: the owner reloaded the level, continuing locally");}
 // After our own reload, never resume ownership from our old record echoed by the other PC.
 bool staleSelf=remote.owner==localOwner&&!state.revision;
 if(remote.revision&&!staleSelf&&(!state.revision||(remote.owner==state.owner&&(int32_t)(remote.revision-state.revision)>0)||
    (remote.owner<state.owner))){
  state=remote;
 }
 if(!state.revision&&M<unsigned>(0x52f578)>=2){
  CoopBoss candidate={};candidate.owner=localOwner;candidate.revision=1;
  if(ReadBoss(candidate))state=candidate;
 }
 if(!state.revision)return;
 if(state.owner!=localOwner)WriteBoss(state);
 else{
  unsigned peer=1-localOwner;
  if(bossDamage[peer]>bossDamageApplied[peer]){
   unsigned damage=bossDamage[peer]-bossDamageApplied[peer];
   // Native Tin Robot defeat begins below 10 HP, not at generic enemy zero.
   int health=M<short>(TinRobot+0x7e);
   if(health>=10)M<short>(TinRobot+0x7e)=(short)max(9,health-(int)min(damage,20u));
   bossDamageApplied[peer]=bossDamage[peer];
  }
 }
 bossLastHealth=M<short>(TinRobot+0x7e);
}
static void EndBossTick(){
 if(cachedLevel!=1||!SameLevel())return;
 ObserveBossDamage();CoopBoss& state=localFrame.boss;
 if(!state.revision){BeginBossTick();return;}
 if(state.owner==localOwner){CoopBoss next=state;if(ReadBoss(next)){++next.revision;state=next;}
  else if(M<short>(TinRobot+0x14)==0||M<short>(TinRobot+0x7e)==0){state.pose.health=0;state.phase=3;++state.revision;}}
 else WriteBoss(state);
 for(unsigned i=0;i<2;i++)state.damage[i]=bossDamage[i];
 bossLastHealth=M<short>(TinRobot+0x7e);
}

static PoseSample bossPoseSamples[32];static unsigned bossPoseCount;static bool bossPoseReady,bossPresenting;
static CoopPose bossDisplayPose;static unsigned bossPresentationOwner=2;
static unsigned char bossSavedRender[32];
static void ResetBossPresentation(){bossPoseReady=false;bossPoseCount=0;bossPresenting=false;bossPresentationOwner=2;}
static const CoopPose& SmoothBossPoseAt(const CoopPose& received,uint32_t sequence,DWORD now){
 if(!bossPoseReady){bossPoseCount=0;bossPoseReady=true;}
 if(!bossPoseCount||(int32_t)(sequence-bossPoseSamples[bossPoseCount-1].sequence)>0){
  if(bossPoseCount){
   const PoseSample& last=bossPoseSamples[bossPoseCount-1];
   int64_t dx=(int64_t)received.x-last.pose.x,dy=(int64_t)received.y-last.pose.y,dz=(int64_t)received.z-last.pose.z;
   if(now-last.time>500||dx>0x40000||dx< -0x40000||dy>0x40000||dy< -0x40000||dz>0x40000||dz< -0x40000)bossPoseCount=0;
  }
  if(bossPoseCount==32){memmove(bossPoseSamples,bossPoseSamples+1,31*sizeof(PoseSample));--bossPoseCount;}
  PoseSample& sample=bossPoseSamples[bossPoseCount++];sample.pose=received;sample.time=now;sample.sequence=sequence;
 }
 DWORD renderTime=now-100;
 while(bossPoseCount>2&&(int32_t)(renderTime-bossPoseSamples[1].time)>=0){
  memmove(bossPoseSamples,bossPoseSamples+1,(bossPoseCount-1)*sizeof(PoseSample));--bossPoseCount;
 }
 const PoseSample& a=bossPoseSamples[0];
 if(bossPoseCount==1||(int32_t)(renderTime-a.time)<=0){bossDisplayPose=a.pose;return bossDisplayPose;}
 const PoseSample& b=bossPoseSamples[1];
 unsigned duration=b.time-a.time,elapsed=renderTime-a.time;
 // Hold the last authoritative position on a stall, rather than predicting
 // through walls. Reconnection/teleport discards the old history above.
 if(!duration||elapsed>=duration){bossDisplayPose=b.pose;return bossDisplayPose;}
 bossDisplayPose=a.pose;
 bossDisplayPose.x=CoopLerp(a.pose.x,b.pose.x,elapsed,duration);
 bossDisplayPose.y=CoopLerp(a.pose.y,b.pose.y,elapsed,duration);
 bossDisplayPose.z=CoopLerp(a.pose.z,b.pose.z,elapsed,duration);
 bossDisplayPose.yaw=CoopLerpAngle(a.pose.yaw,b.pose.yaw,elapsed,duration);
 bossDisplayPose.pitch=CoopLerpAngle(a.pose.pitch,b.pose.pitch,elapsed,duration);
 bossDisplayPose.roll=CoopLerpAngle(a.pose.roll,b.pose.roll,elapsed,duration);
 if(a.pose.animation==b.pose.animation&&b.pose.frame>=a.pose.frame)
  bossDisplayPose.frame=CoopLerp(a.pose.frame,b.pose.frame,elapsed,duration);
 if(a.pose.secondary==b.pose.secondary&&b.pose.secondaryFrame>=a.pose.secondaryFrame)
  bossDisplayPose.secondaryFrame=CoopLerp(a.pose.secondaryFrame,b.pose.secondaryFrame,elapsed,duration);
 return bossDisplayPose;
}

static void BeginBossPresentation(){
 if(!BossReplicated()||localFrame.boss.owner==localOwner||localFrame.boss.pose.health<=0||M<short>(TinRobot+0x14)!=5){ResetBossPresentation();return;}
 const CoopBoss& state=localFrame.boss;
 if(bossPresentationOwner!=state.owner){ResetBossPresentation();bossPresentationOwner=state.owner;}
 const CoopPose& p=SmoothBossPoseAt(state.pose,state.revision,GetTickCount());
 memcpy(bossSavedRender,&M<unsigned char>(TinRobot),32);bossPresenting=true;
 memcpy(&M<unsigned char>(TinRobot),&p.x,12);
 M<short>(TinRobot+0xc)=p.pitch;M<short>(TinRobot+0xe)=p.yaw;M<short>(TinRobot+0x10)=p.roll;
 M<short>(TinRobot+0x12)=p.animation;M<short>(TinRobot+0x16)=p.secondary;
 M<int>(TinRobot+0x18)=BossFrame(p.animation,p.frame);M<int>(TinRobot+0x1c)=BossFrame(p.secondary,p.secondaryFrame);
}
static void EndBossPresentation(){if(bossPresenting){memcpy(&M<unsigned char>(TinRobot),bossSavedRender,32);bossPresenting=false;}}
