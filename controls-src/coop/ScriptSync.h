// Levels 3-15: scripted encounter engine (bosses and special actors) plus
// whitelisted level-script globals. Extends the Tin Robot pattern:
// - The first player whose native script activates the encounter owns its
//   simulation; the host wins a simultaneous claim. Ownership never moves
//   back to "nobody" inside a level.
// - The owner publishes value-only actor snapshots (pose, motion, command
//   offset relative to a native script base, timer, flags, collision byte).
//   Observers write them before and after every native actor/script pass.
// - Observers never let their native script consume a local hit: the hit is
//   recorded as a cumulative request and the owner applies it once.
// - One-way script progress (phase, damage, puzzle stages) merges with max/min
//   and replays the native threshold side effects exactly once (adopt hook).
// Native pointers never cross the network.
// Max/Min: symmetric one-way progress (either player may advance it).
// Follow: one-way, but only the encounter owner advances it (boss phase and
// damage); the owner never adopts an observer's locally-run-ahead value.
// Owned: the owner's value is written on observers every native pass.
// Bits: one-way OR of the masked bits (independent switches/sub-puzzles).
// Countdown: a one-shot timer 0 (idle) -> n..1 (running) -> -1 (done); it is sent and merged as
// monotonic progress (0, 0x3d-v, 0x3d) so a running timer never pulls a finished one back.
enum {ScriptMax=1,ScriptMin=2,ScriptOwned=3,ScriptFollow=4,ScriptBits=5,ScriptCountdown=6};
static int CountdownProgress(int v){return v==0?0:v<0?0x3d:v>0x3c?1:0x3d-v;}
static int CountdownValue(int p){return p<=0?0:p>=0x3d?-1:0x3d-p;}
struct ScriptVarDesc {uintptr_t address;unsigned char merge;int mask;};
typedef void(__cdecl *ScriptColliderRotateFn)(int,int,int,int);
static ScriptColliderRotateFn scriptColliderRotate=(ScriptColliderRotateFn)0x488aa0;
// slotVar != 0: the record follows the actor slot stored in that level global
// (range slot..slotHigh), e.g. level 9 minion pairs. A slot already used by an
// earlier record leaves this record inactive (never double-counted).
struct ScriptActorDesc {unsigned slot;uintptr_t commandBase;unsigned short flagMask;uintptr_t slotVar;unsigned slotHigh;};
typedef bool (*ScriptStateFn)();
typedef unsigned (*ScriptDamageFn)(unsigned actor,unsigned pending);
typedef void (*ScriptAdoptFn)(unsigned index,int from,int to);
// Observer-only guard around each native pass (enter after the snapshot write,
// leave before reading back): neutralizes native branches that would otherwise
// consume progress on a local, unconfirmed state.
typedef void (*ScriptGuardFn)(bool enter);
struct ScriptLevelDesc {
 unsigned level;const ScriptActorDesc* actors;unsigned actorCount;const ScriptVarDesc* vars;unsigned varCount;
 ScriptStateFn active,retired;ScriptDamageFn applyDamage;ScriptAdoptFn adopt;bool countHits;
 ScriptGuardFn guard;uint64_t reserved;   // reserved: extra slots excluded from generic enemy sync
};
static unsigned scriptOwner=2;
static unsigned scriptOwnerDiagnostic(){return scriptOwner;}
static uint32_t scriptRequests[4],scriptIncorporated[4],scriptOwnDamage[4];
static int scriptLastHealth[4],scriptSlot[4];static short scriptModel[4];static bool scriptGuarded;
static void ResetScriptPresentation();
static void ResetScript(){
 scriptOwner=2;memset(localFrame.scriptActors,0,sizeof(localFrame.scriptActors));localFrame.script={};
 memset(scriptRequests,0,sizeof(scriptRequests));memset(scriptIncorporated,0,sizeof(scriptIncorporated));memset(scriptOwnDamage,0,sizeof(scriptOwnDamage));
 for(unsigned i=0;i<4;i++){scriptLastHealth[i]=-1;scriptModel[i]=0;scriptSlot[i]=-1;}scriptGuarded=false;
 ResetScriptPresentation();
}
static const ScriptLevelDesc* ScriptLevel();
static int ScriptResolveSlot(const ScriptLevelDesc* d,unsigned i){
 const ScriptActorDesc& s=d->actors[i];if(!s.slotVar)return (int)s.slot;
 int slot=M<int>(s.slotVar);if(slot<(int)s.slot||slot>(int)s.slotHigh)return -1;
 for(unsigned k=0;k<i;k++)if(ScriptResolveSlot(d,k)==slot)return -1;
 return slot;
}
static void ResetScriptSamples(unsigned i);
// A record whose slot changed starts over: no damage inferred across actors.
static void TrackScriptSlots(const ScriptLevelDesc* d){
 for(unsigned i=0;i<d->actorCount;i++){int slot=ScriptResolveSlot(d,i);
  if(slot!=scriptSlot[i]){scriptSlot[i]=slot;scriptModel[i]=0;scriptLastHealth[i]=-1;ResetScriptSamples(i);}}
}
static uintptr_t ScriptActorAddress(const ScriptLevelDesc*,unsigned i){return 0x52c840+scriptSlot[i]*0x9c;}
static bool ScriptActorSlot(unsigned slot){
 const ScriptLevelDesc* d=ScriptLevel();if(!d)return false;
 if(slot<64&&(d->reserved>>slot)&1)return true;
 for(unsigned i=0;i<d->actorCount;i++){const ScriptActorDesc& s=d->actors[i];
  if(slot==s.slot||(s.slotVar&&slot>=s.slot&&slot<=s.slotHigh))return true;}
 return false;
}
static const CoopActorRule* ScriptRule(unsigned index){
 for(unsigned k=0;k<CoopActorRuleCount;k++)if(CoopActorRules[k].level==cachedLevel&&CoopActorRules[k].index==index)return &CoopActorRules[k];
 return 0;
}
static int ScriptClampVar(unsigned index,int value){
 for(unsigned k=0;k<CoopScriptBoundCount;k++){const CoopScriptBound& b=CoopScriptBounds[k];
  if(b.level==cachedLevel&&b.index==index)return value<b.low?b.low:value>b.high?b.high:value;}
 return 0;
}
static bool ReadScriptActor(const ScriptLevelDesc* d,unsigned i,CoopScriptActor& a){
 if(scriptSlot[i]<0)return false;
 const ScriptActorDesc& s=d->actors[i];uintptr_t actor=ScriptActorAddress(d,i);const CoopActorRule* rule=ScriptRule(i);
 short model=M<short>(actor+0x14);if(!rule||model<=0||model!=scriptModel[i])return false;
 a.slot=(uint8_t)scriptSlot[i];a.live=1;CoopPose& p=a.pose;
 memcpy(&p.x,&M<unsigned char>(actor),12);
 p.pitch=M<short>(actor+0xc);p.yaw=M<short>(actor+0xe);p.roll=M<short>(actor+0x10);
 p.animation=M<short>(actor+0x12);p.secondary=M<short>(actor+0x16);
 int frame=NativeEnemyFrame(model,p.animation,M<int>(actor+0x18)),secondary=NativeEnemyFrame(model,p.secondary,M<int>(actor+0x1c));
 if(frame<0){p.animation=-1;frame=0;}if(secondary<0){p.secondary=-1;secondary=0;}
 p.frame=frame;p.secondaryFrame=secondary;
 int health=M<short>(actor+0x7e);p.health=(int16_t)(health<rule->healthLow?rule->healthLow:health>rule->healthHigh?rule->healthHigh:health);
 memcpy(a.motion,&M<unsigned char>(actor+0x44),40);memcpy(a.render,&M<unsigned char>(actor+0x24),20);
 a.commandOffset=-1;
 if(s.commandBase){uintptr_t base=M<uintptr_t>(s.commandBase),command=M<uintptr_t>(actor+0x80);
  if(base&&command>=base&&command-base<=(uintptr_t)rule->commandHigh)a.commandOffset=(int)(command-base);}
 a.timer=M<short>(actor+0x78);if(a.timer< -1)a.timer=-1;
 a.flags=M<unsigned short>(actor+0x40)&s.flagMask;
 uintptr_t descriptor=M<uintptr_t>(actor+0x98);a.collision=Readable(descriptor,0x1b)?M<unsigned char>(descriptor+0x1a):0;
 const int bound=0x10000000;
 if(p.x<-bound||p.x>bound||p.y<-bound||p.y>bound||p.z<-bound||p.z>bound||p.pitch<-4096||p.pitch>4096||p.yaw<-4096||p.yaw>4096||p.roll<-4096||p.roll>4096||
    p.animation< -1||p.animation>=100||p.secondary< -1||p.secondary>=100)return false;
 for(unsigned k=0;k<10;k++)if(a.motion[k]<-bound||a.motion[k]>bound)return false;
 return true;
}
static void WriteScriptActor(const ScriptLevelDesc* d,unsigned i,const CoopScriptActor& a){
 if(scriptSlot[i]<0||a.slot!=scriptSlot[i])return;
 const ScriptActorDesc& s=d->actors[i];uintptr_t actor=ScriptActorAddress(d,i);
 short model=M<short>(actor+0x14);if(!a.revision||!a.live||model<=0||model!=scriptModel[i])return;
 const CoopPose& p=a.pose;
 memcpy(&M<unsigned char>(actor),&p.x,12);
 M<short>(actor+0xc)=p.pitch;M<short>(actor+0xe)=p.yaw;M<short>(actor+0x10)=p.roll;
 int frame=NativeEnemyFrame(model,p.animation,p.frame),secondary=NativeEnemyFrame(model,p.secondary,p.secondaryFrame);
 if(frame>=0){M<short>(actor+0x12)=p.animation;M<int>(actor+0x18)=frame;}
 if(secondary>=0){M<short>(actor+0x16)=p.secondary;M<int>(actor+0x1c)=secondary;}
 M<short>(actor+0x7e)=p.health;scriptLastHealth[i]=p.health;
 memcpy(&M<unsigned char>(actor+0x44),a.motion,40);memcpy(&M<unsigned char>(actor+0x24),a.render,20);
 if(s.commandBase&&a.commandOffset>=0){uintptr_t base=M<uintptr_t>(s.commandBase);
  if(base&&Readable(base+a.commandOffset,2))M<uintptr_t>(actor+0x80)=base+a.commandOffset;}
 if(a.timer>=0)M<short>(actor+0x78)=a.timer;
 M<unsigned short>(actor+0x40)=(M<unsigned short>(actor+0x40)&~s.flagMask)|(a.flags&s.flagMask);
 uintptr_t descriptor=M<uintptr_t>(actor+0x98);if(Readable(descriptor,0x1b))M<unsigned char>(descriptor+0x1a)=a.collision;
}
static void ObserveScriptDamage(const ScriptLevelDesc* d){
 for(unsigned i=0;i<d->actorCount;i++){
  if(scriptSlot[i]<0)continue;
  uintptr_t actor=ScriptActorAddress(d,i);short model=M<short>(actor+0x14);
  if(!scriptModel[i]&&model>0)scriptModel[i]=model;
  if(model<=0||model!=scriptModel[i]){scriptLastHealth[i]=-1;continue;}
  int health=M<short>(actor+0x7e),last=scriptLastHealth[i];
  if(last>=0&&health>=0&&health<last&&last-health<=200){
   unsigned amount=d->countHits?1u:(unsigned)(last-health);
   if(scriptOwner!=2&&scriptOwner!=localOwner)scriptRequests[i]=min(1000000u,scriptRequests[i]+amount);
   else scriptOwnDamage[i]=min(1000000u,scriptOwnDamage[i]+amount);
  }
  scriptLastHealth[i]=health;
 }
}
// One-way merges in both directions; the adopt hook replays native side
// effects of the threshold the local script skipped. Never rewinds.
static void MergeScriptVars(const ScriptLevelDesc* d){
 const CoopScriptVars& r=remoteFrame.script;if(!r.revision)return;
 for(unsigned k=0;k<d->varCount;k++){
  const ScriptVarDesc& v=d->vars[k];
  if(v.merge==ScriptBits){
   int local=M<int>(v.address),merged=local|(ScriptClampVar(k,r.value[k])&v.mask);
   if(merged!=local){M<int>(v.address)=merged;++syncStats.varAdoptions;SyncLog("value %x bits %x -> %x",(unsigned)v.address,local,merged);if(d->adopt)d->adopt(k,local,merged);}
   continue;
  }
  if(v.merge==ScriptCountdown){
   int local=M<int>(v.address),remote=ScriptClampVar(k,r.value[k]);
   if(remote>CountdownProgress(local)){int to=CountdownValue(remote);M<int>(v.address)=to;++syncStats.varAdoptions;SyncLog("value %x %d -> %d",(unsigned)v.address,local,to);if(d->adopt)d->adopt(k,local,to);}
   continue;
  }
  bool follow=v.merge==ScriptFollow&&scriptOwner!=2&&scriptOwner!=localOwner&&r.owner==scriptOwner;
  if(v.merge!=ScriptMax&&v.merge!=ScriptMin&&!follow)continue;
  int local=M<int>(v.address),remote=ScriptClampVar(k,r.value[k]);
  if(((v.merge==ScriptMax||follow)&&remote>local)||(v.merge==ScriptMin&&remote<local)){M<int>(v.address)=remote;++syncStats.varAdoptions;SyncLog("value %x %d -> %d",(unsigned)v.address,local,remote);if(d->adopt)d->adopt(k,local,remote);}
 }
}
static void WriteOwnedVars(const ScriptLevelDesc* d){
 const CoopScriptVars& r=remoteFrame.script;
 if(!r.revision||r.owner!=scriptOwner||scriptOwner==localOwner)return;
 for(unsigned k=0;k<d->varCount;k++)if(d->vars[k].merge==ScriptOwned)M<int>(d->vars[k].address)=ScriptClampVar(k,r.value[k]);
}
static void SetScriptOwner(unsigned owner){
 if(owner==scriptOwner)return;
 if(owner==localOwner){
  // Requests made before this ownership were already reflected by the
  // snapshot we adopted (or there was no encounter yet); apply newer ones only.
  for(unsigned i=0;i<4;i++){const CoopScriptActor& r=remoteFrame.scriptActors[i];
   unsigned peer=1-localOwner;scriptIncorporated[i]=max(scriptIncorporated[i],r.revision?r.damage[peer]:0u);}
 }else for(unsigned i=0;i<4;i++)scriptRequests[i]=max(scriptRequests[i],localFrame.scriptActors[i].damage[localOwner]);
 scriptOwner=owner;++syncStats.scriptOwnerChanges;SyncLog("encounter owner -> %s",owner==0?"host":owner==1?"guest":"none");
}
// Level reload on one PC (game over, menu, travel): the reloaded PC starts with no snapshot of its
// own and publishes none. The PC that stayed continues the encounter as its owner (its native
// script kept running while unlinked, so a stale owner snapshot is never written back over it),
// and the reloaded PC never resumes ownership from its own old record still echoed by the other
// PC: it observes, adopts the Follow/Owned values and the snapshots, and replays one-shot effects.
static void ResolveScriptOwner(const ScriptLevelDesc* d){
 const CoopScriptActor& r=remoteFrame.scriptActors[0];
 if(!d->actorCount||!d->active)return;   // progress-only level: no encounter owner
 if(scriptOwner<=1&&scriptOwner!=localOwner&&!r.revision&&localFrame.scriptActors[0].revision){
  SyncLog("encounter: the owner reloaded the level, continuing locally");SetScriptOwner(localOwner);}
 bool staleSelf=r.owner==localOwner&&!localFrame.scriptActors[0].revision;
 if(r.revision&&r.owner<=1&&!staleSelf&&(scriptOwner==2||r.owner<scriptOwner))SetScriptOwner(r.owner);
 if(scriptOwner==2&&d->active())SetScriptOwner(localOwner);
}
// Level 6 boss chase target: the tick copies the LOCAL Buzz x/z into the boss target +0x5c/+0x64
// (420587/420591, operands redirected to bossChaseTarget at load). On the encounter owner the
// target is the nearest living player (3/4 hysteresis, as for ordinary enemies); everywhere else
// it stays the local Buzz, exactly as native.
static int bossChaseTarget[3];static unsigned bossChaseChoice;
static void UpdateBossChaseTarget(){
 for(unsigned a=0;a<3;a++)bossChaseTarget[a]=M<int>(0x52f300+a*4);
 if(cachedLevel!=6||!SameLevel()||scriptOwner!=localOwner){bossChaseChoice=0;return;}
 int64_t distance[2]={0,0};const int peer[3]={remoteFrame.pose.x,remoteFrame.pose.y,remoteFrame.pose.z};
 for(unsigned a=0;a<3;a++){int64_t l=((int64_t)M<int>(0x52c840+a*4)-bossChaseTarget[a])/256,r=((int64_t)M<int>(0x52c840+a*4)-peer[a])/256;distance[0]+=l*l;distance[1]+=r*r;}
 bossChaseChoice=ChooseEnemyTarget(bossChaseChoice,distance[0],distance[1],M<short>(0x52f396)>0,remoteFrame.pose.health>0);
 if(bossChaseChoice)memcpy(bossChaseTarget,peer,sizeof(peer));
}
static bool InstallBossChaseTarget(){
 if(M<uintptr_t>(0x420588)!=0x52f300||M<uintptr_t>(0x420592)!=0x52f308)return false;
 return SetImport(0x420588,(uintptr_t)&bossChaseTarget[0])&&SetImport(0x420592,(uintptr_t)&bossChaseTarget[2]);
}
static void BeginScript(){
 UpdateBossChaseTarget();
 const ScriptLevelDesc* d=ScriptLevel();if(!d||!SameLevel())return;
 // Evaluate retirement BEFORE following the owner's phase: the snapshot that
 // carries the defeat (final health/pose) must still be written once so the
 // local native retirement path (e.g. health <= 10 removal) can run.
 TrackScriptSlots(d);ObserveScriptDamage(d);ResolveScriptOwner(d);bool retired=d->retired&&d->retired();MergeScriptVars(d);
 if(scriptOwner==2)return;
 unsigned peer=1-localOwner;
 if(scriptOwner!=localOwner){
  for(unsigned i=0;i<d->actorCount;i++){
   const CoopScriptActor& r=remoteFrame.scriptActors[i];CoopScriptActor& s=localFrame.scriptActors[i];
   if(r.revision&&r.owner==scriptOwner&&(!s.revision||s.owner!=r.owner||(int32_t)(r.revision-s.revision)>0)){
    s=r;s.damage[localOwner]=scriptRequests[i];
   }
   if(!retired)WriteScriptActor(d,i,s);
  }
  WriteOwnedVars(d);
  if(d->guard&&!retired){d->guard(true);scriptGuarded=true;}
 }else{
  for(unsigned i=0;i<d->actorCount;i++){
   const CoopScriptActor& r=remoteFrame.scriptActors[i];
   if(!r.revision||r.owner!=localOwner||r.damage[peer]<=scriptIncorporated[i])continue;
   unsigned pending=r.damage[peer]-scriptIncorporated[i];if(pending>1000000)continue;
   unsigned used=d->applyDamage?d->applyDamage(i,pending):pending;
   scriptIncorporated[i]+=min(used,pending);
   if(scriptSlot[i]>=0)scriptLastHealth[i]=M<short>(ScriptActorAddress(d,i)+0x7e);
  }
 }
}
static void PublishScriptVars(const ScriptLevelDesc* d){
 CoopScriptVars next=localFrame.script;memset(next.value,0,sizeof(next.value));
 for(unsigned k=0;k<d->varCount&&k<32;k++){int v=M<int>(d->vars[k].address);if(d->vars[k].merge==ScriptBits)v&=d->vars[k].mask;if(d->vars[k].merge==ScriptCountdown)v=CountdownProgress(v);next.value[k]=ScriptClampVar(k,v);}
 next.owner=scriptOwner;
 if(!localFrame.script.revision||next.owner!=localFrame.script.owner||memcmp(next.value,localFrame.script.value,sizeof(next.value))){
  ++next.revision;if(!next.revision)next.revision=1;localFrame.script=next;
 }
}
static void EndScript(){
 const ScriptLevelDesc* d=ScriptLevel();if(!d||!SameLevel())return;
 if(scriptGuarded){scriptGuarded=false;if(d->guard)d->guard(false);}
 TrackScriptSlots(d);ObserveScriptDamage(d);ResolveScriptOwner(d);MergeScriptVars(d);
 if(scriptOwner==localOwner){
  unsigned peer=1-localOwner;
  for(unsigned i=0;i<d->actorCount;i++){
   CoopScriptActor& s=localFrame.scriptActors[i];CoopScriptActor next=s;
   if(ReadScriptActor(d,i,next)){next.owner=(uint8_t)localOwner;}
   else{next.live=0;next.owner=(uint8_t)localOwner;next.slot=(uint8_t)(scriptSlot[i]>=0?scriptSlot[i]:(int)d->actors[i].slot);const CoopActorRule* rule=ScriptRule(i);
    if(rule&&(next.pose.health<rule->healthLow||next.pose.health>rule->healthHigh))next.pose.health=rule->healthLow;}
   next.damage[localOwner]=scriptOwnDamage[i];next.damage[peer]=scriptIncorporated[i];
   if(!s.revision||memcmp(&next,&s,sizeof(next))){next.revision=s.revision+1;if(!next.revision)next.revision=1;s=next;}
  }
 }else if(scriptOwner!=2){
  bool retired=d->retired&&d->retired();
  for(unsigned i=0;i<d->actorCount;i++){
   CoopScriptActor& s=localFrame.scriptActors[i];
   if(!retired)WriteScriptActor(d,i,s);
   if(s.revision)s.damage[localOwner]=scriptRequests[i];
  }
  WriteOwnedVars(d);
 }
 PublishScriptVars(d);
}

// Observer presentation: 100 ms buffered pose, restored after drawing.
static PoseSample scriptSamples[4][32];static unsigned scriptSampleCount[4];static bool scriptPresenting[4];
static unsigned char scriptSavedRender[4][32];static unsigned scriptPresentationOwner=2;
static uintptr_t scriptPresentAddress[4];
static void ResetScriptSamples(unsigned i){if(i<4)scriptSampleCount[i]=0;}
static void ResetScriptPresentation(){memset(scriptSampleCount,0,sizeof(scriptSampleCount));memset(scriptPresenting,0,sizeof(scriptPresenting));scriptPresentationOwner=2;}
static CoopPose SmoothScriptPose(unsigned i,const CoopPose& received,uint32_t sequence,DWORD now){
 PoseSample* s=scriptSamples[i];unsigned& n=scriptSampleCount[i];
 if(!n||(int32_t)(sequence-s[n-1].sequence)>0){
  if(n){const PoseSample& last=s[n-1];
   int64_t dx=(int64_t)received.x-last.pose.x,dy=(int64_t)received.y-last.pose.y,dz=(int64_t)received.z-last.pose.z;
   if(now-last.time>500||dx>0x40000||dx< -0x40000||dy>0x40000||dy< -0x40000||dz>0x40000||dz< -0x40000)n=0;}
  if(n==32){memmove(s,s+1,31*sizeof(PoseSample));--n;}
  s[n].pose=received;s[n].time=now;s[n].sequence=sequence;++n;
 }
 DWORD renderTime=now-100;
 while(n>2&&(int32_t)(renderTime-s[1].time)>=0){memmove(s,s+1,(n-1)*sizeof(PoseSample));--n;}
 const PoseSample& a=s[0];
 if(n==1||(int32_t)(renderTime-a.time)<=0)return a.pose;
 const PoseSample& b=s[1];unsigned duration=b.time-a.time,elapsed=renderTime-a.time;
 if(!duration||elapsed>=duration)return b.pose;
 CoopPose p=a.pose;
 p.x=CoopLerp(a.pose.x,b.pose.x,elapsed,duration);p.y=CoopLerp(a.pose.y,b.pose.y,elapsed,duration);p.z=CoopLerp(a.pose.z,b.pose.z,elapsed,duration);
 p.yaw=CoopLerpAngle(a.pose.yaw,b.pose.yaw,elapsed,duration);p.pitch=CoopLerpAngle(a.pose.pitch,b.pose.pitch,elapsed,duration);p.roll=CoopLerpAngle(a.pose.roll,b.pose.roll,elapsed,duration);
 if(a.pose.animation==b.pose.animation&&b.pose.frame>=a.pose.frame)p.frame=CoopLerp(a.pose.frame,b.pose.frame,elapsed,duration);
 if(a.pose.secondary==b.pose.secondary&&b.pose.secondaryFrame>=a.pose.secondaryFrame)p.secondaryFrame=CoopLerp(a.pose.secondaryFrame,b.pose.secondaryFrame,elapsed,duration);
 return p;
}
static bool ScriptPresentable(const ScriptLevelDesc* d,unsigned i){
 if(scriptSlot[i]<0||localFrame.scriptActors[i].slot!=scriptSlot[i])return false;
 const CoopScriptActor& s=localFrame.scriptActors[i];uintptr_t actor=ScriptActorAddress(d,i);
 return s.revision&&s.live&&s.pose.health>0&&M<short>(actor+0x14)==scriptModel[i]&&scriptModel[i]>0;
}
static void BeginScriptPresentation(){
 memset(scriptPresenting,0,sizeof(scriptPresenting));
 const ScriptLevelDesc* d=ScriptLevel();if(!d||!SameLevel()||scriptOwner==2||scriptOwner==localOwner||d->retired()){ResetScriptPresentation();return;}
 if(scriptPresentationOwner!=scriptOwner){ResetScriptPresentation();scriptPresentationOwner=scriptOwner;}
 DWORD now=GetTickCount();
 for(unsigned i=0;i<d->actorCount;i++){
  if(!ScriptPresentable(d,i)){scriptSampleCount[i]=0;continue;}
  uintptr_t actor=ScriptActorAddress(d,i);short model=scriptModel[i];
  CoopPose p=SmoothScriptPose(i,localFrame.scriptActors[i].pose,localFrame.scriptActors[i].revision,now);
  int frame=NativeEnemyFrame(model,p.animation,p.frame),secondary=NativeEnemyFrame(model,p.secondary,p.secondaryFrame);
  memcpy(scriptSavedRender[i],&M<unsigned char>(actor),32);scriptPresenting[i]=true;scriptPresentAddress[i]=actor;
  memcpy(&M<unsigned char>(actor),&p.x,12);
  M<short>(actor+0xc)=p.pitch;M<short>(actor+0xe)=p.yaw;M<short>(actor+0x10)=p.roll;
  if(frame>=0){M<short>(actor+0x12)=p.animation;M<int>(actor+0x18)=frame;}
  if(secondary>=0){M<short>(actor+0x16)=p.secondary;M<int>(actor+0x1c)=secondary;}
 }
}
static void EndScriptPresentation(){
 for(unsigned i=0;i<4;i++)if(scriptPresenting[i]){memcpy(&M<unsigned char>(scriptPresentAddress[i]),scriptSavedRender[i],32);scriptPresenting[i]=false;}
}
// Keep an owner-driven encounter renderable when local activation did not
// list it. Local Buzz stays last; capacity 64 objects.
static unsigned IncludeScriptActors(uintptr_t* list,unsigned rendered){
 const ScriptLevelDesc* d=ScriptLevel();if(!d||!SameLevel()||scriptOwner==2||scriptOwner==localOwner||d->retired())return rendered;
 for(unsigned i=0;i<d->actorCount;i++){
  if(!ScriptPresentable(d,i))continue;uintptr_t actor=ScriptActorAddress(d,i);
  bool found=false;for(unsigned k=0;k<rendered;k++)if(list[k]==actor)found=true;
  M<unsigned short>(actor+0x40)|=1;
  if(!found&&rendered>=1&&rendered<64){list[rendered]=list[rendered-1];list[rendered-1]=actor;list[++rendered]=0;}
 }
 return rendered;
}

// ---- Level 3 boss (tick 41aa10, init 41a8a0) -------------------------------
// Actor slot 0. Phase 52f754: 0 idle -> 1 (local proximity 49f3c0, 0x17c,
// cutscene) -> 0x3e7 fight (41abba) -> 0x3e8 defeat (41ad8e) -> 0x3e9.. removal
// (41b029, 405d20 mode 1) -> 0x406 level complete (830cc4). A hit is any health
// different from 52f730 (41ac7f); native restores health and adds 0x800 to
// 52f6f0 (0x1000 -> 0x3800, five hits). At >=0x3000 mesh 0x32 and pickup 0 are
// revealed (41acf6); every hit re-arms type-0x3f objects in 529e7c (41accf).
static const ScriptActorDesc level3Actors[]={{0,0x4e02fc,0x18c}};
static const ScriptVarDesc level3Vars[]={{0x52f754,ScriptFollow},{0x52f6f0,ScriptFollow},{0x52f75c,ScriptOwned},{0x52f704,ScriptOwned},
 {0x52f730,ScriptOwned},{0x52f710,ScriptOwned},{0x52f758,ScriptOwned}};
static bool Level3Active(){return M<int>(0x52f754)>=1;}
static bool Level3Retired(){return M<int>(0x52f754)>=0x3e9;}
static unsigned Level3Damage(unsigned,unsigned pending){
 int phase=M<int>(0x52f754);
 if(phase>=0x3e8)return pending;      // already defeated: requests are moot
 if(phase!=0x3e7)return 0;            // keep until the fight begins
 int expected=M<int>(0x52f730);short& health=M<short>(0x52c8be);
 if(health!=expected)return 0;        // a native hit is already pending this tick
 health=(short)(expected>0?expected-1:expected+1);return 1;  // 41ac7f processes it once
}
static void Level3RearmObjects(){for(uintptr_t e=0x529e7c;e<0x52ad7c;e+=0x3c)if(M<unsigned char>(e+9)==0x3f)M<short>(e)=1;}
static void Level3Adopt(unsigned index,int from,int to){
 if(index==0&&from<0x3e8&&to>=0x3e8)M<int>(0x52f6f4)=M<int>(0x52c844)-0x2ee0;   // 41ade8: fall start of mesh 0
 if(index==1){
  for(int step=from;step<to;step+=0x800)Level3RearmObjects();
  if(from<0x3000&&to>=0x3000&&M<int>(0x52f72c)!=0){
   uintptr_t table=M<uintptr_t>(0x559d6c);
   mechScale(0x32,0x1000,0x1000,0x1000);
   if(Readable(table,12))M<int>(table+8)=M<int>(0x52f72c);
   M<int>(0x52f72c)=0;
  }
 }
}
// ---- Level 6 boss (tick 420060, init 41ffb0) -------------------------------
// Actor slot 0. Phase 52f9a4: 0 -> 1 when the LOCAL player x < 0x31f9e (420242,
// cutscene) -> 2 when the cutscene ends (42043c) -> 3 defeat (42011e: health
// <= 10, token bit 0x80) -> 4 level complete (4207e9, 830cc4). A hit is any
// nonzero health different from 52f98c (42008e); real HP decreases. Attack
// cycle 52f9a0/52f9b8, hurt timer 52f990 and oscillation 52f994/52f99c drive
// the actor target fields (motion +0x5c..+0x64), shared via the owner snapshot.
static const ScriptActorDesc level6Actors[]={{0,0,0x10c}};
static const ScriptVarDesc level6Vars[]={{0x52f9a4,ScriptFollow},{0x52f98c,ScriptOwned},{0x52f990,ScriptOwned},{0x52f9a0,ScriptOwned},
 {0x52f9b8,ScriptOwned},{0x52f994,ScriptOwned},{0x52f99c,ScriptOwned}};
static bool Level6Active(){return M<int>(0x52f9a4)>=1;}
static bool Level6Retired(){return M<int>(0x52f9a4)>=3;}
static unsigned Level6Damage(unsigned,unsigned pending){
 short& health=M<short>(0x52c8be);
 if(M<int>(0x52f9a4)>=3||health<=10)return pending;   // defeated: requests are moot
 if(health!=M<int>(0x52f98c))return 0;               // native hit already pending
 // 42008e processes any changed nonzero health once; never write 0 (skipped).
 health=(short)max(1,(int)health-(int)min(pending,100u));return pending;
}
// ---- Level 12 boss (tick 42b3a0, init 42b300) ------------------------------
// Actor slot 0. Phase 52fe24: 0 -> 1 when the LOCAL player x > -0x18b2b
// (42b5be, cutscene, boss rises) -> 2 when the cutscene ends (42b72d) -> 3 when
// health < 10 (42b84f, token bit 0x80) -> 4 level complete (42bd8a). Hits are
// only processed in phase 2: health differs from 52fe00 (42b7d6). The attack
// cycle 52fe30/52fe34 and its projectiles stay local to each player (they only
// hurt the local Buzz); the boss pose/yaw they are aimed from is the owner's.
static const ScriptActorDesc level12Actors[]={{0,0,0x10c}};
static const ScriptVarDesc level12Vars[]={{0x52fe24,ScriptFollow},{0x52fe00,ScriptOwned},{0x52fe04,ScriptOwned},{0x52fe2c,ScriptOwned}};
static bool Level12Active(){return M<int>(0x52fe24)>=1;}
static bool Level12Retired(){return M<int>(0x52fe24)>=3;}
static unsigned Level12Damage(unsigned,unsigned pending){
 int phase=M<int>(0x52fe24);short& health=M<short>(0x52c8be);
 if(phase>=3||health<10)return pending;
 if(phase!=2||health!=M<int>(0x52fe00))return 0;      // hold until the fight / pending native hit
 health=(short)max(1,(int)health-(int)min(pending,100u));return pending;
}
// ---- Level 4 one-way puzzle progress (tick 41c640) ------------------------
// [0] 52f784 bits 1/2/4: targets on colliders 0x16..0x18 (48b640 hit flag 0x12,
//     41dad0..41dbd4) rotate the collider (488aa0) and mesh 0x41+k to -0x180;
//     the lift (collider 0x15, mesh 0x44) then converges to the height of the
//     highest set bit (41dbdc). Bit 8 is local lift motion state, not shared.
// [1] 52f770 bits 1/2/4: completed button-sequence rounds; token 3 at 7
//     (41d4cc, deduplicated by token sync). Marker meshes are derived per tick.
// [2] 52f804 rescued-friend stage (see RewardStages below).
static void RewardAdopt(int from,int to);
static const ScriptVarDesc level4Vars[]={{0x52f784,ScriptBits,7},{0x52f770,ScriptBits,7},{0x52f804,ScriptMax}};
static void Level4Adopt(unsigned index,int from,int to){
 if(index==0)for(int k=0;k<3;k++)if((to&(1<<k))&&!(from&(1<<k))){scriptColliderRotate(0x16+k,-0x180,0,0);mechRotate(0x41+k,-0x180,0,0);}
 if(index==2)RewardAdopt(from,to);
}
// ---- Rescued-friend reward stage, common to levels 4,5,7,8,10,11,13,14 -----
// Native pattern (e.g. 41f80c..41f8b4): 0 -> 1 when the LOCAL player reaches the
// friend (dialogue) -> 2 when the dialogue ends: the friend's command script
// starts at [base]+offset, timer +0x78 = 0, 830d44 = 0xb4, descriptor+0xf = 0
// -> waits until the friend actor leaves (model 0) -> 3..0x78 -> token 4 (shared
// and deduplicated by token sync) -> 0xc8. Max-merged; crossing stage 2 replays
// the native start once so the other player sees the friend leave too.
struct RewardStage {unsigned level;uintptr_t stage;unsigned slot;uintptr_t commandBase;unsigned offset;bool descriptor;};
static const RewardStage rewardStages[]={
 {4,0x52f804,24,0x4e030c,0x18,false},{5,0x52f8e8,3,0x4e0334,0x1c,true},{7,0x52fa14,0,0x4e0324,0x1c,true},{8,0x52fae0,40,0x4e0350,0x1c,true},
 {10,0x52fc4c,8,0x4e0330,0x1c,true},{11,0x52fd40,11,0x4e0348,0x1c,true},{13,0x52ff1c,32,0x4e0368,0x1c,true},{14,0x52ffac,46,0x4e0360,0x1c,true},
};
static void RewardAdopt(int from,int to){
 if(from>=2||to<2)return;
 for(unsigned i=0;i<sizeof(rewardStages)/sizeof(rewardStages[0]);i++){
  const RewardStage& r=rewardStages[i];if(r.level!=cachedLevel)continue;
  uintptr_t actor=0x52c840+r.slot*0x9c;if(M<short>(actor+0x14)==0)return;  // already gone locally
  M<short>(actor+0x78)=0;M<uintptr_t>(actor+0x80)=M<uintptr_t>(r.commandBase)+r.offset;M<int>(0x830d44)=0xb4;
  if(r.descriptor){uintptr_t descriptor=M<uintptr_t>(actor+0x98);if(Readable(descriptor,0x10))M<unsigned char>(descriptor+0xf)=0;}
  if(cachedLevel==8){M<short>(actor+0x8a)=0x190;M<int>(0x52fb9c)=0x258;}   // 424305/42430e
  return;
 }
}
static void RewardOnlyAdopt(unsigned index,int from,int to){if(index==0)RewardAdopt(from,to);}
// Level 5 [1]: button collider 4 (41eb39..41ebd9) sets 52f938 bit 1 once, raises collider 4 by 0xe10
// and scales meshes 0x10/0x11; it starts the gear machine (meshes 0xa/0xb/0x14, animated locally;
// bit 2 is its local oscillation and is never shared). Replayed once on adoption.
static const ScriptVarDesc level5Vars[]={{0x52f8e8,ScriptMax},{0x52f938,ScriptBits,1}};
static void Level5Adopt(unsigned index,int from,int to){
 if(index==0)RewardAdopt(from,to);
 if(index==1&&(to&1)&&!(from&1)){
  uintptr_t geometry=ColliderGeometry(4);
  if(geometry)colliderPlace(4,M<int>(geometry),M<int>(geometry+4)+0xe10,M<int>(geometry+8));   // 41eb94: +0xe10 on y
  mechScale(0x10,0x1000,0x960,0x1000);mechScale(0x11,0x1000,0x960,0x1000);
 }
}
// Level 7 [1]: NPC door target 52fa10 (0 -> 0x4000 after the slot-13 dialogue, 422360); the door
// 52fa08, collider 0x12 enable and mesh 0x1f converge natively from it every tick.
static const ScriptVarDesc level7Vars[]={{0x52fa14,ScriptMax},{0x52fa10,ScriptMax}};
static const ScriptVarDesc level8Vars[]={{0x52fae0,ScriptMax}};
// Level 10 [1]: targets 48b640(4)/(5) set 52fc14 bits 1/2 and turn collider 4 (0x180,0,0)/mesh 0x11
// and collider 5 (0,0,0x180)/mesh 0x10 once (427a74..427b09); replayed once on adoption.
// Bit 4: the floor-switch puzzle is solved (427047, checked only during the LOCAL player's landing):
// collider 0x13 off, lamp meshes 0x46/0x48/0x4e shown and 0x47/0x49/0x4f hidden, the path movers
// start (4272ac). Replayed once. Bit 8 is the local lamp blink and is not shared.
static const ScriptVarDesc level10Vars[]={{0x52fc4c,ScriptMax},{0x52fc14,ScriptBits,7}};
static void Level10Adopt(unsigned index,int from,int to){
 if(index==0)RewardAdopt(from,to);
 if(index==1){
  if((to&1)&&!(from&1)){scriptColliderRotate(4,0x180,0,0);mechRotate(0x11,0x180,0,0);}
  if((to&2)&&!(from&2)){scriptColliderRotate(5,0,0,0x180);mechRotate(0x10,0,0,0x180);}
  if((to&4)&&!(from&4)){mechDisable(0x13);for(int mesh=0x46;mesh<0x50;mesh++)if(mesh<0x4a||mesh>0x4d){int v=mesh&1?0:0x1000;mechScale(mesh,v,v,v);}}
 }
}
// Level 11 [1]/[2]: bridges 52fd54/52fd38 rise from 0 when guard actors in slots 10/9 are down
// (42ab8f/42aba7); at 1 the native tick turns collider 0x17 (0,0x3ae,0) / 0x16 (0,-0x424,0) once
// (42a6da/42a775); meshes follow per tick. Max-merged; the one-shot turn is replayed once.
// [3..8] six pour switches 52fd58..52fd6c (429910): 0 -> 0x3c once when a player lands on collider
// c; the button mesh a is pressed (y 0x800), collider c moves +0xe10 in y, mesh h is hidden and
// the local cutscene plays. The timer then runs 0x3c..1 and ends at -1 (42a85d..42a896): the pour
// 429800(4*i) disables turret i (52fd48 bit i). Countdown-merged (a Max merge pulled finished
// switches back to 0 and made them pressable again); the press and the pour are replayed once.
static const ScriptVarDesc level11Vars[]={{0x52fd40,ScriptMax},{0x52fd54,ScriptMax},{0x52fd38,ScriptMax},
 {0x52fd58,ScriptCountdown},{0x52fd5c,ScriptCountdown},{0x52fd60,ScriptCountdown},{0x52fd64,ScriptCountdown},{0x52fd68,ScriptCountdown},{0x52fd6c,ScriptCountdown}};
typedef void(__cdecl *PourFn)(int);
static PourFn level11Pour=(PourFn)0x429800;
static const unsigned char level11Pours[6][3]={{0x02,0x0b,0x30},{0x11,0x38,0x30},{0x12,0x39,0x53},{0x05,0x05,0x30},{0x0b,0x07,0x30},{0x0a,0x06,0x30}};   // 4f3fb4: collider, button mesh, hidden mesh
static void Level11Adopt(unsigned index,int from,int to){
 if(index==0)RewardAdopt(from,to);
 if(index==1&&from<=0&&to>1)scriptColliderRotate(0x17,0,0x3ae,0);
 if(index==2&&from<=0&&to>1)scriptColliderRotate(0x16,0,-0x424,0);
 if(index>=3&&index<=8&&from==0&&to!=0){const unsigned char* p=level11Pours[index-3];
  mechScale(p[1],0x1000,0x800,0x1000);uintptr_t geometry=ColliderGeometry(p[0]);
  if(geometry)colliderPlace(p[0],M<int>(geometry),M<int>(geometry+4)+0xe10,M<int>(geometry+8));
  mechScale(p[2],0,0,0);}
 if(index>=3&&index<=8&&from!=-1&&to==-1){int i=(int)index-3;level11Pour(4*i);M<int>(0x52fd48)|=1<<i;}
}
static const ScriptVarDesc level13Vars[]={{0x52ff1c,ScriptMax}};
static const ScriptVarDesc level14Vars[]={{0x52ffac,ScriptMax}};
// ---- Level 9 boss (tick 424490, init 424390) ------------------------------
// Slot 0 boss; slots 1..6 orbiting satellites (positions derived each tick from
// orbit globals 52fc0c/52fc08/52fbec around the boss); slots 7..11 minions,
// released two at a time. Phase 52fc04: 0 -> 1 when the LOCAL player z >
// -0x1bb58 (424ae7, cutscene) -> 2 when it ends (424c16) -> 3 when 52fbc0 runs
// out with 52fbd4 == 7 (424a62) -> 4 defeat when health <= 10 (4249de, token
// 0x80) -> 5 boss removal 405d20 (42511b) -> level complete (42514d).
// A boss hit is health != 52fbb8 (424870): while 52fbd4 < 6 health is restored
// to 0x1a-52fbd4; the next minion pair comes from table 4f2f58 via 52fbf4
// (+2, 4248d1) into 52fbc8/52fbc4. During the OWNER's cutscene satellite
// #52fbd4 opens (405cf0 anim 1/0x17, 424d17), the pair is spawned (406cd0,
// 424d32/424d4d) and the satellite is removed (405d20, 424fcb). 52fbd4 (starts
// at 1) increments when both minions have health 0 (42505e).
// Observers: the owner's pair/count advance replays spawn, opening and removal
// once; while they observe, 52fbc0 is held at 0 around native passes so their
// tick never counts a pair as dead (minions are hidden until the owner spawns
// them) and never runs the release path.
typedef void(__cdecl *ScriptSpawnFn)(uintptr_t,int);
typedef void(__cdecl *ScriptAnimationFn)(uintptr_t,int,int);
static ScriptSpawnFn scriptSpawnActor=(ScriptSpawnFn)0x406cd0;
static ScriptAnimationFn scriptActorAnimation=(ScriptAnimationFn)0x405cf0;
static const int level9Pairs[13]={-1,8,8,7,7,8,9,10,7,11,8,11,7};   // 4f2f58
static const ScriptActorDesc level9Actors[]={{0,0,0x10c},{7,0,0x10c,0x52fbc8,11},{7,0,0x10c,0x52fbc4,11}};
static const ScriptVarDesc level9Vars[]={{0x52fc04,ScriptFollow},{0x52fbd4,ScriptFollow},{0x52fbf4,ScriptFollow},
 {0x52fbc8,ScriptOwned},{0x52fbc4,ScriptOwned},{0x52fbb8,ScriptOwned},{0x52fbc0,ScriptOwned},{0x52fbf0,ScriptOwned},
 {0x52fc0c,ScriptOwned},{0x52fc08,ScriptOwned},{0x52fbec,ScriptOwned},{0x52fbcc,ScriptOwned}};
static bool Level9Active(){return M<int>(0x52fc04)>=1;}
static bool Level9Retired(){return M<int>(0x52fc04)>=4;}
static unsigned Level9Damage(unsigned i,unsigned pending){
 if(i==0){
  short& health=M<short>(0x52c8be);
  if(M<int>(0x52fc04)>=4||health<=10)return pending;   // defeated: requests are moot
  if(health!=M<int>(0x52fbb8))return 0;                // native hit already pending
  health=(short)max(1,(int)health-(int)min(pending,100u));return pending;
 }
 if(scriptSlot[i]<0)return pending;                    // pair already replaced
 short& health=M<short>(ScriptActorAddress(0,i)+0x7e);
 if(health<=0)return pending;
 health=(short)max(0,(int)health-(int)min(pending,100u));return pending;
}
static void Level9Spawn(int slot){
 if(slot<7||slot>11)return;uintptr_t actor=0x52c840+slot*0x9c;if(M<short>(actor+0x14)<=0)return;
 scriptSpawnActor(actor,0);   // 424d32: same native respawn, then the same field writes
 M<short>(actor+0x42)=0xaf0;M<short>(actor+0x2c)=0;M<short>(actor+0x2e)=0;M<short>(actor+0x30)=0;M<short>(actor+0x32)=(short)-0x8000;
}
static void Level9Adopt(unsigned index,int from,int to){
 if(index==1)for(int k=max(from,1);k<to&&k<=6;k++){   // satellites removed in the owner's cutscene
  uintptr_t actor=0x52c840+k*0x9c;if(M<short>(actor+0x14)>0)remoteActorDeath(actor,1);
 }
 if(index==2&&to>from&&to>=2&&to<=12){
  int a=level9Pairs[to-1],b=level9Pairs[to];Level9Spawn(a);if(b!=a)Level9Spawn(b);
  int satellite=M<int>(0x52fbd4);
  if(satellite>=1&&satellite<=6){uintptr_t actor=0x52c840+satellite*0x9c;
   if(M<short>(actor+0x14)>0&&M<short>(actor+0x12)==0)scriptActorAnimation(actor,1,0x17);}
 }
}
static int level9HeldTimer;
static void Level9Guard(bool enter){
 if(enter){level9HeldTimer=M<int>(0x52fbc0);M<int>(0x52fbc0)=0;}
 else if(!M<int>(0x52fbc0))M<int>(0x52fbc0)=level9HeldTimer;
}
// ---- Level 15 boss (tick 42fc50, init 42faa0) -----------------------------
// Three boss actors, slots 0/1/2, each with its own native callback (42f310,
// 42f530, 42f7b0; registered by the spawn switch at 40700d/406faa/407044).
// Fight flags 530008/530004/53000c: 0 -> 2 when the LOCAL intro (52fff8) ends
// (4301e0, command scripts 4e0360/4e0348/4e0368 +0x1c) -> 3 when that actor's
// health < 10 while its flag is 2 (callback: defeat command +0x68, 530064++,
// 52fff4 = last actor; sites 42f4fe/42f78d/42fa59). A hit is health != the
// expected health 530028/530030/53002c (hurt timer 530018/530020/53001c).
// 530064 == 3 -> 4 (tick 4305bf, cutscene on 52fff4) -> 5 (430662, token 0x80).
// Observers hold flag 2 at 1 around native passes, so a local, unconfirmed hit
// can never run a callback's defeat branch; the owner applies their requests.
static const ScriptActorDesc level15Actors[]={{0,0x4e0360,0x10c},{1,0x4e0348,0x10c},{2,0x4e0368,0x10c}};
static const ScriptVarDesc level15Vars[]={{0x530064,ScriptFollow},{0x530008,ScriptFollow},{0x530004,ScriptFollow},{0x53000c,ScriptFollow},
 {0x530028,ScriptOwned},{0x530030,ScriptOwned},{0x53002c,ScriptOwned},{0x530018,ScriptOwned},{0x530020,ScriptOwned},{0x53001c,ScriptOwned},
 {0x52fff4,ScriptOwned}};
static const uintptr_t level15Flags[3]={0x530008,0x530004,0x53000c},level15Expected[3]={0x530028,0x530030,0x53002c};
static bool Level15Active(){for(unsigned k=0;k<3;k++)if(M<int>(level15Flags[k])>=2)return true;return false;}
static bool Level15Retired(){return M<int>(0x530064)>=3;}
static unsigned Level15Damage(unsigned i,unsigned pending){
 if(i>2)return pending;
 short& health=M<short>(0x52c840+i*0x9c+0x7e);
 if(M<int>(level15Flags[i])>=3||health<10)return pending;   // defeated: requests are moot
 if(M<int>(level15Flags[i])!=2||health!=M<int>(level15Expected[i]))return 0;  // before the fight / native hit pending
 health=(short)max(1,(int)health-(int)min(pending,100u));return pending;
}
static unsigned level15Held;
static void Level15Guard(bool enter){
 if(enter){level15Held=0;for(unsigned k=0;k<3;k++)if(M<int>(level15Flags[k])==2){M<int>(level15Flags[k])=1;level15Held|=1u<<k;}}
 else{for(unsigned k=0;k<3;k++)if((level15Held>>k&1)&&M<int>(level15Flags[k])==1)M<int>(level15Flags[k])=2;level15Held=0;}
}
static const ScriptLevelDesc scriptLevels[]={
 {3,level3Actors,1,level3Vars,7,Level3Active,Level3Retired,Level3Damage,Level3Adopt,true},
 {4,0,0,level4Vars,3,0,0,0,Level4Adopt,false},
 {5,0,0,level5Vars,2,0,0,0,Level5Adopt,false},
 {6,level6Actors,1,level6Vars,7,Level6Active,Level6Retired,Level6Damage,0,false},
 {7,0,0,level7Vars,2,0,0,0,RewardOnlyAdopt,false},
 {8,0,0,level8Vars,1,0,0,0,RewardOnlyAdopt,false},
 {9,level9Actors,3,level9Vars,12,Level9Active,Level9Retired,Level9Damage,Level9Adopt,false,Level9Guard,0x7eull},
 {10,0,0,level10Vars,2,0,0,0,Level10Adopt,false},
 {11,0,0,level11Vars,9,0,0,0,Level11Adopt,false},
 {12,level12Actors,1,level12Vars,4,Level12Active,Level12Retired,Level12Damage,0,false},
 {13,0,0,level13Vars,1,0,0,0,RewardOnlyAdopt,false},
 {14,0,0,level14Vars,1,0,0,0,RewardOnlyAdopt,false},
 {15,level15Actors,3,level15Vars,11,Level15Active,Level15Retired,Level15Damage,0,false,Level15Guard,0},
};
static const ScriptLevelDesc* ScriptLevel(){
 for(unsigned i=0;i<sizeof(scriptLevels)/sizeof(scriptLevels[0]);i++)if(scriptLevels[i].level==cachedLevel)return &scriptLevels[i];
 return 0;
}
