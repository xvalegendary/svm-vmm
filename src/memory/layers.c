#include <ntifs.h>
#include <intrin.h>
#include "layers.h"
#include "npt.h"
#include "stealth.h"
#include "vmcb.h"
#include "communication.h"

#define APIC_BASE_GPA 0xFEE00000ULL
#define ACPI_PM_GPA   0x00000400ULL
#define SMM_TRAP_GPA  0x000A0000ULL
#define MMIO_DOORBELL 0x0000C000ULL

static VOID HvPrimeCloaking(VCPU* V)
{
    VMCB_CONTROL_AREA* c = VmcbControl(V->Vmcb);

   
    V->CloakedTscOffset = (__rdtsc() ^ 0xC0FFEEULL);
    c->TscOffset = V->CloakedTscOffset;

   
    StealthEnable();
    StealthCleanVmcb(V);
}

static VOID HvPrimeHardwareEntry(VCPU* V)
{
    CommInit(V, APIC_BASE_GPA);

    NptSetupHardwareTriggers(&V->Npt, APIC_BASE_GPA, ACPI_PM_GPA, SMM_TRAP_GPA, MMIO_DOORBELL);
}

VOID HvActivateLayeredPipeline(VCPU* V)
{
    if (!V || !V->Vmcb)
        return;

    HvPrimeCloaking(V);
    HvPrimeHardwareEntry(V);

   
    V->Exec.ExitBudget = 0x100;
}

BOOLEAN HvHandleLayeredNpf(VCPU* V, UINT64 faultGpa)
{
    if (!V)
        return FALSE;

    UINT64 mailbox = 0;
    if (NptHandleHardwareTriggers(&V->Npt, faultGpa, &mailbox))
    {
        CommHandleDoorbell(V, mailbox);
        return TRUE;
    }

    return FALSE;
}

VOID HvRefreshExecLayer(VCPU* V, UINT64 exitCode)
{
    if (!V || !V->Vmcb)
        return;

    VMCB_CONTROL_AREA* c = VmcbControl(V->Vmcb);

    V->Exec.ExitCount++;
    V->Exec.LastExitCode = exitCode;


    if ((V->Exec.ExitCount % V->Exec.ExitBudget) == 0)
    {
        NptRearmHardwareTriggers(&V->Npt);
        StealthCleanVmcb(V);
    }


    c->TscOffset = V->CloakedTscOffset;
}
