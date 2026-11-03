#include <windows.h>
#include <stdint.h>
#include <assert.h>
#include <stdio.h>
#include "../src/buffer_policy.h"
extern uintptr_t qq_buffer_return;
extern void qq_buffer_detour(void);
/* Same stack slot and preserved ESI/EDI as the validated native instruction
 * contract. Runs the real x86 assembly bridge in an isolated process. */
__attribute__((naked)) unsigned __cdecl bridge(void *object,unsigned requested,unsigned mode){
 __asm__ __volatile__(
 ".intel_syntax noprefix\n"
 "push ebp\n mov ebp,esp\n push esi\n push edi\n sub esp,16\n"
 "mov esi,[ebp+8]\n mov edi,[ebp+12]\n mov eax,[ebp+16]\n mov [esp+12],eax\n"
 "lea eax,[qq_test_resume]\n mov dword ptr [_qq_buffer_return],eax\n"
 "jmp _qq_buffer_detour\n"
 "qq_test_resume:\n mov eax,edi\n add esp,16\n pop edi\n pop esi\n pop ebp\n ret\n"
 ".att_syntax prefix\n");
}
int main(void){assert(qq_buffer_hint(16384,2048,44100,1)==16384);assert(qq_buffer_hint(16384,2048,44100,0)==2048);assert(qq_buffer_hint(176128,22016,44100,1)==22016);assert(qq_buffer_hint(4096,2048,44100,1)==2048);assert(qq_buffer_hint(21504,2688,44100,1)==21504);assert(qq_buffer_hint(441,512,44100,1)==512);assert(qq_buffer_hint(16384,0,44100,1)==0);assert(qq_buffer_hint(1,128,0,1)==128);unsigned char obj[32]={0},format[40]={0};*(void**)(obj+0x10)=format;*(unsigned*)(obj+0x18)=16384;*(unsigned*)(format+4)=44100;assert(bridge(obj,2048,1)==16384);assert(bridge(obj,2048,0)==2048);*(unsigned*)(obj+0x18)=44100;assert(bridge(obj,2048,1)==2048);puts("PASS: buffer compatibility bounds, native shared selection and real x86 register/stack bridge");return 0;}
