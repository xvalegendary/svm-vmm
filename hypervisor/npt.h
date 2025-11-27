#pragma once
#include <ntifs.h>

#define PAGE_PRESENT     1ULL
#define PAGE_WRITE       (1ULL << 1)
#define PAGE_USER        (1ULL << 2)
#define PAGE_NX          (1ULL << 63)

typedef union _NPT_ENTRY
{
    UINT64 Value;
    struct {
        UINT64 Present : 1;
        UINT64 Write : 1;
        UINT64 User : 1;
        UINT64 WriteThrough : 1;
        UINT64 CacheDisable : 1;
        UINT64 Accessed : 1;
        UINT64 Dirty : 1;
        UINT64 LargePage : 1;
        UINT64 Global : 1;
        UINT64 Reserved1 : 3;
        UINT64 PageFrame : 40;
        UINT64 Reserved2 : 11;
        UINT64 Nx : 1;
    };
} NPT_ENTRY;

typedef struct _NPT_STATE
{
    NPT_ENTRY* Pml4;
    PHYSICAL_ADDRESS Pml4Pa;

    UINT64 ShadowCr3;

    struct
    {
        UINT64 TargetGpaPage;
        UINT64 NewHpaPage;
        BOOLEAN Active;
    } ShadowHook;
} NPT_STATE;

NTSTATUS NptInitialize(NPT_STATE* State);
VOID NptDestroy(NPT_STATE* State);

PHYSICAL_ADDRESS NptTranslateGvaToHpa(NPT_STATE* State, UINT64 Gva);
PHYSICAL_ADDRESS NptTranslateGpaToHpa(NPT_STATE* State, UINT64 Gpa);
BOOLEAN NptHookPage(NPT_STATE* State, UINT64 GuestPhysical, UINT64 NewHostPhysical);
VOID NptUpdateShadowCr3(NPT_STATE* State, UINT64 GuestCr3);
BOOLEAN NptInstallShadowHook(NPT_STATE* State, UINT64 TargetGpa, UINT64 NewHpa);
VOID NptClearShadowHook(NPT_STATE* State);
