
option casemap:none

EXTERN ShadowIdtCommonHandler:PROC
EXTERN g_CurrentVcpu:QWORD ;      VCPU*

_TEXT SEGMENT ALIGN(16)

PUBLIC ShadowIdtAsmHandler

ShadowIdtAsmHandler PROC


    push rax
    push rcx
    push rdx
    push rbx
    push rbp
    push rsi
    push rdi
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15


    mov rax, qword ptr [rsp + 78h]     ; vector
    mov rbx, qword ptr [rsp + 80h]     ; error code


  

    mov rcx, g_CurrentVcpu    
    mov rdx, rax               
    mov r8,  rbx            

    sub rsp, 20h         

    call ShadowIdtCommonHandler

    add rsp, 20h

    ;
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rdi
    pop rsi
    pop rbp
    pop rbx
    pop rdx
    pop rcx
    pop rax

   
    iretq

ShadowIdtAsmHandler ENDP

_TEXT ENDS
END
