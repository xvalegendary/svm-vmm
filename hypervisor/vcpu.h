#pragma once
#include <ntifs.h>
#include "vmcb.h"
#include "npt.h"

typedef struct _VCPU
{
    VMCB* Vmcb;
    PHYSICAL_ADDRESS VmcbPa;

    PVOID HostSave;
    PHYSICAL_ADDRESS HostSavePa;

    PVOID GuestStack;
    SIZE_T GuestStackSize;

    NPT_STATE Npt;

    UINT64 HostCr3;

    // Execution layer bookkeeping
    struct
    {
        UINT64 ExitCount;
        UINT64 LastExitCode;
        UINT64 ExitBudget;
    } Exec;

    // IPC-style mailbox (APIC/ACPI/MMIO-backed)
    struct
    {
        UINT64 MailboxGpa;
        UINT64 LastMessage;
        BOOLEAN Active;
    } Ipc;

    // TSC cloaking offset
    UINT64 CloakedTscOffset;

    BOOLEAN Active;
} VCPU;
