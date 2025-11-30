#include "npt.h"
#include <ntifs.h>

#ifndef PAGE_ALIGN
#define PAGE_ALIGN(x) ((x) & ~0xFFFULL)
#endif

static NPT_ENTRY* NptAllocTable(PHYSICAL_ADDRESS* outPa)
{
    PHYSICAL_ADDRESS low = { 0 };
    PHYSICAL_ADDRESS high = { .QuadPart = ~0ULL };
    PHYSICAL_ADDRESS skip = { 0 };

    NPT_ENTRY* tbl = MmAllocateContiguousMemorySpecifyCache(PAGE_SIZE, low, high, skip, MmCached);
    if (!tbl)
        return NULL;

    RtlZeroMemory(tbl, PAGE_SIZE);
    *outPa = MmGetPhysicalAddress(tbl);
    return tbl;
}

//
// Internal page walk for NPT
//
static NPT_ENTRY* NptGetEntry(
    NPT_STATE* State,
    UINT64 gpa,
    UINT64* outLevel)
{
    UINT64 gpaPage = gpa >> 12;

    UINT64 pml4_i = (gpa >> 39) & 0x1FF;
    UINT64 pdpt_i = (gpa >> 30) & 0x1FF;
    UINT64 pd_i = (gpa >> 21) & 0x1FF;
    UINT64 pt_i = (gpa >> 12) & 0x1FF;

    NPT_ENTRY* pml4 = State->Pml4;
    if (!pml4[pml4_i].Present)
        return NULL;

    NPT_ENTRY* pdpt = (NPT_ENTRY*)MmGetVirtualForPhysical(
        (PHYSICAL_ADDRESS) {
        pml4[pml4_i].PageFrame << 12
    });

    if (!pdpt[pdpt_i].Present)
        return NULL;

    if (pdpt[pdpt_i].LargePage)
    {
        *outLevel = 1;
        return &pdpt[pdpt_i];
    }

    NPT_ENTRY* pd = (NPT_ENTRY*)MmGetVirtualForPhysical(
        (PHYSICAL_ADDRESS) {
        pdpt[pdpt_i].PageFrame << 12
    });

    if (!pd[pd_i].Present)
        return NULL;

    if (pd[pd_i].LargePage)
    {
        *outLevel = 2;
        return &pd[pd_i];
    }

    NPT_ENTRY* pt = (NPT_ENTRY*)MmGetVirtualForPhysical(
        (PHYSICAL_ADDRESS) {
        pd[pd_i].PageFrame << 12
    });

    *outLevel = 3;
    return &pt[pt_i];
}

static BOOLEAN NptReadGuestQword(NPT_STATE* State, UINT64 gpa, UINT64* outValue)
{
    PHYSICAL_ADDRESS hpa = NptTranslateGpaToHpa(State, gpa);
    if (!hpa.QuadPart)
        return FALSE;

    PVOID mapped = MmMapIoSpace(hpa, sizeof(UINT64), MmNonCached);
    if (!mapped)
        return FALSE;

    *outValue = *(volatile UINT64*)mapped;
    MmUnmapIoSpace(mapped, sizeof(UINT64));
    return TRUE;
}

static VOID NptProtectPageForTrap(NPT_STATE* State, UINT64 gpa, NPT_ENTRY* entry,
    UINT64* originalFrame,
    BOOLEAN arm)
{
    if (!entry)
        return;

    if (arm)
    {
        *originalFrame = entry->PageFrame;
        entry->Present = 0; // force NPF on first touch
    }
    else
    {
        entry->PageFrame = *originalFrame;
        entry->Present = 1;
    }
}

//
// GPA → HPA
//
PHYSICAL_ADDRESS NptTranslateGpaToHpa(NPT_STATE* State, UINT64 gpa)
{
    PHYSICAL_ADDRESS pa = { 0 };
    UINT64 level;

    NPT_ENTRY* entry = NptGetEntry(State, gpa, &level);
    if (!entry)
        return pa;

    UINT64 offset = gpa & 0xFFFULL;
    pa.QuadPart = (entry->PageFrame << 12) + offset;
    return pa;
}

//
// GVA → HPA through NPT (relies on guest CR3 page walk)
//
PHYSICAL_ADDRESS NptTranslateGvaToHpa(NPT_STATE* State, UINT64 gva)
{
    PHYSICAL_ADDRESS pa = { 0 };

    if (!State->ShadowCr3)
        return pa;

    UINT64 cr3 = State->ShadowCr3 & ~0xFFFULL;
    UINT64 index = (gva >> 39) & 0x1FF;

    UINT64 pml4e;
    if (!NptReadGuestQword(State, cr3 + index * sizeof(UINT64), &pml4e) || !(pml4e & PAGE_PRESENT))
        return pa;

    UINT64 pdpt = pml4e & ~0xFFFULL;
    index = (gva >> 30) & 0x1FF;

    UINT64 pdpte;
    if (!NptReadGuestQword(State, pdpt + index * sizeof(UINT64), &pdpte) || !(pdpte & PAGE_PRESENT))
        return pa;

    if (pdpte & (1ULL << 7))
    {
        pa.QuadPart = (pdpte & ~0x3FFFFFFFULL) + (gva & 0x3FFFFFFFULL);
        pa = NptTranslateGpaToHpa(State, pa.QuadPart);
        return pa;
    }

    UINT64 pd = pdpte & ~0xFFFULL;
    index = (gva >> 21) & 0x1FF;

    UINT64 pde;
    if (!NptReadGuestQword(State, pd + index * sizeof(UINT64), &pde) || !(pde & PAGE_PRESENT))
        return pa;

    if (pde & (1ULL << 7))
    {
        pa.QuadPart = (pde & ~0x1FFFFFULL) + (gva & 0x1FFFFFULL);
        pa = NptTranslateGpaToHpa(State, pa.QuadPart);
        return pa;
    }

    UINT64 pt = pde & ~0xFFFULL;
    index = (gva >> 12) & 0x1FF;

    UINT64 pte;
    if (!NptReadGuestQword(State, pt + index * sizeof(UINT64), &pte) || !(pte & PAGE_PRESENT))
        return pa;

    pa.QuadPart = (pte & ~0xFFFULL) + (gva & 0xFFFULL);
    pa = NptTranslateGpaToHpa(State, pa.QuadPart);
    return pa;
}

//
// Hook GPA → другой HPA (EPT-like hook)
//
BOOLEAN NptHookPage(NPT_STATE* State, UINT64 targetGpaPage, UINT64 newHpaPage)
{
    UINT64 level;

    NPT_ENTRY* entry = NptGetEntry(State, targetGpaPage, &level);
    if (!entry)
        return FALSE;

    entry->PageFrame = (newHpaPage >> 12);
    entry->Dirty = 1;
    entry->Accessed = 1;

    return TRUE;
}

VOID NptUpdateShadowCr3(NPT_STATE* State, UINT64 GuestCr3)
{
    State->ShadowCr3 = GuestCr3;
}

static BOOLEAN NptArmTrap(NPT_STATE* State, UINT64 gpa, NPT_ENTRY* entry,
    UINT64* originalFrame,
    BOOLEAN* armed)
{
    if (!entry)
        return FALSE;

    NptProtectPageForTrap(State, gpa, entry, originalFrame, TRUE);
    *armed = TRUE;
    return TRUE;
}

static BOOLEAN NptPromoteTrapToFake(NPT_STATE* State, NPT_ENTRY* entry)
{
    if (!entry)
        return FALSE;

    ULONG slot = State->FakePageIndex & 1;
    PHYSICAL_ADDRESS fakePa = State->FakePagePa[slot];
    if (!fakePa.QuadPart)
        return FALSE;

    entry->PageFrame = fakePa.QuadPart >> 12;
    entry->Present = 1;
    entry->Write = 1;
    entry->Accessed = 1;
    entry->Dirty = 1;

    State->FakePageIndex ^= 1; // alternate between the two fake pages
    return TRUE;
}

static BOOLEAN NptHandleSingleTrigger(NPT_STATE* State,
    UINT64 gpa,
    NPT_ENTRY* entry,
    UINT64* originalFrame,
    BOOLEAN* armed,
    BOOLEAN* usingFake,
    UINT64* mailboxValue)
{
    if (!*armed || !entry)
        return FALSE;

    if ((gpa & ~0xFFFULL) != (entry->PageFrame << 12) && entry->Present)
    {
        // re-arm later when backing is restored
        return FALSE;
    }

    if (!*usingFake)
    {
        *usingFake = NptPromoteTrapToFake(State, entry);
        *armed = FALSE;
        if (mailboxValue)
            *mailboxValue = gpa;
        return *usingFake;
    }

    return FALSE;
}

BOOLEAN NptSetupHardwareTriggers(NPT_STATE* State, UINT64 apicGpa, UINT64 acpiGpa, UINT64 smmGpa, UINT64 mmioGpa)
{
    UINT64 level;

    NPT_ENTRY* apic = NptGetEntry(State, apicGpa, &level);
    NPT_ENTRY* acpi = NptGetEntry(State, acpiGpa, &level);
    NPT_ENTRY* smm = NptGetEntry(State, smmGpa, &level);
    NPT_ENTRY* mmio = NptGetEntry(State, mmioGpa, &level);

    BOOLEAN ok = TRUE;
    ok &= NptArmTrap(State, apicGpa, apic, &State->Apic.OriginalPageFrame, &State->Apic.Armed);
    ok &= NptArmTrap(State, acpiGpa, acpi, &State->Acpi.OriginalPageFrame, &State->Acpi.Armed);
    ok &= NptArmTrap(State, smmGpa, smm, &State->Smm.OriginalPageFrame, &State->Smm.Armed);
    ok &= NptArmTrap(State, mmioGpa, mmio, &State->Mmio.OriginalPageFrame, &State->Mmio.Armed);

    State->Apic.GpaPage = apicGpa & ~0xFFFULL;
    State->Acpi.GpaPage = acpiGpa & ~0xFFFULL;
    State->Smm.GpaPage = smmGpa & ~0xFFFULL;
    State->Mmio.GpaPage = mmioGpa & ~0xFFFULL;

    State->Apic.UsingFakePage = FALSE;
    State->Acpi.UsingFakePage = FALSE;
    State->Smm.UsingFakePage = FALSE;
    State->Mmio.UsingFakePage = FALSE;

    State->Mailbox.GpaPage = apicGpa & ~0xFFFULL;
    State->Mailbox.Active = TRUE;
    State->Mailbox.LastMessage = 0;

    return ok;
}

BOOLEAN NptHandleHardwareTriggers(NPT_STATE* State, UINT64 faultGpa, UINT64* mailboxValue)
{
    UINT64 level;

    NPT_ENTRY* apic = NptGetEntry(State, State->Apic.GpaPage, &level);
    NPT_ENTRY* acpi = NptGetEntry(State, State->Acpi.GpaPage, &level);
    NPT_ENTRY* smm = NptGetEntry(State, State->Smm.GpaPage, &level);
    NPT_ENTRY* mmio = NptGetEntry(State, State->Mmio.GpaPage, &level);

    if (NptHandleSingleTrigger(State, faultGpa, apic, &State->Apic.OriginalPageFrame, &State->Apic.Armed, &State->Apic.UsingFakePage, mailboxValue))
        return TRUE;
    if (NptHandleSingleTrigger(State, faultGpa, acpi, &State->Acpi.OriginalPageFrame, &State->Acpi.Armed, &State->Acpi.UsingFakePage, mailboxValue))
        return TRUE;
    if (NptHandleSingleTrigger(State, faultGpa, smm, &State->Smm.OriginalPageFrame, &State->Smm.Armed, &State->Smm.UsingFakePage, mailboxValue))
        return TRUE;
    if (NptHandleSingleTrigger(State, faultGpa, mmio, &State->Mmio.OriginalPageFrame, &State->Mmio.Armed, &State->Mmio.UsingFakePage, mailboxValue))
        return TRUE;

    return FALSE;
}

VOID NptRearmHardwareTriggers(NPT_STATE* State)
{
    UINT64 level;
    NPT_ENTRY* apic = NptGetEntry(State, State->Apic.GpaPage, &level);
    NPT_ENTRY* acpi = NptGetEntry(State, State->Acpi.GpaPage, &level);
    NPT_ENTRY* smm = NptGetEntry(State, State->Smm.GpaPage, &level);
    NPT_ENTRY* mmio = NptGetEntry(State, State->Mmio.GpaPage, &level);

    if (State->Apic.UsingFakePage)
    {
        if (apic)
        {
            apic->PageFrame = State->Apic.OriginalPageFrame;
            apic->Present = 0;
        }
        State->Apic.UsingFakePage = FALSE;
        State->Apic.Armed = TRUE;
    }

    if (State->Acpi.UsingFakePage)
    {
        if (acpi)
        {
            acpi->PageFrame = State->Acpi.OriginalPageFrame;
            acpi->Present = 0;
        }
        State->Acpi.UsingFakePage = FALSE;
        State->Acpi.Armed = TRUE;
    }

    if (State->Smm.UsingFakePage)
    {
        if (smm)
        {
            smm->PageFrame = State->Smm.OriginalPageFrame;
            smm->Present = 0;
        }
        State->Smm.UsingFakePage = FALSE;
        State->Smm.Armed = TRUE;
    }

    if (State->Mmio.UsingFakePage)
    {
        if (mmio)
        {
            mmio->PageFrame = State->Mmio.OriginalPageFrame;
            mmio->Present = 0;
        }
        State->Mmio.UsingFakePage = FALSE;
        State->Mmio.Armed = TRUE;
    }
}

BOOLEAN NptInstallShadowHook(NPT_STATE* State, UINT64 TargetGpa, UINT64 NewHpa)
{
    if (!State)
        return FALSE;

    State->ShadowHook.TargetGpaPage = TargetGpa & ~0xFFFULL;
    State->ShadowHook.NewHpaPage = NewHpa & ~0xFFFULL;
    State->ShadowHook.Active = TRUE;
    return TRUE;
}

VOID NptClearShadowHook(NPT_STATE* State)
{
    if (!State)
        return;

    State->ShadowHook.Active = FALSE;
    State->ShadowHook.TargetGpaPage = 0;
    State->ShadowHook.NewHpaPage = 0;
}

//
// Initialize full NPT (identity map) for entire 48-bit GPA
//
NTSTATUS NptInitialize(NPT_STATE* State)
{
    if (!State) return STATUS_INVALID_PARAMETER;
    RtlZeroMemory(State, sizeof(*State));

    // Fake pages
    for (ULONG i = 0; i < 2; i++)
    {
        State->FakePageVa[i] = MmAllocateContiguousMemorySpecifyCache(PAGE_SIZE,
            (PHYSICAL_ADDRESS) { 0 }, (PHYSICAL_ADDRESS) { .QuadPart = ~0ULL }, (PHYSICAL_ADDRESS) { 0 }, MmCached);
        if (!State->FakePageVa[i]) return STATUS_INSUFFICIENT_RESOURCES;

        RtlZeroMemory(State->FakePageVa[i], PAGE_SIZE);
        State->FakePagePa[i] = MmGetPhysicalAddress(State->FakePageVa[i]);
    }

    // Allocate PML4
    PHYSICAL_ADDRESS pml4Pa;
    NPT_ENTRY* pml4 = NptAllocTable(&pml4Pa);
    if (!pml4) return STATUS_INSUFFICIENT_RESOURCES;

    State->Pml4 = pml4;
    State->Pml4Pa = pml4Pa;

    // PDPT
    PHYSICAL_ADDRESS pdptPa;
    NPT_ENTRY* pdpt = NptAllocTable(&pdptPa);
    if (!pdpt) return STATUS_INSUFFICIENT_RESOURCES;

    pml4[0].Present = 1;
    pml4[0].Write = 1;
    pml4[0].PageFrame = pdptPa.QuadPart >> 12;

    // PD
    PHYSICAL_ADDRESS pdPa;
    NPT_ENTRY* pd = NptAllocTable(&pdPa);
    if (!pd) return STATUS_INSUFFICIENT_RESOURCES;

    pdpt[0].Present = 1;
    pdpt[0].Write = 1;
    pdpt[0].PageFrame = pdPa.QuadPart >> 12;

    // Identity map 0 → 1GB using 2MB huge pages
    for (UINT64 i = 0; i < 512; i++)
    {
        pd[i].Present = 1;
        pd[i].Write = 1;
        pd[i].LargePage = 1;

        // 2MB * i
        UINT64 phys = i * 0x200000ULL;
        pd[i].PageFrame = phys >> 12;
    }

    return STATUS_SUCCESS;
}


//
// Cleanup
//
VOID NptDestroy(NPT_STATE* State)
{
    if (!State)
        return;

    for (ULONG i = 0; i < 2; i++)
    {
        if (State->FakePageVa[i])
            MmFreeContiguousMemory(State->FakePageVa[i]);
    }

    if (State->Pml4)
        MmFreeContiguousMemory(State->Pml4);

    // All lower level tables are allocated contiguously and reachable from the PML4.
    // Since they are not individually tracked, free the initial PDPT/PD pages if present.
    if (State->Pml4)
    {
        NPT_ENTRY* pdpt = (NPT_ENTRY*)MmGetVirtualForPhysical((PHYSICAL_ADDRESS){ State->Pml4[0].PageFrame << 12 });
        if (pdpt)
        {
            NPT_ENTRY* pd = (NPT_ENTRY*)MmGetVirtualForPhysical((PHYSICAL_ADDRESS){ pdpt[0].PageFrame << 12 });
            if (pd)
                MmFreeContiguousMemory(pd);

            MmFreeContiguousMemory(pdpt);
        }
    }
}

