// Mouse hit testing for native menus. All rectangles use normalized client
// coordinates, so the launcher resolution does not change the hit areas.
struct MenuHit {float l,t,r,b;int action;char label[96];bool selected;};
static MenuHit menuHits[64];
static int menuHitCount;
static DWORD menuPaintTime,menuPulseTime;
static bool menuLeftDown,menuRightDown,menuCursorShown;
static int cursorShowCalls;
static char menuClickTarget[96];
static float menuArrowY=-1;
static WNDPROC originalMenuWndProc;
static int menuWheel,menuHorizontalWheel;
static bool MenuContext();
static const UINT_PTR DisplayRestoreTimer=0x545332;
static unsigned displayRestoreAttempts;
static void RestoreDisplayBounds(HWND window){
 if(localTestSlot)return;
 if(IsIconic(window)||GetForegroundWindow()!=window)return;
 POINT origin={0,0};MONITORINFO monitor={sizeof(MONITORINFO)};
 if(!GetMonitorInfo(MonitorFromPoint(origin,MONITOR_DEFAULTTOPRIMARY),&monitor))return;
 RECT current={};GetWindowRect(window,&current);const RECT& target=monitor.rcMonitor;
 if(EqualRect(&current,&target))return;
 // Run after activation has unwound: resizing from WM_SIZE reenters the
 // wrapper while its DirectDraw surfaces are still being restored.
 SetWindowPos(window,HWND_NOTOPMOST,target.left,target.top,target.right-target.left,
  target.bottom-target.top,SWP_NOACTIVATE|SWP_NOOWNERZORDER);
}
static LONG LocalTestWindowStyle(LONG existing){
 return WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU|WS_MINIMIZEBOX|
  (existing&(WS_VISIBLE|WS_DISABLED|WS_CLIPCHILDREN|WS_CLIPSIBLINGS));
}
static void ConfigureLocalTestWindow(HWND window){
  if(localTestSlot){
   SetWindowTextA(window,localTestSlot==1?"Toy Story 2 - Player 1 (host)":"Toy Story 2 - Player 2 (join)");
   LONG style=LocalTestWindowStyle(GetWindowLong(window,GWL_STYLE));
   SetWindowLong(window,GWL_STYLE,style);
   POINT origin={0,0};MONITORINFO monitor={sizeof(MONITORINFO)};
   if(GetMonitorInfo(MonitorFromPoint(origin,MONITOR_DEFAULTTOPRIMARY),&monitor)){
    // Size the outer windows to exactly cover the primary work area's width.
    // Subtract the caption/borders before computing the 16:9 client height.
    RECT frame={0,0,0,0};AdjustWindowRect(&frame,style,FALSE);
    int available=monitor.rcWork.right-monitor.rcWork.left;
    int leftWidth=available/2;
    int width=localTestSlot==1?leftWidth:available-leftWidth;
    int clientWidth=max(1,width-(frame.right-frame.left));
    int height=clientWidth*9/16+(frame.bottom-frame.top);
    height=min(height,monitor.rcWork.bottom-monitor.rcWork.top);
    int x=monitor.rcWork.left+(localTestSlot==2?leftWidth:0);
    SetWindowPos(window,HWND_NOTOPMOST,x,monitor.rcWork.top,width,height,SWP_FRAMECHANGED|SWP_NOACTIVATE);
   }
  }
}
static void LocalTestWindowMoved(HWND window){
 if(!localTestSlot||IsIconic(window))return;
 uintptr_t context=mem<uintptr_t>(0x884008);
 if(!context||*(int*)(context+4))return;
 POINT origin={0,0};if(!ClientToScreen(window,&origin))return;
 ((void(__cdecl*)(int,int))0x4abdb0)(origin.x,origin.y);
}
static LRESULT CALLBACK MenuWindowProc(HWND window,UINT message,WPARAM w,LPARAM l){
 if(localTestSlot&&(message==WM_MOVE||message==WM_EXITSIZEMOVE))LocalTestWindowMoved(window);
 if(message==WM_ACTIVATE){
  if(LOWORD(w)!=WA_INACTIVE){displayRestoreAttempts=0;SetTimer(window,DisplayRestoreTimer,250,NULL);}
  else KillTimer(window,DisplayRestoreTimer);
 }
 if(message==WM_TIMER&&w==DisplayRestoreTimer){
  RestoreDisplayBounds(window);
  if(++displayRestoreAttempts>=4)KillTimer(window,DisplayRestoreTimer);
  return 0;
 }
 if(message==WM_CLOSE)FlushAutosave();
 if(MenuContext()){
  if(message==WM_SETCURSOR&&LOWORD(l)==HTCLIENT){SetCursor(LoadCursor(NULL,IDC_ARROW));return TRUE;}
  if(message==WM_MOUSEWHEEL){
   if(w&(MK_SHIFT|MK_MBUTTON))menuHorizontalWheel+=(short)HIWORD(w);
   else menuWheel+=(short)HIWORD(w);
   return 0;
  }
  if(message==WM_MOUSEHWHEEL){menuHorizontalWheel+=(short)HIWORD(w);return 0;}
 }
 return CallWindowProc(originalMenuWndProc,window,message,w,l);
}
static bool MenuContext(){
 int level=mem<int>(0x88278c);
 return level==0||level==16||mem<short>(0x52b818)!=0;
}
static bool MouseMenusActive(){return MenuContext()&&Focused();}
static void AddMenuHit(const char* label,float x,float y,float w,float h,
                       float width,float height,bool selected,int action=0){
 if(menuHitCount>=64||!label||width<=0||height<=0)return;
 MenuHit& hit=menuHits[menuHitCount++];
 hit.l=(x-w/2)/width;hit.r=(x+w/2)/width;
 hit.t=(y-3)/height;hit.b=(y+h)/height;
 hit.action=action;hit.selected=selected;
 strncpy(hit.label,label,sizeof(hit.label)-1);hit.label[sizeof(hit.label)-1]=0;
 menuPaintTime=GetTickCount();
}
static bool InteractiveLabel(const char* label){
 if(!label||strlen(label)>80)return false;
 static const char* words[]={"start game","new game","continue game","continue",
  "options","load game","save game","exit game","exit level","quit game",
  "quit","exit","return","back","cancel","yes","no","music","sound",
  "sfx","volume","controls","control","stereo","mono","vibration",
  "camera","screen","language","empty","slot","resume","gamepad","keyboard",
  "movie viewer","configure controller","configure gfx"};
 char lower[96];strncpy(lower,label,95);lower[95]=0;CharLowerBuffA(lower,(DWORD)strlen(lower));
 for(unsigned i=0;i<sizeof(words)/sizeof(words[0]);++i)
  if(!strcmp(lower,words[i])||strstr(lower,words[i])==lower)return true;
 return false;
}
typedef void (__cdecl *MenuTextFn)(int,const char*,int);
typedef void (__cdecl *HudTextFn)(int,const char*,int,int,int,int);
typedef void (__cdecl *WideTextFn)(const char*,int,int);
typedef short (__cdecl *MenuSpriteFn)(int,int,int,int,int,int,int,int,int,int);
static MenuTextFn originalMenuText;
static MenuTextFn originalSmallMenuText;
typedef short (__cdecl *SimpleSpriteFn)(int,int,int,int);
static SimpleSpriteFn originalSimpleSprite;
static HudTextFn originalHudText;
static WideTextFn originalWideText;
static MenuSpriteFn originalMenuSprite;
static float MenuWidth(){float n=mem<float>(0x4f7414);return n>=256&&n<=1024?n:320;}
static float MenuHeight(){float n=mem<float>(0x4f7418);return n>=200&&n<=1024?n:240;}
static void __cdecl MouseMenuText(int y,const char* label,int color){
 label=AutosaveMenuLabel(label);
 if(MenuContext()&&InteractiveLabel(label))
  AddMenuHit(label,160,(float)y,(float)max(96,(int)strlen(label)*12),14,MenuWidth(),MenuHeight(),color!=128);
 originalMenuText(y,label,color);
}
static void __cdecl MouseSmallMenuText(int y,const char* label,int color){
 if(MenuContext()&&InteractiveLabel(label)&&y>60)
  AddMenuHit(label,160,(float)y,(float)max(100,(int)strlen(label)*8),12,MenuWidth(),MenuHeight(),false);
 originalSmallMenuText(y,label,color);
}
static short __cdecl MouseSimpleSprite(int x,int y,int sheet,int frame){
 if(MenuContext()){
  if(sheet==0x34)menuArrowY=(y+14)/MenuHeight();
  if(sheet==0x38&&(frame==2||frame==3))
   AddMenuHit(frame==2?"increase":"decrease",(float)x+12,(float)y,36,24,MenuWidth(),MenuHeight(),false,frame==2?0x20:0x80);
 }
 return originalSimpleSprite(x,y,sheet,frame);
}
static const char* PauseLabel(const char* label){
 if(mem<short>(0x52b818)&&
    ((uintptr_t)label==0x5026f0||(uintptr_t)label==0x5027b4))return "quit game";
 return label;
}
static void __cdecl MouseHudText(int y,const char* label,int r,int g,int b,int wide){
 label=PauseLabel(label);
 if(mem<short>(0x52b818)&&InteractiveLabel(label))
  AddMenuHit(label,wide?256.0f:160.0f,(float)y,(float)max(80,(int)strlen(label)*8),8,
             wide?512.0f:320.0f,MenuHeight(),r!=128||g!=128);
 originalHudText(y,label,r,g,b,wide);
}
static void __cdecl MouseWideText(const char* label,int y,int x){
 if(MenuContext()&&InteractiveLabel(label)&&y>=60)
  AddMenuHit(label,(float)x,(float)y,(float)max(80,(int)strlen(label)*13),16,MenuWidth(),MenuHeight(),false);
 originalWideText(label,y,x);
}
static short __cdecl MouseMenuSprite(int x,int y,int sheet,int frame,int r,int g,int b,int flags,int sx,int sy){
 // The main menu's two brackets move to the selected text row.
 if(MenuContext()&&sheet==0x3e)menuArrowY=y/MenuHeight();
 return originalMenuSprite(x,y,sheet,frame,r,g,b,flags,sx,sy);
}
static void RegisterLevelMouse(){
 // Level selection has horizontal navigation rather than a text list.
 if(coopLoadConfirm){mem<unsigned short>(0x88279c)|=0x4000;mem<unsigned short>(0x882794)&=~0x4000;}
 menuHitCount=0;menuArrowY=-1;
 if(replayPrompt){
  AddMenuHit("replay yes",220,170,56,20,512,256,replayYes,0x10001);
  AddMenuHit("replay no",292,170,56,20,512,256,!replayYes,0x10002);
 }else{
  AddMenuHit("previous level",32,100,60,48,512,256,false,0x80);
  AddMenuHit("next level",448,100,60,48,512,256,false,0x20);
  AddMenuHit("play level",256,24,280,160,512,256,false,0x4000);
 }
 AddMenuHit("back",256,202,260,34,512,256,false,0x1000);
}
static int HitMenu(float x,float y){
 for(int i=menuHitCount-1;i>=0;--i){const MenuHit& h=menuHits[i];
  if(x>=h.l&&x<=h.r&&y>=h.t&&y<=h.b)return i;
 }
 return -1;
}
static unsigned MenuActionAt(int hit,bool click,bool right,DWORD now){
 if(right){menuClickTarget[0]=0;return 0x1000;}
 if(hit>=0&&click){
  const MenuHit& h=menuHits[hit];
  if(h.action){
   if(h.action==0x10001||h.action==0x10002){replayYes=h.action==0x10001;return 0x4000;}
   return h.action;
  }
  strcpy(menuClickTarget,h.label);
 }
 // Retain a clicked target while the native highlight animates towards it.
 int target=-1,selected=-1;
 for(int i=0;i<menuHitCount;++i){
  if(menuClickTarget[0]&&!strcmp(menuHits[i].label,menuClickTarget))target=i;
  if(menuHits[i].selected)selected=i;
 }
 if(target<0){menuClickTarget[0]=0;return 0;}
 if(menuArrowY>=0){
  float best=1;
  for(int i=0;i<menuHitCount;++i){float d=menuHits[i].t-menuArrowY;if(d<0)d=-d;
   if(d<best){best=d;selected=i;}}
 }
 if(selected<0)return 0; // Never confirm an unverified selection.
 if(now-menuPulseTime<90)return 0;
 menuPulseTime=now;
 if(selected==target){menuClickTarget[0]=0;return 0x4000;}
 float delta=menuHits[target].t-menuHits[selected].t;
 if(delta>-0.015f&&delta<0.015f)return menuHits[target].l<menuHits[selected].l?0x80:0x20;
 return delta<0?0x10:0x40;
}
static void __cdecl MenuMousePoll(){
 if(!originalMenuWndProc&&GameWindow()){
  HWND window=GameWindow();
  originalMenuWndProc=(WNDPROC)SetWindowLongPtr(window,GWLP_WNDPROC,(LONG_PTR)MenuWindowProc);

 }
 bool context=MouseMenusActive();
 bool left=(GetAsyncKeyState(VK_LBUTTON)&0x8000)!=0;
 bool right=(GetAsyncKeyState(VK_RBUTTON)&0x8000)!=0;
 bool click=left&&!menuLeftDown,back=right&&!menuRightDown;
 menuLeftDown=left;menuRightDown=right;
 if(context){
  ReleaseMouse();ClipCursor(NULL);
  if(!menuCursorShown){cursorShowCalls=0;do{++cursorShowCalls;}while(ShowCursor(TRUE)<0&&cursorShowCalls<16);menuCursorShown=true;}
  SetCursor(LoadCursor(NULL,IDC_ARROW));
  POINT pt;RECT client;unsigned action=0;
  if(GetCursorPos(&pt)&&ScreenToClient(GameWindow(),&pt)&&GetClientRect(GameWindow(),&client)&&client.right>0&&client.bottom>0){
   int hit=HitMenu((float)pt.x/client.right,(float)pt.y/client.bottom);
   if(GetTickCount()-menuPaintTime<300)action=MenuActionAt(hit,click,back,GetTickCount());
   if(!menuHitCount&&click)action=0x4000; // title, 3D slot carousels, Continue screens
   if(!action&&menuWheel){action=menuWheel>0?0x10:0x40;menuWheel=0;menuClickTarget[0]=0;}
   if(!action&&menuHorizontalWheel){action=menuHorizontalWheel>0?0x20:0x80;menuHorizontalWheel=0;menuClickTarget[0]=0;}
  }
  if(action){mem<unsigned short>(0x88279c)|=(unsigned short)action;
   mem<unsigned short>(0x882794)&=(unsigned short)~action;}
 }else{
  menuWheel=menuHorizontalWheel=0;
  menuClickTarget[0]=0;
  if(menuCursorShown){for(int i=0;i<cursorShowCalls;++i)ShowCursor(FALSE);menuCursorShown=false;}
 }
 if(coopLoadConfirm){mem<unsigned short>(0x88279c)|=0x4000;mem<unsigned short>(0x882794)&=~0x4000;}
 menuHitCount=0;menuArrowY=-1;
}
static DWORD menuPollResume=0x45219f;
__declspec(naked) static void MenuMousePollHook(){__asm{
 mov word ptr ds:[0x88279c],cx
 pushfd
 pushad
 call MenuMousePoll
 popad
 popfd
 jmp dword ptr [menuPollResume]
}}
static void* MenuTrampoline(uintptr_t va,const unsigned char* bytes,unsigned size,void* hook){
 if(memcmp((void*)va,bytes,size))return NULL;
 unsigned char* code=(unsigned char*)VirtualAlloc(NULL,size+5,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE);
 if(!code)return NULL;
 memcpy(code,bytes,size);code[size]=0xe9;
 *(int32_t*)(code+size+1)=(int32_t)(va+size-(uintptr_t)(code+size+5));
 DWORD old;VirtualProtect(code,size+5,PAGE_EXECUTE_READ,&old);
 FlushInstructionCache(GetCurrentProcess(),code,size+5);
 if(!Patch(va,bytes,size,hook))return NULL;
 return code;
}
static bool InstallMenuMouse(){
 const unsigned char a[]={0x55,0x8b,0x6c,0x24,0x0c};
 const unsigned char b[]={0x8b,0x44,0x24,0x18,0x33,0xc9};
 const unsigned char c[]={0x8b,0x44,0x24,0x0c,0x8b,0x4c,0x24,8};
 const unsigned char d[]={0x66,0x8b,0x44,0x24,0x0c};
 const unsigned char e[]={0x66,0x89,0x0d,0x9c,0x27,0x88,0};
 const unsigned char f[]={0x53,0x8b,0x5c,0x24,0x0c};
 const unsigned char g[]={0x0f,0xbf,0x44,0x24,0x0c};
 if(memcmp((void*)0x49b580,a,sizeof(a))||memcmp((void*)0x401fb0,b,sizeof(b))||
    memcmp((void*)0x49d750,c,sizeof(c))||memcmp((void*)0x493f40,d,sizeof(d))||
    memcmp((void*)0x452198,e,sizeof(e))||memcmp((void*)0x49b2a0,f,sizeof(f))||
    memcmp((void*)0x494820,g,sizeof(g)))return false;
 originalMenuText=(MenuTextFn)MenuTrampoline(0x49b580,a,sizeof(a),MouseMenuText);
 originalHudText=(HudTextFn)MenuTrampoline(0x401fb0,b,sizeof(b),MouseHudText);
 originalWideText=(WideTextFn)MenuTrampoline(0x49d750,c,sizeof(c),MouseWideText);
 originalMenuSprite=(MenuSpriteFn)MenuTrampoline(0x493f40,d,sizeof(d),MouseMenuSprite);
 originalSmallMenuText=(MenuTextFn)MenuTrampoline(0x49b2a0,f,sizeof(f),MouseSmallMenuText);
 originalSimpleSprite=(SimpleSpriteFn)MenuTrampoline(0x494820,g,sizeof(g),MouseSimpleSprite);
 return originalMenuText&&originalHudText&&originalWideText&&originalMenuSprite&&originalSmallMenuText&&originalSimpleSprite&&
  Patch(0x452198,e,sizeof(e),MenuMousePollHook);
}
