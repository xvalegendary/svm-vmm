#include <ntifs.h>
#include <intrin.h>
#include "svm.h"
#include "msr.h"
#include "vmcb.h"
#include "npt.h"

extern VOID VmrunAsm(UINT64 VmcbPa);
extern VOID GuestEntry();

static NTSTATUS SvmCheckSupport()
{
    int info[4];
    __cpuid(info, 0x80000001);
    if (!(info[2] & (1 << 2)))
        return STATUS_NOT_SUPPORTED;

    if (MsrRead(MSR_VM_CR) & VM_CR_SVMDIS)
        return STATUS_NOT_SUPPORTED;

    return STATUS_SUCCESS;
}

static VOID SvmEnable()
{
    UINT64 efer = MsrRead(MSR_EFER);
    if (!(efer & EFER_SVME)) {
        efer |= EFER_SVME;
        MsrWrite(MSR_EFER, efer);
    }
}

static NTSTATUS AllocHostSave(VCPU* V)
{
    V->HostSave = ExAllocatePoolWithTag(NonPagedPoolNx, PAGE_SIZE, 'hsvm');
    if (!V->HostSave) return STATUS_INSUFFICIENT_RESOURCES;

    RtlZeroMemory(V->HostSave, PAGE_SIZE);
    V->HostSavePa = MmGetPhysicalAddress(V->HostSave);

    MsrWrite(MSR_VM_HSAVE_PA, V->HostSavePa.QuadPart);
    return STATUS_SUCCESS;
}

static NTSTATUS AllocVmcb(VCPU* V)
{
    V->Vmcb = ExAllocatePoolWithTag(NonPagedPoolNx, PAGE_SIZE, 'vmcb');
    if (!V->Vmcb) return STATUS_INSUFFICIENT_RESOURCES;

    RtlZeroMemory(V->Vmcb, PAGE_SIZE);
    V->VmcbPa = MmGetPhysicalAddress(V->Vmcb);
    return STATUS_SUCCESS;
}

static NTSTATUS AllocGuestStack(VCPU* V)
{
    V->GuestStackSize = PAGE_SIZE;
    V->GuestStack = ExAllocatePoolWithTag(NonPagedPoolNx, V->GuestStackSize, 'gstk');
    if (!V->GuestStack) return STATUS_INSUFFICIENT_RESOURCES;

    RtlZeroMemory(V->GuestStack, V->GuestStackSize);
    return STATUS_SUCCESS;
}

static VOID SetupGuest(VCPU* V)
{
    VMCB_STATE_SAVE_AREA* st = VmcbState(V->Vmcb);

    st->Rip = (UINT64)GuestEntry;
    st->Rsp = (UINT64)V->GuestStack + V->GuestStackSize - 0x20;

    st->Rflags = 0x2;

    st->Cr0 = __readcr0();
    st->Cr3 = __readcr3();
    st->Cr4 = __readcr4();

    st->Efer = MsrRead(MSR_EFER);

    NptUpdateShadowCr3(&V->Npt, st->Cr3);
}

static VOID SetupControls(VCPU* V)
{
    VMCB_CONTROL_AREA* c = VmcbControl(V->Vmcb);

    c->GuestAsid = 1;
    c->VmcbCleanBits = 0;

    c->InterceptCrRead = 0xFFFFFFFF;
    c->InterceptCrWrite = 0xFFFFFFFF;
    c->InterceptDrRead = 0xFFFFFFFF;
    c->InterceptDrWrite = 0xFFFFFFFF;

    c->InterceptInstruction1 =
        (1 << SVM_EXIT_CPUID) |
        (1 << SVM_EXIT_HLT);

    c->InterceptInstruction2 =
        (1ULL << SVM_EXIT_MSR);

    c->NptControl = 1;  // Enable NPT

    c->IopmBasePa = 0;
    c->MsrpmBasePa = 0;
}

NTSTATUS SvmInit(VCPU** Out)
{
    NTSTATUS st = SvmCheckSupport();
    if (!NT_SUCCESS(st)) return st;

    SvmEnable();

    VCPU* V = ExAllocatePoolWithTag(NonPagedPoolNx, sizeof(VCPU), 'vcpu');
    if (!V) return STATUS_INSUFFICIENT_RESOURCES;

    RtlZeroMemory(V, sizeof(*V));

    if (!NT_SUCCESS(st = AllocHostSave(V))) goto fail;
    if (!NT_SUCCESS(st = AllocVmcb(V))) goto fail;
    if (!NT_SUCCESS(st = AllocGuestStack(V))) goto fail;

    NptInitialize(&V->Npt);

    SetupGuest(V);
    SetupControls(V);

    *Out = V;
    return STATUS_SUCCESS;

fail:
    SvmShutdown(V);
    return st;
}

NTSTATUS SvmLaunch(VCPU* V)
{
    VmrunAsm(V->VmcbPa.QuadPart);
    return HypervisorHandleExit(V);
}

VOID SvmShutdown(VCPU* V)
{
    if (!V) return;

    if (V->GuestStack)
        ExFreePoolWithTag(V->GuestStack, 'gstk');

    if (V->Vmcb)
        ExFreePoolWithTag(V->Vmcb, 'vmcb');

    if (V->HostSave)
        ExFreePoolWithTag(V->HostSave, 'hsvm');

    NptDestroy(&V->Npt);

    ExFreePoolWithTag(V, 'vcpu');
}
