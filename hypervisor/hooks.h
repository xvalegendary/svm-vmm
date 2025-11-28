#pragma once
#include <ntifs.h>
#include "vcpu.h"

// CPUID
VOID HookCpuidEmulate(UINT32 leaf, UINT32 subleaf,
    UINT32* eax, UINT32* ebx, UINT32* ecx, UINT32* edx);

// MSR read/write
UINT64 HookHandleMsrRead(VCPU* V, UINT64 msr);
VOID   HookHandleMsrWrite(VCPU* V, UINT64 msr, UINT64 value);

// Syscall hook control
VOID HookInstallSyscall(VCPU* V);
VOID HookRemoveSyscall();

// CR3 encryption/decryption
UINT64 HookEncryptCr3(UINT64 cr3);
UINT64 HookDecryptCr3(struct _VCPU* V, UINT64 cr3_enc);
VOID HookEnableCr3Encryption();
VOID HookDisableCr3Encryption();

// NPT helpers
BOOLEAN HookNptHandleFault(VCPU* V, UINT64 faultingGpa);

// VMMCALL API
UINT64 HookVmmcallDispatch(VCPU* V, UINT64 code, UINT64 a1, UINT64 a2, UINT64 a3);

// IO intercept
VOID HookIoIntercept(VCPU* V);
