/* v0.3 gap candidate: hook projection, retain original movement/setter logic.
 * No legacy opposing-Z clamp. No network or per-frame Win32 calls/file writes.
 */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdint.h>
#include "gap_geometry.h"
#include "signatures.h"
_Static_assert(sizeof(void*)==4,"x86 required");
typedef struct { DWORD sequence,algorithm,applied,reason,actor,data,anim;float inputY,dt;
    DWORD segment,selectedRef,neighborRef;Vec contact,original,result,nearEnd,farEnd,neighbor;
    float gap2;DWORD committed; } Record;
_Static_assert(sizeof(Record)==128,"record layout");
typedef struct { volatile DWORD algorithm,dirs,recording,producer,phase,eligible,bridges,
    missing,ambiguous,rejected;DWORD magic;unsigned char reserved[212];Record ring[4096]; } State;
_Static_assert(__builtin_offsetof(State,ring)==256,"ring layout");
static State state;
__declspec(dllexport) volatile DWORD POP3ChainFixStatus;
__declspec(dllexport) volatile DWORD POP3ChainFixState;
/* Assembly references are global, unexported symbols. */
void *g_projector=(void*)0x4B4440u;
void *g_return=(void*)0x73564Au;
static HMODULE self;
static volatile LONG initialized;
static unsigned char *site;
static WCHAR ini[MAX_PATH],logpath[MAX_PATH];
static void bytes(void *out,const void *in,unsigned n){
    volatile unsigned char *d=out;const unsigned char *s=in;for(unsigned i=0;i<n;++i)d[i]=s[i];
}
static int equal(const void *a,const void *b,unsigned n){
    const unsigned char *x=a,*y=b;for(unsigned i=0;i<n;++i)if(x[i]!=y[i])return 0;return 1;
}
static void log_text(const char *text){
    if(!logpath[0])return;
    HANDLE h=CreateFileW(logpath,GENERIC_WRITE,FILE_SHARE_READ,NULL,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);
    if(h==INVALID_HANDLE_VALUE)return;DWORD n=0,written=0;while(text[n])++n;
    WriteFile(h,text,n,&written,NULL);CloseHandle(h);
}
static int paths(void){
    DWORD n=GetModuleFileNameW(self,ini,MAX_PATH);if(!n||n>=MAX_PATH-1)return 0;
    unsigned p=n;while(p&&ini[p-1]!=L'.'&&ini[p-1]!=L'\\')--p;
    if(!p||ini[p-1]!=L'.'||p+3>=MAX_PATH)return 0;
    for(unsigned i=0;i<p;++i)logpath[i]=ini[i];
    ini[p]=L'i';ini[p+1]=L'n';ini[p+2]=L'i';ini[p+3]=0;
    logpath[p]=L'l';logpath[p+1]=L'o';logpath[p+2]=L'g';logpath[p+3]=0;return 1;
}
static unsigned word(const void *p){return *(const unsigned*)p;}
static unsigned parent(unsigned actor){
    unsigned c=word((void*)(actor+0x14)),node=c?word((void*)(c+8)):0;
    return node?word((void*)(node+4)):0;
}
static int eligible(unsigned actor,unsigned data,unsigned *anim,float *y){
    if(!actor||!data||word((void*)(data+0x126C))!=0x3A0)return 0;
    unsigned c=word((void*)(actor+0x14)),a=c?word((void*)c):0;
    if(!a)return 0;*anim=*(unsigned short*)(a+0x38);
    if(word((void*)(data+0x60))||word((void*)(data+0x100)))return 0;
    FloatBits x;x.u=word((void*)(data+0xDE0));if(x.u&0x7FFFFFFFu)return 0;
    *y=*(float*)(data+0xDE4);unsigned mode=word((void*)(data+0x64));
    return ((*y==1.0f&&*anim==0x3A0&&mode==1&&(state.dirs&1))||
            (*y==-1.0f&&*anim==0x39F&&mode==0&&(state.dirs&2)));
}
typedef void (__cdecl *Endpoints)(unsigned,unsigned,unsigned,Vec*);
/* saved registers are the post-original-projector PUSHAD frame. */
__declspec(dllexport) void __cdecl ChainGapTick(const DWORD *saved){
    unsigned char *sp=(unsigned char*)saved+36;
    unsigned data=saved[1],actor=word((void*)0xDF972C),anim=0,index=word(sp+4);
    float y=0;if(word(sp)!=0||!eligible(actor,data,&anim,&y))return;
    ++state.eligible;
    Vec *contact=(Vec*)(uintptr_t)word(sp+8),*projection=(Vec*)(uintptr_t)word(sp+12);
    Vec original=*projection,result=original,nearEnd={0,0,0},farEnd={0,0,0},neighbor={0,0,0};
    float gap2=0;unsigned ref=0,neighborRef=0,reason=0,matches=0;
    unsigned manager=word((void*)0xE3FA20)+0x318,count=word((void*)(manager+0x44));
    if(count>10||index>=count){reason=5;++state.rejected;goto record;}
    unsigned entry=manager+0x48+12*index,shape=word((void*)(entry+8));ref=word((void*)(entry+4));
    if(!shape||!ref){reason=5;++state.rejected;goto record;}
    Endpoints endpoints=(Endpoints)0x4B4280u;
    Vec a={0,0,0},b={0,0,0};endpoints(0,index,0,&a);endpoints(0,index,1,&b);
    if(!outside_endpoint(*contact,original,a,b,&nearEnd,&farEnd))goto record;
    unsigned owner=parent(ref);
    for(unsigned i=0;i<count;++i){
        if(i==index)continue;
        unsigned e=manager+0x48+12*i,r=word((void*)(e+4));
        if(!r||r==ref||word((void*)(e+8))!=shape||parent(r)!=owner)continue;
        Vec n,q;float g;
        endpoints(0,i,0,&a);endpoints(0,i,1,&b);
        if(!bridge_candidate(*contact,nearEnd,farEnd,a,b,&n,&q,&g))continue;
        if(matches&&r==neighborRef)continue; /* repeated entry for same reference */
        ++matches;neighborRef=r;neighbor=n;result=q;gap2=g;
    }
    if(matches==0){reason=1;++state.missing;result=original;}
    else if(matches>1){reason=2;++state.ambiguous;result=original;}
    else if(state.algorithm==1){reason=3;*projection=result;++state.bridges;}
    else {reason=4;result=original;}
record:
    if(state.recording){
        unsigned seq=state.producer+1;Record *r=&state.ring[seq&4095];r->committed=0;
        r->sequence=seq;r->algorithm=state.algorithm;r->applied=(reason==3);r->reason=reason;
        r->actor=actor;r->data=data;r->anim=anim;r->inputY=y;r->dt=*(float*)0xA05180;
        r->segment=index;r->selectedRef=ref;r->neighborRef=neighborRef;
        r->contact=*contact;r->original=original;r->result=result;r->nearEnd=nearEnd;r->farEnd=farEnd;
        r->neighbor=neighbor;r->gap2=gap2;
        __asm__ __volatile__("" ::: "memory");r->committed=seq;__asm__ __volatile__("" ::: "memory");state.producer=seq;
    }
}
/* Preserve post-projector integer state and the complete FPU/SSE environment.
 * Helper's scratch allocation is aligned for FXSAVE; original arguments stay put.
 */
__asm__(".text\n.global _ChainGapHook\n_ChainGapHook:\n"
        "call *_g_projector\n"
        "pushfl\npushal\nmovl %esp,%ebp\nsubl $544,%esp\nandl $-16,%esp\n"
        "fxsave (%esp)\npushl %ebp\ncall _ChainGapTick\naddl $4,%esp\n"
        "fxrstor (%esp)\nmovl %ebp,%esp\npopal\npopfl\njmp *_g_return\n");
__declspec(dllexport) extern void ChainGapHook(void);
__declspec(dllexport) void InitializeASI(void){
    if(InterlockedCompareExchange(&initialized,1,0)!=0)return;
    if(!paths())return;POP3ChainFixStatus=3;
    unsigned char *base=(unsigned char*)GetModuleHandleW(NULL);
    if((uintptr_t)base!=0x400000u){log_text("Gap fix INACTIVE: unsupported base.\r\n");return;}
    IMAGE_DOS_HEADER *dos=(IMAGE_DOS_HEADER*)base;
    if(dos->e_magic!=IMAGE_DOS_SIGNATURE||dos->e_lfanew<=0||dos->e_lfanew>0x1000)return;
    IMAGE_NT_HEADERS32 *pe=(IMAGE_NT_HEADERS32*)(base+dos->e_lfanew);
    unsigned char *s=base+GAP_SITE_RVA;
    if(pe->Signature!=IMAGE_NT_SIGNATURE||pe->FileHeader.Machine!=0x14C||
       pe->OptionalHeader.SizeOfImage!=GAP_IMAGE_SIZE||pe->FileHeader.TimeDateStamp!=GAP_TIMESTAMP||
       !equal(s,kGapCall,5)||!equal(s-16,kGapCaller,16)||
       !equal(base+0x335270,kGapFunction,32)||!equal(base+0xB4440,kGapProjector,32)||
       !equal(base+0xB4280,kGapEndpoints,32)||!equal(base+0x3357C8,kOldSetterCall,5)){
        log_text("Gap fix INACTIVE: unsupported build or conflicting hook.\r\n");return;
    }
    state.algorithm=1;state.magic=0x43475033u;
    state.dirs=(GetPrivateProfileIntW(L"ChainFix",L"EnableDescent",1,ini)?1:0)|
               (GetPrivateProfileIntW(L"ChainFix",L"EnableAscent",1,ini)?2:0);
    if(!state.dirs||!GetPrivateProfileIntW(L"ChainFix",L"EnableGapBridge",1,ini)){
        POP3ChainFixStatus=2;log_text("Gap fix DISABLED.\r\n");return;
    }
    DWORD old=0,unused=0;if(!VirtualProtect(s,5,PAGE_EXECUTE_READWRITE,&old)){
        POP3ChainFixStatus=4;log_text("Gap fix INACTIVE: code protection failed.\r\n");return;}
    unsigned char jump[5]={0xE9,0,0,0,0};
    DWORD displacement=(DWORD)((uintptr_t)ChainGapHook-(uintptr_t)(s+5));bytes(jump+1,&displacement,4);bytes(s,jump,5);
    BOOL flushed=FlushInstructionCache(GetCurrentProcess(),s,5);
    BOOL restored=VirtualProtect(s,5,old,&unused);
    if(!flushed||!restored){
        DWORD rollback=0;if(VirtualProtect(s,5,PAGE_EXECUTE_READWRITE,&rollback)){
            bytes(s,kGapCall,5);FlushInstructionCache(GetCurrentProcess(),s,5);VirtualProtect(s,5,old,&unused);
        }
        POP3ChainFixStatus=4;log_text("Gap fix startup failed; restart without plugin.\r\n");return;
    }
    site=s;POP3ChainFixState=(DWORD)(uintptr_t)&state;POP3ChainFixStatus=0x300|state.dirs;
    log_text("POP3ChainFix v0.3: GAP BRIDGE ACTIVE. Legacy Z clamp removed. Geometry gameplay test pending.\r\n");
}
BOOL WINAPI DllMain(HINSTANCE module,DWORD reason,LPVOID reserved){
    if(reason==DLL_PROCESS_ATTACH)self=module;
    else if(reason==DLL_PROCESS_DETACH&&!reserved&&site){
        DWORD old=0,unused=0;if(VirtualProtect(site,5,PAGE_EXECUTE_READWRITE,&old)){
            bytes(site,kGapCall,5);FlushInstructionCache(GetCurrentProcess(),site,5);VirtualProtect(site,5,old,&unused);
        }
    }
    return TRUE;
}
