#define INTERCEPT_CPUID   (1ULL << 0)
#define INTERCEPT_HLT     (1ULL << 7)
#define INTERCEPT_MSR     (1ULL << 0)

#include <ntifs.h>
#include <intrin.h>
#include "svm.h"
#include "msr.h"
#include "vmcb.h"
#include "npt.h"
#include "layers.h"

#ifndef PAGE_SIZE
#define PAGE_SIZE 0x1000
#endif

extern VOID VmrunAsm(UINT64 VmcbPa);
extern VOID GuestEntry();

#define MSRPM_SIZE 0x6000
#define IOPM_SIZE  0x2000



static NTSTATUS SvmCheckSupport()
{
    int info[4];

    __cpuid(info, 0x80000001);
    if (!(info[2] & (1 << 2)))
    {
        DbgPrint("SVM-HV: CPU does not support SVM.\n");
        return STATUS_NOT_SUPPORTED;
    }

    __cpuid(info, 1);
    if (info[2] & (1 << 31))
    {
        DbgPrint("SVM-HV: Another hypervisor is already active.\n");
        return STATUS_HV_FEATURE_UNAVAILABLE;
    }

    if (MsrRead(MSR_VM_CR) & VM_CR_SVMDIS)
    {
        DbgPrint("SVM-HV: SVM disabled (SVMDIS=1).\n");
        return STATUS_NOT_SUPPORTED;
    }

    return STATUS_SUCCESS;
}


static VOID SvmEnable()
{
    UINT64 efer = MsrRead(MSR_EFER);
    if (!(efer & EFER_SVME))
    {
        efer |= EFER_SVME;
        MsrWrite(MSR_EFER, efer);
    }
}


static PVOID AllocAlignedContiguous(SIZE_T size, PHYSICAL_ADDRESS* pa)
{
    PHYSICAL_ADDRESS low = { 0 };
    PHYSICAL_ADDRESS high = { .QuadPart = ~0ULL };
    PHYSICAL_ADDRESS skip = { 0 };

    PVOID mem = MmAllocateContiguousMemorySpecifyCache(size, low, high, skip, MmCached);
    if (!mem)
        return NULL;

    RtlZeroMemory(mem, size);
    *pa = MmGetPhysicalAddress(mem);
    return mem;
}



static NTSTATUS AllocHostSave(VCPU* V)
{
    V->HostSave = AllocAlignedContiguous(PAGE_SIZE, &V->HostSavePa);
    if (!V->HostSave) return STATUS_INSUFFICIENT_RESOURCES;

    MsrWrite(MSR_VM_HSAVE_PA, V->HostSavePa.QuadPart);
    return STATUS_SUCCESS;
}



static NTSTATUS AllocVmcb(VCPU* V)
{
    V->Vmcb = AllocAlignedContiguous(PAGE_SIZE, &V->VmcbPa);
    if (!V->Vmcb) return STATUS_INSUFFICIENT_RESOURCES;

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



static NTSTATUS AllocMsrpm(VCPU* V)
{
    V->Msrpm = AllocAlignedContiguous(MSRPM_SIZE, &V->MsrpmPa);
    if (!V->Msrpm) return STATUS_INSUFFICIENT_RESOURCES;

    return STATUS_SUCCESS;
}

static NTSTATUS AllocIopm(VCPU* V)
{
    V->Iopm = AllocAlignedContiguous(IOPM_SIZE, &V->IopmPa);
    if (!V->Iopm) return STATUS_INSUFFICIENT_RESOURCES;

    return STATUS_SUCCESS;
}


static VOID SetupGuest(VCPU* V)
{
    VMCB_STATE_SAVE_AREA* s = VmcbState(V->Vmcb);

    s->Rip = (UINT64)GuestEntry;
    s->Rsp = (UINT64)V->GuestStack + V->GuestStackSize - 0x20;
    s->Rflags = 0x2;

    
    s->CsSelector = 0x10;
    s->CsAttributes = 0xA09B; 
    s->CsLimit = 0xFFFFFFFF;
    s->CsBase = 0;

    s->DsSelector = 0x18;
    s->DsAttributes = 0xC093;
    s->DsLimit = 0xFFFFFFFF;
    s->DsBase = 0;

    s->EsSelector = 0x18;
    s->EsAttributes = 0xC093;
    s->EsLimit = 0xFFFFFFFF;
    s->EsBase = 0;

    s->SsSelector = 0x18;
    s->SsAttributes = 0xC093;
    s->SsLimit = 0xFFFFFFFF;
    s->SsBase = 0;

    s->FsSelector = 0x0;
    s->FsAttributes = 0x0;
    s->FsLimit = 0;
    s->FsBase = 0;

    s->GsSelector = 0x0;
    s->GsAttributes = 0x0;
    s->GsLimit = 0;
    s->GsBase = 0;

    s->GdtrBase = 0;
    s->GdtrLimit = 0;
    s->IdtrBase = 0;
    s->IdtrLimit = 0;

    s->Cr0 = __readcr0();
    s->Cr3 = __readcr3();
    s->Cr4 = __readcr4();
    s->Efer = MsrRead(MSR_EFER);

    
    NptUpdateShadowCr3(&V->Npt, s->Cr3);
}




static VOID SetupControls(VCPU* V)
{
    VMCB_CONTROL_AREA* c = VmcbControl(V->Vmcb);

    c->GuestAsid = 1;

    c->InterceptCrRead = 0;
    c->InterceptCrWrite = 0;
    c->InterceptDrRead = 0;
    c->InterceptDrWrite = 0;

    c->InterceptInstruction1 = (1 << 0); 
    c->InterceptInstruction2 = 0;       

    c->IopmBasePa = 0;
    c->MsrpmBasePa = 0;

    c->NptControl = 1; 

    
    c->VmcbCleanBits = 0;
}


NTSTATUS SvmInit(VCPU** Out)
{
    NTSTATUS st = SvmCheckSupport();
    if (!NT_SUCCESS(st))
        return st;

    SvmEnable();

    VCPU* V = ExAllocatePoolWithTag(NonPagedPoolNx, sizeof(VCPU), 'vcpu');
    if (!V)
        return STATUS_INSUFFICIENT_RESOURCES;

    RtlZeroMemory(V, sizeof(*V));

 
    if (!NT_SUCCESS(st = AllocHostSave(V))) goto fail;
    if (!NT_SUCCESS(st = AllocVmcb(V))) goto fail;
    if (!NT_SUCCESS(st = AllocGuestStack(V))) goto fail;

    if (!NT_SUCCESS(st = AllocMsrpm(V))) goto fail;
    if (!NT_SUCCESS(st = AllocIopm(V))) goto fail;

    if (!NT_SUCCESS(st = NptInitialize(&V->Npt))) goto fail;

    SetupGuest(V);
    SetupControls(V);

    VMCB_CONTROL_AREA* c = VmcbControl(V->Vmcb);
    c->MsrpmBasePa = V->MsrpmPa.QuadPart;
    c->IopmBasePa = V->IopmPa.QuadPart;

    c->Ncr3 = V->Npt.Pml4Pa.QuadPart;

    HvActivateLayeredPipeline(V);

    *Out = V;   
    return STATUS_SUCCESS;

fail:
    SvmShutdown(V);
    return st;
}




NTSTATUS SvmLaunch(VCPU* V)
{
    if (!V) return STATUS_INVALID_PARAMETER;

    __try
    {
        VmrunAsm(V->VmcbPa.QuadPart);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return GetExceptionCode();
    }

    return HypervisorHandleExit(V);
}



VOID SvmShutdown(VCPU* V)
{
    if (!V) return;

    if (V->GuestStack) ExFreePoolWithTag(V->GuestStack, 'gstk');
    if (V->Vmcb)       MmFreeContiguousMemory(V->Vmcb);
    if (V->HostSave)   MmFreeContiguousMemory(V->HostSave);
    if (V->Msrpm)      MmFreeContiguousMemory(V->Msrpm);
    if (V->Iopm)       MmFreeContiguousMemory(V->Iopm);

    NptDestroy(&V->Npt);

    ExFreePoolWithTag(V, 'vcpu');
}
