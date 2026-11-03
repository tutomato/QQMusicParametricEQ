#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <tlhelp32.h>
#include <stdint.h>
#include "buffer_policy.h"
/* Runtime-only, feature-checked x86 compatibility fix. No client file changes.
 * Pin the plugin because the native output path can outlive DSP enable/disable.
 * The native shared/exclusive selection remains authoritative. */
__attribute__((used)) uintptr_t qq_buffer_return;
static volatile LONG guardState,guardCorrections;
static HMODULE guardModule;
static void *notificationCookie;
static volatile LONG guardStarted;
__attribute__((used,noinline)) unsigned __cdecl qq_accept_buffer(void *object,unsigned requested,unsigned mode){
 unsigned char *p=object,*format=*(unsigned char**)(p+0x10);
 unsigned actual=*(unsigned*)(p+0x18),rate=*(unsigned*)(format+4);
 unsigned hint=qq_buffer_hint(actual,requested,rate,mode);
 if(hint!=requested)InterlockedIncrement(&guardCorrections);
 return hint;
}
__attribute__((naked,used)) void qq_buffer_detour(void){
 __asm__ __volatile__(
 ".intel_syntax noprefix\n"
 "pushfd\n pushad\n"
 "mov eax,[esp]\n mov edx,[esp+4]\n mov ecx,[esp+48]\n"
 "push ecx\n push eax\n push edx\n call _qq_accept_buffer\n add esp,12\n"
 "mov [esp],eax\n popad\n popfd\n"
 "mov ecx,[esi+0x18]\n lea eax,[edi+edi]\n"
 "jmp dword ptr [_qq_buffer_return]\n"
 ".att_syntax prefix\n");
}
static unsigned char *findSite(HMODULE module){
 unsigned char *base=(unsigned char*)module;
 IMAGE_DOS_HEADER *dos=(IMAGE_DOS_HEADER*)base;
 if(dos->e_magic!=IMAGE_DOS_SIGNATURE||dos->e_lfanew<=0||dos->e_lfanew>4096)return NULL;
 IMAGE_NT_HEADERS32 *nt=(IMAGE_NT_HEADERS32*)(base+dos->e_lfanew);
 if(nt->Signature!=IMAGE_NT_SIGNATURE||nt->FileHeader.Machine!=IMAGE_FILE_MACHINE_I386)return NULL;
 if(!GetProcAddress(module,"QMP_CreateWSAPIRender")||!GetProcAddress(module,"QMP_DestroyWSAPIRender"))return NULL;
 /* Whole instruction contract surrounding the size ratio check. The short JA
  * displacement may change; all other bytes must match exactly and uniquely. */
 static const unsigned char pattern[]={0x8b,0x4e,0x18,0x8d,0x04,0x3f,0x33,0xd2,0x3b,0xc8,0x77,0,0x8b,0x46,0x10,0x66,0x0f,0x6e,0xc1,0xf3,0x0f,0xe6,0xc0,0x8b,0x40,0x04,0x66,0x0f,0x6e,0xc8};
 unsigned char *answer=NULL;IMAGE_SECTION_HEADER *sec=IMAGE_FIRST_SECTION(nt);
 for(unsigned k=0;k<nt->FileHeader.NumberOfSections;k++,sec++){
  if(!(sec->Characteristics&IMAGE_SCN_MEM_EXECUTE))continue;
  DWORD size=sec->Misc.VirtualSize;
  if(sec->VirtualAddress>=nt->OptionalHeader.SizeOfImage||size>nt->OptionalHeader.SizeOfImage-sec->VirtualAddress)continue;
  unsigned char *start=base+sec->VirtualAddress;
  for(DWORD i=0;i+sizeof(pattern)<=size;i++){
   unsigned j=0;for(;j<sizeof(pattern);j++)if(j!=11&&start[i+j]!=pattern[j])break;
   if(j==sizeof(pattern)){
    if(answer)return NULL;
    intptr_t destination=(intptr_t)(start+i+12)+(signed char)start[i+11];
    if(destination<(intptr_t)start||destination>=(intptr_t)(start+size))return NULL;
    answer=start+i;
   }
  }
 }
 return answer;
}
static int installGuard(HMODULE module,int newlyLoading){
 if(guardState==1&&guardModule==module)return 1;
 unsigned char *site=findSite(module);if(!site){InterlockedExchange(&guardState,-1);return 0;}
 qq_buffer_return=(uintptr_t)(site+6);
 HANDLE threads[512];unsigned count=0,suspended=0;int ok=1;
 if(!newlyLoading){
  /* Resolve/open everything before suspending, avoiding heap work while another
   * thread might own a process heap lock. Relocate an IP at either overwritten
   * instruction to our equivalent instruction sequence. */
  HANDLE snapshot=CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD,0);
  if(snapshot==INVALID_HANDLE_VALUE){InterlockedExchange(&guardState,-2);return 0;}
  THREADENTRY32 te={sizeof(te)};
  if(Thread32First(snapshot,&te))do{
   if(te.th32OwnerProcessID==GetCurrentProcessId()&&te.th32ThreadID!=GetCurrentThreadId()){
    if(count==512){ok=0;break;}
    HANDLE h=OpenThread(THREAD_SUSPEND_RESUME|THREAD_GET_CONTEXT|THREAD_SET_CONTEXT|THREAD_QUERY_INFORMATION,FALSE,te.th32ThreadID);
    if(h)threads[count++]=h;else {ok=0;break;}
   }
  }while(Thread32Next(snapshot,&te));
  CloseHandle(snapshot);
  for(unsigned i=0;ok&&i<count;i++){
   if(SuspendThread(threads[i])==(DWORD)-1){ok=0;break;}suspended++;
   CONTEXT c={0};c.ContextFlags=CONTEXT_CONTROL;
   if(!GetThreadContext(threads[i],&c)){ok=0;break;}
   if(c.Eip>=(DWORD)(uintptr_t)site&&c.Eip<(DWORD)(uintptr_t)(site+6)){
    if(c.Eip!=(DWORD)(uintptr_t)site&&c.Eip!=(DWORD)(uintptr_t)(site+3)){ok=0;break;}
    c.Eip=(DWORD)(uintptr_t)qq_buffer_detour;if(!SetThreadContext(threads[i],&c)){ok=0;break;}
   }
  }
 }
 DWORD oldProtect=0;
 if(ok&&!VirtualProtect(site,6,PAGE_EXECUTE_READWRITE,&oldProtect))ok=0;
 if(ok){
  qq_buffer_return=(uintptr_t)(site+6);
  unsigned char patch[6]={0xe9,0,0,0,0,0x90};
  *(uint32_t*)(patch+1)=(uint32_t)((uintptr_t)qq_buffer_detour-(uintptr_t)(site+5));
  for(unsigned i=0;i<6;i++)site[i]=patch[i];
  FlushInstructionCache(GetCurrentProcess(),site,6);
  DWORD ignored;VirtualProtect(site,6,oldProtect,&ignored);
  guardModule=module;InterlockedExchange(&guardState,1);
 }
 for(unsigned i=0;i<suspended;i++)ResumeThread(threads[i]);
 for(unsigned i=0;i<count;i++)CloseHandle(threads[i]);
 if(!ok)InterlockedExchange(&guardState,-2);
 return ok;
}
typedef struct {USHORT Length,MaximumLength;PWSTR Buffer;} QQUnicode;
typedef struct {ULONG Flags;const QQUnicode *FullDllName,*BaseDllName;PVOID DllBase;ULONG SizeOfImage;} QQDllData;
typedef union {QQDllData Loaded;QQDllData Unloaded;} QQNotification;
static int isOutputModule(const QQUnicode *name){
 static const wchar_t target[]=L"QMP_WSAPI.DLL";
 if(!name||name->Length!=(sizeof(target)-sizeof(wchar_t)))return 0;
 for(unsigned i=0;i<sizeof(target)/sizeof(wchar_t)-1;i++){wchar_t c=name->Buffer[i];if(c>=L'a'&&c<=L'z')c-=32;if(c!=target[i])return 0;}
 return 1;
}
static VOID CALLBACK outputLoaded(ULONG reason,const QQNotification*data,PVOID context){
 if(reason==1&&isOutputModule(data->Loaded.BaseDllName))installGuard((HMODULE)data->Loaded.DllBase,1);
 if(reason==2&&data->Unloaded.DllBase==guardModule){guardModule=NULL;InterlockedExchange(&guardState,0);}
}
void qq_guard_start(void){
 if(InterlockedCompareExchange(&guardStarted,1,0))return;
 HMODULE own=NULL;
 if(!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_PIN,(LPCWSTR)(uintptr_t)qq_guard_start,&own)){guardState=-2;return;}
 typedef LONG(NTAPI *RegisterNotify)(ULONG,void(CALLBACK*)(ULONG,const QQNotification*,PVOID),PVOID,PVOID*);
 RegisterNotify reg=(RegisterNotify)GetProcAddress(GetModuleHandleW(L"ntdll.dll"),"LdrRegisterDllNotification");
 if(!reg||reg(0,outputLoaded,NULL,&notificationCookie)<0){guardState=-2;return;}
 HMODULE output=GetModuleHandleW(L"QMP_WSAPI.dll");if(output)installGuard(output,0);
}
int qq_guard_state(void){return (int)InterlockedCompareExchange(&guardState,0,0);}
int qq_guard_corrections(void){return (int)InterlockedCompareExchange(&guardCorrections,0,0);}
