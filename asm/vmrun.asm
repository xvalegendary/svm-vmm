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
    cli

guest_loop:
    hlt
    jmp     guest_loop

GuestEntry ENDP

_TEXT ENDS
END
