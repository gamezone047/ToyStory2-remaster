// Poll both native gameplay and pause loops so networking never stops on pause.
static bool pauseApplied,sessionReady;static int lastTrack=-1;static DWORD musicRetryAt;
typedef void (__cdecl *SessionTickFn)();
static SessionTickFn playTick=(SessionTickFn)0x49dfe0,pauseTick=(SessionTickFn)0x49e330;
typedef int (__cdecl *MusicFn)(int,int);static MusicFn playMusic=(MusicFn)0x413150;
static void ChangeEvent(CoopEvent& e,int value){++e.revision;if(!e.revision)e.revision=1;e.owner=localOwner;e.value=value;}
static void MergeEvent(CoopEvent& e,const CoopEvent& r){if(NewerState(r.revision,r.owner,e.revision,e.owner))e=r;}
static void SetNativePause(bool paused){
 bool before=M<unsigned short>(0x52b818)!=0;
 if(paused&&!before){M<short>(0x529e48)=0;M<short>(0x52b7e4)=0;M<short>(0x52adb0)=0;M<int>(0x830d50)=0;
  memcpy(&M<unsigned char>(0x830cb0),&M<unsigned char>(0x52adc0),20);M<int>(0x830cc8)=0xec4;}
 M<unsigned short>(0x52b818)=paused?1:0;pauseApplied=paused;
}
static void ResetSession(){localFrame.pause={};localFrame.music={};localFrame.damageFlash=0;sessionReady=false;pauseApplied=false;lastTrack=-1;musicRetryAt=0;}
static void SessionPoll(){
 ++localFrame.sequence;Exchange();
 if(!SameLevel()){
  if(sessionReady&&pauseApplied&&localFrame.pause.owner!=localOwner)SetNativePause(false);
  sessionReady=false;return;
 }
 bool nativePaused=M<unsigned short>(0x52b818)!=0;int track=M<int>(0x5282cc);
 if(!sessionReady){pauseApplied=nativePaused;lastTrack=track;sessionReady=true;if(nativePaused)ChangeEvent(localFrame.pause,1);if(localOwner==0&&track>=0&&track<22&&!localFrame.music.revision)ChangeEvent(localFrame.music,track);}
 if(nativePaused!=pauseApplied)ChangeEvent(localFrame.pause,nativePaused?1:0);
 MergeEvent(localFrame.pause,remoteFrame.pause);
 if(localFrame.pause.revision)SetNativePause(localFrame.pause.value!=0);
 MergeEvent(localFrame.music,remoteFrame.music);
 bool rcAuthority=!RcLevel()||!localFrame.garage.revision||localFrame.garage.owner==localOwner;
 if(rcEndMusicPending){rcEndMusicPending=false;if(rcAuthority&&rcBackgroundTrack>=0){playMusic(rcBackgroundTrack,1);track=M<int>(0x5282cc);ChangeEvent(localFrame.music,rcBackgroundTrack);}}
 if(rcAuthority&&track!=lastTrack&&track>=0&&track<22)ChangeEvent(localFrame.music,track);
 if(localFrame.music.revision&&localFrame.music.value!=track&&(int32_t)(GetTickCount()-musicRetryAt)>=0){playMusic(localFrame.music.value,1);track=M<int>(0x5282cc);musicRetryAt=GetTickCount()+1000;}
 lastTrack=track;
 int flash=M<short>(0x52f394);localFrame.damageFlash=(unsigned)max(0,min(flash,180));
 Exchange();
}
static void __cdecl SharedGameTick(){
 diagnosticPhase="game/pause loop";DiagnosticHeartbeat();
 PrepareLevel();SessionPoll();
 if(M<unsigned short>(0x52b818))pauseTick();else playTick();
 SessionPoll();
}

// Session-level travel survives per-level sharing resets. The native level loop
// on each machine consumes this event through ModernControls' console polling.
static uint32_t travelConsumed;static unsigned travelOwnerConsumed=2;
extern "C" __declspec(dllexport) int __cdecl CoopConsoleLevel(int request){
 if(!shared||GetTickCount()-helperTick>2000)return request;
 MergeEvent(localFrame.travel,remoteFrame.travel);
 if(request>=1&&request<=15){ChangeEvent(localFrame.travel,request);++localFrame.sequence;}
 const CoopEvent& t=localFrame.travel;
 if(t.revision&&(t.revision!=travelConsumed||t.owner!=travelOwnerConsumed)){
  travelConsumed=t.revision;travelOwnerConsumed=t.owner;Exchange();return t.value;
 }
 return 0;
}

extern "C" __declspec(dllexport) int __cdecl CoopCampaignActive(){return Connected()?1:0;}
// Console "follow": the other player's current level (0 when unknown).
extern "C" __declspec(dllexport) int __cdecl CoopPeerLevel(){return Connected()&&remoteFrame.level>=1&&remoteFrame.level<=15?(int)remoteFrame.level:0;}
