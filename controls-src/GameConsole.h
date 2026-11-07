// Debug console: input is consumed before native action mapping. Level switches
// leave through the native level loop, never load resources inside an input hook.
static bool consoleOpen,consoleKeys[256];static char consoleLine[32],consoleReply[64]="lvl1 to lvl15    unstuck";
static int consoleLength;static DWORD consoleSafeAt;static unsigned consoleSafeLevel;
struct ConsoleSafePoint {int position[3];DWORD time;};static ConsoleSafePoint consoleSafe[32];static unsigned consoleSafeCount;
static int ParseConsoleCommand(const char* text){
 if(!strcmp(text,"unstuck"))return -1;
 if(strncmp(text,"lvl",3)||text[3]<'1'||text[3]>'9')return 0;
 int n=text[3]-'0';unsigned i=4;if(text[i]>='0'&&text[i]<='9')n=n*10+text[i++]-'0';
 return !text[i]&&n>=1&&n<=15?n:0;
}
static bool ConsoleGameplay(){int level=mem<int>(0x88278c);return level>=1&&level<=15&&mem<short>(0x52f396)>0&&!mem<short>(0x52b7dc);}
static bool ConsoleClearPoint(const int* foot){
 const int rays[5][3]={{0,-0x2800,0},{0x1000,0,0},{-0x1000,0,0},{0,0,0x1000},{0,0,-0x1000}};
 for(unsigned i=0;i<5;i++){int origin[3]={foot[0],foot[1]-0x100,foot[2]},delta[3];memcpy(delta,rays[i],12);if(i)origin[1]-=0x1400;if(worldRay(origin,delta,0x8000,1,256))return false;}
 int origin[3]={foot[0],foot[1]-0x400,foot[2]},down[3]={0,0x800,0};return worldRay(origin,down,0x8000,1,256)!=0;
}
static void ConsoleRememberGround(){
 unsigned level=mem<unsigned>(0x88278c);DWORD now=GetTickCount();
 if(level!=consoleSafeLevel){consoleSafeCount=0;consoleSafeLevel=level;consoleSafeAt=0;}
 if(!ConsoleGameplay()||mem<short>(0x52b818)||!mem<short>(0x52f38e)||mem<int>(0x53c668)>0||mem<int>(0x53c66c)||(mem<unsigned>(0x53c828)&~1u)||now-consoleSafeAt<250)return;
 consoleSafeAt=now;int position[3];memcpy(position,&mem<int>(0x52f300),12);if(!ConsoleClearPoint(position))return;
 if(consoleSafeCount==32){memmove(consoleSafe,consoleSafe+1,31*sizeof(ConsoleSafePoint));--consoleSafeCount;}
 memcpy(consoleSafe[consoleSafeCount].position,position,12);consoleSafe[consoleSafeCount++].time=now;
}
static void ConsoleTeleport(const int* position){
 memcpy(&mem<int>(0x52f300),position,12);memcpy(&mem<int>(0x52f35c),position,12);
 mem<int>(0x53c668)=-1;mem<int>(0x53c66c)=0;mem<int>(0x53c660)=0;mem<int>(0x53c618)=0;mem<int>(0x53c648)=0;mem<int>(0x53c61c)=0;mem<int>(0x53c658)=12;
 mem<int>(0x52f368)=mem<int>(0x52f36c)=mem<int>(0x52f370)=0;mem<short>(0x52f38e)=mem<short>(0x52f39c)=0;
 mem<short>(0x52f38c)=5;mem<short>(0x52f390)=3;runSpeed=0;pitchInitialized=false;rayCameraTime=0;
}
static bool ConsoleUnstuck(){
 int current[3];memcpy(current,&mem<int>(0x52f300),12);DWORD now=GetTickCount();
 for(unsigned i=consoleSafeCount;i>0;--i){const ConsoleSafePoint& point=consoleSafe[i-1];if(now-point.time<750)continue;
  int64_t distance=0;for(unsigned a=0;a<3;a++){int64_t d=((int64_t)point.position[a]-current[a])/256;distance+=d*d;}
  if(distance<24*24||distance>768*768||!ConsoleClearPoint(point.position))continue;ConsoleTeleport(point.position);return true;
 }
 // Search nearby ground and reject candidates without floor/head clearance.
 for(int radius=0x2000;radius<=0x8000;radius+=0x2000)for(int direction=0;direction<8;direction++){
  double angle=direction*6.28318530718/8;int point[3]={current[0]+(int)(sin(angle)*radius),current[1]-0x3000,current[2]+(int)(cos(angle)*radius)},down[3]={0,0x9000,0};
  if(!worldRay(point,down,0x8000,1,256))continue;point[1]+=down[1];if(!ConsoleClearPoint(point))continue;ConsoleTeleport(point);return true;
 }
 return false;
}
static int ConsolePeerLevel(int request){typedef int(__cdecl *Fn)(int);Fn fn=(Fn)CoopEntry("CoopConsoleLevel");return fn?fn(request):request;}
static void ConsoleQueueLevel(int level){
 if(level<1||level>15||!ConsoleGameplay())return;
 consoleLevelRequest=level;restorePending=false;consoleSafeCount=0;consoleOpen=false;
 mem<short>(0x52b818)=0;mem<short>(0x52b7dc)=6;mem<short>(0x52f2dc)=0;
}
static void ConsoleExecute(){
 int command=ParseConsoleCommand(consoleLine);
 if(command>0)ConsoleQueueLevel(ConsolePeerLevel(command));
 else if(command==-1)strcpy(consoleReply,ConsoleUnstuck()?"moved to nearby clear ground":"no safe nearby ground found");
 else strcpy(consoleReply,"commands    lvl1 to lvl15    unstuck");
 consoleLength=0;consoleLine[0]=0;
}
static bool ConsolePoll(){
 int target=ConsoleGameplay()?ConsolePeerLevel(0):0;if(target)ConsoleQueueLevel(target);
 if(ConsoleGameplay()&&LevelComplete(mem<int>(0x88278c)))CoopAdvanceCampaign(mem<int>(0x88278c));
 if(!Focused())return consoleOpen;
 bool wasOpen=consoleOpen;ConsoleRememberGround();
 bool down[256]={};for(int key=0;key<256;key++)down[key]=(GetAsyncKeyState(key)&0x8000)!=0;
 if(down[VK_OEM_3]&&!consoleKeys[VK_OEM_3]&&ConsoleGameplay()){consoleOpen=!consoleOpen;consoleLength=0;consoleLine[0]=0;}
 if(consoleOpen){
  if(down[VK_ESCAPE]&&!consoleKeys[VK_ESCAPE])consoleOpen=false;
  else if(down[VK_RETURN]&&!consoleKeys[VK_RETURN])ConsoleExecute();
  else if(down[VK_BACK]&&!consoleKeys[VK_BACK]&&consoleLength)consoleLine[--consoleLength]=0;
  else for(int key='0';key<='Z';key++)if((key<='9'||key>='A')&&down[key]&&!consoleKeys[key]&&consoleLength<30){consoleLine[consoleLength++]=(char)(key>='A'?key+32:key);consoleLine[consoleLength]=0;}
 }
 bool consume=wasOpen||consoleOpen||consoleKeys[VK_OEM_3]||down[VK_OEM_3];memcpy(consoleKeys,down,sizeof(down));return consume;
}
static void DrawGameConsole(){
 if(!consoleOpen)return;typedef void(__cdecl *Glyph)(int,int,int,int,int,int,int);Glyph glyph=(Glyph)0x49b630;
 const char* lines[3]={"console",consoleLine,consoleReply};for(unsigned row=0;row<3;row++)for(unsigned i=0;lines[row][i]&&i<38;i++){int x=10+i*8,y=20+row*12;glyph(x+1,y+1,lines[row][i],0,0,0,0);glyph(x,y,lines[row][i],255,255,255,0);}
}

static int CoopCampaignConnected(){typedef int(__cdecl *Fn)();Fn connected=(Fn)CoopEntry("CoopCampaignActive");return connected?connected():0;}
static int(*campaignConnected)()=CoopCampaignConnected;
static bool CoopAdvanceCampaign(int level){
 if(level<1||level>=15)return false;
 if(!campaignConnected())return false;
 int target=ConsolePeerLevel(level+1);if(target){ConsoleQueueLevel(target);return consoleLevelRequest==target;}return false;
}
