#include <ntifs.h>
#include "svm.h"
#include "vcpu.h"
#include "vmcb.h"
#include "guest_mem.h"
#include "npt.h"
#include "hooks.h"
#include "stealth.h"
#include "shadow_idt.h"

//
// Основной VMEXIT обработчик
//

static VOID HvAdvanceRIP(VCPU* V)
{
    VMCB_CONTROL_AREA* c = VmcbControl(V->Vmcb);
    VMCB_STATE_SAVE_AREA* s = VmcbState(V->Vmcb);

    if (c->Nrip)
        s->Rip = c->Nrip;
    else
        s->Rip += 2; // fallback
}

//
// CPUID hook
//
static VOID HvHandleCpuid(VCPU* V)
{
    VMCB_STATE_SAVE_AREA* s = VmcbState(V->Vmcb);

    UINT64 leaf = s->Rax;
    UINT64 sub = s->Rcx;

    UINT32 eax, ebx, ecx, edx;
    __cpuidex((int*)&eax, (int)leaf, (int)sub);

    //
    // СТЕЛС: убираем Hypervisor present / SVM bit
    //
    if (leaf == 1)
        ecx &= ~(1 << 31);

    if (leaf == 0x80000001)
        edx &= ~(1 << 2);

    //
    // Выполняем твики CPUID
    //
    HookCpuidEmulate(leaf, sub, &eax, &ebx, &ecx, &edx);

    s->Rax = eax;
    s->Rbx = ebx;
    s->Rcx = ecx;
    s->Rdx = edx;

    HvAdvanceRIP(V);
}

//
// MSR Read/Write
//
static VOID HvHandleMsr(VCPU* V)
{
    VMCB_STATE_SAVE_AREA* s = VmcbState(V->Vmcb);

    BOOLEAN write = (s->Rcx >> 63) & 1;
    UINT64 msr = s->Rcx & ~0x8000000000000000ULL;

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

    HvAdvanceRIP(V);
}

//
// VMMCALL hypercalls — ring-3 / kernel-mode
//
static VOID HvHandleVmmcall(VCPU* V)
{
    VMCB_STATE_SAVE_AREA* s = VmcbState(V->Vmcb);

    UINT64 code = s->Rax;
    UINT64 arg1 = s->Rbx;
    UINT64 arg2 = s->Rcx;
    UINT64 arg3 = s->Rdx;

    UINT64 result = HookVmmcallDispatch(V, code, arg1, arg2, arg3);

    s->Rax = result;

    HvAdvanceRIP(V);
}

//
// Nested Page Fault → NPT Hook / memory monitor
//
static VOID HvHandleNpf(VCPU* V)
{
    VMCB_CONTROL_AREA* c = VmcbControl(V->Vmcb);

    UINT64 fault_gpa = c->ExitInfo2;

    HookNptHandleFault(V, fault_gpa);

    HvAdvanceRIP(V);
}

//
// HLT
//
static VOID HvHandleHlt(VCPU* V)
{
    // Просто двигаем RIP
    HvAdvanceRIP(V);
}

//
// IOIO
//
static VOID HvHandleIo(VCPU* V)
{
    HookIoIntercept(V);
    HvAdvanceRIP(V);
}

//
// Основной обработчик VMEXIT
//
NTSTATUS HypervisorHandleExit(VCPU* V)
{
    if (!V || !V->Vmcb) return STATUS_INVALID_PARAMETER;

    VMCB_CONTROL_AREA* c = VmcbControl(V->Vmcb);

    VMCB_STATE_SAVE_AREA* s = VmcbState(V->Vmcb);
    NptUpdateShadowCr3(&V->Npt, HookDecryptCr3(s->Cr3));

    UINT64 exit = c->ExitCode;

    switch (exit)
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
        HvHandleNpf(V);
        return STATUS_SUCCESS;

    case SVM_EXIT_IOIO:
        HvHandleIo(V);
        return STATUS_SUCCESS;

    case SVM_EXIT_HLT:
        HvHandleHlt(V);
        return STATUS_SUCCESS;

    default:
        DbgPrint("HV: Unknown VMEXIT = 0x%llx\n", exit);
        return STATUS_SUCCESS;
    }
}
