#include <ntifs.h>
#include "svm.h"
#include "vcpu.h"
#include "vmcb.h"
#include "guest_mem.h"
#include "npt.h"
#include "hooks.h"
#include "stealth.h"
#include "shadow_idt.h"
#include "layers.h"



static VOID HvAdvanceRIP(VCPU* V, UINT8 len)
{
    VMCB_CONTROL_AREA* c = VmcbControl(V->Vmcb);
    VMCB_STATE_SAVE_AREA* s = VmcbState(V->Vmcb);

    if (c->NextRip)
        s->Rip = c->NextRip;
    else
        s->Rip += len;
}


static VOID HvHandleCpuid(VCPU* V)
{
    VMCB_STATE_SAVE_AREA* s = VmcbState(V->Vmcb);

    UINT64 leaf = s->Rax;
    UINT64 sub = V->GuestRegs.Rcx;

    UINT32 eax, ebx, ecx, edx;
    __cpuidex((int*)&eax, (int)leaf, (int)sub);

   
    if (leaf == 1)
        ecx &= ~(1 << 31);

    if (leaf == 0x80000001)
        edx &= ~(1 << 2);

    
    StealthMaskCpuid((UINT32)leaf, &ecx, &edx);
    HookCpuidEmulate(leaf, sub, &eax, &ebx, &ecx, &edx);

    s->Rax = eax;
    V->GuestRegs.Rbx = ebx;
    V->GuestRegs.Rcx = ecx;
    V->GuestRegs.Rdx = edx;

    HvAdvanceRIP(V, 2);
}


static VOID HvHandleMsr(VCPU* V)
{
    VMCB_STATE_SAVE_AREA* s = VmcbState(V->Vmcb);

    UINT64 rcx = V->GuestRegs.Rcx;
    BOOLEAN write = (rcx >> 63) & 1;
    UINT64 msr = rcx & ~0x8000000000000000ULL;

    if (write)
    {
        UINT64 value = s->Rax;
        HookHandleMsrWrite(V, msr, value);
    }
    else
    {
        UINT64 value = HookHandleMsrRead(V, msr);
        s->Rax = value;
    }

    HvAdvanceRIP(V, 2);
}


static VOID HvHandleVmmcall(VCPU* V)
{
    VMCB_STATE_SAVE_AREA* s = VmcbState(V->Vmcb);

    UINT64 code = s->Rax;
    UINT64 arg1 = V->GuestRegs.Rbx;
    UINT64 arg2 = V->GuestRegs.Rcx;
    UINT64 arg3 = V->GuestRegs.Rdx;

    UINT64 result = HookVmmcallDispatch(V, code, arg1, arg2, arg3);

    s->Rax = result;

    HvAdvanceRIP(V, 3);
}


static VOID HvHandleNpf(VCPU* V)
{
    VMCB_CONTROL_AREA* c = VmcbControl(V->Vmcb);

    UINT64 fault_gpa = c->ExitInfo2;

   
    if (HvHandleLayeredNpf(V, fault_gpa))
    {
        HvAdvanceRIP(V, fault_gpa);
        return;
    }

    HookNptHandleFault(V, fault_gpa);
}


static VOID HvHandleHlt(VCPU* V)
{
    HvAdvanceRIP(V, 1);
}


static VOID HvHandleIo(VCPU* V)
{
    HookIoIntercept(V);
    HvAdvanceRIP(V, 2);
}


NTSTATUS HypervisorHandleExit(VCPU* V)
{
    VMCB_CONTROL_AREA* c = VmcbControl(V->Vmcb);
    UINT64 exitCode = c->ExitCode;

    V->Exec.LastExitCode = exitCode;

    switch (exitCode)
    {
    case SVM_EXIT_CPUID:
        HvHandleCpuid(V);
        return STATUS_SUCCESS;

    case SVM_EXIT_MSR:
        HvHandleMsr(V);
        return STATUS_SUCCESS;

    case SVM_EXIT_VMMCALL:
        HvHandleVmmcall(V);
        return STATUS_SUCCESS;

    case SVM_EXIT_NPF:
        if (HvHandleLayeredNpf(V, c->ExitInfo1))
            return STATUS_SUCCESS;

        HvHandleNpf(V);
        return STATUS_SUCCESS;

    case SVM_EXIT_HLT:
        HvHandleHlt(V);
        return STATUS_SUCCESS;

    default:
        return STATUS_SUCCESS;
    }
}


