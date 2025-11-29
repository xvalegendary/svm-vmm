#include <ntifs.h>
#include "stealth.h"
#include "vcpu.h"
#include "vmcb.h"
#include "msr.h"



static BOOLEAN g_StealthEnabled = FALSE;
static BOOLEAN g_HideSvmMsr = TRUE;
static BOOLEAN g_HideVmcbMemory = TRUE;
static BOOLEAN g_HideHostSave = TRUE;
static BOOLEAN g_HideCr3Xor = TRUE;


static UINT64 g_Cr3XorKey = 0xA5A5A5A5CAFEBABEULL;


VOID StealthMaskCpuid(UINT32 leaf, UINT32* ecx, UINT32* edx)
{
    if (!g_StealthEnabled)
        return;

    if (leaf == 1)
    {
        *ecx &= ~(1 << 31);
    }

    if (leaf == 0x80000001)
    {
        *edx &= ~(1 << 2);
    }
}


UINT64 StealthMaskMsrRead(UINT32 msr, UINT64 value)
{
    if (!g_StealthEnabled)
        return value;

    if (g_HideSvmMsr)
    {
        if (msr == MSR_EFER)
        {
            
            return value & ~((UINT64)1 << 12);
        }
    }

    return value;
}


UINT64 StealthEncryptCr3(UINT64 cr3)
{
    if (!g_StealthEnabled || !g_HideCr3Xor)
        return cr3;

    return cr3 ^ g_Cr3XorKey;
}

UINT64 StealthDecryptCr3(UINT64 cr3_enc)
{
    if (!g_StealthEnabled || !g_HideCr3Xor)
        return cr3_enc;

    return cr3_enc ^ g_Cr3XorKey;
}


VOID StealthHideHypervisorMemory(VCPU* V)
{
    if (!g_StealthEnabled)
        return;


    if (g_HideVmcbMemory && V->Vmcb)
    {
        RtlSecureZeroMemory(V->Vmcb, PAGE_SIZE);
    }

    if (g_HideHostSave && V->HostSave)
    {
        RtlSecureZeroMemory(V->HostSave, PAGE_SIZE);
    }
}


BOOLEAN StealthPreventVmrunDetection()
{
    if (!g_StealthEnabled)
        return TRUE;


    return TRUE;
}


VOID StealthCleanVmcb(VCPU* V)
{
    if (!g_StealthEnabled)
        return;

    VMCB_CONTROL_AREA* c = VmcbControl(V->Vmcb);

    c->VmcbCleanBits = 0xFFFFFFFFFFFFFFFFULL;
}


VOID StealthEnable()
{
    g_StealthEnabled = TRUE;
}

VOID StealthDisable()
{
    g_StealthEnabled = FALSE;
}

BOOLEAN StealthIsEnabled()
{
    return g_StealthEnabled;
}

