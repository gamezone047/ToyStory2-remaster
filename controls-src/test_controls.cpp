#define CONTROLS_TEST
#include "ModernControls.cpp"
#include <assert.h>
#include <float.h>
static int rayCalls,wallAt,enemyAt;
static int deathCalls;
static bool immediateDeath;
static void __cdecl MockDeath(uintptr_t actor,int mode){
 assert(mem<short>(actor+0x7a)==0);++deathCalls;
 if(mode&2){mem<short>(actor+0x14)=0;mem<short>(actor+0x7e)=0;}
 else {
  mem<short>(actor+0x7a)=10000; // special death animation sets a timer
  mem<short>(actor+0x7e)=immediateDeath?0:999;
 }
}
static int __cdecl MockWorld(int*,int* delta,int,int,int){
 ++rayCalls;if(rayCalls==wallAt){delta[0]/=2;return 1;}return 0;
}
static int __cdecl MockActors(int*,int* delta,int damage){
 assert(damage==1);
 if(rayCalls==enemyAt){delta[0]/=2;return 4;}return 0;
}
static int testActorRoom=2,soundCalls,soundVolume;
static const int* soundPosition;
static int centeredCalls;
static int __cdecl MockCentered(int sample,int pitch,int left,int right){assert(sample==7&&pitch==5120&&left==right);++centeredCalls;soundVolume=left;return 999;}
static short __cdecl MockTokenSprite(int x,int y,int sprite,int frame,int,int,int,int,int sx,int sy){
 assert(x==14&&y==205&&sprite==-1&&frame==0&&sx==0x1000&&sy==0x1000);
 const TokenSpriteDescriptor* d=(const TokenSpriteDescriptor*)mem<uintptr_t>(0x557704);
 assert(d->texture==31&&d->width==24&&d->height==16&&d->u==160&&d->v==128);return 0;
}
static int __cdecl MockRoom(const int* pos){return IsActorSoundPosition((uintptr_t)pos)?testActorRoom:2;}
static int __cdecl MockOne(int,int,int volume,const int* pos){++soundCalls;soundVolume=volume;soundPosition=pos;return volume;}
static int __cdecl MockLoop(int,int,int,int volume,const int* pos,int){return MockOne(0,0,volume,pos);}
static DWORD mockVoiceStatus=1,mockVoiceCursor;
static HRESULT __stdcall MockSoundStatus(void*,DWORD* status){*status=mockVoiceStatus;return S_OK;}
static HRESULT __stdcall MockSoundCursor(void*,DWORD* cursor,DWORD* write){*cursor=mockVoiceCursor;*write=32;return S_OK;}
static int __cdecl MockSaveLoad(int level){return level==1?0:1;}
static void __cdecl MockSaveCamera(void* camera,void* player){assert(camera==&mem<unsigned char>(0x52f3a0)&&player==&mem<unsigned char>(0x52f300));}
static void __cdecl MockSaveCampaign(void*){}
static void TestAutosaveRoundTrip(){
 static unsigned char pickupData[20]={};*(unsigned short*)pickupData=1;
 *(int*)(pickupData+4)=123;*(int*)(pickupData+8)=456;pickupData[16]=0x10;
 mem<uintptr_t>(0x559d6c)=(uintptr_t)pickupData;
 saveNativeLoad=MockSaveLoad;saveResetCamera=MockSaveCamera;saveCaptureCampaign=MockSaveCampaign;
 mem<int>(0x88278c)=1;mem<int>(0x52ada0)=-1;mem<short>(0x52ad8a)=0;
 mem<short>(0x52f396)=14;mem<short>(0x52f39e)=20;
 mem<int>(0x52f300)=101;mem<int>(0x52f304)=202;mem<int>(0x52f308)=303;
 mem<short>(0x52c854)=3;mem<short>(0x52c8be)=10;mem<int>(0x52c840)=707;
 mem<unsigned char>(0x52ad58)=7;
 assert(AutosaveLoadLevel(1)==0);saveReady=true;CaptureAutosave();saveReady=false;
 assert(saveAvailable&&savedGame.position[0]==101&&savedGame.coins==20);
 mem<int>(0x52f300)=0;mem<short>(0x52f39e)=0;mem<int>(0x52c840)=0;mem<unsigned char>(0x52ad58)=0;
 savedGame.pickups[0][4]=0;savedGame.pickups[0][5]=0;savedGame.pickups[0][6]=0;savedGame.pickups[0][7]=0x80;
 restorePending=true;AutosaveLoadLevel(1);
 assert(!restorePending&&mem<int>(0x52f300)==101&&mem<short>(0x52f39e)==20&&mem<int>(0x52c840)==707);
 assert(mem<unsigned char>(0x52ad58)==7&&*(int*)(pickupData+8)==INT_MIN);
 // A different level layout must never receive coordinates/items from this save.
 *(int*)(pickupData+4)=999;mem<int>(0x52f300)=808;restorePending=true;AutosaveLoadLevel(1);
 assert(mem<int>(0x52f300)==808);
 saveReady=true;
 assert(!CompleteAutosave()); // Incomplete progress must not be marked complete.
 mem<unsigned char>(0x52f0d8)=31;
 mem<uintptr_t>(0x559d6c)=0; // Summary may already have freed the level.
 assert(CompleteAutosave()&&ValidSave(savedGame)&&savedGame.completed==1);
 assert(savedGame.campaign[0x148]==31);
 saveAvailable=false;mem<int>(0x88278c)=3;mem<short>(0x52ad8a)=2;
 assert(CompleteAutosave(true)&&ValidSave(savedGame)&&savedGame.completed==1&&savedGame.level==3);
 saveReady=false;
 savedGame={};saveAvailable=false;saveLevel=0;pitchInitialized=false;thirdYaw=thirdPitch=0;lastThird=0;
 memset(fakeMemory,0,sizeof(fakeMemory));
}
static int MockCampaignConnected(){return 1;}
static int __cdecl ConsoleNoFloor(int*,int*,int,int,int){return 0;}
static int __cdecl ConsoleFloorRay(int*,int* delta,int,int,int){return delta[1]>0?1:0;}
int main(){
 assert(ParseConsoleCommand("lvl1")==1&&ParseConsoleCommand("lvl15")==15&&ParseConsoleCommand("unstuck")==-1);
 const char* badCommands[]={"lvl0","lvl16","lvl1x","lvl01","lvl","","unstuck x"};
 for(const char* bad:badCommands)assert(ParseConsoleCommand(bad)==0);
 mem<int>(0x88278c)=2;mem<short>(0x52f396)=14;mem<short>(0x52b7dc)=0;
 for(int i=0;i<15;i++)mem<int>(0x50268c+i*4)=15-i;
 ConsoleQueueLevel(3);assert(consoleLevelRequest==3&&mem<short>(0x52b7dc)==6&&mem<short>(0x52f2dc)==0);
 assert(AutosaveLevelSelect()==0&&mem<short>(0x52ad8a)==12&&consoleLevelRequest==0);
 mem<short>(0x52b7dc)=0;worldRay=ConsoleFloorRay;int clear[3]={8192,0,8192};assert(ConsoleClearPoint(clear));
 consoleSafeCount=1;consoleSafeLevel=2;memcpy(consoleSafe[0].position,clear,12);consoleSafe[0].time=GetTickCount()-1000;
 mem<int>(0x53c668)=1;mem<int>(0x53c66c)=5;mem<int>(0x52f368)=500;assert(ConsoleUnstuck());
 assert(mem<int>(0x52f300)==8192&&mem<int>(0x52f308)==8192&&mem<int>(0x53c668)==-1&&!mem<int>(0x53c66c)&&mem<int>(0x52f368)==0);
 worldRay=ConsoleNoFloor;assert(!ConsoleUnstuck()&&mem<int>(0x52f300)==8192&&mem<int>(0x52f308)==8192);
 mem<int>(0x53c668)=1;mem<int>(0x53c658)=999;mem<short>(0x52f30e)=0;ropeJumpWasDown=false;RopeJumpOff(true);
 assert(mem<int>(0x53c668)==-1&&mem<int>(0x53c658)==12&&mem<int>(0x52f36c)==-1024);
 mem<int>(0x53c668)=1;RopeJumpOff(true);assert(mem<int>(0x53c668)==1);RopeJumpOff(false);mem<short>(0x52b818)=1;RopeJumpOff(true);assert(mem<int>(0x53c668)==1);
 mem<short>(0x52b818)=0;mem<short>(0x52b7dc)=0;mem<int>(0x88278c)=1;mem<short>(0x52f396)=14;mem<unsigned char>(0x52f0d8)=31;campaignConnected=MockCampaignConnected;
 assert(TokenExitDecision()==0&&consoleLevelRequest==2&&mem<short>(0x52b7dc)==6);assert(AutosaveLevelSelect()==0&&mem<short>(0x52ad8a)==13);
 assert(!CoopAdvanceCampaign(15));campaignConnected=CoopCampaignConnected;
 memset(fakeMemory,0,sizeof(fakeMemory));consoleSafeCount=0;consoleOpen=false;ropeJumpWasDown=false;worldRay=MockWorld;
 puts("PASS: console parser/bounds, native level-selection handoff, nearby grounded unstuck and attached-rope fresh jump with cooldown/pause/held-key protection.");

 assert(LocalTestWindowStyle(WS_VISIBLE|WS_POPUP)&WS_VISIBLE);
 assert(!(LocalTestWindowStyle(WS_POPUP)&WS_POPUP));
 mem<unsigned short>(0x52ad8a)=9;
 mem<unsigned short>(0x52ad88)=0xffff;
 ClearInactiveInput();
 assert(mem<unsigned short>(0x52ad8a)==9);
 assert(mem<unsigned short>(0x52ad88)==0);
 coopLevelStart=true;mem<short>(0x52ad8a)=7;
 assert(AutosaveLevelSelect()==0 && !coopLevelStart && mem<short>(0x52ad8a)==0);
 coopLoadConfirm=true;MenuMousePoll();
 assert((mem<unsigned short>(0x88279c)&0x4000)!=0 && (mem<unsigned short>(0x882794)&0x4000)==0);
 coopLoadConfirm=false;mem<unsigned short>(0x88279c)=0;

 interactPending=false;
 assert(CubeInteractionAllowed(16,true,1000)); // Coins remain automatic.
 assert(!CubeInteractionAllowed(4,true,1000)&&cubeHintTime==1000);
 interactPending=true;interactTime=1000;
 assert(!CubeInteractionAllowed(4,false,1010)&&interactPending);
 assert(!CubeInteractionAllowed(4,true,1251)&&interactPending);
 mem<unsigned short>(0x52b816)=1;
 assert(!CubeInteractionAllowed(4,true,1010));
 mem<unsigned short>(0x52b816)=0;
 mem<short>(0x52b818)=1;assert(!CubeInteractionAllowed(4,true,1010));mem<short>(0x52b818)=0;
 assert(CubeInteractionAllowed(4,true,1010)&&!interactPending&&!cubeHintTime);
 assert(!CubeInteractionAllowed(4,true,1020)); // Holding E cannot reopen a cube.
 cubeHintTime=0;
 uintptr_t statusTable[10]={};statusTable[4]=(uintptr_t)MockSoundCursor;statusTable[9]=(uintptr_t)MockSoundStatus;uintptr_t* statusObject=statusTable;
 DWORD playing=0,unused=0;assert(TaggedVoiceStatus(&statusObject,&playing,&unused)==S_OK&&playing==1);
 assert(AllocationVoiceCursor(&statusObject,&playing,&unused)==S_OK&&playing==1&&unused==32);
 mockVoiceCursor=900;assert(AllocationVoiceCursor(&statusObject,&playing,&unused)==S_OK&&playing==900);
 mockVoiceStatus=0;assert(AllocationVoiceCursor(&statusObject,&playing,&unused)==S_OK&&playing==0);
 mockVoiceStatus=1;mockVoiceCursor=0;
 TestAutosaveRoundTrip();
 puts("PASS: playing sound status and autosave restore round trip; incompatible level data rejected.");
 nativeCenteredOne=MockCentered;
 assert(RoomOneShot(7,5120,96,(const int*)0x52f300)==96&&centeredCalls==1&&soundVolume==96);
 assert(RoomOneShot(7,5120,200,(const int*)0x52f300)==128&&centeredCalls==2);
 mem<uintptr_t>(0x557704)=1234;drawTokenSprite=MockTokenSprite;DrawTokenIcon(7);assert(mem<uintptr_t>(0x557704)==1234);
 soundRoom=MockRoom;nativeSpatialOne=MockOne;nativeSpatialLoop=MockLoop;nativeSpatialEffect=MockLoop;
 const int* actorSound=(const int*)0x52c840;
 assert(RoomOneShot(1,2,90,actorSound)==90&&soundCalls==1);
 testActorRoom=3;assert(RoomOneShot(1,2,90,actorSound)==0&&soundCalls==1);
 RoomLoop(10,1,2,90,actorSound,1);assert(soundVolume==0&&soundPosition==NULL);
 testActorRoom=2;RoomLoop(10,1,2,90,actorSound,1);assert(soundVolume==90&&soundPosition==actorSound);
 testActorRoom=3;RoomEffect(10,1,2,90,actorSound,1);assert(soundVolume==0&&soundPosition==NULL);
 assert(RoomOneShot(1,2,90,NULL)==90); // non-positional UI path is untouched
 RoomLoop(10,1,2,90,(const int*)0x52f300,1);assert(soundPosition==NULL&&soundVolume==90);
 puts("PASS: local Buzz sound centered, gain/return semantics preserved, Pizza Planet sprite and temporary descriptor restoration.");
 assert(TwiceRangeSquared(10000)==40000);
 assert(TwiceRangeSquared(9900,100)==39900);
 assert(TwiceRangeSquared(0x60000000)==0x7fffffff);
 assert(SameAudioRoom(2,2)&&!SameAudioRoom(2,3)&&!SameAudioRoom(-1,-1));
 assert(IsActorSoundPosition(0x52c840)&&IsActorSoundPosition(0x52c8dc));
 assert(!IsActorSoundPosition(0x52f300)&&!IsActorSoundPosition(0)&&!IsActorSoundPosition(0x52c841));
 puts("PASS: exact 2x radial range, squared-distance bias, overflow bound, actor audio scope and room matching.");
 mem<short>(0x52b818)=1;mem<short>(0x52adb0)=3;
 assert(!strcmp(PauseLabel((const char*)0x5026f0),"quit game"));
 assert(!strcmp(PauseLabel((const char*)0x5027b4),"quit game"));
 mem<short>(0x52b7e4)=0;assert(!ConfirmPauseQuit()&&quitRequests==0);
 mem<short>(0x52b7e4)=1;assert(ConfirmPauseQuit()&&quitRequests==1);
 mem<short>(0x52adb0)=0;assert(!ConfirmPauseQuit());
 mem<short>(0x52b818)=0;assert(!ConfirmPauseQuit());
 assert(PauseLabel((const char*)0x5026f0)==(const char*)0x5026f0);
 // Simulate consistent 0.8ms scheduler delays. Deadlines must not drift by
 // 48ms per second, and a non-divisible QPC frequency must retain its remainder.
 frameFrequency.QuadPart=10000001;framePrevious.QuadPart=100;frameRemainder=0;
 for(int n=0;n<600;n++)AdvanceFrameClock(framePrevious.QuadPart+175000,17500,1);
 assert(framePrevious.QuadPart==100+100000010&&frameRemainder==0);
 AdvanceFrameClock(123456789,1000000,1);assert(framePrevious.QuadPart==123456789);
 framePrevious.QuadPart=0;frameRemainder=0;
 puts("PASS: pause Quit Game requires Yes; other menus unaffected; deadline jitter and long-pause reset.");
 assert(FrameTicks(16666,false)==1&&FrameWaitUs(16666,false)==1);
 for(int us=16667;us<=24000;us++){
  assert(FrameTicks(us,false)==1);assert(FrameWaitUs(us,false)==0);
 }
 assert(FrameTicks(33334,false)==2&&FrameWaitUs(33334,false)==0);
 assert(FrameTicks(50001,false)==3&&FrameTicks(200000,false)==3);
 assert(FrameTicks(1000000,false)==1&&FrameWaitUs(1000000,false)==0);
 assert(FrameTicks(16667,true)==2&&FrameWaitUs(16667,true)==16667);
 assert(FrameTicks(33334,true)==2&&FrameWaitUs(33334,true)==0);
 // Absolute camera heading has no accumulated drift or position parallax.
 for(int heading=-32768;heading<32768;heading+=127){
  SkyUV a=ProjectSky(.5f,.5f,0,heading,.72f,.5625f,1,1);
  SkyUV b=ProjectSky(.5f,.5f,0,heading+65536,.72f,.5625f,1,1);
  assert(fabsf((a.u-b.u)-roundf(a.u-b.u))<.00001f);
  SkyUV c=ProjectSky(.5f,.5f,0,heading-16,.72f,.5625f,1,1);
  float turn=c.u-a.u;turn-=roundf(turn/SkyRepeats(1))*SkyRepeats(1);
  assert(turn>0); // right turn samples further right, including the seam
 }
 SkyUV left=ProjectSky(0,.5f,0,0,.72f,.5625f,1,1);
 SkyUV right=ProjectSky(1,.5f,0,0,.72f,.5625f,1,1);
 assert(left.u<0&&right.u>0&&fabsf(left.u+right.u)<.00001f);
 SkyUV up=ProjectSky(.5f,.5f,4096,0,.72f,.5625f,1,1);
 assert(up.v<.5f);
 assert(SkyPitch(65520)==-16);
 float identity[16]={1,0,0,0,0,1,0,0,0,0,1,0,123,456,789,1};
 SkyRay centre=SkyWorldRay(.5f,.5f,1,1,identity);
 assert(centre.x==0&&centre.y==0&&centre.z==1); // translation never moves the sky
 float yaw90[16]={0,0,-1,0,0,1,0,0,1,0,0,0,0,0,0,1};
 SkyRay turned=SkyWorldRay(.5f,.5f,1,1,yaw90);
 assert(turned.x==1&&turned.y==0&&turned.z==0);
 // A fixed world direction must sample the same texel under camera yaw.
 SkyUV fixed0=SkyRayUV(SkyWorldRay(.75f,.5f,1,1,identity),1,1);
 float angle=.2f,c=cosf(angle),s=sinf(angle);
 float rotated[16]={c,0,-s,0,0,1,0,0,s,0,c,0,0,0,0,1};
 float cameraX=(.5f*c-s)/(.5f*s+c);
 SkyUV fixed1=SkyRayUV(SkyWorldRay((cameraX+1)/2,.5f,1,1,rotated),1,1);
 assert(fabsf(fixed0.u-fixed1.u)<.00001f&&fabsf(fixed0.v-fixed1.v)<.00001f);
 BuildSkyMesh(identity,1,.7f,2560,1440,1,1,.999f);
 for(int n=0;n<SkyIndexCount;n++)assert(skyIndices[n]<SkyVertexCount);
 for(int n=0;n<SkyVertexCount;n++)assert(_finite(skyVertices[n].u)&&_finite(skyVertices[n].v)&&skyVertices[n].v>0&&skyVertices[n].v<1);
 assert(skyVertices[SkyVertexCount-1].x==2560&&skyVertices[SkyVertexCount-1].y==1440);
 puts("PASS: sky camera-basis alignment, fixed-world direction, translation independence, mesh bounds, wrapping and full-turn closure.");
 SavedGame checkpoint={};checkpoint.magic=0x32595354;checkpoint.format=1;checkpoint.bytes=sizeof(checkpoint);
 checkpoint.level=1;checkpoint.health=14;checkpoint.checksum=SaveChecksum(checkpoint);
 assert(ValidSave(checkpoint));checkpoint.position[0]++;assert(!ValidSave(checkpoint));
 checkpoint.checksum=SaveChecksum(checkpoint);assert(ValidSave(checkpoint));
 checkpoint.pickupCount=513;checkpoint.checksum=SaveChecksum(checkpoint);assert(!ValidSave(checkpoint));
 puts("PASS: autosave integrity and bounds; damaged checkpoints rejected.");
 worldRay=MockWorld;cameraOverride=true;mem<int>(0x50a13c)=0;mem<int>(0x88278c)=1;
 mem<int>(0x52f300)=0;mem<int>(0x52f304)=0x1800;mem<int>(0x52f308)=0;
 thirdYaw=3072;thirdPitch=0;rayCameraTime=0;
 int finalCamera[16]={0x8000,0,0};rayCalls=0;wallAt=1;ResolveFinalCamera(finalCamera);
 assert(abs(finalCamera[0]-0x3c00)<=1&&finalCamera[3]==finalCamera[0]);
 int stable=finalCamera[0];finalCamera[0]=123;rayCalls=0;ResolveFinalCamera(finalCamera);
 assert(finalCamera[0]==stable); // legacy collision output cannot feed back
 rayCameraTime=0;rayCalls=0;wallAt=0;ResolveFinalCamera(finalCamera);assert(abs(finalCamera[0]-0x8000)<=1);
 thirdYaw=0;thirdPitch=0;rayCameraTime=0;ResolveFinalCamera(finalCamera);
 assert(finalCamera[0]==0&&finalCamera[2]==-32768);
 thirdYaw=1024;rayCameraTime=0;ResolveFinalCamera(finalCamera);assert(abs(finalCamera[0]+32768)<=1);
 puts("PASS: independent raycast orbit, wall clearance, stable repeated wall contact and cardinal mouse angles.");
 mem<int>(0x88278c)=1;mem<int>(0x50a13c)=0;
 assert(TokenHudOffset()==-1);
 mem<int>(0x50a13c)=4;assert(TokenHudOffset()==-1);
 mem<short>(0x52f2e4)=1024;mem<short>(0x4fe788+2048)=16384;
 assert(TokenHudOffset()==0);
 mem<short>(0x52b818)=1;assert(TokenHudOffset()==-1);mem<short>(0x52b818)=0;
 mem<unsigned char>(0x52b816)=1;assert(TokenHudOffset()==-1);mem<unsigned char>(0x52b816)=0;
 mem<int>(0x88278c)=16;assert(TokenHudOffset()==-1);
 menuHitCount=0;menuArrowY=-1;
 AddMenuHit("continue",160,80,120,12,320,240,true);
 AddMenuHit("exit",160,120,120,12,320,240,false);
 assert(HitMenu(.5f,.5f)==1&&HitMenu(.02f,.02f)==-1);
 assert(MenuActionAt(1,true,false,1000)==0x40); // move highlight before confirming
 menuHits[0].selected=false;menuHits[1].selected=true;
 assert(MenuActionAt(-1,false,false,1100)==0x4000);
 assert(MenuActionAt(-1,false,false,1200)==0); // only one confirmation
 assert(MenuActionAt(-1,false,true,1300)==0x1000);
 menuHitCount=0;replayPrompt=true;replayYes=false;RegisterLevelMouse();
 assert(MenuActionAt(HitMenu(220.f/512,175.f/256),true,false,1400)==0x4000&&replayYes);
 assert(MenuActionAt(HitMenu(292.f/512,175.f/256),true,false,1500)==0x4000&&!replayYes);
 replayPrompt=false;RegisterLevelMouse();
 assert(MenuActionAt(HitMenu(448.f/512,112.f/256),true,false,1600)==0x20);
 menuHitCount=0;menuClickTarget[0]=0;
 memset(fakeMemory,0,sizeof(fakeMemory));
 puts("PASS: aim-only HUD follows collectibles animation; scaled menu hits, delayed confirmation, back, replay and level arrows.");
 mem<unsigned char>(0x52b816)=0x81;mem<unsigned char>(0x52f340)=0x25;
 mem<int>(0x830cc4)=9;mem<int>(0x50a1f4)=1000;
 ContinueAfterToken();
 assert(mem<unsigned char>(0x52b816)==0x80&&mem<unsigned char>(0x52f340)==0x21);
 assert(mem<int>(0x830cc4)==0&&mem<int>(0x50a1f4)==0&&mem<int>(0x50a140)==64&&mem<short>(0x52f38c)==5);
 for(unsigned mask=0;mask<32;++mask){
  mem<unsigned char>(0x52f0d8)=(unsigned char)(mask|0x80);
  unsigned bits=mask;int expected=0;while(bits){expected+=bits&1;bits>>=1;}
  assert(TokenCount(1)==expected);
 }
 assert(TokenCount(0)==0&&TokenCount(16)==0);
 mem<unsigned char>(0x52f0dd)=0x80;assert(TokenCount(6)==1);
 mem<unsigned char>(0x52f0dd)=0;assert(TokenCount(6)==0);
 puts("PASS: token continuation restores gameplay and counter counts all 32 token masks correctly.");
 mem<int>(0x88278c)=1;mem<int>(0x830ca8)=15;mem<int>(0x830cac)=0;
 mem<unsigned char>(0x52f0d8)=15;assert(!TokenExitDecision());
 mem<unsigned char>(0x52f0d8)=31;assert(TokenExitDecision());
 mem<int>(0x830ca8)=31;assert(!TokenExitDecision()); // replay stays playable
 mem<int>(0x830cac)=1;assert(TokenExitDecision()); // preserve scripted exit
 mem<unsigned char>(0x52f0d8)=15;assert(!TokenExitDecision()); // no early forced exit
 mem<unsigned char>(0x52f0d8)=31;
 mem<int>(0x830cac)=0;
 assert(!LevelComplete(0)&&!LevelComplete(16));
 mem<unsigned char>(0x52f0dd)=128;assert(LevelComplete(6));
 mem<int>(0x502694)=6;assert(MenuLevelId(3)==6&&!MenuLevelId(0));
 assert(FilterReplayInput(1,0,61,0x4000)==0&&replayPrompt&&!replayYes);
 assert(FilterReplayInput(1,0,62,0x4000)==0&&replayPrompt); // held key
 FilterReplayInput(1,0,63,0);
 assert(FilterReplayInput(1,0,64,0x4000)==0&&!replayPrompt); // default No
 assert(FilterReplayInput(1,0,65,0x4000)==0&&!replayPrompt);
 FilterReplayInput(1,0,66,0);
 FilterReplayInput(1,0,67,0x4000);assert(replayPrompt);
 FilterReplayInput(1,0,68,0x20);assert(replayYes);
 assert(FilterReplayInput(1,0,69,0x4000)==0x4000&&!replayPrompt);
 FilterReplayInput(1,1,70,0); // fade-out cannot reopen prompt
 assert(FilterReplayInput(1,1,71,0x4000)==0x4000&&!replayPrompt);
 FilterReplayInput(1,0,72,0);FilterReplayInput(1,0,73,0x4000);
 assert(FilterReplayInput(1,0,74,0x1000)==0&&!replayPrompt);
 FilterReplayInput(2,0,75,0);
 assert(FilterReplayInput(2,0,76,0x4000)==0x4000&&!replayPrompt);
 nativeDeath=MockDeath;
 mem<short>(0x52c840+0x7a)=600;mem<short>(0x52c840+0x14)=3;
 DefeatedActor(0x52c840,1);assert(mem<short>(0x52c840+0x7a)==0&&deathCalls==1);
 DefeatedActor(0x52c840,2);assert(mem<short>(0x52c840+0x14)==0&&deathCalls==2);
 immediateDeath=true;mem<short>(0x52c840+0x14)=3;
 DefeatedActor(0x52c840,1);assert(mem<short>(0x52c840+0x14)==0);
 memset(fakeMemory,0,sizeof(fakeMemory));
 puts("PASS: final-token exit, playable completed levels, replay Yes/No and held-key debounce, permanent actor retirement.");
 assert(!KeyboardRetryDue(false,false,1000));
 assert(!KeyboardRetryDue(true,true,1000));
 assert(KeyboardRetryDue(false,true,1000));
 assert(!KeyboardRetryDue(false,true,1200));
 assert(KeyboardRetryDue(false,true,1500));
 assert(!KeyboardRetryDue(true,true,2000));
 keyboardRetryTime=0xfffffff0u;keyboardRetryStarted=true;
 assert(!KeyboardRetryDue(false,true,0x10));
 assert(KeyboardRetryDue(false,true,0x200));
 int* b=&mem<int>(0x529b10);
 const int masks[]={0x10,0x40,0x80,0x20,0x400,0x8000,0x8,0x2000};
 const int keys[]={DIK_UP,DIK_DOWN,DIK_LEFT,DIK_RIGHT,DIK_TAB,DIK_LCONTROL,DIK_RETURN,DIK_SPACE};
 for(int i=0;i<38;i++){b[i*2]=-1;b[i*2+1]=0;}
 for(int i=0;i<8;i++){b[i*2]=keys[i];b[i*2+1]=masks[i];}
 mem<unsigned char>(0x529a00+DIK_UP)=0x80;
 mem<unsigned char>(0x529a00+DIK_ESCAPE)=0x80;
 RemapMovement();
 assert(b[0]==DIK_W&&b[2]==DIK_S&&b[4]==DIK_A&&b[6]==DIK_D);
 assert(b[8]==DIK_TAB&&b[10]==DIK_F&&b[14]==DIK_LCONTROL);
 assert(mem<unsigned char>(0x529a00+DIK_W)==0x80);
 assert(mem<unsigned char>(0x529a00+DIK_RETURN)==0x80);
 SetAction(0x8000);assert(mem<unsigned char>(0x529a00+DIK_F)==0x80);
 assert(mem<unsigned char>(0x529a00+DIK_LCONTROL)==0);
 SetAction(0x2000);assert(mem<unsigned char>(0x529a00+DIK_LCONTROL)==0x80);
 SetAction(0x400);assert(mem<unsigned char>(0x529a00+DIK_TAB)==0x80);
 dx=dy=0;lastMouseMode=-1;mem<int>(0x50a13c)=0;sensitivity=5;
 AccumulateMouse(5,-3);AccumulateMouse(0,0);AccumulateMouse(2,1);assert(dx==7&&dy==-2);
 dx=dy=0;sensitivity=1;for(int i=0;i<5;i++)AccumulateMouse(1,-1);assert(dx==1&&dy==-1);
 dx=dy=0;aimSensitivity=5;mem<int>(0x50a13c)=4;AccumulateMouse(3,2);assert(dx==3&&dy==2);
 dx=dy=0;aimSensitivity=1;for(int i=0;i<5;i++)AccumulateMouse(-1,1);assert(dx==-1&&dy==1);
 dx=dy=0;mem<int>(0x50a13c)=0;sensitivity=10;AccumulateMouse(3,2);assert(dx==6&&dy==4);
 puts("PASS: old minimum equals new 5/10; fractional input retained for both sensitivity modes.");
 unsigned char camera[64]={};active=true;enabled=true;dx=10;dy=20;
 *(short*)(camera+0x28)=5;mem<unsigned char>(0x52f0cc)=0x41;
 Third(camera);assert(*(short*)(camera+0x28)==15);assert(*(short*)(camera+0x2e)==20);assert(mem<unsigned char>(0x52f0cc)==1);assert(dx==0&&dy==0);
 *(short*)(camera+0x28)=600;*(short*)(camera+0x2e)=64;
 Third(camera);assert(*(short*)(camera+0x28)==15);assert(*(short*)(camera+0x2e)==20); // Ignore native auto-centering.
 for(int n=0;n<20;n++){dy=10000;Third(camera);}assert(*(short*)(camera+0x2e)==512);
 for(int n=0;n<20;n++){dy=-10000;Third(camera);}assert(*(short*)(camera+0x2e)==-256);
 *(short*)(camera+0x28)=100;*(short*)(camera+0x2e)=30;
 AcceptCameraCollision(camera);Third(camera);
 assert(*(short*)(camera+0x28)==15&&*(short*)(camera+0x2e)==-256);
 for(int n=0;n<120;n++){
  *(short*)(camera+0x28)=(short)(n%2?100:4000);*(short*)(camera+0x2e)=30;
  AcceptCameraCollision(camera);Third(camera);
  assert(*(short*)(camera+0x28)==15&&*(short*)(camera+0x2e)==-256);
 }
 dx=10000;Third(camera);assert(*(short*)(camera+0x28)==111);
 unsigned short npcFlags=0x241;
 interactPending=false;assert(NpcContact(npcFlags,&npcFlags)==0x41&&npcFlags==0x41);
 npcFlags=0x241;interactPending=true;interactTime=GetTickCount();
 assert(NpcContact(npcFlags,&npcFlags)==0x241&&!interactPending);
 assert(NpcContact(npcFlags,&npcFlags)==0x41);
 npcFlags=0x241;interactPending=true;interactTime=GetTickCount()-300;
 assert(NpcContact(npcFlags,&npcFlags)==0x41);
 npcFlags=0x241;interactPending=true;interactTime=GetTickCount();mem<unsigned short>(0x52b816)=1;
 assert(NpcContact(npcFlags,&npcFlags)==0x41);mem<unsigned short>(0x52b816)=0;
 mem<short>(0x52f30e)=4090;*(int*)(camera+0xc)=0;dx=10;dy=-10000;
 Aim(camera);assert(mem<short>(0x52f30e)==4);assert(SignedAngle(*(int*)(camera+0xc))==-824);
 dy=10000;Aim(camera);assert(*(int*)(camera+0xc)==800);
 enabled=false;short yaw=mem<short>(0x52f30e);dx=20;Aim(camera);assert(mem<short>(0x52f30e)==yaw);
 unsigned char player[160]={};int movementParameters[]={128,48,40,-1536,896,1024};
 *(short*)(player+0x8e)=1;
 enabled=true;active=true;mem<int>(0x50a13c)=4;mem<unsigned>(0x53c828)=0x101;
 mem<short>(0x4fe788)=0;mem<short>(0x4fe788+1024*2)=16384;
 mem<short>(0x4fe788+2048*2)=0;mem<short>(0x52f30e)=0;
 *(int*)(player+0x6c)=123;
 mem<unsigned short>(0x52ad88)=0x10;
 for(int n=0;n<30;n++){
  // Reproduce the native visor friction before every physics update.
  *(int*)(player+0x68)=Approach(*(int*)(player+0x68),0,128);
  *(int*)(player+0x70)=Approach(*(int*)(player+0x70),0,48);
  AimMovement(player,movementParameters);
 }
 assert(*(int*)(player+0x68)==0&&*(int*)(player+0x70)==1024);
 assert(*(int*)(player+0x6c)==123&&mem<short>(0x52f30e)==0);
 mem<unsigned short>(0x52ad88)=0x20;
 dx=dy=0;
 for(int n=0;n<30;n++){
  // Reproduce the native visor friction before every physics update.
  *(int*)(player+0x68)=Approach(*(int*)(player+0x68),0,128);
  *(int*)(player+0x70)=Approach(*(int*)(player+0x70),0,48);
  AimMovement(player,movementParameters);
 }
 assert(*(int*)(player+0x68)==1024&&*(int*)(player+0x70)==0);
 mem<unsigned short>(0x52ad88)=0x50;
 for(int n=0;n<30;n++){
  // Reproduce the native visor friction before every physics update.
  *(int*)(player+0x68)=Approach(*(int*)(player+0x68),0,128);
  *(int*)(player+0x70)=Approach(*(int*)(player+0x70),0,48);
  AimMovement(player,movementParameters);
 }
 assert(*(int*)(player+0x68)==0&&*(int*)(player+0x70)==0);
 mem<unsigned short>(0x52ad88)=0x30;
 for(int n=0;n<30;n++){
  // Reproduce the native visor friction before every physics update.
  *(int*)(player+0x68)=Approach(*(int*)(player+0x68),0,128);
  *(int*)(player+0x70)=Approach(*(int*)(player+0x70),0,48);
  AimMovement(player,movementParameters);
 }
 assert(*(int*)(player+0x68)==724&&*(int*)(player+0x70)==724);
 mem<unsigned>(0x53c828)=0x105;mem<unsigned short>(0x52ad88)=0x40;
 AimMovement(player,movementParameters);assert(*(int*)(player+0x70)==724);
 mem<unsigned>(0x53c828)=0x101;mem<short>(0x52f30e)=1024;
 for(int n=0;n<30;n++){
  // Reproduce the native visor friction before every physics update.
  *(int*)(player+0x68)=Approach(*(int*)(player+0x68),0,128);
  *(int*)(player+0x70)=Approach(*(int*)(player+0x70),0,48);
  AimMovement(player,movementParameters);
 }
 assert(*(int*)(player+0x68)==-1024&&*(int*)(player+0x70)==0);
 assert(ScaleVolume(60,0)==0&&ScaleVolume(60,50)==30&&ScaleVolume(60,100)==60);
 assert(ScaleVolume(120,25)==30);
 // Sustained A/D must retain one camera-relative direction despite model yaw.
 mem<int>(0x50a13c)=0;mem<unsigned>(0x53c828)=1;
 mem<unsigned short>(0x52b816)=0;pitchInitialized=true;thirdYaw=0;
 mem<short>(0x4fe788+3072*2)=-16384;
 mem<unsigned short>(0x52ad88)=0x20;
 for(int n=0;n<120;n++){
  lastThird=GetTickCount();
  *(short*)(player+0xe)=(short)(n*30); // simulate native steering
  AimMovement(player,movementParameters);
  assert(*(short*)(player+0xe)==1024);
  assert(*(int*)(player+0x70)==0&&*(int*)(player+0x68)>0);
 }
 assert(*(int*)(player+0x68)==1024&&*(int*)(player+0x78)==0);
 // A brief loss of ground contact preserves speed and momentum.
 *(short*)(player+0x8e)=0;
 for(int n=0;n<4;n++)AimMovement(player,movementParameters);
 assert(runSpeed==1024&&*(int*)(player+0x68)==1024);
 *(short*)(player+0x8e)=1;AimMovement(player,movementParameters);
 assert(*(int*)(player+0x68)==1024);
 mem<unsigned short>(0x52ad88)=0x80;
 lastThird=GetTickCount();AimMovement(player,movementParameters);
 assert(*(short*)(player+0xe)==3072&&*(int*)(player+0x68)==-1024);
 runHeld=false;thirdYaw=1024;mem<unsigned short>(0x52ad88)=0x10;
 lastThird=GetTickCount();AimMovement(player,movementParameters);
 assert(*(short*)(player+0xe)==1024&&*(int*)(player+0x68)==1024);
 // Fresh mouse input is used immediately, before the next camera update.
 dx=64;AimMovement(player,movementParameters);
 assert(*(short*)(player+0xe)==1088);dx=0;
 thirdYaw=2048;AimMovement(player,movementParameters);
 assert(*(short*)(player+0xe)==2048&&*(int*)(player+0x70)==-1024);
 assert(*(int*)(player+0x6c)==123);
 assert(DirectionOffset(1,1)==512&&DirectionOffset(-1,-1)==2560);
 mem<unsigned>(0x53c828)=5;int savedVx=*(int*)(player+0x68);
 AimMovement(player,movementParameters);assert(*(int*)(player+0x68)==savedVx);
 // Model faces elsewhere; entering aim must use the last resolved free camera.
 pitchInitialized=true;thirdYaw=1234;thirdPitch=-128;lastThird=GetTickCount();
 mem<short>(0x52f30e)=3000;StartAim(camera);
 assert(mem<short>(0x52f30e)==1234&&mem<int>(0x50a290)==1234);
 assert(SignedAngle(*(int*)(camera+0xc))==-128&&SignedAngle(mem<int>(0x50a28c))==-128);
 mem<int>(0x50a13c)=0;mem<unsigned>(0x53c828)=1;
 *(int*)(player+0x68)=2200;*(int*)(player+0x70)=1400;*(int*)(player+0x6c)=-3000;
 lastThird=GetTickCount();AimMovement(player,movementParameters);
 assert(*(int*)(player+0x68)==2200&&*(int*)(player+0x70)==1400&&*(int*)(player+0x6c)==-3000);
 *(short*)(player+0x8e)=0;*(int*)(player+0x68)=600;*(int*)(player+0x70)=300;
 AimMovement(player,movementParameters);
 assert(*(int*)(player+0x68)==600&&*(int*)(player+0x70)==300);
 mem<int>(0x50a13c)=4;AimMovement(player,movementParameters);
 assert(*(int*)(player+0x68)==600&&*(int*)(player+0x70)==300);
 musicVolume=20;MusicVolume(100);
 for(int n=0;n<100;n++)MusicVolume(mem<int>(0x5282c8));
 assert(lastMusicOutput==20&&mem<int>(0x5282c8)==100);
 musicVolume=0;MusicVolume(100);assert(lastMusicOutput==0&&mem<int>(0x5282c8)==100);
 assert(NearbyNpc(1,0,0,0));
 assert(!NearbyNpc(0,0,0,0)&&!NearbyNpc(1,0x10000,0,0)&&!NearbyNpc(1,0,0x7000,0));
 puts("PASS: fresh mouse heading, uninterrupted speed across contact gaps, preserved launch momentum, stable music, hint proximity.");
 // Boss tracking must not re-enter the camera's later look-at pass.
 active=enabled=true;mem<unsigned>(0x50a118)=123;Third(camera);
 assert(mem<unsigned>(0x50a118)==0x80000000u);
 worldRay=MockWorld;actorRay=MockActors;extendedShot=true;shotBeam=0x580000;shotDamage=1;
 mem<uintptr_t>(0x52f1d0)=0x52c840;mem<uintptr_t>(0x52f1d4)=0x52c8dc;mem<uintptr_t>(0x52f1d8)=0;
 mem<int>(0x52c840)=500;mem<int>(0x52c8dc)=900000;
 int start[3]={0,0,0},delta[3]={131072,0,0};
 wallAt=3;enemyAt=0;rayCalls=0;
 assert(ExtendedWorldRay(start,delta,0x8000,0,256)==1&&rayCalls==3);
 assert(delta[0]==327680&&mem<int>(shotBeam+12)==327680);
 assert(mem<uintptr_t>(0x52f1d0)==0x52c840&&mem<uintptr_t>(0x52f1d4)==0x52c8dc&&mem<uintptr_t>(0x52f1d8)==0);
 wallAt=0;enemyAt=4;rayCalls=0;delta[0]=131072;
 assert(ExtendedWorldRay(start,delta,0x8000,0,256)==0&&rayCalls==4&&delta[0]==458752);
 assert(ExtendedActorRay(start,delta,1)==4); // damage must not run twice
 wallAt=enemyAt=0;rayCalls=0;delta[0]=131072;
 ExtendedWorldRay(start,delta,0x8000,0,256);assert(rayCalls>4&&rayCalls<=64);
 assert(delta[0]==rayCalls*131072&&mem<int>(shotBeam+12)==delta[0]);
 extendedShot=false;
 puts("PASS: boss camera target discarded; distant laser hits, first-wall stopping, actor-list restoration, miss endpoints, no duplicate damage.");
 puts("PASS: manual NPC gates, stable camera-relative running, special-state protection, freelook-to-aim orientation.");
 puts("PASS: keyboard, mouse cameras, visor movement directions, diagonal speed, opposing keys, preserved vertical physics and special states.");
 // Exercise the actual Windows timer without loading or running the game.
 QueryPerformanceFrequency(&frameFrequency);framePrevious.QuadPart=0;
 mem<int>(0x52ad94)=0;
 LARGE_INTEGER begin,end;QueryPerformanceCounter(&begin);
 int twoTickFrames=0;
 for(int n=0;n<60;n++){PaceFrame(1);if(mem<int>(0x52f2d4)>1)++twoTickFrames;}
 QueryPerformanceCounter(&end);
 printf("Timer smoke check: 60 frames in %.2f ms, %d frames over one simulation tick.\n",
  (end.QuadPart-begin.QuadPart)*1000.0/frameFrequency.QuadPart,twoTickFrames);
}
