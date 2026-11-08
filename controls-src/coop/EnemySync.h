// Host-owned ordinary enemy transforms and health. Script pointers stay local.
// NPCs, quest actors, crib targets and the existing Tin Robot adapter are excluded.
static bool BackyardBurrow(unsigned i){return cachedLevel==2&&i==4&&M<short>(0x52cab0+20)==13&&M<short>(0x52cab0+0x7e)==102;}
static bool BackyardMower(unsigned i){return cachedLevel==2&&i==0&&M<short>(0x52c840+20)==12&&M<short>(0x52c840+0x7e)==101;}
static short enemyModel[64],enemyLastHealth[64];
static uint32_t enemyDamage[64],enemyDamageApplied[64],enemyRevision[64];
static DWORD enemyArrival[64],enemyAwakeUntil[64];
static int enemyFrom[64][3],enemyDisplay[64][3];
static bool enemyReady[64],enemyExcluded[64];
static unsigned char enemyTargetOwner[64];
static void ResetEnemySync(){
 memset(enemyTargetOwner,0,sizeof(enemyTargetOwner));memset(enemyModel,0,sizeof(enemyModel));memset(enemyLastHealth,0xff,sizeof(enemyLastHealth));
 memset(enemyDamage,0,sizeof(enemyDamage));memset(enemyDamageApplied,0,sizeof(enemyDamageApplied));
 memset(enemyExcluded,0,sizeof(enemyExcluded));
 memset(enemyAwakeUntil,0,sizeof(enemyAwakeUntil));memset(enemyRevision,0,sizeof(enemyRevision));memset(enemyReady,0,sizeof(enemyReady));
 memset(localFrame.enemies,0,sizeof(localFrame.enemies));memset(localFrame.enemyAnimations,0,sizeof(localFrame.enemyAnimations));
}
// A new life of the slot (host respawn): start pose interpolation and damage observation over.
static void ResetEnemySlot(unsigned i){if(i<64){enemyReady[i]=false;enemyLastHealth[i]=-1;enemyAwakeUntil[i]=0;}}
static bool OrdinaryEnemy(unsigned i){
 uintptr_t actor=0x52c840+i*0x9c;
 if((cachedLevel==1||cachedLevel==2)&&i==29)return false;
 if(ScriptActorSlot(i))return false;
 if(cachedLevel==1&&(i==0||i==2||(i==8&&M<short>(actor+0x14)==5)))return false;
 short model=M<short>(actor+0x14),health=M<short>(actor+0x7e);
 if(model>0&&health>=80&&!enemyModel[i]&&!enemyExcluded[i]){enemyExcluded[i]=true;SyncLog("actor slot %u model %d health %d is a special actor (not ordinary enemy sync)",i,model,health);}
 if(enemyExcluded[i])return false;
 if(!enemyModel[i]&&model>0&&model<=255&&health>0&&health<80)enemyModel[i]=model;
 return enemyModel[i]>0&&model==enemyModel[i]&&health>=0&&health<80;
}
static bool EnemyDead(unsigned i){return CoopBit(localFrame.deadActors,i)||CoopBit(remoteFrame.deadActors,i);}
// Resolve animation limits from this level's native model asset. The renderer
// uses scene+274 -> model table, model+28 -> clips and model+2c -> clip count.
static int NativeEnemyFrame(short model,short animation,int frame){
 if(animation==-1)return 0;
 uintptr_t scene=M<uintptr_t>(0xb62410);if(!Readable(scene,0x27c))return -1;
 unsigned count=M<unsigned>(scene+0x278);uintptr_t models=M<uintptr_t>(scene+0x274);
 if(model<=0||(unsigned)model>=count||count>1024||!Readable(models,count*4))return -1;
 uintptr_t asset=M<uintptr_t>(models+model*4);if(!Readable(asset,0x30))return -1;
 int clips=M<int>(asset+0x2c);uintptr_t list=M<uintptr_t>(asset+0x28);
 if(animation<0||clips<1||clips>100||animation>=clips||!Readable(list,clips*4))return -1;
 uintptr_t clip=M<uintptr_t>(list+animation*4);if(!Readable(clip,6))return -1;
 int frames=M<unsigned short>(clip+4)&32767;if(frames<2)return -1;
 return max(0,min(frame,(frames-1)<<16));
}
static void CaptureEnemyAnimation(unsigned i,uintptr_t actor){
 CoopAnimation& a=localFrame.enemyAnimations[i];a.animation=M<short>(actor+18);a.secondary=M<short>(actor+22);
 a.frame=NativeEnemyFrame(enemyModel[i],a.animation,M<int>(actor+24));
 a.secondaryFrame=NativeEnemyFrame(enemyModel[i],a.secondary,M<int>(actor+28));
 if(a.frame<0){a.animation=-1;a.frame=0;}if(a.secondaryFrame<0){a.secondary=-1;a.secondaryFrame=0;}
}
static CoopAnimation enemySavedAnimation[64];static bool enemyPresenting[64];
static void BeginEnemyPresentation(){
 memset(enemyPresenting,0,sizeof(enemyPresenting));if(!SameLevel()||localOwner!=1)return;
 for(unsigned i=0;i<64;i++){
  if(!OrdinaryEnemy(i)||EnemyDead(i)||!remoteFrame.enemies[i].revision||remoteFrame.enemies[i].model!=enemyModel[i])continue;
  const CoopAnimation& a=remoteFrame.enemyAnimations[i];
  int frame=NativeEnemyFrame(enemyModel[i],a.animation,a.frame),secondary=NativeEnemyFrame(enemyModel[i],a.secondary,a.secondaryFrame);
  if(frame<0||secondary<0)continue;
  uintptr_t actor=0x52c840+i*0x9c;CoopAnimation& saved=enemySavedAnimation[i];
  saved.animation=M<short>(actor+18);saved.secondary=M<short>(actor+22);saved.frame=M<int>(actor+24);saved.secondaryFrame=M<int>(actor+28);
  M<short>(actor+18)=a.animation;M<short>(actor+22)=a.secondary;M<int>(actor+24)=frame;M<int>(actor+28)=secondary;enemyPresenting[i]=true;
 }
}
static void EndEnemyPresentation(){for(unsigned i=0;i<64;i++)if(enemyPresenting[i]){
 uintptr_t actor=0x52c840+i*0x9c;const CoopAnimation& a=enemySavedAnimation[i];
 M<short>(actor+18)=a.animation;M<short>(actor+22)=a.secondary;M<int>(actor+24)=a.frame;M<int>(actor+28)=a.secondaryFrame;enemyPresenting[i]=false;
}}
static void ObserveEnemyDamage(unsigned i){
 int health=M<short>(0x52c840+i*0x9c+0x7e);
 if(localOwner==1&&enemyLastHealth[i]>0&&health>=0&&health<enemyLastHealth[i])
  enemyDamage[i]=min(1000000u,enemyDamage[i]+(unsigned)(enemyLastHealth[i]-health));
 enemyLastHealth[i]=(short)health;
}
static void ApplyEnemy(unsigned i,DWORD now){
 const CoopEnemy& state=remoteFrame.enemies[i];uintptr_t actor=0x52c840+i*0x9c;
 if(!state.revision||state.model!=enemyModel[i]||EnemyDead(i)||state.health==0)return;
 if(enemyReady[i]&&(int32_t)(state.revision-enemyRevision[i])<0)return;
 if(!enemyReady[i]||state.revision!=enemyRevision[i]){
  bool snap=!enemyReady[i];
  for(int axis=0;axis<3;axis++)if(abs((int64_t)state.position[axis]-enemyDisplay[i][axis])>0x20000)snap=true;
  for(int axis=0;axis<3;axis++)enemyFrom[i][axis]=snap?state.position[axis]:enemyDisplay[i][axis];
  enemyRevision[i]=state.revision;enemyArrival[i]=now;enemyReady[i]=true;
 }
 unsigned elapsed=now-enemyArrival[i];
 for(int axis=0;axis<3;axis++){enemyDisplay[i][axis]=CoopLerp(enemyFrom[i][axis],state.position[axis],elapsed,33);M<int>(actor+axis*4)=enemyDisplay[i][axis];}
 M<short>(actor+0xc)=state.pitch;M<short>(actor+0xe)=state.yaw;M<short>(actor+0x10)=state.roll;
 M<short>(actor+0x7e)=state.health;
 // Keep native collision/render membership local; share only facing/motion state.
 M<unsigned short>(actor+0x40)=(M<unsigned short>(actor+0x40)&~0x10c)|(state.flags&0x10c);
 enemyLastHealth[i]=state.health;
}
static void SyncEnemies(){
 if(!SameLevel()){memset(enemyReady,0,sizeof(enemyReady));return;}
 DWORD now=GetTickCount();
 for(unsigned i=0;i<64;i++){
  if(!OrdinaryEnemy(i))continue;
  uintptr_t actor=0x52c840+i*0x9c;ObserveEnemyDamage(i);
  CoopEnemy& state=localFrame.enemies[i];
  if(localOwner==0){
   CaptureEnemyAnimation(i,actor);
   const CoopEnemy& peer=remoteFrame.enemies[i];
   if(peer.revision&&peer.model==enemyModel[i]&&peer.damage>enemyDamageApplied[i]){
    unsigned delta=peer.damage-enemyDamageApplied[i];enemyDamageApplied[i]=peer.damage;
    if(!EnemyDead(i))M<short>(actor+0x7e)=(short)max(0,(int)M<short>(actor+0x7e)-(int)min(delta,79u));
   }
   state.revision=localFrame.sequence+1;if(!state.revision)state.revision=1;
   memcpy(state.position,&M<unsigned char>(actor),12);
   state.pitch=M<short>(actor+0xc);state.yaw=M<short>(actor+0xe);state.roll=M<short>(actor+0x10);
   state.model=enemyModel[i];state.health=M<short>(actor+0x7e);state.flags=M<unsigned short>(actor+0x40)&0x10c;
  }else{
   state.revision=1;state.model=enemyModel[i];state.damage=enemyDamage[i];
   ApplyEnemy(i,now);
  }
 }
}
// The native actor pass culls against the host. Temporarily include enemies
// near the joining player too, without extending player-facing draw distance.
static bool enemyForced[64],enemyFlagsSaved[64];static unsigned short enemySavedActivation[64];
static void ActivatePeerEnemies(){
 memset(enemyForced,0,sizeof(enemyForced));memset(enemyFlagsSaved,0,sizeof(enemyFlagsSaved));if(!SameLevel()||localOwner!=0)return;
 for(unsigned i=0;i<64;i++)if((OrdinaryEnemy(i)&&!EnemyDead(i))||BackyardBurrow(i)||BackyardMower(i)){
  uintptr_t actor=0x52c840+i*0x9c;int64_t distance[2]={0,0};
  const int players[2][3]={{M<int>(0x52f300),M<int>(0x52f304),M<int>(0x52f308)},
   {remoteFrame.pose.x,remoteFrame.pose.y,remoteFrame.pose.z}};
  for(unsigned player=0;player<2;player++)for(unsigned axis=0;axis<3;axis++){
   int64_t d=((int64_t)M<int>(actor+4*axis)-players[player][axis])/256;distance[player]+=d*d;
  }
  // Render range is already configured to 2x stock. Keep every visible enemy's
  // native jump/gravity/script ticking near EITHER player, including the host.
  int radius=256+max(400,((max(0,(int)M<short>(actor+0x42))*362)>>9))+(M<unsigned short>(actor+0x3e)>>3);
  DWORD now=GetTickCount();
  if(distance[0]<=(int64_t)radius*radius||distance[1]<=(int64_t)radius*radius)enemyAwakeUntil[i]=now+2000;
  // Finish in-flight movement instead of freezing at the activation boundary.
  if(enemyAwakeUntil[i]&&(int32_t)(enemyAwakeUntil[i]-now)>0)enemyForced[i]=true;
 }
}
// Reapply activation AFTER native host-camera room/frustum culling. Never
// inflate range: native range*range*16 overflows when range is set to 32767.
static void RepairEnemyCandidates(){
 if(!SameLevel()||localOwner!=0)return;
 unsigned count=0;while(count<64&&M<uintptr_t>(0x52f1d0+count*4))++count;
 for(unsigned i=0;i<64;i++)if(enemyForced[i]&&((OrdinaryEnemy(i)&&!EnemyDead(i))||BackyardBurrow(i)||BackyardMower(i))){
  uintptr_t actor=0x52c840+i*0x9c;bool found=false;
  for(unsigned j=0;j<count;j++)if(M<uintptr_t>(0x52f1d0+j*4)==actor){found=true;break;}
  if(!found){if(count>=64)continue;M<uintptr_t>(0x52f1d0+count++*4)=actor;M<uintptr_t>(0x52f1d0+count*4)=0;}
  if(!enemyFlagsSaved[i])enemySavedActivation[i]=M<unsigned short>(actor+0x40)&0x2001;enemyFlagsSaved[i]=true;
  M<unsigned short>(actor+0x40)=(M<unsigned short>(actor+0x40)&~0x2000)|1;
 }
}
typedef void (__cdecl *EnemyCullFn)(void*);
static EnemyCullFn nativeEnemyCull=(EnemyCullFn)0x447bd0;
static void __cdecl SharedEnemyCull(void* camera){nativeEnemyCull(camera);RepairEnemyCandidates();}
static void RestoreEnemyRanges(){for(unsigned i=0;i<64;i++)if(enemyFlagsSaved[i]){
 uintptr_t actor=0x52c840+i*0x9c;
 M<unsigned short>(actor+0x40)=(M<unsigned short>(actor+0x40)&~0x2001)|enemySavedActivation[i];enemyFlagsSaved[i]=false;
}}
static int enemyTargetPosition[3];
static unsigned ChooseEnemyTarget(unsigned previous,int64_t hostDistance,int64_t peerDistance,bool hostAlive,bool peerAlive){
 if(!peerAlive)return 0;if(!hostAlive)return 1;
 // Keep the previous target unless the other player is appreciably closer.
 return previous==1?(hostDistance*4<peerDistance*3?0:1):(peerDistance*4<hostDistance*3?1:0);
}
typedef void (__cdecl *EnemyThinkFn)(uintptr_t);
static EnemyThinkFn nativeEnemyThink=(EnemyThinkFn)0x4076f0;
static void __cdecl SharedEnemyThink(uintptr_t actor){
 if(SameLevel()&&localOwner==1&&actor==0x52c840&&BackyardMower(0)&&remoteFrame.mower.revision)return;
 if(SameLevel()&&localOwner==1&&actor==0x52cab0&&BackyardBurrow(4)&&remoteFrame.burrow.revision)return;
 for(unsigned axis=0;axis<3;axis++)enemyTargetPosition[axis]=M<int>(0x52f300+axis*4);
 if(SameLevel()&&localOwner==0&&actor>=0x52c840&&actor<0x52c840+64*0x9c&&(actor-0x52c840)%0x9c==0){
  unsigned i=(unsigned)((actor-0x52c840)/0x9c);
  if((OrdinaryEnemy(i)&&!EnemyDead(i))||BackyardBurrow(i)||BackyardMower(i)){
   int peer[3]={remoteFrame.pose.x,remoteFrame.pose.y,remoteFrame.pose.z};int64_t distance[2]={0,0};
   for(unsigned axis=0;axis<3;axis++){int64_t a=((int64_t)M<int>(actor+axis*4)-enemyTargetPosition[axis])/256,b=((int64_t)M<int>(actor+axis*4)-peer[axis])/256;distance[0]+=a*a;distance[1]+=b*b;}
   enemyTargetOwner[i]=(unsigned char)ChooseEnemyTarget(enemyTargetOwner[i],distance[0],distance[1],M<short>(0x52f396)>0,remoteFrame.pose.health>0);
   if(enemyTargetOwner[i])memcpy(enemyTargetPosition,peer,sizeof(peer));
  }
 }
 diagnosticPhase="native enemy AI";diagnosticActor=actor;nativeEnemyThink(actor);diagnosticActor=0;diagnosticPhase="actor update";
}
struct EnemyTargetOperand {uintptr_t site;unsigned axis;};
static const EnemyTargetOperand enemyTargetOperands[]={
 {0x407a89,0},{0x407a92,2},{0x407af5,0},{0x407afd,2},{0x407c0b,2},{0x407c16,0},
 {0x407c68,0},{0x407c75,1},{0x407c7d,2},{0x407c8e,2},{0x407c96,0},{0x407cfd,0},
 {0x407d0a,2},{0x407d9a,2},{0x407da5,0},{0x407e0e,0},{0x407e1e,2},{0x407ee7,1}};
static bool EnemyTargetSignatures(){for(unsigned i=0;i<sizeof(enemyTargetOperands)/sizeof(enemyTargetOperands[0]);i++)
 if(M<uintptr_t>(enemyTargetOperands[i].site)!=0x52f300+4*enemyTargetOperands[i].axis)return false;return true;}
static bool InstallEnemyTargetOperands(){
 for(unsigned i=0;i<sizeof(enemyTargetOperands)/sizeof(enemyTargetOperands[0]);i++)
  if(!SetImport(enemyTargetOperands[i].site,(uintptr_t)&enemyTargetPosition[enemyTargetOperands[i].axis]))return false;
 FlushInstructionCache(GetCurrentProcess(),NULL,0);return true;
}

// Native Actors builds its render candidates before guest pose reconciliation.
// Reconsider missing live enemies using the corrected position and LOCAL camera.
// Preserve native draw radius, renderer capacity and local-Buzz-last ordering.
static unsigned IncludeVisibleEnemies(uintptr_t* list,unsigned count){
 if(!SameLevel()||count==0)return count;
 for(unsigned i=0;i<64&&count<64;i++){
  if((!OrdinaryEnemy(i)||EnemyDead(i))&&!BackyardMower(i)&&!BackyardBurrow(i))continue;
  uintptr_t actor=0x52c840+i*0x9c;
  if(M<short>(actor+0x7e)<=0)continue;
  bool found=false;for(unsigned j=0;j<count;j++)if(list[j]==actor){found=true;break;}
  if(found)continue;
  int range=M<short>(actor+0x42),radius=M<short>(actor+0x3e);
  if(range<=0||radius<0)continue;
  int limit=((range*362)>>9)+(radius>>3);int64_t distance=0;
  for(unsigned axis=0;axis<3;axis++){
   int64_t d=((int64_t)M<int>(0x52adc0+axis*4)-M<int>(actor+axis*4)-M<short>(actor+0x38+axis*2))>>8;
   distance+=d*d;
  }
  if(distance>=(int64_t)limit*limit)continue;
  list[count]=list[count-1];list[count-1]=actor;list[++count]=0;
 }
 return count;
}

// The native draw pass independently gates models on actor flag bit 0.
// Peer activation restores that same bit; list insertion alone is insufficient.
// Scope the override to drawing so it cannot change next tick's AI activation.
static bool enemyRenderOverride[64];
static void BeginEnemyRender(const uintptr_t* list){
 memset(enemyRenderOverride,0,sizeof(enemyRenderOverride));if(!SameLevel())return;
 for(unsigned j=0;j<64&&list[j];j++){
  uintptr_t actor=list[j];if(actor<0x52c840||actor>=0x52c840+64*0x9c||(actor-0x52c840)%0x9c)continue;
  unsigned i=(unsigned)((actor-0x52c840)/0x9c);
  if((!OrdinaryEnemy(i)||EnemyDead(i)||M<short>(actor+0x7e)<=0)&&!BackyardMower(i)&&!BackyardBurrow(i))continue;
  if(!(M<unsigned short>(actor+0x40)&1)){enemyRenderOverride[i]=true;M<unsigned short>(actor+0x40)|=1;}
 }
}
static void EndEnemyRender(){
 for(unsigned i=0;i<64;i++)if(enemyRenderOverride[i])M<unsigned short>(0x52c840+i*0x9c+0x40)&=~1;
 memset(enemyRenderOverride,0,sizeof(enemyRenderOverride));
}
