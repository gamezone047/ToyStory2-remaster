#include <mmsystem.h>
#pragma comment(lib,"winmm.lib")
static LARGE_INTEGER frameFrequency,framePrevious;
static int64_t frameRemainder;
static int FrameTicks(int64_t elapsedUs,bool demo){
 // Round to the nearest simulation tick, not up at the 16.67ms boundary.
 // Long pauses/loading do not generate a burst of catch-up simulation.
 if(elapsedUs>250000||elapsedUs<0)return demo?2:1;
 return Clamp((int)((elapsedUs+8333)/16667),demo?2:1,3);
}
static int64_t FrameWaitUs(int64_t elapsedUs,bool demo){
 int64_t remaining=(demo?33334:16667)-elapsedUs;
 return remaining>0?remaining:0;
}
static void AdvanceFrameClock(int64_t now,int64_t elapsedUs,int ticks){
 if(elapsedUs>250000||elapsedUs<0){framePrevious.QuadPart=now;frameRemainder=0;return;}
 // Carry sub-frame lateness into the next deadline instead of accumulating
 // it each frame. Retain the QPC division remainder for an exact 60 Hz cadence.
 int64_t step=frameFrequency.QuadPart*ticks+frameRemainder;
 framePrevious.QuadPart+=step/60;frameRemainder=step%60;
}
static int __cdecl PaceFrame(int){
 LARGE_INTEGER now;QueryPerformanceCounter(&now);
 bool demo=mem<int>(0x52ad94)!=0;
 if(!framePrevious.QuadPart)framePrevious=now;
 int64_t elapsed=(now.QuadPart-framePrevious.QuadPart)*1000000/frameFrequency.QuadPart;
 int64_t remaining=FrameWaitUs(elapsed,demo);
 // Timer resolution is balanced on every path, unlike a permanent global
 // timer request. Never turn a negative remaining delay into an unsigned Sleep.
 bool timer=remaining>0&&timeBeginPeriod(1)==TIMERR_NOERROR;
 while(remaining>0){
  if(remaining>2000)Sleep((DWORD)((remaining-1000)/1000));
  else if(remaining>300)Sleep(0);
  else YieldProcessor();
  QueryPerformanceCounter(&now);
  elapsed=(now.QuadPart-framePrevious.QuadPart)*1000000/frameFrequency.QuadPart;
  remaining=FrameWaitUs(elapsed,demo);
 }
 if(timer)timeEndPeriod(1);
 int ticks=FrameTicks(elapsed,demo);
 mem<int>(0x52f2d4)=ticks;
 AdvanceFrameClock(now.QuadPart,elapsed,ticks);
 return (int)(now.QuadPart*1000/frameFrequency.QuadPart);
}
static bool InstallFramePacing(){
 // Install after plugins initialize, at display startup. Accept only native
 // calls or the verified installed ToyStory2Fix timing routine.
 if(!QueryPerformanceFrequency(&frameFrequency)||!frameFrequency.QuadPart)return false;
 HMODULE fix=GetModuleHandleA("ToyStory2Fix.asi");
 const uintptr_t sites[]={0x49bbe2,0x44190b,0x4419f6};
 uintptr_t targets[3];
 for(int n=0;n<3;n++){
  if(*(unsigned char*)sites[n]!=0xe8)return false;
  targets[n]=sites[n]+5+*(int32_t*)(sites[n]+1);
  if(targets[n]!=0x490860&&(!fix||targets[n]!=(uintptr_t)fix+0x9610))return false;
 }
 for(int n=0;n<3;n++)if(!CallHook(sites[n],targets[n],PaceFrame))return false;
 framePrevious.QuadPart=0;frameRemainder=0;return true;
}
