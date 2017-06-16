
; ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
;                               syscall.asm
; ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
;                                                     Forrest Yu, 2005
; ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

%include "sconst.inc"


INT_VECTOR_SYS_CALL  equ 0x90
_NR_get_ticks        equ 0 ; 要跟 global.c 中 sys_call_table 的定义相对应！
_NR_print_str	     equ 1
_NR_print_color_str  equ 2
_NR_sleep	     equ 3
_NR_sem_p	     equ 4
_NR_sem_v	     equ 5

; 导出符号
global	get_ticks
global  print_str
global  print_color_str
global  sleep
global  sem_p
global  sem_v

bits 32
[section .text]

; ====================================================================
;                            get_ticks
; ====================================================================
get_ticks:
	mov	eax, _NR_get_ticks
	int	INT_VECTOR_SYS_CALL
	ret
; ====================================================================
;                     void print_str(char* buf);
; ====================================================================
print_str:
        mov     eax, _NR_print_str
        mov     ebx, [esp + 4]
        int     INT_VECTOR_SYS_CALL
        ret

; ====================================================================
;             void print_color_str(char* buf,int color);
; ====================================================================
print_color_str:
        mov     eax, _NR_print_color_str
	mov     ecx, [esp + 8]
        mov     ebx, [esp + 4]
        int     INT_VECTOR_SYS_CALL
        ret

; ====================================================================
;                      void sleep(int milli_sec);
; ====================================================================
sleep:
        mov     eax, _NR_sleep
        mov     ebx, [esp + 4]
        int     INT_VECTOR_SYS_CALL
        ret

; ====================================================================
;                void sem_p(SEMAPHORE* sem,int index);
; ====================================================================
sem_p:
        mov     eax, _NR_sem_p
	mov     ecx, [esp + 8]
        mov     ebx, [esp + 4]
        int     INT_VECTOR_SYS_CALL
        ret

; ====================================================================
;                      void sem_v(SEMAPHORE* sem);
; ====================================================================
sem_v:
        mov     eax, _NR_sem_v
        mov     ebx, [esp + 4]
        int     INT_VECTOR_SYS_CALL
        ret
