// Diagnostic-only instrumentation: timestamps in memory on the game thread;
// CSV formatting and disk I/O run on a separate low-priority writer thread.
#include <math.h>
struct ProfileRow{
 unsigned frame,mask;int level,mode,focused,ticks,nativeDistance;
 int64_t stamp[9],previousExit,entry,exit;
 int player[3],camera[3],yaw,pitch,velocity[3];
};
static ProfileRow profileRows[4096],profileCurrent;
static volatile LONG profileWrite,profileRead,profileDropped;
static bool profileEnabled,profileF9Down;
static LARGE_INTEGER profileFrequency;
static int64_t profileLastExit;
static unsigned profileFrame;
static FILE* profileFile;
static float profileNativeWorldDistance;
static bool profileWorldDistanceKnown;
static DWORD profileModeNotice;
static const char* DistanceModeLabel(int mode){
 return mode==DistanceEnemies?"distance: far enemies only":
        mode==DistanceItems?"distance: far items only":"distance: original";
}
static void DrawProfileStatus(){
 if(!profileModeNotice||GetTickCount()-profileModeNotice>3000)return;
 ((void(__cdecl*)(int,const char*,int,int,int,int))0x401fb0)(28,
  DistanceModeLabel(distanceMode),255,255,0,0);
}
static int64_t ProfileNow(){LARGE_INTEGER t;QueryPerformanceCounter(&t);return t.QuadPart;}
static void __cdecl ProfileStamp(int stage){
 if(!profileEnabled)return;
 if(stage==0)profileCurrent.mask=0;
 profileCurrent.stamp[stage]=ProfileNow();profileCurrent.mask|=1u<<stage;
}
#define PROFILE_BRIDGE(name,index,target) \
 static DWORD name##Target=target; \
 __declspec(naked) static void name(){__asm{pushfd} __asm{pushad} \
 __asm{push index} __asm{call ProfileStamp} __asm{add esp,4} \
 __asm{popad} __asm{popfd} __asm{jmp dword ptr [name##Target]}}
PROFILE_BRIDGE(ProfilePrepare,0,0x4909e0)
PROFILE_BRIDGE(ProfileBegin,1,0x4b2d50)
PROFILE_BRIDGE(ProfileWorld,2,0x4cddd0)
PROFILE_BRIDGE(ProfileActors,3,0x4cdc20)
PROFILE_BRIDGE(ProfileSubmit,4,0x4b6a50)
PROFILE_BRIDGE(ProfileFlush,5,0x4b8460)
PROFILE_BRIDGE(ProfileAudio,6,0x4907e0)
PROFILE_BRIDGE(ProfilePresent,7,0x4b2de0)
#undef PROFILE_BRIDGE
static void __cdecl ProfileWorldDistance(float farDistance,float otherDistance){
 profileNativeWorldDistance=farDistance*farDistance;profileWorldDistanceKnown=true;
 ((void(__cdecl*)(float,float))0x4bc410)(farDistance,otherDistance);
 // Geometry and hidden-room rendering are never extended in balanced mode.
 mem<float>(0x5088b0)=profileNativeWorldDistance;
}
static bool SetDiagnosticDistance(int mode){
 struct Site{uintptr_t va;unsigned size;unsigned char original[6];};
 const Site sites[]={
  {0x4087b3,2,{0x7d,0x10}}, {0x447c8e,2,{0x7d,0x6d}}
 };
 const unsigned char nops[]={0x90,0x90,0x90,0x90,0x90,0x90};
 if(mode!=DistanceOriginal&&mode!=DistanceEnemies&&mode!=DistanceItems)return false;
 if(mode==distanceMode)return true;
 if(!profileWorldDistanceKnown)return false;
 for(unsigned i=0;i<sizeof(sites)/sizeof(sites[0]);i++)if(memcmp((void*)sites[i].va,DistantActors(distanceMode)?nops:sites[i].original,sites[i].size))return false;
 for(unsigned i=0;i<sizeof(sites)/sizeof(sites[0]);i++)if(!ReplaceBytes(sites[i].va,DistantActors(distanceMode)?nops:sites[i].original,DistantActors(mode)?nops:sites[i].original,sites[i].size))return false;
 distanceMode=mode;distantItems=DistantItems(mode);
 mem<float>(0x5088b0)=profileNativeWorldDistance;
 return true;
}
static double ProfileMs(int64_t ticks){return ticks*1000.0/profileFrequency.QuadPart;}
static bool ProfileEnqueue(const ProfileRow& row){
 LONG write=profileWrite,read=InterlockedCompareExchange(&profileRead,0,0);
 if((unsigned)(write-read)>=4096){InterlockedIncrement(&profileDropped);return false;}
 profileRows[(unsigned)write%4096]=row;InterlockedIncrement(&profileWrite);return true;
}
static DWORD WINAPI ProfileWriter(void*){
 SetThreadPriority(GetCurrentThread(),THREAD_PRIORITY_BELOW_NORMAL);
 for(;;){
  Sleep(250);
  LONG end=InterlockedCompareExchange(&profileWrite,0,0);
  while(profileRead!=end){
   ProfileRow r=profileRows[(unsigned)profileRead%4096];
   InterlockedIncrement(&profileRead);
   fprintf(profileFile,"%u,%d,%d,%d,%d,%d,%u,%.4f,%.4f,%.4f",
    r.frame,r.level,r.mode,r.focused,r.ticks,r.nativeDistance,r.mask,
    r.previousExit?ProfileMs(r.exit-r.previousExit):0.0,
    r.previousExit?ProfileMs(r.entry-r.previousExit):0.0,ProfileMs(r.exit-r.entry));
   fprintf(profileFile,",%.4f",r.previousExit&&(r.mask&1)?ProfileMs(r.stamp[0]-r.previousExit):-1.0);
   for(int i=0;i<8;i++)fprintf(profileFile,",%.4f",
    (r.mask&(3u<<i))==(3u<<i)?ProfileMs(r.stamp[i+1]-r.stamp[i]):-1.0);
   fprintf(profileFile,",%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%ld\n",
    r.player[0],r.player[1],r.player[2],r.camera[0],r.camera[1],r.camera[2],
    r.yaw,r.pitch,r.velocity[0],r.velocity[1],r.velocity[2],profileDropped);
  }
  fflush(profileFile);
 }
}
static void ProfileFinish(int64_t entry,int64_t exit,int ticks){
 if(!profileEnabled)return;
 ProfileRow& r=profileCurrent;
 r.frame=++profileFrame;r.previousExit=profileLastExit;r.entry=entry;r.exit=exit;
 r.stamp[8]=entry;r.mask|=1u<<8;r.ticks=ticks;
 r.level=mem<int>(0x88278c);r.mode=mem<int>(0x50a13c);r.focused=Focused();
 r.nativeDistance=distanceMode; // old captures: 0 unlimited, 2 balanced
 for(int i=0;i<3;i++){
  r.player[i]=mem<int>(0x52f300+i*4);r.camera[i]=mem<int>(0x52adc0+i*4);
  r.velocity[i]=mem<int>(0x52f368+i*4);
 }
 r.pitch=SignedAngle(mem<short>(0x52adcc));r.yaw=mem<short>(0x52adce)&4095;
 ProfileEnqueue(r);
 profileLastExit=exit;r.mask=0;
 bool f9=(GetAsyncKeyState(VK_F9)&0x8000)!=0;
 if(f9&&!profileF9Down&&Focused()&&r.level>=1&&r.level<=15){
  if(SetDiagnosticDistance(NextDistanceMode(distanceMode))){
   profileModeNotice=GetTickCount();
   Log(DistanceModeLabel(distanceMode));
  }
  else Log("F9 comparison refused: draw-limit state not ready or signature mismatch.");
 }
 profileF9Down=f9;
}
static bool InstallFrameProfile(){
 if(profileEnabled)return true;
 const uintptr_t sites[]={0x4411b6,0x441236,0x44127b,0x441285,0x4418e9,0x4418f5,0x4418fa,0x441901,0x4cdd3b,0x4cde6d,0x4cdf4a};
 const uintptr_t targets[]={0x4909e0,0x4b2d50,0x4cddd0,0x4cdc20,0x4b6a50,0x4b8460,0x4907e0,0x4b2de0,0x4bc410,0x4bc410,0x4bc410};
 void* hooks[]={ProfilePrepare,ProfileBegin,ProfileWorld,ProfileActors,ProfileSubmit,ProfileFlush,ProfileAudio,ProfilePresent,ProfileWorldDistance,ProfileWorldDistance,ProfileWorldDistance};
 for(int i=0;i<11;i++)if(*(unsigned char*)sites[i]!=0xe8||sites[i]+5+*(int32_t*)(sites[i]+1)!=targets[i])return false;
 QueryPerformanceFrequency(&profileFrequency);
 char path[MAX_PATH];strcpy(path,logPath);char* slash=strrchr(path,'\\');if(!slash)return false;
 SYSTEMTIME t;GetLocalTime(&t);
 sprintf(slash+1,"FrameProfile-%04d%02d%02d-%02d%02d%02d.csv",t.wYear,t.wMonth,t.wDay,t.wHour,t.wMinute,t.wSecond);
 profileFile=fopen(path,"w");if(!profileFile)return false;
 fprintf(profileFile,"frame,level,camera_mode,focused,simulation_ticks,native_distance,stage_mask,frame_ms,work_ms,limiter_ms,logic_and_prepare_ms,prepare_ms,background_ms,world_ms,actors_and_effects_ms,submit_ms,flush_ms,audio_ms,present_ms,px,py,pz,cx,cy,cz,yaw,pitch,vx,vy,vz,dropped\n");fflush(profileFile);
 HANDLE thread=CreateThread(NULL,0,ProfileWriter,NULL,0,NULL);
 if(!thread){fclose(profileFile);profileFile=NULL;return false;}CloseHandle(thread);
 for(int i=0;i<11;i++)if(!CallHook(sites[i],targets[i],hooks[i]))return false;
 profileEnabled=true;Log("Frame profiling active. F9 cycles original, far enemies only, far items only; geometry/rooms unchanged.");return true;
}
