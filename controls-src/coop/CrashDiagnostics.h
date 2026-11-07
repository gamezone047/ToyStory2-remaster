#pragma comment(lib,"user32.lib")
// Persistent diagnostics survive local-test directory cleanup. No per-frame disk IO.
static HANDLE diagnosticSession=INVALID_HANDLE_VALUE;
static char diagnosticDirectory[MAX_PATH],diagnosticCrash[MAX_PATH];
static volatile LONG diagnosticExceptions;
static const char* diagnosticPhase="startup";
static uintptr_t diagnosticActor;
static DWORD diagnosticLastWrite;
static LPTOP_LEVEL_EXCEPTION_FILTER previousExceptionFilter;
static void DiagnosticWrite(HANDLE file,const char* text){if(file!=INVALID_HANDLE_VALUE){DWORD written;WriteFile(file,text,(DWORD)strlen(text),&written,NULL);}}
static void DiagnosticState(HANDLE file){
 char line[512];wsprintfA(line,"build=2026.11.7.5 pid=%lu role=%s tick=%lu phase=%s actor=%08lx level=%u sequence=%u peerLevel=%u peerSequence=%u helperTick=%lu\r\n",
  GetCurrentProcessId(),localOwner?"client":"host",GetTickCount(),diagnosticPhase,(DWORD)diagnosticActor,cachedLevel,localFrame.sequence,remoteFrame.level,remoteFrame.sequence,helperTick);
 DiagnosticWrite(file,line);
 wsprintfA(line,"host/local position=%ld,%ld,%ld hp=%d peer=%ld,%ld,%ld hp=%d\r\n",localFrame.pose.x,localFrame.pose.y,localFrame.pose.z,localFrame.pose.health,remoteFrame.pose.x,remoteFrame.pose.y,remoteFrame.pose.z,remoteFrame.pose.health);DiagnosticWrite(file,line);
}
static void DiagnosticException(EXCEPTION_POINTERS* exception,const char* kind){
 if(!diagnosticCrash[0]||!exception||!exception->ExceptionRecord||!exception->ContextRecord||InterlockedIncrement(&diagnosticExceptions)>4)return;
 HANDLE file=CreateFileA(diagnosticCrash,GENERIC_WRITE,FILE_SHARE_READ,NULL,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);if(file==INVALID_HANDLE_VALUE)return;
 DiagnosticWrite(file,kind);DiagnosticWrite(file,"\r\n");DiagnosticState(file);
 const EXCEPTION_RECORD& e=*exception->ExceptionRecord;const CONTEXT& c=*exception->ContextRecord;
 char line[768];wsprintfA(line,"exception=%08lx address=%08lx EIP=%08lx ESP=%08lx EBP=%08lx EAX=%08lx EBX=%08lx ECX=%08lx EDX=%08lx\r\n",e.ExceptionCode,(DWORD)e.ExceptionAddress,c.Eip,c.Esp,c.Ebp,c.Eax,c.Ebx,c.Ecx,c.Edx);DiagnosticWrite(file,line);
 if(e.NumberParameters>=2){wsprintfA(line,"operation=%lu memory=%08lx\r\n",e.ExceptionInformation[0],e.ExceptionInformation[1]);DiagnosticWrite(file,line);}
 MEMORY_BASIC_INFORMATION info;char module[MAX_PATH]={};
 if(VirtualQuery(e.ExceptionAddress,&info,sizeof(info))&&GetModuleFileNameA((HMODULE)info.AllocationBase,module,MAX_PATH)){
  wsprintfA(line,"module=%s base=%08lx offset=%08lx\r\n",module,(DWORD)info.AllocationBase,(DWORD)e.ExceptionAddress-(DWORD)info.AllocationBase);DiagnosticWrite(file,line);
 }
 DWORD stack[32]={};SIZE_T bytes=0;
 if(ReadProcessMemory(GetCurrentProcess(),(void*)c.Esp,stack,sizeof(stack),&bytes))for(unsigned i=0;i<bytes/4;i++){
  wsprintfA(line,"stack+%03x=%08lx\r\n",i*4,stack[i]);DiagnosticWrite(file,line);
 }
 FlushFileBuffers(file);CloseHandle(file);
}
static LONG CALLBACK DiagnosticVectored(EXCEPTION_POINTERS* e){
 DWORD code=e->ExceptionRecord->ExceptionCode;
 if(code==EXCEPTION_ACCESS_VIOLATION||code==EXCEPTION_ILLEGAL_INSTRUCTION||code==EXCEPTION_INT_DIVIDE_BY_ZERO||code==EXCEPTION_INT_OVERFLOW)
  DiagnosticException(e,"Severe exception observed; Windows may still handle this exception. This report does not suppress it.");
 return EXCEPTION_CONTINUE_SEARCH;
}
static LONG WINAPI DiagnosticUnhandled(EXCEPTION_POINTERS* e){
 DiagnosticException(e,"Unhandled game exception");
 return previousExceptionFilter?previousExceptionFilter(e):EXCEPTION_CONTINUE_SEARCH;
}
static void DiagnosticHeartbeat(){
 if(diagnosticSession==INVALID_HANDLE_VALUE)return;
 DWORD now=GetTickCount();if(now-diagnosticLastWrite<2000)return;diagnosticLastWrite=now;
 if(GetFileSize(diagnosticSession,NULL)>2*1024*1024){SetFilePointer(diagnosticSession,0,NULL,FILE_BEGIN);SetEndOfFile(diagnosticSession);}
 DiagnosticState(diagnosticSession);
}
static void InstallDiagnostics(){
 DWORD count=GetEnvironmentVariableA("LOCALAPPDATA",diagnosticDirectory,MAX_PATH);
 if(!count||count>MAX_PATH-64){diagnosticDirectory[0]=0;return;}
 strcat(diagnosticDirectory,"\\ToyStory2");CreateDirectoryA(diagnosticDirectory,NULL);
 strcat(diagnosticDirectory,"\\Diagnostics");CreateDirectoryA(diagnosticDirectory,NULL);
 char session[MAX_PATH],previous[MAX_PATH];const char* role=localOwner?"client":"host";
 wsprintfA(session,"%s\\%s-session.log",diagnosticDirectory,role);wsprintfA(previous,"%s\\%s-previous-session.log",diagnosticDirectory,role);
 MoveFileExA(session,previous,MOVEFILE_REPLACE_EXISTING);
 diagnosticSession=CreateFileA(session,GENERIC_WRITE,FILE_SHARE_READ,NULL,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);
 wsprintfA(diagnosticCrash,"%s\\%s-crash.log",diagnosticDirectory,role);
 DiagnosticState(diagnosticSession);
 AddVectoredExceptionHandler(1,DiagnosticVectored);previousExceptionFilter=SetUnhandledExceptionFilter(DiagnosticUnhandled);
}
