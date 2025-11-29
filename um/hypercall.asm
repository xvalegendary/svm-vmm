option casemap:none

.code

; uint64_t hv_vmcall(uint64_t code, uint64_t arg1, uint64_t arg2, uint64_t arg3)
hv_vmcall PROC
    push rbx

    ; map windows x64 calling convention arguments to the registers
    ; expected by HookVmmcallDispatch inside the hypervisor
    mov rax, rcx ; code
    mov rbx, rdx ; arg1
    mov rcx, r8  ; arg2
    mov rdx, r9  ; arg3

    ; vmmcall (0f 01 d9) transfers control to the hypervisor when
    ; the svm vmmcall intercept is enabled
    db 0fh, 01h, 0d9h

    pop rbx
    ret
hv_vmcall ENDP

END
