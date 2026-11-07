#define _CRT_SECURE_NO_WARNINGS
#define WIN32_LEAN_AND_MEAN
#define DIRECTINPUT_VERSION 0x0500
#include <windows.h>
#include <dinput.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static uintptr_t base;
static DWORD inputResume, thirdResume, aimResume;
static DWORD inputEntryResume=0x414af5, alternateThirdResume=0x404ab4;
static DWORD aimStartResume=0x403996;
static int localTestSlot;
static int runSpeed;
static bool runHeld;
static DWORD npcHintTime,cubeHintTime;
static DWORD keyboardRetryTime;
static bool keyboardRetryStarted;
static DWORD movementResume=0x436a3b;
static DWORD cameraLateResume=0x4053b9, recenterResume=0x4052c7, recenterSkip=0x405357;
static bool cameraOverride, eWasDown, interactPending;
static DWORD interactTime;
static LPDIRECTINPUTDEVICEA mouse;
static volatile DWORD lastCamera;
static volatile bool active, enabled=true;
static bool f8WasDown;
static LONG dx,dy;
static int mouseRemainderX,mouseRemainderY,lastMouseGain,lastMouseMode=-1;
static int sensitivity=2, aimSensitivity=2, invertY=0;
static int musicVolume=100, sfxVolume=100;
static int aimForward,aimRight;
static DWORD lastMovement;
static int thirdPitch, thirdYaw;
static bool pitchInitialized;
static DWORD lastThird;
static char logPath[MAX_PATH];
static unsigned inputCount, thirdCount, aimCount;

#ifdef CONTROLS_TEST
static unsigned char fakeMemory[0x500000];
template<class T> static T& mem(uintptr_t va) { return *reinterpret_cast<T*>(fakeMemory+va-0x400000); }
#else
template<class T> static T& mem(uintptr_t va) { return *reinterpret_cast<T*>(base+va-0x400000); }
#endif
static void Log(const char* message) { FILE* f=fopen(logPath,"a"); if(f){fprintf(f,"%s\n",message);fclose(f);} }
static int Clamp(int n,int low,int high){return n<low?low:n>high?high:n;}
static int SignedAngle(int n){n&=4095;return n>=2048?n-4096:n;}
static void AccumulateMouse(LONG x,LONG y){
 int mode=mem<int>(0x50a13c)==4?1:0,gain=mode?aimSensitivity:sensitivity;
 if(mode!=lastMouseMode||gain!=lastMouseGain){mouseRemainderX=mouseRemainderY=0;lastMouseMode=mode;lastMouseGain=gain;}
 // 5/10 is the former minimum. Carry sub-unit motion between polls so
 // 1/10 remains responsive to individual mouse counts instead of truncating.
 int x5=Clamp(x,-400,400)*gain+mouseRemainderX;
 int y5=Clamp(y,-400,400)*gain*(invertY?-1:1)+mouseRemainderY;
 dx=Clamp(dx+x5/5,-2048,2048);dy=Clamp(dy+y5/5,-2048,2048);
 mouseRemainderX=x5%5;mouseRemainderY=y5%5;
}
static HWND GameWindow(){return mem<HWND>(0x5344b4);}
static bool Focused(){
#ifdef CONTROLS_TEST
return true;
#else
HWND w=GameWindow(); return w && GetForegroundWindow()==w;
#endif
}
static bool KeyboardRetryDue(bool ready,bool focused,DWORD now){
 if(ready||!focused)return false;
 if(keyboardRetryStarted&&now-keyboardRetryTime<500)return false;
 keyboardRetryStarted=true;keyboardRetryTime=now;return true;
}
static void __cdecl RecoverKeyboard(){
 // The original initializer permanently disables polling when its first
 // foreground Acquire fails. Retry on the input thread once focus is ready.
 if(!KeyboardRetryDue(mem<int>(0x529d3c)!=0,Focused(),GetTickCount()))return;
 LPDIRECTINPUTDEVICEA keyboard=mem<LPDIRECTINPUTDEVICEA>(0x529c9c);
 if(!keyboard)return;
 HRESULT acquired=keyboard->Acquire();
 if(FAILED(acquired)){
  if(FAILED(keyboard->SetDataFormat((LPCDIDATAFORMAT)0x4a8470)))return;
  if(FAILED(keyboard->SetCooperativeLevel(GameWindow(),DISCL_FOREGROUND|DISCL_NONEXCLUSIVE)))return;
  acquired=keyboard->Acquire();
 }
 if(SUCCEEDED(acquired)){
  mem<int>(0x529d3c)=1;
  Log("Startup keyboard recovered after foreground acquisition retry.");
 }
}
static void ReleaseMouse(){if(mouse)mouse->Unacquire();if(active)ClipCursor(NULL);active=false;dx=dy=0;mouseRemainderX=mouseRemainderY=0;}
static bool CreateMouse(){
 if(mouse)return true;
 LPDIRECTINPUTA di=mem<LPDIRECTINPUTA>(0x529c94);
 if(!di||!GameWindow())return false;
 if(FAILED(di->CreateDevice(GUID_SysMouse,&mouse,NULL)))return false;
 if(FAILED(mouse->SetDataFormat(&c_dfDIMouse)) || FAILED(mouse->SetCooperativeLevel(GameWindow(),DISCL_FOREGROUND|DISCL_NONEXCLUSIVE))){mouse->Release();mouse=NULL;return false;}
 Log("DirectInput mouse initialized.");return true;
}
static void SetAction(unsigned mask){
 int* bindings=&mem<int>(0x529b10); unsigned char* keys=&mem<unsigned char>(0x529a00);
 for(int i=0;i<38;i++)if((unsigned)bindings[i*2+1]==mask && bindings[i*2]>=0 && bindings[i*2]<256)keys[bindings[i*2]]=0x80;
}
static void RemapMovement(){
 int* bindings=&mem<int>(0x529b10);
 for(int i=0;i<38;i++){
  int& key=bindings[i*2]; if(key<0||key>=256)continue;
  switch(bindings[i*2+1]){case 0x10:key=DIK_W;break;case 0x40:key=DIK_S;break;case 0x80:key=DIK_A;break;case 0x20:key=DIK_D;break;case 0x8000:key=DIK_F;break;case 0x2000:key=DIK_LCONTROL;break;}
 }
 unsigned char* keys=&mem<unsigned char>(0x529a00);
 keys[DIK_W]|=keys[DIK_UP];keys[DIK_S]|=keys[DIK_DOWN];keys[DIK_A]|=keys[DIK_LEFT];keys[DIK_D]|=keys[DIK_RIGHT];
 if(keys[DIK_ESCAPE])keys[DIK_RETURN]=0x80;
}
static bool MouseMenusActive();
static bool ConsolePoll();
static void DrawGameConsole();
static int consoleLevelRequest;
static void ClearInactiveInput(){
 memset(&mem<unsigned char>(0x529a00),0,256);mem<unsigned short>(0x52ad88)=0;interactPending=false;eWasDown=false;ReleaseMouse();
}
static bool ropeJumpWasDown;
static bool RopeAttached(){return mem<int>(0x53c668)>0;}
static void RopeJumpOff(bool jump){
 bool pressed=jump&&!ropeJumpWasDown;ropeJumpWasDown=jump;
 if(!pressed||!RopeAttached()||mem<short>(0x52b818)||(mem<unsigned short>(0x52b816)&1))return;
 // Match native 435922..4359b3. Leave via the detached (-1) state so the
 // rope cannot immediately capture Buzz again. A stale attach cooldown must
 // not swallow a fresh jump press while the player is already on the rope.
 int yaw=mem<unsigned short>(0x52f30e)&4095;
 mem<int>(0x52f36c)=-1024;mem<int>(0x52f368)=mem<short>(0x4fe788+yaw*2)/2;
 mem<int>(0x52f370)=mem<short>(0x4fe788+((yaw+1024)&4095)*2)/2;
 mem<short>(0x52f38c)=1;mem<short>(0x52f390)=2;mem<short>(0x52f38e)=mem<short>(0x52f39c)=0;
 mem<int>(0x53c61c)=0;mem<int>(0x53c668)=-1;mem<int>(0x53c658)=12;
 mem<short>(0x52f348)=mem<short>(0x52f30e);runSpeed=0;
}
static void __cdecl Input(){
 if(ConsolePoll()){ClearInactiveInput();return;}
 if(localTestSlot&&!Focused()){ClearInactiveInput();return;}
 RopeJumpOff((mem<unsigned char>(0x529a00+DIK_SPACE)&0x80)!=0);
 RemapMovement();
 bool eDown=(mem<unsigned char>(0x529a00+DIK_E)&0x80)!=0;
 if(Focused()&&eDown&&!eWasDown){interactPending=true;interactTime=GetTickCount();}
 if(!Focused())interactPending=false;
 eWasDown=eDown;
 if(++inputCount==1)Log("Keyboard hook active: WASD movement and Escape menu enabled.");
 bool f8=(GetAsyncKeyState(VK_F8)&0x8000)!=0;
 if(Focused() && f8 && !f8WasDown){enabled=!enabled;Log(enabled?"Mouse look enabled.":"Mouse look disabled (F8).");}
 f8WasDown=f8;
 if(!Focused()||!enabled){ReleaseMouse();return;}
 if(MouseMenusActive()){ReleaseMouse();return;}
 if(!CreateMouse())return;
 mouse->Acquire();
 DIMOUSESTATE state={};
 if(FAILED(mouse->GetDeviceState(sizeof(state),&state))){dx=dy=0;return;}
 bool inCamera=(GetTickCount()-lastCamera)<150;
 active=inCamera;
 if(!inCamera){dx=dy=0;return;}
 // Input can be polled several times per camera update. Empty polls must
 // not erase movement that the camera has not yet consumed.
 AccumulateMouse(state.lX,state.lY);
 // Visor is a native toggle: holding RMB produces only its initial rising edge.
 if((state.rgbButtons[1]&0x80)&&!RopeAttached())SetAction(0x400);
 if(state.rgbButtons[0]&0x80)SetAction(0x8000);
 // Keep the cursor inside the game only during camera-controlled gameplay.
 RECT r; if(GetClientRect(GameWindow(),&r)){POINT a={r.left,r.top},b={r.right,r.bottom};ClientToScreen(GameWindow(),&a);ClientToScreen(GameWindow(),&b);RECT clip={a.x,a.y,b.x,b.y};ClipCursor(&clip);}
 SetCursor(NULL);
}
static void __cdecl Third(void* camera){
 DWORD now=GetTickCount();lastCamera=now;
 if(++thirdCount==1)Log("Third-person camera hook active.");
 cameraOverride=active&&Focused()&&enabled;
 if(!cameraOverride){pitchInitialized=false;return;}
 // Boss scripts supply a look-at target each frame. Discard the camera target
 // before the later steering pass; combat scripts and boss state stay intact.
 mem<unsigned>(0x50a118)=0x80000000u;
 unsigned char* c=(unsigned char*)camera;
 short& yaw=*(short*)(c+0x28);short& pitch=*(short*)(c+0x2e);
 if(!pitchInitialized || now-lastThird>500){thirdPitch=SignedAngle(pitch);thirdYaw=yaw&4095;pitchInitialized=true;}
 lastThird=now;
 // Passive camera leaves the orbit independent of Buzz's facing direction.
 mem<unsigned char>(0x52f0cc)&=~0x40;
 // Limit individual orbit steps so a fast mouse swipe cannot jump across walls.
 int mouseYaw=Clamp(dx,-96,96);
 thirdYaw=(thirdYaw+mouseYaw)&4095;
 yaw=(short)thirdYaw;
 thirdPitch=Clamp(thirdPitch+Clamp(dy,-64,64),-256,512);
 // This field is signed; the native camera compares it to -512 and +768.
 pitch=(short)thirdPitch;
 dx=dy=0;
}
static void __cdecl AcceptCameraCollision(void* camera){
 if(!cameraOverride)return;
 (void)camera;
 // Collision moves the camera position, never the user's requested orbit.
 // Feeding collision angles back into mouse input makes a stationary wall
 // push the orbit back and forth on successive frames.
}
static bool NearbyNpc(unsigned flags,int x,int y,int z){
 int64_t rx=(int64_t)x-mem<int>(0x52f300),ry=(int64_t)y-mem<int>(0x52f304),rz=(int64_t)z-mem<int>(0x52f308);
 return (flags&1)&&ry>-0x6000&&ry<0x6000&&rx*rx+rz*rz<(int64_t)0xc000*0xc000;
}
static unsigned __cdecl NpcContact(unsigned flags,unsigned short* actorFlags){
 uintptr_t va=(uintptr_t)actorFlags;
 bool nearby=false;
 if(va>=0x52c880&&va<0x52ef54)
  nearby=NearbyNpc(flags,mem<int>(va-0x40),mem<int>(va-0x3c),mem<int>(va-0x38));
 if(nearby)npcHintTime=GetTickCount();
 if(!(flags&0x200)&&!nearby)return flags;
 bool allow=interactPending&&Focused()&&(GetTickCount()-interactTime<=250)&&!(mem<unsigned short>(0x52b816)&1);
 if(allow){interactPending=false;npcHintTime=0;return flags|0x200;}
 *actorFlags&=~0x200; // Discard contact now; don't save a remote interaction for later.
 return flags&~0x200;
}
static void __cdecl Aim(void* camera){
 lastCamera=GetTickCount();pitchInitialized=false;
 if(++aimCount==1)Log("First-person aim camera hook active.");
 if(!active||!Focused()||!enabled)return;
 int& pitch=*(int*)((unsigned char*)camera+0xc);
 short& yaw=mem<short>(0x52f30e);
 yaw=(short)((yaw+dx)&4095);
 mem<short>(0x52f348)=yaw;
 pitch=Clamp(SignedAngle(pitch)+dy,-824,800)&4095;
 dx=dy=0;
}
static void __cdecl StartAim(void* camera){
 if(!pitchInitialized||GetTickCount()-lastThird>500||!enabled||!Focused())return;
 unsigned char* c=(unsigned char*)camera;
 int yaw=thirdYaw&4095, pitch=Clamp(thirdPitch,-824,800)&4095;
 mem<short>(0x52f30e)=mem<short>(0x52f348)=(short)yaw;
 *(int*)(c+0xc)=pitch;
 *(short*)(c+0x20)=(short)pitch;*(short*)(c+0x22)=(short)yaw;
 mem<int>(0x50a28c)=pitch;mem<int>(0x50a290)=yaw;
 runSpeed=0;
}
// Only the visor camera loses directional input; physics still receives WASD.
static void __cdecl MouseOnlyAim(void* camera){
 unsigned short saved=mem<unsigned short>(0x52ad88);
 mem<unsigned short>(0x52ad88)=saved&~0xf0;
 mem<int>(0x50a130)=mem<int>(0x50a138)=0;
 ((void(__cdecl*)(void*))0x4038e0)(camera);
 mem<unsigned short>(0x52ad88)=saved;
}
static int Approach(int value,int target,int step){return value<target?min(value+step,target):max(value-step,target);}
static int DirectionOffset(int forward,int right){
 if(forward>0)return right>0?512:right<0?3584:0;
 if(forward<0)return right>0?1536:right<0?2560:2048;
 return right>0?1024:3072;
}
static bool NativeMomentum(void* player){
 unsigned char* c=(unsigned char*)player;
 // 8e is the native ground-contact field. A negative Y speed is upward.
 return !*(short*)(c+0x8e)||*(int*)(c+0x6c)<-128||
  *(int*)(c+0x68)>1100||*(int*)(c+0x68)<-1100||
  *(int*)(c+0x70)>1100||*(int*)(c+0x70)<-1100;
}
static void __cdecl RunMovement(void* player,const int* parameters){
 if(mem<int>(0x50a13c)!=0||!active||!enabled||!Focused()||!pitchInitialized||
    GetTickCount()-lastThird>500||(mem<unsigned>(0x53c828)&~0x81u)||
    (mem<unsigned short>(0x52b816)&1)) {runSpeed=0;runHeld=false;return;}
 // A one-frame ground-contact gap must not restart the acceleration ramp.
 // Keep horizontal impulses untouched while airborne or externally boosted.
 if(RopeAttached()||mem<int>(0x53c66c)||NativeMomentum(player))return;
 unsigned pad=mem<unsigned short>(0x52ad88);
 int forward=((pad&0x10)?1:0)-((pad&0x40)?1:0);
 int right=((pad&0x20)?1:0)-((pad&0x80)?1:0);
 unsigned char* c=(unsigned char*)player;
 int yaw=*(unsigned short*)(c+0xe)&4095;
 if(forward||right){
  runHeld=true;
  yaw=(thirdYaw+Clamp(dx,-96,96)+DirectionOffset(forward,right))&4095;
 }else runHeld=false;
 runSpeed=Approach(runSpeed,(forward||right)?Clamp(parameters[5],0,1024):0,Clamp(parameters[2]+parameters[1],1,1024));
 *(short*)(c+0xe)=(short)yaw;*(short*)(c+0x48)=(short)yaw;
 *(int*)(c+0x68)=runSpeed*mem<short>(0x4fe788+yaw*2)/16384;
 *(int*)(c+0x70)=runSpeed*mem<short>(0x4fe788+((yaw+1024)&4095)*2)/16384;
 *(int*)(c+0x74)=runSpeed;*(int*)(c+0x78)=0;
 // Only horizontal locomotion changes. Native vertical physics/collision follow.
}
static void __cdecl AimMovement(void* player,const int* parameters){
 RunMovement(player,parameters);
 if(mem<int>(0x50a13c)!=4 || !active || !enabled || !Focused()){aimForward=aimRight=0;lastMovement=0;return;}
 if(RopeAttached()||mem<int>(0x53c66c)||NativeMomentum(player)){aimForward=aimRight=0;lastMovement=0;return;}
 // Do not override knockback, climbing, scripted movement, or other special states.
 if(mem<unsigned>(0x53c828)&~0x181u){aimForward=aimRight=0;lastMovement=0;return;}
 unsigned pad=mem<unsigned short>(0x52ad88);
 int forward=((pad&0x10)?1:0)-((pad&0x40)?1:0);
 int right=((pad&0x20)?1:0)-((pad&0x80)?1:0);
 int speed=Clamp(parameters[5],0,1024);
 if(forward&&right)speed=speed*181/256;
 DWORD now=GetTickCount();
 if(!lastMovement||now-lastMovement>250)aimForward=aimRight=0;
 lastMovement=now;
 // The native visor applies friction before this hook. Keep the requested
 // movement separately so that friction cannot cancel strafe acceleration.
 int step=Clamp(parameters[2]+parameters[1],1,1024);
 aimForward=Approach(aimForward,forward*speed,step);
 aimRight=Approach(aimRight,right*speed,step);
 int yaw=mem<unsigned short>(0x52f30e)&4095;
 int sine=mem<short>(0x4fe788+yaw*2),cosine=mem<short>(0x4fe788+((yaw+1024)&4095)*2);
 int vx=(aimForward*sine+aimRight*cosine)/16384;
 int vz=(aimForward*cosine-aimRight*sine)/16384;
 unsigned char* c=(unsigned char*)player;
 *(int*)(c+0x68)=vx;
 *(int*)(c+0x70)=vz;
 *(int*)(c+0x74)=aimForward;
 *(int*)(c+0x78)=aimRight;
 // Native integration and collision run immediately after this hook.
}
__declspec(naked) static void MovementHook(){__asm{
pushfd
pushad
lea eax,[esp+0x34]
push eax
push esi
call AimMovement
add esp,8
popad
popfd
mov edx,dword ptr [esi]
mov eax,dword ptr [esi+4]
jmp dword ptr [movementResume]
}}
__declspec(naked) static void InputHook(){__asm{
pushfd
pushad
call Input
popad
popfd
mov si,word ptr ds:[0x529b00]
jmp dword ptr [inputResume]
}}
__declspec(naked) static void InputEntryHook(){__asm{
pushfd
pushad
call RecoverKeyboard
popad
popfd
mov eax,dword ptr ds:[0x529d3c]
jmp dword ptr [inputEntryResume]
}}
__declspec(naked) static void AlternateThirdHook(){__asm{
pushfd
pushad
push esi
call Third
add esp,4
popad
popfd
movsx edx,word ptr [esi+0x2e]
mov ebx,dword ptr ds:[0x50a128]
jmp dword ptr [alternateThirdResume]
}}
__declspec(naked) static void ThirdHook(){__asm{
pushfd
pushad
push esi
call Third
add esp,4
popad
popfd
movsx ecx,word ptr [esi+0x2e]
and word ptr [esi+0x28],0xfff
jmp dword ptr [thirdResume]
}}
__declspec(naked) static void CameraLateHook(){__asm{
pushfd
pushad
push esi
call AcceptCameraCollision
add esp,4
popad
popfd
movsx edx,word ptr [esi+0x2e]
movsx edi,word ptr [esi+0x26]
jmp dword ptr [cameraLateResume]
}}
__declspec(naked) static void RecenterHook(){__asm{
cmp byte ptr [cameraOverride],0
jne skip
test cl,0xc
jne skip
jmp dword ptr [recenterResume]
skip:
jmp dword ptr [recenterSkip]
}}
__declspec(naked) static void AimHook(){__asm{
pushfd
pushad
push ebp
call Aim
add esp,4
popad
popfd
mov ecx,dword ptr ds:[0x50a510]
jmp dword ptr [aimResume]
}}
__declspec(naked) static void AimStartHook(){__asm{
pushfd
pushad
push esi
call StartAim
add esp,4
popad
popfd
mov dword ptr ds:[0x50a4fc],ecx
jmp dword ptr [aimStartResume]
}}
static bool Patch(uintptr_t va,const unsigned char* expected,size_t len,void* dest){
 unsigned char* at=(unsigned char*)(base+va-0x400000);
 if(memcmp(at,expected,len)){Log("Unsupported executable bytes: hook refused.");return false;}
 DWORD old;if(!VirtualProtect(at,len,PAGE_EXECUTE_READWRITE,&old))return false;
 at[0]=0xe9;*(int32_t*)(at+1)=(int32_t)((unsigned char*)dest-at-5);for(size_t i=5;i<len;i++)at[i]=0x90;
 DWORD ignored;VirtualProtect(at,len,old,&ignored);FlushInstructionCache(GetCurrentProcess(),at,len);return true;
}
struct NpcSite{uintptr_t va;unsigned reg;uintptr_t flags;};
#include "NpcSites.h"
static bool InstallNpcGates(){
 // Each tiny bridge preserves all registers and gates only a verified dialogue
 // contact test. Enemy collision, damage and quest scripts remain native.
 for(unsigned n=0;n<sizeof(npcSites)/sizeof(npcSites[0]);n++){
  const NpcSite& s=npcSites[n];
  unsigned char original[6]={0x81,(unsigned char)(0xe0+s.reg),0,2,0,0};
  unsigned len=6;if(s.reg==0){original[0]=0x25;original[1]=0;original[2]=2;original[3]=original[4]=0;len=5;}
  if(memcmp((void*)s.va,original,len))return false;
 }
 for(unsigned n=0;n<sizeof(npcSites)/sizeof(npcSites[0]);n++){
  const NpcSite& s=npcSites[n];unsigned len=s.reg==0?5:6;
  unsigned char expected[6];memcpy(expected,(void*)s.va,len);
  unsigned char* code=(unsigned char*)VirtualAlloc(NULL,64,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE);
  if(!code)return false;unsigned k=0,offset=28-s.reg*4;
  code[k++]=0x9c;code[k++]=0x60;
  if(s.flags>1){code[k++]=0x68;*(uint32_t*)(code+k)=(uint32_t)s.flags;k+=4;}
  else {code[k++]=0x8d;code[k++]=(s.flags==1?0x80:0x86);*(uint32_t*)(code+k)=0x52c880;k+=4;code[k++]=0x50;}
  code[k++]=0xff;code[k++]=0x74;code[k++]=0x24;code[k++]=(unsigned char)(offset+4);
  code[k++]=0xb8;*(uint32_t*)(code+k)=(uint32_t)NpcContact;k+=4;
  code[k++]=0xff;code[k++]=0xd0;code[k++]=0x83;code[k++]=0xc4;code[k++]=8;
  code[k++]=0x89;code[k++]=0x44;code[k++]=0x24;code[k++]=(unsigned char)offset;
  code[k++]=0x61;code[k++]=0x9d;memcpy(code+k,expected,len);k+=len;
  code[k++]=0xe9;*(int32_t*)(code+k)=(int32_t)(s.va+len-(uintptr_t)(code+k+4));k+=4;
  DWORD old;if(!VirtualProtect(code,64,PAGE_EXECUTE_READ,&old))return false;
  FlushInstructionCache(GetCurrentProcess(),code,k);
  if(!Patch(s.va,expected,len,code))return false;
 }
 return true;
}
static bool InstallFramePacing();
static void ConfigureLocalTestWindow(HWND window);
typedef int (__thiscall *LocalSurfaceFn)(void*,void*,unsigned);
static int __fastcall LocalSurfaceInit(void* context,void*,void* mode,unsigned flags){
 ConfigureLocalTestWindow(*(HWND*)context);
 return ((LocalSurfaceFn)0x4af110)(context,mode,flags);
}

static void __cdecl FixedDisplay(){
 if(localTestSlot&&GameWindow())ConfigureLocalTestWindow(GameWindow());
 mem<int>(0x508d84)=0; // dgVoodoo exposes the launcher's selected resolution.
 ((void(__cdecl*)())0x431e70)();
 mem<uintptr_t>(0x884030)=mem<uintptr_t>(0x53c580);
 mem<int>(0x508d7c)=mem<int>(0x53c58c);
 mem<int>(0x508d80)=mem<int>(0x53c590);
 mem<int>(0x508d84)=mem<int>(0x53c594);
 uintptr_t renderer=mem<uintptr_t>(0x53c588);
 // Use the engine's windowed DirectDraw path as well as a borderless wrapper.
 // An exclusive engine underneath a forced-windowed wrapper still loses its
 // primary surfaces on focus changes.
 if(renderer)*(int*)(renderer+0x140)=1;
 if(localTestSlot&&renderer){uintptr_t mode=*(uintptr_t*)(renderer+0x144);char detail[200];sprintf(detail,"Local display: renderer=%08x windowed=%d mode=%08x size=%dx%d",(unsigned)renderer,*(int*)(renderer+0x140),(unsigned)mode,mode?*(int*)(mode+12):0,mode?*(int*)(mode+8):0);Log(detail);}
 Log("Launcher display selected; legacy resolution menu bypassed.");
 Log(InstallFramePacing()?"Frame pacing installed: late frames no longer force an extra frame wait.":"Frame pacing signature mismatch; original timing retained.");
}
static int ScaleVolume(int volume,int percent){return volume*percent/100;}
#ifdef CONTROLS_TEST
static int lastMusicOutput;
#endif
static void __cdecl MusicVolume(int volume){
#ifdef CONTROLS_TEST
 lastMusicOutput=ScaleVolume(volume,musicVolume);
 mem<int>(0x5282c8)=lastMusicOutput;
#else
 ((void(__cdecl*)(int))0x49ae40)(ScaleVolume(volume,musicVolume));
#endif
 // The track-start code reads this value back. Keep the unscaled setting,
 // otherwise every track restart multiplies the gain again until it is silent.
 mem<int>(0x5282c8)=volume;
}
static void __cdecl SfxVolume(int volume){((void(__cdecl*)(int))0x49ae20)(ScaleVolume(volume,sfxVolume));}
#include "CoopIntegration.h"
static void DrawTokenCounter();
static void __cdecl HudWithTalkHint(){
 ((void(__cdecl*)())0x49fd40)();
 DrawTokenCounter();
 CoopNameplates();
 DrawGameConsole();
 bool npcHint=npcHintTime&&GetTickCount()-npcHintTime<200;
 bool cubeHint=cubeHintTime&&GetTickCount()-cubeHintTime<200;
 if((npcHint||cubeHint)&&Focused()&&
    !(mem<unsigned short>(0x52b816)&1)&&GetTickCount()-lastCamera<150){
  // In-level dialogue/HUD font sheet 0x14, not the menu-only sheet 0x32.
  ((void(__cdecl*)(int,const char*,int,int,int,int))0x401fb0)(204,npcHint?"press e to talk":"press e to read",255,255,255,0);
 }
}
static bool CallHook(uintptr_t va,uintptr_t original,void* dest){
 unsigned char expected[5]={0xe8};*(int32_t*)(expected+1)=(int32_t)(original-va-5);
 if(memcmp((void*)va,expected,5))return false;
 DWORD old;if(!VirtualProtect((void*)va,5,PAGE_EXECUTE_READWRITE,&old))return false;
 *(int32_t*)(va+1)=(int32_t)((uintptr_t)dest-va-5);
 DWORD ignored;VirtualProtect((void*)va,5,old,&ignored);FlushInstructionCache(GetCurrentProcess(),(void*)va,5);return true;
}
static DWORD WINAPI FocusWatch(void*){
 bool clipped=false;
 for(;;){Sleep(50);bool capture=active&&enabled&&Focused()&&(GetTickCount()-lastCamera<150);if(!capture){if(clipped||active)ClipCursor(NULL);active=false;}clipped=capture;}
}
#include "Combat.h"
#include "CameraSafety.h"
static bool CoopAdvanceCampaign(int level);
#include "Progression.h"
#include "Autosave.h"
#include "GameConsole.h"
#include "CubeInteraction.h"
#include "MenuMouse.h"
#include "Sky.h"
#include "RoomAudio.h"
#include "FramePacing.h"
#include "PauseQuit.h"
// The legacy CPU benchmark can measure zero milliseconds on a modern CPU.
// Bound its divisor to avoid both division by zero and the following MHz*1e6
// signed overflow. Actual frame timing uses our QPC pacer, not this estimate.
static DWORD benchmarkResume=0x47f06b;
__declspec(naked) static void BenchmarkDivide(){__asm{
 push ecx
 mov ecx,dword ptr ds:[726f44h]
 cmp ecx,28
 jge valid
 mov ecx,28
valid:
 idiv ecx
 pop ecx
 jmp dword ptr [benchmarkResume]
}}
#ifdef LOCAL_RENDER_DIAGNOSTICS
#include "RenderProbe.h"
#endif
BOOL WINAPI DllMain(HINSTANCE instance,DWORD reason,LPVOID){
 if(reason==DLL_PROCESS_ATTACH){
  char testSlot[8]={};GetEnvironmentVariableA("TOY2_LOCAL_TEST",testSlot,sizeof(testSlot));
  localTestSlot=(testSlot[0]=='1'||testSlot[0]=='2')?testSlot[0]-'0':0;
  DisableThreadLibraryCalls(instance);base=(uintptr_t)GetModuleHandle(NULL);
  if(base!=0x400000)return TRUE;
  char path[MAX_PATH];GetModuleFileNameA(instance,path,MAX_PATH);char* slash=strrchr(path,'\\');if(slash)slash[1]=0;
  strcpy(logPath,path);strcat(logPath,"ModernControls.log");
#ifdef LOCAL_RENDER_DIAGNOSTICS
  if(localTestSlot)CallHook(0x4abab0,0x4abd40,ProbePresent);
#endif
  if(localTestSlot)CallHook(0x4aee4d,0x4af110,LocalSurfaceInit);
  const unsigned char benchmark[]={0xf7,0x3d,0x44,0x6f,0x72,0};
  if(Patch(0x47f065,benchmark,sizeof(benchmark),BenchmarkDivide))Log("Legacy CPU benchmark zero/overflow guard installed.");
  char ini[MAX_PATH];strcpy(ini,path);strcat(ini,"ModernControls.ini");
  sensitivity=Clamp(GetPrivateProfileIntA("Mouse","Sensitivity",2,ini),1,10);invertY=GetPrivateProfileIntA("Mouse","InvertY",0,ini);
  aimSensitivity=Clamp(GetPrivateProfileIntA("Mouse","AimSensitivity",2,ini),1,10);
  musicVolume=Clamp(GetPrivateProfileIntA("Audio","MusicVolume",100,ini),0,100);
  sfxVolume=Clamp(GetPrivateProfileIntA("Audio","SfxVolume",100,ini),0,100);
  const unsigned char input[]={0xe9,0xb3,0x71,0x0c,0,0x90,0x90};
  const unsigned char entry[]={0xa1,0x3c,0x9d,0x52,0};
  const unsigned char alternate[]={0x0f,0xbf,0x56,0x2e,0x8b,0x1d,0x28,0xa1,0x50,0};
  const unsigned char third[]={0x0f,0xbf,0x4e,0x2e,0x66,0x81,0x66,0x28,0xff,0x0f};
  const unsigned char late[]={0x0f,0xbf,0x56,0x2e,0x0f,0xbf,0x7e,0x26};
  const unsigned char recenter[]={0xf6,0xc1,0x0c,0x0f,0x85,0x90,0,0,0};
  const unsigned char aim[]={0x8b,0x0d,0x10,0xa5,0x50,0};
  const unsigned char aimStart[]={0x89,0x0d,0xfc,0xa4,0x50,0};
  // Validate all sites before installing any hook.
  if(memcmp((void*)0x414af0,entry,sizeof(entry))||memcmp((void*)0x404aaa,alternate,sizeof(alternate))||memcmp((void*)0x414b98,input,sizeof(input))||memcmp((void*)0x404ea1,third,sizeof(third))||memcmp((void*)0x4053b1,late,sizeof(late))||memcmp((void*)0x4052be,recenter,sizeof(recenter))||memcmp((void*)0x404031,aim,sizeof(aim))){Log("Unsupported game build; no changes installed.");return TRUE;}
  inputResume=0x414b9f;thirdResume=0x404eab;aimResume=0x404037;
  if(memcmp((void*)0x403990,aimStart,sizeof(aimStart))){Log("Unsupported aim transition; changes refused.");return TRUE;}
  if(Patch(0x414af0,entry,sizeof(entry),InputEntryHook)&&Patch(0x404aaa,alternate,sizeof(alternate),AlternateThirdHook)&&Patch(0x414b98,input,sizeof(input),InputHook)&&Patch(0x404ea1,third,sizeof(third),ThirdHook)&&Patch(0x4053b1,late,sizeof(late),CameraLateHook)&&Patch(0x4052be,recenter,sizeof(recenter),RecenterHook)&&Patch(0x404031,aim,sizeof(aim),AimHook)){
   const unsigned char movement[]={0x8b,0x16,0x8b,0x46,0x04};
   const unsigned char display[]={0x81,0xec,0xa8,0x02,0,0};
   if(Patch(0x403990,aimStart,sizeof(aimStart),AimStartHook))Log("Aim entry inherits freelook yaw and pitch.");
   if(CallHook(0x405976,0x4038e0,MouseOnlyAim)&&Patch(0x436a36,movement,sizeof(movement),MovementHook))Log("Visor movement installed: WASD moves; mouse aims.");
   if(Patch(0x432020,display,sizeof(display),FixedDisplay))Log("Launcher display startup installed.");
   if(CallHook(0x41327a,0x49ae40,MusicVolume)&&CallHook(0x4a3ed5,0x49ae40,MusicVolume)&&CallHook(0x4a3edf,0x49ae20,SfxVolume))Log("Independent launcher music and SFX volumes installed.");
   if(CallHook(0x49e30c,0x49fd40,HudWithTalkHint)&&CallHook(0x49e5eb,0x49fd40,HudWithTalkHint))Log("Native-font NPC talk hint installed.");
   if(InstallNpcGates())Log("E interaction installed for NPC contact dialogue.");
   if(InstallCubeInteraction())Log("E interaction installed for floating information cubes.");
   if(InstallCombat())Log("Enemy and pickup draw ranges doubled from original; native room/world limits retained.");
   if(InstallCameraSafety())Log("Independent camera orbit with near-plane wall rays and finite 2x geometry range installed.");
   if(InstallProgression())Log("Automatic final-token exit, completed-level replay confirmation, and defeated actor retirement installed.");
   if(InstallMenuMouse())Log("Native menu cursor and mouse selection installed.");
   if(InstallRoomAudio())Log("Actor positional sounds restricted to the player's room.");
   if(InstallSky())Log("Camera-basis sky mesh with wrapped panorama installed.");
   if(InstallAutosave())Log("Single-player autosave and Continue installed.");
   if(InstallPauseQuit())Log("Pause menu Quit Game with native No/Yes confirmation installed.");
   Log("ModernControls v27 loaded: native windowed restore, safe voice allocation, camera-basis sky mesh and visible startup menu.");HANDLE h=CreateThread(NULL,0,FocusWatch,NULL,0,NULL);if(h)CloseHandle(h);
  }
 }return TRUE;
}



