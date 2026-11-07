// Native token completion, level-selection UI, and defeated actor retirement.
static bool LevelComplete(int level){
 if(level<1||level>15)return false;
 unsigned bits=mem<unsigned char>(0x52f0d7+level);
 return level%3==0 ? (bits&0x80)!=0 : (bits&0x1f)==0x1f;
}
static int __cdecl TokenExitDecision(){
 // Called after the native pickup animation has stopped Buzz on the ground.
 // Ordinary levels require all five tokens even if the native forced-exit
 // flag is set. Boss stages retain their own completion sequence.
 int level=mem<int>(0x88278c);
 if(level<1||level>15)return 0;
 if(LevelComplete(level)&&CoopAdvanceCampaign(level))return 0;
 if(level%3==0)return mem<int>(0x830cac)||LevelComplete(level);
 return LevelComplete(level) &&
  (mem<int>(0x830cac)||(mem<unsigned>(0x830ca8)&0x1f)!=0x1f);
}
static DWORD tokenDecisionResume=0x4a1878;
static void __cdecl ContinueAfterToken(){
 // Same state restoration as the native token menu's Continue action.
 mem<unsigned char>(0x52b816)&=0xfe;
 mem<unsigned char>(0x52f340)&=0xfb;
 mem<int>(0x830cc4)=0;
 mem<int>(0x50a1f4)=0;
 mem<int>(0x50a140)=0x40;
 mem<short>(0x52f38c)=5;
}
static DWORD tokenContinueResume=0x4a18b3;
__declspec(naked) static void TokenContinueHook(){__asm{
 pushfd
 pushad
 call ContinueAfterToken
 popad
 popfd
 jmp dword ptr [tokenContinueResume]
}}
static int TokenCount(int level){
 if(level<1||level>15)return 0;
 unsigned bits=mem<unsigned char>(0x52f0d7+level);
 if(level%3==0)return (bits&0x80)?1:0;
 int count=0;for(unsigned bit=1;bit<=16;bit<<=1)if(bits&bit)++count;
 return count;
}
static int TokenHudOffset(){
 int level=mem<int>(0x88278c);
 if(level<1||level>15||mem<int>(0x50a13c)!=4||
    mem<short>(0x52b818)||(mem<unsigned char>(0x52b816)&1))return -1;
 // Follow the existing lower-left coin panel's slide-in position without
 // advancing its animation twice in one frame. Place tokens one row above it.
 int phase=Clamp(mem<short>(0x52f2e4),0,1024);
 if(!phase)return -1;
 return 64-(mem<short>(0x4fe788+phase*2)>>8);
}
struct TokenSpriteDescriptor {short texture;unsigned char width,height;short reserved[2];unsigned char u,v;};
// Original Pizza Planet animation's front frame, copied into unused HUD atlas
// space at build time. Sprite -1 uses the native temporary descriptor slot.
static const TokenSpriteDescriptor tokenSprite={31,24,16,{0,0},160,128};
typedef short (__cdecl *TokenSpriteFn)(int,int,int,int,int,int,int,int,int,int);
static TokenSpriteFn drawTokenSprite=(TokenSpriteFn)0x4942d0;
static void DrawTokenIcon(int offset){
 uintptr_t previous=mem<uintptr_t>(0x557704);
 mem<uintptr_t>(0x557704)=(uintptr_t)&tokenSprite;
 drawTokenSprite(14,198+offset,-1,0,128,128,128,255,0x1000,0x1000);
 mem<uintptr_t>(0x557704)=previous;
}
static void DrawTokenCounter(){
 int offset=TokenHudOffset();if(offset<0)return;
 int level=mem<int>(0x88278c);
 char label[24];sprintf(label,"tokens %d of %d",TokenCount(level),level%3==0?1:5);
 // The gameplay font has digits and letters but no slash glyph. Use "of"
 // instead of sending '/' to the font renderer (which would draw 'z').
 typedef void (__cdecl *GlyphFn)(int,int,int,int,int,int,int);
 GlyphFn glyph=(GlyphFn)0x49b630;
 DrawTokenIcon(offset);
 for(int i=0;label[i];++i)glyph(42+i*8,202+offset,label[i],255,255,255,0);
}
__declspec(naked) static void TokenDecisionHook(){__asm{
 pushfd
 pushad
 call TokenExitDecision
 mov [esp+28],eax
 popad
 popfd
 jmp dword ptr [tokenDecisionResume]
}}

typedef void (__cdecl *DeathFn)(uintptr_t,int);
static DeathFn nativeDeath=(DeathFn)0x405d20;
static void __cdecl DefeatedActor(uintptr_t actor,int mode){
 // These call sites are the generic damage/death timers, not scripted NPC
 // despawning. Zero is the engine's permanent removal setting: the final
 // death phase clears model ID instead of starting the respawn countdown.
 CoopDeathEvent(actor,mode,true);
 mem<short>(actor+0x7a)=0;
 nativeDeath(actor,mode);
 CoopDeathEvent(actor,mode,false);
 mem<short>(actor+0x7a)=0;
 // A few models finish their death inside phase one and install their own
 // respawn delay. They are already retired (health zero); remove the model
 // too, otherwise clearing that delay alone would immediately respawn them.
 if(mem<short>(actor+0x7e)==0)mem<short>(actor+0x14)=0;
}

static int menuLevel;
static bool replayPrompt, replayYes;
static unsigned short replayPrevious;
static int MenuLevelId(int selected){
 return selected>=1&&selected<=15?mem<int>(0x50268c+(selected-1)*4):0;
}
static unsigned short FilterReplayInput(int level,int transition,int age,unsigned short keys){
 unsigned short pressed=(unsigned short)(keys&~replayPrevious);
 replayPrevious=keys;menuLevel=level;
 if(transition!=0){replayPrompt=false;return keys;}
 if(replayPrompt){
  if(pressed&0x1000){replayPrompt=false;return 0;}
  if(pressed&0xf0)replayYes=!replayYes;
  if(pressed&0x4000){
   bool accept=replayYes;replayPrompt=false;
   return accept?0x4000:0;
  }
  return 0;
 }
 if(age>60 && (pressed&0x4000) && LevelComplete(level)){
  replayPrompt=true;replayYes=false;return 0;
 }
 // A held confirm after cancelling must not reopen the prompt or start a level.
 if(LevelComplete(level)&&!(pressed&0x4000))keys&=~0x4000;
 return keys;
}
static void __cdecl LevelMenuInput(int* stack){
 unsigned short keys=mem<unsigned short>(0x88279c);
 bool wasPrompt=replayPrompt;
 keys=FilterReplayInput(MenuLevelId(stack[5]),stack[8],stack[7],keys);
 mem<unsigned short>(0x88279c)=keys;
 if(wasPrompt||replayPrompt)mem<unsigned short>(0x882794)=0;
}
static DWORD levelInputResume=0x439320;
__declspec(naked) static void LevelMenuHook(){__asm{
 pushfd
 pushad
 lea eax,[esp+36]
 push eax
 call LevelMenuInput
 add esp,4
 popad
 popfd
 mov dx,word ptr ds:[0x88279c]
 jmp dword ptr [levelInputResume]
}}
static void RegisterLevelMouse();
static void __cdecl LevelTitle(const char* title,int y,int x){
 typedef void (__cdecl *TextFn)(const char*,int,int);
 TextFn text=(TextFn)0x49d750;
 text(title,y,x);
 if(LevelComplete(menuLevel))text("completed",46,256);
 if(replayPrompt){
  // Use the menu's own loaded font and its 512-wide coordinate space.
  text("are you sure you want",130,256);
  text("to replay this level?",146,256);
  typedef void (__cdecl *ColorTextFn)(const char*,int,int,int,int,int,int);
  ColorTextFn colorText=(ColorTextFn)0x49d390;
  colorText("yes",170,220,replayYes?255:128,replayYes?255:128,replayYes?0:128,0x60);
  colorText("no",170,292,replayYes?128:255,replayYes?128:255,replayYes?128:0,0x60);
 }
 RegisterLevelMouse();
}
static int __cdecl SkipTransitionMovie(int,int){
 // 49eb20 has already recorded the movie's seen flag. Skip only its playback
 // call, before renderer teardown, music changes and the movie startup delay.
 // The movie viewer's separate call remains available.
 return 1;
}
static bool InstallProgression(){
 const unsigned char token[]={0xa1,0xac,0x0c,0x83,0};
 const unsigned char menu[]={0x66,0x8b,0x15,0x9c,0x27,0x88,0};
 const unsigned char tokenMenu[]={0xc7,0x05,0xc4,0x0c,0x83,0,9,0,0,0};
 if(memcmp((void*)0x4a1873,token,sizeof(token))||
    memcmp((void*)0x439319,menu,sizeof(menu))||
    memcmp((void*)0x4a18a9,tokenMenu,sizeof(tokenMenu)))return false;
 return Patch(0x4a1873,token,sizeof(token),TokenDecisionHook)&&
  Patch(0x439319,menu,sizeof(menu),LevelMenuHook)&&
  Patch(0x4a18a9,tokenMenu,sizeof(tokenMenu),TokenContinueHook)&&
  CallHook(0x4397bd,0x49d750,LevelTitle)&&
  CallHook(0x408bc7,0x405d20,DefeatedActor)&&
  CallHook(0x408942,0x405d20,DefeatedActor)&&
  CallHook(0x408a26,0x405d20,DefeatedActor)&&
  CallHook(0x49eb46,0x49ab90,SkipTransitionMovie);
}

