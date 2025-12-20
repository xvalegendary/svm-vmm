option casemap:none

_TEXT SEGMENT ALIGN(16)

PUBLIC VmrunAsm
VmrunAsm PROC
    ; rcx = GUEST_REGS*, rdx = VMCB PA
    push    rbp
    push    rbx
    push    rsi
    push    rdi
    push    r12
    push    r13
    push    r14
    push    r15

    sub     rsp, 16
    mov     qword ptr [rsp], rcx
    mov     qword ptr [rsp + 8], rdx

    ; load guest regs
    mov     rax, rcx
    mov     rbx, qword ptr [rax + 0]
    mov     rcx, qword ptr [rax + 8]
    mov     rdx, qword ptr [rax + 16]
    mov     rsi, qword ptr [rax + 24]
    mov     rdi, qword ptr [rax + 32]
    mov     rbp, qword ptr [rax + 40]
    mov     r8,  qword ptr [rax + 48]
    mov     r9,  qword ptr [rax + 56]
    mov     r10, qword ptr [rax + 64]
    mov     r11, qword ptr [rax + 72]
    mov     r12, qword ptr [rax + 80]
    mov     r13, qword ptr [rax + 88]
    mov     r14, qword ptr [rax + 96]
    mov     r15, qword ptr [rax + 104]

    mov     rax, qword ptr [rsp + 8]
    vmrun   rax

    ; save guest regs
    mov     rax, qword ptr [rsp]
    mov     qword ptr [rax + 0], rbx
    mov     qword ptr [rax + 8], rcx
    mov     qword ptr [rax + 16], rdx
    mov     qword ptr [rax + 24], rsi
    mov     qword ptr [rax + 32], rdi
    mov     qword ptr [rax + 40], rbp
    mov     qword ptr [rax + 48], r8
    mov     qword ptr [rax + 56], r9
    mov     qword ptr [rax + 64], r10
    mov     qword ptr [rax + 72], r11
    mov     qword ptr [rax + 80], r12
    mov     qword ptr [rax + 88], r13
    mov     qword ptr [rax + 96], r14
    mov     qword ptr [rax + 104], r15

    add     rsp, 16
    pop     r15
    pop     r14
    pop     r13
    pop     r12
    pop     rdi
    pop     rsi
    pop     rbx
    pop     rbp
    ret
VmrunAsm ENDP

PUBLIC GuestEntry
GuestEntry PROC
guest_loop:
    hlt
    jmp guest_loop
GuestEntry ENDP

_TEXT ENDS
END
