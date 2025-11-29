option casemap:none

_TEXT SEGMENT ALIGN(16)

PUBLIC VmrunAsm
VmrunAsm PROC
    mov     rax, rcx
    vmrun   rax
    ret
VmrunAsm ENDP

PUBLIC GuestEntry
GuestEntry PROC
GuestLoop:
    mov     rax, 1337h
    vmmcall
    hlt
    jmp     GuestLoop
GuestEntry ENDP

_TEXT ENDS
END
