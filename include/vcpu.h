#pragma once
#include <ntifs.h>
#include "vmcb.h"
#include "npt.h"

typedef struct _VCPU
{
    //
    // VMCB region
    //
    VMCB* Vmcb;
    PHYSICAL_ADDRESS VmcbPa;

    //
    // Host save area for vmrun
    //
    PVOID HostSave;
    PHYSICAL_ADDRESS HostSavePa;

    //
    // Guest stack
    //
    PVOID GuestStack;
    SIZE_T GuestStackSize;

    //
    // Nested Page Tables
    //
    NPT_STATE Npt;

    //
    // -------------------------
    // NEW FIELDS (REQUIRED BY svm.c)
    // -------------------------
    //

    // MSR Permission Map (3 pages = 0x6000)
    PVOID Msrpm;
    PHYSICAL_ADDRESS MsrpmPa;

    // I/O Permission Map (0x2000)
    PVOID Iopm;
    PHYSICAL_ADDRESS IopmPa;

    //
    // Runtime statistics
    //
    struct
    {
        UINT64 ExitCount;
        UINT64 LastExitCode;
        UINT64 ExitBudget;
    } Exec;

    //
    // IPC / Mailbox subsystem (optional)
    //
    struct
    {
        UINT64 MailboxGpa;
        UINT64 LastMessage;
        BOOLEAN Active;
    } Ipc;

    //
    // Extra metadata
    //
    UINT64 CloakedTscOffset;

    BOOLEAN Active;

} VCPU;
