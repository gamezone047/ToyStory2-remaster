// Player-only segmented hitscan. Keep native geometry, damage and beam effects.
static bool extendedShot;
static int shotDamage, shotActorHit;
static uintptr_t shotBeam;
typedef int (__cdecl *WorldRayFn)(int*,int*,int,int,int);
typedef int (__cdecl *ActorRayFn)(int*,int*,int);
static WorldRayFn worldRay=(WorldRayFn)0x48c860;
static ActorRayFn actorRay=(ActorRayFn)0x4a5870;

static bool ActorInRaySegment(uintptr_t actor,const int* start,const int* delta){
 for(int axis=0;axis<3;axis++){
  int64_t a=start[axis],b=a+delta[axis],p=mem<int>(actor+axis*4);
  if(p<min(a,b)-0x10000 || p>max(a,b)+0x10000)return false;
 }
 return true;
}
static int SegmentActors(int* start,int* delta,int damage){
 uintptr_t saved[65]={};unsigned count=0,kept=0;
 while(count<64 && mem<uintptr_t>(0x52f1d0+count*4)){
  saved[count]=mem<uintptr_t>(0x52f1d0+count*4);count++;
 }
 for(unsigned i=0;i<count;i++)if(ActorInRaySegment(saved[i],start,delta))
  mem<uintptr_t>(0x52f1d0+kept++*4)=saved[i];
 mem<uintptr_t>(0x52f1d0+kept*4)=0;
 int hit=actorRay(start,delta,damage);
 for(unsigned i=0;i<=count;i++)mem<uintptr_t>(0x52f1d0+i*4)=saved[i];
 return hit;
}
static int ShotSegments(const int* start,const int* step){
 int stride=max(max(abs(step[0]),abs(step[1])),abs(step[2]));
 if(!stride)return 1;
 // Cover every loaded actor, plus empty space beyond them. This has no
 // ordinary weapon range cap; 64 protects the native signed beam lifetime.
 int64_t reach=0x100000;
 for(uintptr_t a=0x52c840;a<0x52ef40;a+=0x9c)
  for(int axis=0;axis<3;axis++){
   int64_t d=(int64_t)mem<int>(a+axis*4)-start[axis];
   if(d<0)d=-d;reach=max(reach,d+0x20000);
  }
 return Clamp((int)(reach/stride+2),1,64);
}
static int __cdecl ExtendedWorldRay(int* start,int* delta,int mask,int mode,int radius){
 if(!extendedShot)return worldRay(start,delta,mask,mode,radius);
 int step[3]={delta[0],delta[1],delta[2]},at[3]={start[0],start[1],start[2]};
 int limit=ShotSegments(start,step),worldHit=0;shotActorHit=0;
 for(int n=0;n<limit;n++){
  int part[3]={step[0],step[1],step[2]};
  worldHit=worldRay(at,part,mask,mode,radius);
  shotActorHit=SegmentActors(at,part,shotDamage);
  for(int axis=0;axis<3;axis++){at[axis]+=part[axis];delta[axis]=at[axis]-start[axis];}
  if(worldHit||shotActorHit)break;
 }
 // Native code only updates this endpoint on collision. Also update misses.
 for(int axis=0;axis<3;axis++)mem<int>(shotBeam+12+axis*4)=at[axis];
 return worldHit;
}
static int __cdecl ExtendedActorRay(int* start,int* delta,int damage){
 return extendedShot?shotActorHit:actorRay(start,delta,damage);
}
static uintptr_t __cdecl PlayerLaser(int type,int yaw,int pitch,int* origin,int* offset,int range,int autoAim){
 int minimum=1000;shotBeam=0x882970;
 for(uintptr_t a=0x882970;a<0x882a20;a+=0x2c){
  int timer=mem<short>(a+0x24);
  if(!timer){shotBeam=a;break;}
  if(timer<minimum){minimum=timer;shotBeam=a;}
 }
 extendedShot=true;shotDamage=type+1;
 uintptr_t result=((uintptr_t(__cdecl*)(int,int,int,int*,int*,int,int))0x4a5d30)(type,yaw,pitch,origin,offset,range,autoAim);
 extendedShot=false;CoopShotEvent(shotBeam);return result;
}
static bool ReplaceBytes(uintptr_t va,const unsigned char* expected,const unsigned char* replacement,size_t len){
 if(memcmp((void*)va,expected,len)){Log("Combat patch signature mismatch.");return false;}
 DWORD old;if(!VirtualProtect((void*)va,len,PAGE_EXECUTE_READWRITE,&old))return false;
 memcpy((void*)va,replacement,len);DWORD ignored;
 VirtualProtect((void*)va,len,old,&ignored);FlushInstructionCache(GetCurrentProcess(),(void*)va,len);return true;
}
// Distances are compared squared: four times the threshold means twice the range.
static int TwiceRangeSquared(int n,int bias=0){
 int64_t result=((int64_t)n+bias)*4-bias;
 return result>0x7fffffff?0x7fffffff:result<0?0:(int)result;
}
static DWORD actorRangeResume=0x4087ad,modelVisible=0x447c90,modelHidden=0x447cfd;
static DWORD coinNormal=0x4413b0,coinHidden=0x44161c;
__declspec(naked) static void ActorRangeHook(){__asm{
 mov eax,edi
 imul eax,edi
 push ecx
 push edx
 push 0
 push eax
 call TwiceRangeSquared
 add esp,8
 pop edx
 pop ecx
 jmp dword ptr [actorRangeResume]
}}
__declspec(naked) static void ModelRangeHook(){__asm{
 add ecx,edi
 push eax
 push edx
 push 0
 push ecx
 call TwiceRangeSquared
 add esp,8
 mov ecx,eax
 pop edx
 pop eax
 cmp ebx,ecx
 jge hidden
 jmp dword ptr [modelVisible]
 hidden:
 jmp dword ptr [modelHidden]
}}
__declspec(naked) static void CoinDistanceHook(){__asm{
 cmp byte ptr [esi+0x0c],0x10
 je doubleRange
 cmp byte ptr [esi+0x0c],0x30
 jb originalDistance
 doubleRange:
 push eax
 push edx
 push 100
 push ecx
 call TwiceRangeSquared
 add esp,8
 mov ecx,eax
 pop edx
 pop eax
 originalDistance:
 cmp eax,ecx
 jge hidden
 jmp dword ptr [coinNormal]
 hidden:
 jmp dword ptr [coinHidden]
}}
static bool InstallCombat(){
 const unsigned char nop6[]={0x90,0x90,0x90,0x90,0x90,0x90};
 // Both native camera modes already have collision casts. Use the radial
 // obstruction mode consistently; the other mode rotates yaw automatically.
 const unsigned char movingCamera[]={0x0f,0x8e,0x11,0x05,0,0};
 const unsigned char otherCamera[]={0x0f,0x85,0x03,0x05,0,0};
 const unsigned char coinDistance[]={0x3b,0xc1,0x0f,0x8d,0x6c,0x02,0,0};
 const unsigned char actorRange[]={0x8b,0xc7,0x0f,0xaf,0xc7};
 const unsigned char modelRange[]={0x03,0xcf,0x3b,0xd9,0x7d,0x6d};
 return Patch(0x4087a8,actorRange,sizeof(actorRange),ActorRangeHook)&&
  Patch(0x447c8a,modelRange,sizeof(modelRange),ModelRangeHook)&&
  Patch(0x4413a8,coinDistance,sizeof(coinDistance),CoinDistanceHook)&&
  ReplaceBytes(0x404704,movingCamera,nop6,6)&&ReplaceBytes(0x404712,otherCamera,nop6,6)&&
  CallHook(0x434cb1,0x4a5d30,PlayerLaser)&&
  CallHook(0x4a60d5,0x48c860,ExtendedWorldRay)&&CallHook(0x4a60ea,0x4a5870,ExtendedActorRay);
}
