#define WIN32_LEAN_AND_MEAN
#define _CRT_SECURE_NO_WARNINGS
#include <windows.h>
#include <stdio.h>
#include <math.h>
#include <float.h>
#include "CoopFrame.h"

// Experimental two-PC adapter. Each PC simulates its own Buzz; only value
// snapshots cross the bridge. All game memory access happens on the game thread.
static HANDLE mapping,bridgeMutex;
static CoopShared* shared;
static CoopFrame localFrame,remoteFrame;
static DWORD helperTick;
static unsigned char remoteActor[0xa0];
static uintptr_t renderList[65];
static unsigned renderLevel;
static bool useRenderList;
static float nameView[16],nameProjection[16],nameCamera[3];
static bool nameCameraReady;
static CoopPose displayedPose;
struct PoseSample { CoopPose pose; DWORD time; uint32_t sequence; };
static PoseSample poseSamples[32];
static unsigned poseCount;
static bool poseReady;
static bool wasLinked;
static bool startConsumed,applyingDeath;
static int deathSource=-1,dropSource[64];
static uint32_t shotSequence,receivedShot;
static char peerName[25];
static unsigned localOwner;
static int lastPushed=-1;
static bool pushUpdated;
static uint32_t bossDamage[2],bossDamageApplied[2];
static int bossLastHealth=-1;
static char assetRoot[MAX_PATH];
static HKEY privateGameKey=(HKEY)0x32504354;
typedef LSTATUS (WINAPI *OpenKeyFn)(HKEY,LPCSTR,DWORD,REGSAM,PHKEY);
typedef LSTATUS (WINAPI *QueryKeyFn)(HKEY,LPCSTR,LPDWORD,LPDWORD,LPBYTE,LPDWORD);
typedef LSTATUS (WINAPI *CloseKeyFn)(HKEY);
static OpenKeyFn nativeOpenKey;
static QueryKeyFn nativeQueryKey;
static CloseKeyFn nativeCloseKey;
static LSTATUS WINAPI OpenGameKey(HKEY key,LPCSTR name,DWORD options,REGSAM access,PHKEY result){
 if(name&&(!_stricmp(name,"SOFTWARE\\TravellersTalesToyStory2")||!_stricmp(name,"TravellersTalesToyStory2"))&&result){*result=privateGameKey;return ERROR_SUCCESS;}
 if(key==HKEY_LOCAL_MACHINE&&name&&!_stricmp(name,"Software"))access=KEY_READ;
 return nativeOpenKey(key,name,options,access,result);
}
static LSTATUS WINAPI QueryGameKey(HKEY key,LPCSTR name,LPDWORD reserved,LPDWORD type,LPBYTE data,LPDWORD size){
 if(key!=privateGameKey)return nativeQueryKey(key,name,reserved,type,data,size);
 if(!name||!size)return ERROR_INVALID_PARAMETER;
 const char* suffix=!_stricmp(name,"path")?"data\\":!_stricmp(name,"cdpath")?"DiscData\\":NULL;
 if(!suffix)return ERROR_FILE_NOT_FOUND;
 char path[MAX_PATH];sprintf(path,"%s%s",assetRoot,suffix);
 DWORD needed=(DWORD)strlen(path)+1,capacity=*size;*size=needed;if(type)*type=REG_SZ;
 if(!data)return ERROR_SUCCESS;if(capacity<needed)return ERROR_MORE_DATA;
 memcpy(data,path,needed);return ERROR_SUCCESS;
}
static LSTATUS WINAPI CloseGameKey(HKEY key){return key==privateGameKey?ERROR_SUCCESS:nativeCloseKey(key);}
static bool SetImport(uintptr_t site,uintptr_t value){
 DWORD old;if(!VirtualProtect((void*)site,sizeof(uintptr_t),PAGE_READWRITE,&old))return false;
 *(uintptr_t*)site=value;DWORD ignored;VirtualProtect((void*)site,sizeof(uintptr_t),old,&ignored);return true;
}
static uintptr_t pickupTable;
static unsigned cachedLevel,pickupCount;
static unsigned char pickupType[512];
static int pickupCategoryCache[512],pickupY[512];
typedef int (__cdecl *CategoryFn)(int);
static CategoryFn categoryFn=(CategoryFn)0x44e520;
static short actorModel[64],actorHealth[64];
static bool actorSeen[64];
typedef void (__cdecl *ActorsFn)();
typedef void (__cdecl *ActorDeathFn)(uintptr_t,int);
static ActorsFn nativeActors=(ActorsFn)0x4086f0;
static ActorDeathFn remoteActorDeath=(ActorDeathFn)0x405d20;
static const unsigned animationFrames[35]={24,24,16,16,17,18,14,24,25,24,42,8,18,30,12,12,38,15,19,24,24,12,17,13,24,24,19,17,23,58,48,32,24,24,19};
#ifdef COOP_TEST
static unsigned testLevel;
static uintptr_t testBossCommands,testScene;
static unsigned char testData[0x40000],testCollision[0x1000],testSession[0x200];
#endif
template<class T> static T& M(uintptr_t va){
#ifdef COOP_TEST
 if(va>=0x830c00&&va<0x830e00)return *(T*)(testSession+va-0x830c00);
 if(va>=0x728720&&va<0x729720)return *(T*)(testCollision+va-0x728720);
 if(va==0xb62410)return *(T*)&testScene;
 if(va==0x4e02d8)return *(T*)&testBossCommands;
 if(va==0x88278c)return *(T*)&testLevel;
 if(va>=0x520000&&va<0x560000)return *(T*)(testData+va-0x520000);
#endif
 return *(T*)va;
}
#include "CrashDiagnostics.h"
static void Log(const char* line){
 FILE* f=fopen("CoopBridge.log","a");if(f){fprintf(f,"%lu %s\n",GetTickCount(),line);fclose(f);}
}
static bool Readable(uintptr_t address,size_t size){
#ifdef COOP_TEST
 if(address>=0x728720&&address+size<=0x729720)return true;
#endif
 if(!address||address+size<address)return false;
 MEMORY_BASIC_INFORMATION info;
 if(!VirtualQuery((void*)address,&info,sizeof(info)))return false;
 return info.State==MEM_COMMIT&&!(info.Protect&(PAGE_NOACCESS|PAGE_GUARD))&&
  address+size<=(uintptr_t)info.BaseAddress+info.RegionSize;
}
// Native removal mode 2 assumes the actor exists in the active list and its
// search has no terminator check (40615f). Remote pickups can be off-screen.
// Give absent actors a temporary one-entry list; preserve the real candidates.
static void SafeActorDeath(uintptr_t actor,int mode){
 const char* previousPhase=diagnosticPhase;uintptr_t previousActor=diagnosticActor;
 diagnosticPhase="remote actor removal";diagnosticActor=actor;
 bool present=false;
 for(unsigned i=0;i<64;i++){uintptr_t entry=M<uintptr_t>(0x52f1d0+i*4);if(!entry)break;if(entry==actor){present=true;break;}}
 if((mode&2)&&!present){
  uintptr_t saved[65];for(unsigned i=0;i<65;i++)saved[i]=M<uintptr_t>(0x52f1d0+i*4);
  M<uintptr_t>(0x52f1d0)=actor;M<uintptr_t>(0x52f1d4)=0;
  remoteActorDeath(actor,mode);
  for(unsigned i=0;i<65;i++)M<uintptr_t>(0x52f1d0+i*4)=saved[i];
 }else remoteActorDeath(actor,mode);
 diagnosticPhase=previousPhase;diagnosticActor=previousActor;
}
static bool Connected(){return shared&&helperTick&&GetTickCount()-helperTick<2000;}
static bool SameLevel(){
 return Connected()&&cachedLevel>=1&&cachedLevel<=15&&localFrame.level==cachedLevel&&localFrame.layout&&
  remoteFrame.level==cachedLevel&&remoteFrame.layout==localFrame.layout&&
  remoteFrame.pickupCount==pickupCount;
}
static void Exchange(){
 if(!shared)return;
 DWORD result=WaitForSingleObject(bridgeMutex,0);
 if(result!=WAIT_OBJECT_0&&result!=WAIT_ABANDONED)return;
 if(shared->magic==CoopMagic&&shared->version==CoopVersion){
  shared->local=localFrame;shared->gameTick=GetTickCount();
  if(CoopValidFrame(shared->remote)){remoteFrame=shared->remote;helperTick=shared->helperTick;}
 }
 ReleaseMutex(bridgeMutex);
}
static uint32_t Hash(uint32_t h,uint32_t n){return (h^n)*16777619u;}
static void ResetEnemySync();
static void ResetBossPresentation();
static void ResetPlatform();
static void ResetGarage();
static void SyncMower();
static void BeginBackyard();
static void EndBackyard();
static void ResetSession();
static void PrepareLevel(){
 unsigned level=M<unsigned>(0x88278c);uintptr_t table=M<uintptr_t>(0x559d6c);
 if(level==cachedLevel&&table==pickupTable)return;
 cachedLevel=level;pickupTable=table;pickupCount=0;
 memset(localFrame.coins,0,sizeof(localFrame.coins));
 memset(localFrame.collections,0,sizeof(localFrame.collections));
 memset(localFrame.deadActors,0,sizeof(localFrame.deadActors));
 memset(localFrame.drops,0,sizeof(localFrame.drops));memset(localFrame.shots,0,sizeof(localFrame.shots));
 memset(localFrame.objects,0,sizeof(localFrame.objects));lastPushed=-1;pushUpdated=false;localFrame.scriptFlags=0;localFrame.boss={};memset(bossDamage,0,sizeof(bossDamage));memset(bossDamageApplied,0,sizeof(bossDamageApplied));bossLastHealth=-1;memset(localFrame.actorCollections,0,sizeof(localFrame.actorCollections));
 for(unsigned i=0;i<64;i++)dropSource[i]=-1;
 receivedShot=0;
 memset(actorSeen,0,sizeof(actorSeen));ResetEnemySync();ResetBossPresentation();ResetPlatform();ResetGarage();localFrame.burrow={};localFrame.mower={};localFrame.duck={};localFrame.tree={};memset(localFrame.backyardPlatforms,0,sizeof(localFrame.backyardPlatforms));ResetSession();
 memset(pickupType,0,sizeof(pickupType));localFrame.layout=0;
 localFrame.pickupCount=0;localFrame.tokens=0;localFrame.spawnedTokens=0;
 if(level<1||level>15||!Readable(table,4))return;
 unsigned count=M<unsigned short>(table);
 if(count>512||!Readable(table,4+count*16)){Log("Unsupported pickup table: sharing disabled for this level.");return;}
 uint32_t hash=Hash(2166136261u,level);
 for(unsigned i=0;i<count;++i){
  uintptr_t p=table+4+i*16;
  hash=Hash(hash,M<unsigned>(p));hash=Hash(hash,M<unsigned>(p+4));
  hash=Hash(hash,M<unsigned>(p+8));hash=Hash(hash,M<unsigned char>(p+12));
  pickupType[i]=M<unsigned char>(p+12);
  pickupY[i]=M<int>(p+4);
  pickupCategoryCache[i]=pickupType[i]>=0x30?categoryFn(pickupType[i]):16;
 }
 pickupCount=count;localFrame.pickupCount=count;localFrame.layout=hash?hash:1;
 char details[128];sprintf(details,"Level %u: %u pickup slots, layout %08x (captured before collection).",level,count,localFrame.layout);Log(details);
}
// A generation per native slot also covers temporary pickups after respawn.
// Do not manufacture native pointers or teleport Buzz to replay a pickup.
static int PickupIndex(uintptr_t slot){
 if(slot<pickupTable+4||(slot-pickupTable-4)%16)return -1;
 unsigned i=(unsigned)((slot-pickupTable-4)/16);return i<pickupCount?(int)i:-1;
}
static bool SharedCategory(int category){return (category>=-1&&category<=10&&category!=4)||category==16;}
static int __cdecl RemotePickup(uintptr_t slot){
 int i=PickupIndex(slot);if(i<0||!SameLevel()||!SharedCategory(pickupCategoryCache[i]))return 0;
 if(remoteFrame.collections[i]<=localFrame.collections[i])return 0;
 // Native power-up handlers require the slot's real Y when renewing a timer.
 if(M<int>(slot+4)==INT32_MIN)M<int>(slot+4)=pickupY[i];
 return 1;
}
static void __cdecl RecordPickup(uintptr_t slot){
 int i=PickupIndex(slot);if(i<0||!SharedCategory(pickupCategoryCache[i]))return;
 unsigned next=localFrame.collections[i]+1;
 if(SameLevel()&&remoteFrame.collections[i]>next)next=remoteFrame.collections[i];
 localFrame.collections[i]=(unsigned short)(next>65535?65535:next);
}
static DWORD pickupCheckResume=0x4a11c6,pickupContact=0x4a121b,pickupAwardResume=0x4a125d;
__declspec(naked) static void SharedPickupCheck(){__asm{
 pushfd
 pushad
 push esi
 call RemotePickup
 add esp,4
 test eax,eax
 jz normal
 popad
 popfd
 mov edi,[esi]
 movzx ebx,byte ptr [esi+0dh]
 and ebx,7fh
 add ebx,0eh
 jmp dword ptr [pickupContact]
normal:
 popad
 popfd
 mov eax,[esi+4]
 cmp eax,80000000h
 jmp dword ptr [pickupCheckResume]
}}
__declspec(naked) static void SharedPickupAward(){__asm{
 pushfd
 pushad
 push esi
 call RecordPickup
 add esp,4
 popad
 popfd
 mov al,[esi+0ch]
 add esp,10h
 jmp dword ptr [pickupAwardResume]
}}
// All ordinary levels use five native token descriptors (mesh, active, pickup-Y,
// reveal timer). Replay the engine's reveal operation, never copy its pointers.
typedef void (__cdecl *RevealTokenFn)(int,int);
static RevealTokenFn revealToken=(RevealTokenFn)0x4a0db0;
static void SyncTokenSpawns(){
 if(!SameLevel()||cachedLevel%3==0)return;
 for(unsigned i=0;i<5;i++){
  uintptr_t descriptor=0x830ccc+i*16;
  if(M<int>(descriptor+4)==1)localFrame.spawnedTokens|=1u<<i;
 }
 localFrame.spawnedTokens|=remoteFrame.spawnedTokens;
 for(unsigned i=0;i<5;i++){
  if(!(localFrame.spawnedTokens&(1u<<i))||((localFrame.tokens|remoteFrame.tokens)&(1u<<i)))continue;
  uintptr_t descriptor=0x830ccc+i*16,slotY=M<uintptr_t>(descriptor+8);
  // A valid level-local pickup descriptor must exist before native replay.
  if(M<int>(descriptor+4)==0&&slotY>=pickupTable+8&&(slotY-pickupTable-8)%16==0&&
     (slotY-pickupTable-8)/16<pickupCount)revealToken(i,1);
 }
}
static void SyncTokens(){
 if(cachedLevel<1||cachedLevel>15)return;
 unsigned char& bits=M<unsigned char>(0x52f0d7+cachedLevel);
 if(SameLevel())bits|=(unsigned char)(remoteFrame.tokens&0x9f);
 localFrame.tokens=bits;
}
static void __cdecl Pickups(){
 PrepareLevel();
 SyncTokenSpawns();
 ((void(__cdecl*)())0x4a0f80)();
 SyncTokenSpawns();
 SyncTokens();
}
#include "BossSync.h"
#include "EnemySync.h"
#include "PlatformSync.h"
#include "GarageSync.h"
#include "BurrowSync.h"
#include "SessionSync.h"
static void ResetPlatform(){memset(backyardAppliedRevision,0,sizeof(backyardAppliedRevision));memset(backyardAppliedOwner,0,sizeof(backyardAppliedOwner));plankAdvanced=false;localFrame.platform={};localFrame.worldContext=0;}
// These ten native quest-collection paths all increment 52b7d8 and remove
// an actor with mode 2. Ordinary enemy deaths must not enter this channel.
static const uintptr_t questCollectionSites[]={0x416aa5,0x41867f,0x41bc12,0x41df61,0x420f64,0x422d0b,0x425a68,0x4286ea,0x42c1eb,0x42d66c};
static int ActorIndex(uintptr_t actor){
 return actor>=0x52c840&&actor<0x52ef40&&(actor-0x52c840)%0x9c==0?(int)((actor-0x52c840)/0x9c):-1;
}
static void __cdecl CollectQuestActor(uintptr_t actor,int mode){
 int index=ActorIndex(actor);
 if(SameLevel()&&index>=0&&index<64&&mode==2){
  // The native handler already awarded this contact before calling us.
  if(CoopBit(localFrame.actorCollections,index))--M<int>(0x52b7d8);
  else CoopSetBit(localFrame.actorCollections,index);
 }
 SafeActorDeath(actor,mode);
}
static void ApplyQuestActors(){
 if(!SameLevel())return;
 for(unsigned i=0;i<64;i++)if(CoopBit(remoteFrame.actorCollections,i)&&!CoopBit(localFrame.actorCollections,i)){
  uintptr_t actor=0x52c840+i*0x9c;
  if(M<short>(actor+0x14)<=0||M<short>(actor+0x7e)!=102)continue;
  CoopSetBit(localFrame.actorCollections,i);++M<int>(0x52b7d8);
  SafeActorDeath(actor,2);
 }
}
static void __cdecl Actors(){
 // Retire received enemy deaths before native actor-list construction. NPCs and
 // special/scripted actors (health >=100) are excluded from this early prototype.
 diagnosticPhase="actor update";
 PrepareLevel();ApplyQuestActors();BeginBossTick();SyncEnemies();SyncRc();SyncBurrow();SyncMower();
 for(unsigned i=0;i<64;++i){
  uintptr_t p=0x52c840+i*0x9c;short model=M<short>(p+0x14),health=M<short>(p+0x7e);
  if((BossReplicated()&&i==8)||((cachedLevel==1||cachedLevel==2)&&i==29))continue;
  if(!actorSeen[i]&&model>0&&health>0&&health<100){actorSeen[i]=true;actorModel[i]=model;actorHealth[i]=health;}
  if(actorSeen[i]&&SameLevel()&&CoopBit(remoteFrame.deadActors,i)&&!CoopBit(localFrame.deadActors,i)&&model==actorModel[i]){
   // Play the real native death sequence (including its coin drop), once.
   CoopSetBit(localFrame.deadActors,i);deathSource=(int)i;applyingDeath=true;
   M<short>(p+0x7a)=0;SafeActorDeath(p,1);M<short>(p+0x7a)=0;
   if(M<short>(p+0x7e)==0)M<short>(p+0x14)=0;
   applyingDeath=false;deathSource=-1;
  }
 }
 ActivatePeerEnemies();nativeActors();RestoreEnemyRanges();EndBossTick();SyncEnemies();SyncRc();SyncBurrow();SyncMower();
 for(unsigned i=0;i<64;++i)if(actorSeen[i]){
  uintptr_t p=0x52c840+i*0x9c;short model=M<short>(p+0x14),health=M<short>(p+0x7e);
  if((model==actorModel[i]||model==0)&&health==0&&actorHealth[i]>0)CoopSetBit(localFrame.deadActors,i);
 }
}
extern "C" __declspec(dllexport) int __cdecl CoopStartRequested(){
 if(!shared||startConsumed)return 0;startConsumed=true;return 1;
}
extern "C" __declspec(dllexport) void __cdecl CoopDeathEvent(uintptr_t actor,int mode,int begin){
 if(!shared||applyingDeath)return;
 if(!begin){deathSource=-1;return;}
 if(!(mode&1)||actor<0x52c840||actor>=0x52ef40||(actor-0x52c840)%0x9c)return;
 PrepareLevel();unsigned index=(unsigned)((actor-0x52c840)/0x9c);
 if(index>=64)return;
 deathSource=(int)index;CoopSetBit(localFrame.deadActors,index);
}
typedef uintptr_t (__cdecl *SpawnEffectFn)(int,int,int,int,int,int,int,int,int,int);
static uintptr_t __cdecl SpawnDeathDrop(int x,int y,int z,int vx,int vy,int vz,int size,int a,int b,int type){
 uintptr_t result=((SpawnEffectFn)0x40fae0)(x,y,z,vx,vy,vz,size,a,b,type);
 if(type==0x3d&&deathSource>=0&&result>=0x529e58&&result<0x52ad58&&(result-0x529e58)%0x3c==0)
  dropSource[(result-0x529e58)/0x3c]=deathSource;
 return result;
}
static int __cdecl CollectDrop(uintptr_t effect){
 if(!shared||effect<0x529e58||effect>=0x52ad58||(effect-0x529e58)%0x3c)return 1;
 int source=dropSource[(effect-0x529e58)/0x3c];if(source<0||source>=64)return 1;
 bool collected=CoopBit(localFrame.drops,source)||(SameLevel()&&CoopBit(remoteFrame.drops,source));
 if(!CoopBit(localFrame.drops,source)){
  CoopSetBit(localFrame.drops,source);
  // If the remote packet already includes this drop, its award is applied here.
  if(collected&&M<short>(0x52f39e)<99)++M<short>(0x52f39e);
 }
 return !collected;
}
static DWORD dropResume=0x4101f0,dropSkipAward=0x410212;
__declspec(naked) static void DropPickupHook(){__asm{
 pushfd
 pushad
 push edi
 call CollectDrop
 add esp,4
 test eax,eax
 jz already
 popad
 popfd
 mov ax,word ptr ds:[52f39eh]
 jmp dword ptr [dropResume]
already:
 popad
 popfd
 // Skip only the award; native cleanup and pickup effects still run.
 mov ax,word ptr ds:[52f39eh]
 or ebp,-1
 mov dword ptr [esp+10h],5
 jmp dword ptr [dropSkipAward]
}}
static void ApplyDrops(){
 if(!SameLevel())return;
 for(unsigned i=0;i<64;i++)if(CoopBit(remoteFrame.drops,i)&&!CoopBit(localFrame.drops,i)){
  CoopSetBit(localFrame.drops,i);if(M<short>(0x52f39e)<99)++M<short>(0x52f39e);M<short>(0x52c828)=180;
 }
 for(unsigned i=0;i<64;i++){
  uintptr_t p=0x529e58+i*0x3c;
  if(M<short>(p+0x24)<=0){dropSource[i]=-1;continue;}
  if(dropSource[i]>=0&&CoopBit(localFrame.drops,dropSource[i]))M<short>(p+0x24)=0;
 }
}
extern "C" __declspec(dllexport) void __cdecl CoopShotEvent(uintptr_t beam){
 if(!shared||beam<0x882970||beam>=0x882a20||(beam-0x882970)%0x2c)return;
 CoopShot& shot=localFrame.shots[shotSequence%8];shot.sequence=++shotSequence;
 memcpy(shot.start,(void*)beam,44);
}
static void ApplyShots(){
 if(!SameLevel())return;
 for(unsigned n=0;n<8;n++){
  const CoopShot* shot=NULL;
  for(unsigned i=0;i<8;i++)if((int32_t)(remoteFrame.shots[i].sequence-receivedShot)>0&&
      (!shot||(int32_t)(remoteFrame.shots[i].sequence-shot->sequence)<0))shot=&remoteFrame.shots[i];
  if(!shot)break;receivedShot=shot->sequence;
  uintptr_t beam=0;for(uintptr_t b=0x882970;b<0x882a20;b+=0x2c)if(!M<short>(b+0x24)){beam=b;break;}
  // Cosmetic replication only: never execute the remote ray's damage twice.
  if(beam)memcpy((void*)beam,shot->start,44);
 }
}
static void ReadObject(CoopObject& out,uintptr_t object){memcpy(out.position,&M<unsigned char>(object),36);}
typedef void (__cdecl *ObjectPositionFn)(int,int,int,int);
typedef void (__cdecl *PushableFn)(void*);
static ObjectPositionFn moveObjectMesh=(ObjectPositionFn)0x4cce30,moveObjectCollision=(ObjectPositionFn)0x488510;
static PushableFn nativePushables=(PushableFn)0x433700;
static bool ObjectMatches(const CoopObject& object,uintptr_t native){
 // A completed fall can use the final path node. Requiring node+1 here
 // discarded the book landing snapshot. Native movement owns path traversal.
 // The path, collision and render IDs are local resources, never remote pointers.
 if(object.path!=M<short>(native+0x22)||object.path<0||object.path>=1024)return false;
 uintptr_t path=M<uintptr_t>(0x559c70+object.path*4);
 if(!Readable(path,4))return false;
 unsigned nodes=M<unsigned short>(path);
 return nodes>=2&&object.node>=0&&(unsigned)object.node<nodes&&
        object.minimum>=0&&object.minimum<=object.node;
}
static void WriteObject(const CoopObject& value,uintptr_t object){
 memcpy(&M<unsigned char>(object),value.position,34); // Keep all three local resource IDs.
 int x=M<int>(object),y=M<int>(object+4),z=M<int>(object+8);
 int render=M<short>(object+0x26),collision=M<unsigned short>(object+0x24)&0x7fff;
 if(render>=0)moveObjectMesh(render,x>>5,y>>5,z>>5);
 moveObjectCollision(collision,x,y,z);
}
static void __cdecl PushableObjects(void* player){
 PrepareLevel();pushUpdated=true;int count=M<int>(0x53c65c);
 if(!SameLevel()||count<0||count>10){nativePushables(player);return;}
 int pushed=M<int>(0x53c648)-1;
 for(int i=0;i<count;i++){
  uintptr_t object=0x53c680+i*0x28;CoopObject& state=localFrame.objects[i];
  const CoopObject& remote=remoteFrame.objects[i];
  if(CoopObjectNewer(remote,state)&&ObjectMatches(remote,object))state=remote;
  if(!state.revision){state.revision=1;state.owner=0;ReadObject(state,object);}
  if(state.owner!=localOwner&&ObjectMatches(state,object))WriteObject(state,object);
  // Only a native local push contact may acquire control. Gravity stays with
  // the last pusher; spectators do not advance and re-broadcast a second fall.
  if(i==pushed&&pushed!=lastPushed){++state.revision;state.owner=localOwner;}
 }
 nativePushables(player);
 // Native contact is discovered inside this call, not before it. Claim the
 // first movement frame before a spectator restore can undo that movement.
 int contact=M<int>(0x53c648)-1;
 if(contact>=0&&contact<count&&contact!=lastPushed){
  CoopObject& state=localFrame.objects[contact];++state.revision;state.owner=localOwner;
 }
 for(int i=0;i<count;i++){
  uintptr_t object=0x53c680+i*0x28;CoopObject& state=localFrame.objects[i];
  if(state.owner==localOwner){CoopObject next=state;ReadObject(next,object);
   if(memcmp(next.position,state.position,36)){next.revision++;state=next;}
  }else if(ObjectMatches(state,object))WriteObject(state,object);
 }
 lastPushed=contact;
}
typedef void (__cdecl *LevelTickFn)(int);
static LevelTickFn nativeLevelTick=(LevelTickFn)0x4a1b00;
static void SyncCribTargets(){
 if(cachedLevel!=1)return;
 for(unsigned i=0;i<2;i++){
  uintptr_t actor=0x52c840+i*2*0x9c,descriptor=M<uintptr_t>(actor+0x98);
  if(!Readable(descriptor,0x1b)||M<short>(actor+0x14)<=0)continue;
  if(M<short>(actor+0x7e)!=80||M<unsigned char>(descriptor+0x1a)==0)localFrame.scriptFlags|=1u<<i;
  if(SameLevel()&&(remoteFrame.scriptFlags&(1u<<i)))localFrame.scriptFlags|=1u<<i;
  // Native room-one script clears target collision, animates both targets,
  // opens the crib collider and drops all three mesh sections after both hits.
  if((localFrame.scriptFlags&(1u<<i))&&M<unsigned char>(descriptor+0x1a)!=0&&M<short>(actor+0x7e)==80)
   M<short>(actor+0x7e)=79;
 }
}
#include "BackyardSync.h"
static void __cdecl SharedLevelTick(int level){
 diagnosticPhase="level script";
 PrepareLevel();SyncCribTargets();BeginBossTick();SyncEnemies();
 if(SameLevel()&&!pushUpdated){
  // Aiming bypasses the native walking call site. Keep autonomous falls
  // advancing without manufacturing a walking/pushing input while aiming.
  unsigned short input=M<unsigned short>(0x52ad88);
  M<unsigned short>(0x52ad88)&=~0xf0;
  PushableObjects((void*)0x52f300);
  M<unsigned short>(0x52ad88)=input;
 }
 SyncTokenSpawns();BeginPlatformTick();BeginShovel();BeginGarage();BeginBurrow();BeginBackyard();nativeLevelTick(level);EndBackyard();SyncBurrow();EndGarage();EndShovel();SyncTokenSpawns();EndPlatformTick();EndBossTick();SyncCribTargets();SyncEnemies();pushUpdated=false;
}
static int SafeAnimationFrame(short animation,int frame){
 if(animation<0||animation>=35)return 0;
 int maximum=(animationFrames[animation]-1)<<16;
 return frame<0?0:frame>maximum?maximum:frame;
}
// Buffer snapshots instead of restarting an easing curve on every packet.
// The small presentation delay absorbs ordinary Internet delivery jitter.
static const CoopPose& SmoothPoseAt(const CoopPose& received,uint32_t sequence,DWORD now){
 if(!poseReady){poseCount=0;poseReady=true;}
 if(!poseCount||(int32_t)(sequence-poseSamples[poseCount-1].sequence)>0){
  if(poseCount){
   const PoseSample& last=poseSamples[poseCount-1];
   int64_t dx=(int64_t)received.x-last.pose.x,dy=(int64_t)received.y-last.pose.y,dz=(int64_t)received.z-last.pose.z;
   if(now-last.time>500||dx>0x40000||dx< -0x40000||dy>0x40000||dy< -0x40000||dz>0x40000||dz< -0x40000)poseCount=0;
  }
  if(poseCount==32){memmove(poseSamples,poseSamples+1,31*sizeof(PoseSample));--poseCount;}
  PoseSample& sample=poseSamples[poseCount++];sample.pose=received;sample.time=now;sample.sequence=sequence;
 }
 DWORD renderTime=now-100;
 while(poseCount>2&&(int32_t)(renderTime-poseSamples[1].time)>=0){
  memmove(poseSamples,poseSamples+1,(poseCount-1)*sizeof(PoseSample));--poseCount;
 }
 const PoseSample& a=poseSamples[0];
 if(poseCount==1||(int32_t)(renderTime-a.time)<=0){displayedPose=a.pose;return displayedPose;}
 const PoseSample& b=poseSamples[1];
 unsigned duration=b.time-a.time,elapsed=renderTime-a.time;
 // Hold the last authoritative position on a stall, rather than predicting
 // through walls. Reconnection/teleport discards the old history above.
 if(!duration||elapsed>=duration){displayedPose=b.pose;return displayedPose;}
 displayedPose=a.pose;
 displayedPose.x=CoopLerp(a.pose.x,b.pose.x,elapsed,duration);
 displayedPose.y=CoopLerp(a.pose.y,b.pose.y,elapsed,duration);
 displayedPose.z=CoopLerp(a.pose.z,b.pose.z,elapsed,duration);
 displayedPose.yaw=CoopLerpAngle(a.pose.yaw,b.pose.yaw,elapsed,duration);
 displayedPose.pitch=CoopLerpAngle(a.pose.pitch,b.pose.pitch,elapsed,duration);
 displayedPose.roll=CoopLerpAngle(a.pose.roll,b.pose.roll,elapsed,duration);
 if(a.pose.animation==b.pose.animation&&b.pose.frame>=a.pose.frame)
  displayedPose.frame=CoopLerp(a.pose.frame,b.pose.frame,elapsed,duration);
 if(a.pose.secondary==b.pose.secondary&&b.pose.secondaryFrame>=a.pose.secondaryFrame)
  displayedPose.secondaryFrame=CoopLerp(a.pose.secondaryFrame,b.pose.secondaryFrame,elapsed,duration);
 return displayedPose;
}
static const CoopPose& SmoothPose(const CoopPose& received,uint32_t sequence){
 return SmoothPoseAt(received,sequence,GetTickCount());
}
static void Capture(){
 localFrame.level=M<unsigned>(0x88278c);++localFrame.sequence;
 SyncTokens();
 if(localFrame.level<1||localFrame.level>15){localFrame.level=0;return;}
 CoopPose& p=localFrame.pose;
 p.x=M<int>(0x52f300);p.y=M<int>(0x52f304);p.z=M<int>(0x52f308);
 p.pitch=M<short>(0x52f30c);p.yaw=M<short>(0x52f30e);p.roll=M<short>(0x52f310);
 p.animation=M<short>(0x52f312);p.secondary=M<short>(0x52f316);
 p.frame=M<int>(0x52f318);p.secondaryFrame=M<int>(0x52f31c);p.health=M<short>(0x52f396);
 p.pitch&=4095;p.yaw&=4095;p.roll&=4095;
 if(p.animation< -1||p.animation>=35)p.animation=0;
 if(p.secondary< -1||p.secondary>=35)p.secondary=-1;
 p.frame=SafeAnimationFrame(p.animation,p.frame);p.secondaryFrame=SafeAnimationFrame(p.secondary,p.secondaryFrame);
 if(p.health< -1)p.health=-1;if(p.health>100)p.health=100;
 if(!CoopValidPose(p)){localFrame.level=0;memset(&p,0,sizeof(p));}
}
static void __cdecl BuildList(){
 diagnosticPhase="render list";
 SyncEnemies();SyncRc();SyncBurrow();SyncMower();((void(__cdecl*)())0x4a28b0)();RefreshBackyardPlatformMeshes();Capture();Exchange();ApplyDrops();ApplyShots();
 uintptr_t remote=0;
 bool linked=SameLevel();if(linked!=wasLinked){Log(linked?"Other game linked: remote Buzz enabled.":"Other game unavailable or level differs: remote Buzz hidden.");wasLinked=linked;}
 if(SameLevel()&&CoopValidPose(remoteFrame.pose)){
  // Only the render prefix is cloned. It contains values, not the actor's
  // collision/AI/animation ownership pointers farther into the native struct.
  memcpy(remoteActor,(void*)0x52f300,0x44);
  const CoopPose& p=SmoothPose(remoteFrame.pose,remoteFrame.sequence);
  M<int>((uintptr_t)remoteActor)=p.x;M<int>((uintptr_t)remoteActor+4)=p.y;M<int>((uintptr_t)remoteActor+8)=p.z;
  M<short>((uintptr_t)remoteActor+0xc)=p.pitch;M<short>((uintptr_t)remoteActor+0xe)=p.yaw;
  M<short>((uintptr_t)remoteActor+0x10)=p.roll;
  M<short>((uintptr_t)remoteActor+0x12)=p.animation;M<short>((uintptr_t)remoteActor+0x16)=p.secondary;
  M<int>((uintptr_t)remoteActor+0x18)=SafeAnimationFrame(p.animation,p.frame);
  M<int>((uintptr_t)remoteActor+0x1c)=SafeAnimationFrame(p.secondary,p.secondaryFrame);
  M<short>((uintptr_t)remoteActor+0x40)=(remoteFrame.damageFlash&&(remoteFrame.damageFlash&4))?0:1;
  remote=(uintptr_t)remoteActor;
 }else poseReady=false;
 unsigned rendered=CoopBuildRenderList(renderList,(uintptr_t*)0x529d48,0x52f300,remote);
 // A boss owned by the other player must remain renderable even when local
 // AI/room activation did not put it in the native list. Depth testing stays native.
 if(BossReplicated()&&localFrame.boss.pose.health>0&&M<short>(TinRobot+0x14)==5){
  bool included=false;for(unsigned i=0;i<rendered;i++)if(renderList[i]==TinRobot)included=true;
  M<unsigned short>(TinRobot+0x40)|=1;
  if(!included){
   if(rendered==64)renderList[rendered-3]=TinRobot;
   else{renderList[rendered]=renderList[rendered-1];renderList[rendered-1]=TinRobot;renderList[++rendered]=0;}
  }
 }
 if(SameLevel()&&RcLevel()&&localFrame.garage.revision&&M<short>(RcActor+0x14)==8){
  bool found=false;for(unsigned i=0;i<rendered;i++)if(renderList[i]==RcActor)found=true;
  if(!found&&rendered<64){renderList[rendered]=renderList[rendered-1];renderList[rendered-1]=RcActor;renderList[++rendered]=0;}
  M<unsigned short>(RcActor+0x40)|=1;
 }
 rendered=IncludeVisibleEnemies(renderList,rendered);
 renderLevel=localFrame.level;
}
static void __cdecl PrepareRender(void* nativeList){
 diagnosticPhase="render matrices";
 unsigned level=M<unsigned>(0x88278c);
 useRenderList=level>=1&&level<=15&&level==renderLevel;
 if(useRenderList){BeginBossPresentation();BeginRcPresentation();BeginEnemyPresentation();}
 ((void(__cdecl*)(void*))0x4cd880)(useRenderList?renderList:nativeList);
}
static void __cdecl DrawRender(void* nativeList){
 diagnosticPhase="draw models";
 // Snapshot world transforms before the HUD switches projection/state.
 nameCameraReady=useRenderList;
 if(nameCameraReady){
  memcpy(nameView,(void*)0xa4c3d0,sizeof(nameView));
  memcpy(nameProjection,(void*)0xa4c290,sizeof(nameProjection));
  memcpy(nameCamera,(void*)0xe4d980,sizeof(nameCamera));
 }
 if(useRenderList)BeginEnemyRender(renderList);
 ((void(__cdecl*)(void*))0x4cdc20)(useRenderList?renderList:nativeList);
 if(useRenderList){EndEnemyRender();EndBossPresentation();EndRcPresentation();EndEnemyPresentation();}
}
static bool ProjectNameplate(const float* view,const float* projection,const float* point,float& sx,float& sy,float width=320,float height=240){
 float v[4]={};
 for(int column=0;column<4;column++)v[column]=point[0]*view[column]+point[1]*view[4+column]+point[2]*view[8+column]+view[12+column];
 float clip[4]={};
 for(int column=0;column<4;column++)for(int row=0;row<4;row++)clip[column]+=v[row]*projection[row*4+column];
 if(!_finite(clip[3])||clip[3]<=0.01f||clip[2]<0||clip[2]>clip[3])return false;
 sx=width*.5f*(1+clip[0]/clip[3]);sy=height*.5f*(1-clip[1]/clip[3]);
 return _finite(sx)&&_finite(sy)&&sx>=0&&sx<=width&&sy>=0&&sy<=height;
}
typedef void (__cdecl *NameQuadFn)(float,float,float,float,const float*,const float*,uintptr_t,unsigned,unsigned);
static NameQuadFn nameQuad=(NameQuadFn)0x4b8cc0;
static float HealthFraction(int health){return max(0,min(health,14))/14.0f;}
static void NameRect(float x,float y,float w,float h,float screenW,float screenH,unsigned color){
 const float uv0[2]={0,0},uv1[2]={1,1};
 nameQuad(x/screenW,y/screenH,w/screenW,h/screenH,uv0,uv1,0,color,0xc00);
}
typedef int (__cdecl *NameRayFn)(int*,int*,int,int,int);
static NameRayFn nameRay=(NameRayFn)0x48c860;
static bool NameplateVisible(const int* camera,const CoopPose& pose){
 int origin[3]={camera[0],camera[1],camera[2]},delta[3]={pose.x-camera[0],pose.y-0x3800-camera[1],pose.z-camera[2]};
 return nameRay(origin,delta,0x8000,1,256)==0;
}
extern "C" __declspec(dllexport) void __cdecl CoopNameplates(){
 if(!SameLevel()||!poseReady||!nameCameraReady||!peerName[0]||M<unsigned short>(0x52b818)||M<unsigned char>(0x52b816)&1)return;
 // Use the actual world-view and projection matrices sent to Direct3D.
 // This includes the native Y inversion and widescreen projection.
 float head[3]={displayedPose.x/32.0f,(displayedPose.y-0x3800)/32.0f,displayedPose.z/32.0f};
 float width=M<float>(0x4f7414),height=M<float>(0x4f7418);
 if(!_finite(width)||!_finite(height)||width<256||height<200||width>2048||height>2048)return;
 int camera[3]={M<int>(0x52adc0),M<int>(0x52adc4),M<int>(0x52adc8)};
 if(!NameplateVisible(camera,displayedPose))return;
 float sx,sy;if(!ProjectNameplate(nameView,nameProjection,head,sx,sy,width,height))return;
 float point[3]={head[0]-nameCamera[0],head[1]-nameCamera[1],head[2]-nameCamera[2]};
 float distance=sqrtf(point[0]*point[0]+point[1]*point[1]+point[2]*point[2]);
 float scale=500.0f/(distance+1200.0f);if(scale>0.40f)scale=0.40f;if(scale<0.16f)scale=0.16f;
 int alpha=distance<1800?255:(int)(255*(5000-distance)/3200);if(alpha<=0)return;
 uintptr_t font=M<uintptr_t>(0x557500+0x14*4);if(!Readable(font,4))return;
 float glyphWidth=M<unsigned char>(font+2)*.5f*scale,glyphHeight=M<unsigned char>(font+3)*.5f*scale;
 float advance=glyphWidth*.8f,textWidth=(strlen(peerName)-1)*advance+glyphWidth;
 float boxWidth=max(18.0f,textWidth+4),boxHeight=glyphHeight+6;
 float top=sy-boxHeight-4,left=sx-boxWidth/2;
 if(left<0||left+boxWidth>width||top<0)return;
 unsigned opacity=(unsigned)alpha<<24;
 // Reverse submission order: native UI inserts new quads at the list head.
 float barWidth=boxWidth-4,barTop=top+glyphHeight+3;
 NameRect(left+2,barTop,barWidth*HealthFraction(remoteFrame.pose.health),1.5f,width,height,opacity|0x55df65);
 NameRect(left+2,barTop,barWidth,1.5f,width,height,opacity|0x552222);
 float start=sx-textWidth/2;sy=top+2;
 typedef void (__cdecl *SpriteFn)(int,int,int,int,int,int,int,int,int,int);
 for(unsigned i=0;peerName[i];i++){
  unsigned char c=(unsigned char)peerName[i];if(c==' ')continue;
  int glyph=c=='?'?0x32:c<='9'?c-22:c-97;
  ((SpriteFn)0x493f40)((int)(start+i*advance),(int)sy,0x14,glyph,255,255,255,
    0x60|((255-alpha)<<8),(int)(2048*scale),(int)(2048*scale));
 }
 NameRect(left,top,boxWidth,boxHeight,width,height,((unsigned)(alpha*0.35f)<<24)|0x101820);
}
static bool JumpHook(uintptr_t site,const unsigned char* expected,size_t length,void* target){
 if(memcmp((void*)site,expected,length))return false;
 DWORD old;if(!VirtualProtect((void*)site,length,PAGE_EXECUTE_READWRITE,&old))return false;
 memset((void*)site,0x90,length);M<unsigned char>(site)=0xe9;M<int32_t>(site+1)=(int32_t)((uintptr_t)target-site-5);
 DWORD ignored;VirtualProtect((void*)site,length,old,&ignored);FlushInstructionCache(GetCurrentProcess(),(void*)site,length);return true;
}
static bool Signature(uintptr_t site,uintptr_t target){
 return M<unsigned char>(site)==0xe8&&M<int32_t>(site+1)==(int32_t)(target-site-5);
}
static bool Hook(uintptr_t site,void* target){
 DWORD old;if(!VirtualProtect((void*)site,5,PAGE_EXECUTE_READWRITE,&old))return false;
 M<int32_t>(site+1)=(int32_t)((uintptr_t)target-site-5);
 DWORD ignored;VirtualProtect((void*)site,5,old,&ignored);FlushInstructionCache(GetCurrentProcess(),(void*)site,5);return true;
}
// Portable asset paths are needed in both single-player and co-op. This only
// changes import pointers inside this process; Windows registry is never written.
static bool InstallPortablePaths(){
 DWORD length=GetModuleFileNameA(NULL,assetRoot,sizeof(assetRoot));
 if(!length||length>=sizeof(assetRoot))return false;
 char* slash=strrchr(assetRoot,'\\');
 if(!slash||slash-assetRoot>MAX_PATH-16)return false;slash[1]=0;
 HMODULE registry=GetModuleHandleA("advapi32.dll");if(!registry)return false;
 nativeOpenKey=M<OpenKeyFn>(0xe4f3b8);nativeQueryKey=M<QueryKeyFn>(0xe4f3b0);nativeCloseKey=M<CloseKeyFn>(0xe4f3b4);
 if((FARPROC)nativeOpenKey!=GetProcAddress(registry,"RegOpenKeyExA")||
    (FARPROC)nativeQueryKey!=GetProcAddress(registry,"RegQueryValueExA")||
    (FARPROC)nativeCloseKey!=GetProcAddress(registry,"RegCloseKey"))return false;
 if(!SetImport(0xe4f3b8,(uintptr_t)OpenGameKey))return false;
 if(!SetImport(0xe4f3b0,(uintptr_t)QueryGameKey)){SetImport(0xe4f3b8,(uintptr_t)nativeOpenKey);return false;}
 if(!SetImport(0xe4f3b4,(uintptr_t)CloseGameKey)){
  SetImport(0xe4f3b0,(uintptr_t)nativeQueryKey);SetImport(0xe4f3b8,(uintptr_t)nativeOpenKey);return false;
 }
 return true;
}
BOOL WINAPI DllMain(HINSTANCE instance,DWORD reason,LPVOID){
 if(reason!=DLL_PROCESS_ATTACH)return TRUE;
 DisableThreadLibraryCalls(instance);
 if((uintptr_t)GetModuleHandle(NULL)!=0x400000)return TRUE;
 if(!Signature(0x49e311,0x4a28b0)||!Signature(0x49e15d,0x4a0f80)||!Signature(0x49e153,0x4086f0)||
    !Signature(0x4411ae,0x4cd880)||!Signature(0x441285,0x4cdc20))return TRUE;
 if(!InstallPortablePaths()){Log("Portable game paths could not be installed.");return TRUE;}

 char key[128];DWORD keyLength=GetEnvironmentVariableA("TOY2_COOP_BRIDGE",key,sizeof(key));
 if(!keyLength||keyLength>=sizeof(key)||keyLength>90)return TRUE;
 char mutexName[160];sprintf(mutexName,"%s_lock",key);
 mapping=OpenFileMappingA(FILE_MAP_ALL_ACCESS,FALSE,key);
 bridgeMutex=OpenMutexA(SYNCHRONIZE|MUTEX_MODIFY_STATE,FALSE,mutexName);
 if(!mapping||!bridgeMutex){Log("Bridge handles missing; co-op hooks not installed.");return TRUE;}
 shared=(CoopShared*)MapViewOfFile(mapping,FILE_MAP_ALL_ACCESS,0,0,sizeof(CoopShared));
 if(!shared||shared->magic!=CoopMagic||shared->version!=CoopVersion){shared=NULL;Log("Bridge ABI mismatch.");return TRUE;}
 char name[128]={};GetEnvironmentVariableA("TOY2_COOP_PEER",name,sizeof(name));
 char role[16]={};GetEnvironmentVariableA("TOY2_COOP_ROLE",role,sizeof(role));localOwner=!strcmp(role,"client")?1:0;InstallDiagnostics();
 for(unsigned i=0;i<24&&name[i];i++){unsigned char c=(unsigned char)name[i];peerName[i]=(c>='A'&&c<='Z')?c+32:(c>='a'&&c<='z')||(c>='0'&&c<='9')||c==' '?c:'?';}
 if(!Signature(0x49e311,0x4a28b0)||!Signature(0x49e15d,0x4a0f80)||!Signature(0x49e153,0x4086f0)||
    !Signature(0x4411ae,0x4cd880)||!Signature(0x441285,0x4cdc20)){
  shared=NULL;Log("Game signature mismatch; co-op hooks refused.");return TRUE;
 }
 const unsigned char pickupCheckBytes[]={0x8b,0x46,0x04,0x3d,0,0,0,0x80};
 const unsigned char pickupAwardBytes[]={0x8a,0x46,0x0c,0x83,0xc4,0x10};
 const unsigned char dropBytes[]={0x66,0xa1,0x9e,0xf3,0x52,0};
 if(!Signature(0x49e175,0x407440)||!Signature(0x4076bb,0x408a60)||!EnemyTargetSignatures()||!Signature(0x40891b,0x4076f0)||!Signature(0x4087ec,0x447bd0)||!Signature(0x49dc74,0x49dfe0)||!Signature(0x49dc7b,0x49e330)||!Signature(0x4183ba,0x49ec00)||!Signature(0x4190de,0x49ec00)||!Signature(0x419103,0x49ec00)||!Signature(0x49e181,0x4a1b00)||!Signature(0x436a78,0x433700)||!Signature(0x405d65,0x40fae0)||memcmp((void*)0x4101ea,dropBytes,sizeof(dropBytes))){shared=NULL;Log("Interaction hook signature mismatch.");return TRUE;}
 for(unsigned i=0;i<sizeof(questCollectionSites)/sizeof(questCollectionSites[0]);i++)
  if(!Signature(questCollectionSites[i],0x405d20)){shared=NULL;Log("Quest collection signature mismatch.");return TRUE;}
 for(unsigned i=0;i<sizeof(questCollectionSites)/sizeof(questCollectionSites[0]);i++)Hook(questCollectionSites[i],CollectQuestActor);
 if(Hook(0x49e175,SharedActorContact)&&Hook(0x4076bb,SharedContactImpulse)&&Hook(0x40891b,SharedEnemyThink)&&Hook(0x4087ec,SharedEnemyCull)&&InstallEnemyTargetOperands()&&Hook(0x49dc74,SharedGameTick)&&Hook(0x49dc7b,SharedGameTick)&&Hook(0x4183ba,BalancePlatform)&&Hook(0x4190de,BalancePlatform)&&Hook(0x419103,BalancePlatform)&&Hook(0x49e181,SharedLevelTick)&&JumpHook(0x4a11be,pickupCheckBytes,sizeof(pickupCheckBytes),SharedPickupCheck)&&
    JumpHook(0x4a1257,pickupAwardBytes,sizeof(pickupAwardBytes),SharedPickupAward)&&Hook(0x436a78,PushableObjects)&&Hook(0x405d65,SpawnDeathDrop)&&JumpHook(0x4101ea,dropBytes,sizeof(dropBytes),DropPickupHook)&&
    Hook(0x49e311,BuildList)&&Hook(0x49e15d,Pickups)&&Hook(0x49e153,Actors)&&
    Hook(0x4411ae,PrepareRender)&&Hook(0x441285,DrawRender))
  Log("Two-player bridge v2 loaded: automatic start, nameplates, shots, native deaths, shared drops and pushable objects.");
 return TRUE;
}
