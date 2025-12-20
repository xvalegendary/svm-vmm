#pragma once
#include <ntifs.h>

#pragma pack(push, 1)

typedef struct _VMCB_SEGMENT
{
    UINT16 Selector;
    UINT16 Attributes;
    UINT32 Limit;
    UINT64 Base;
} VMCB_SEGMENT;

typedef struct _VMCB_CONTROL_AREA
{
    UINT32 Intercepts[6];
    UINT32 Reserved1[9];

    UINT16 PauseFilterThreshold;
    UINT16 PauseFilterCount;

    UINT64 IopmBasePa;
    UINT64 MsrpmBasePa;
    UINT64 TscOffset;

    UINT32 GuestAsid;
    UINT8  TlbControl;
    UINT8  Reserved2[3];

    UINT32 InterruptControl;
    UINT32 InterruptVector;
    UINT32 InterruptState;
    UINT8  Reserved3[4];

    union
    {
        UINT64 ExitCode;
        struct
        {
            UINT32 ExitCodeLow;
            UINT32 ExitCodeHigh;
        };
    };

    UINT64 ExitInfo1;
    UINT64 ExitInfo2;
    UINT32 ExitIntInfo;
    UINT32 ExitIntInfoErrorCode;

    UINT64 NestedControl;
    UINT64 AvicApicBar;
    UINT64 GhcbGpa;
    UINT32 EventInjection;
    UINT32 EventInjectionError;

    UINT64 NestedCr3;
    UINT64 VirtExt;
    UINT32 VmcbClean;
    UINT32 Reserved5;

    UINT64 NextRip;
    UINT8  InstructionLength;
    UINT8  InstructionBytes[15];

    UINT64 AvicBackingPage;
    UINT8  Reserved6[8];
    UINT64 AvicLogicalId;
    UINT64 AvicPhysicalId;
    UINT8  Reserved7[8];
    UINT64 VmsaPa;
    UINT8  Reserved8[16];
    UINT16 BusLockCounter;
    UINT8  Reserved9[22];
    UINT64 AllowedSevFeatures;
    UINT64 GuestSevFeatures;
    UINT8  Reserved10[664];
    UINT8  ReservedSw[32];
} VMCB_CONTROL_AREA;

typedef struct _VMCB_STATE_SAVE_AREA
{
    VMCB_SEGMENT Es;
    VMCB_SEGMENT Cs;
    VMCB_SEGMENT Ss;
    VMCB_SEGMENT Ds;
    VMCB_SEGMENT Fs;
    VMCB_SEGMENT Gs;
    VMCB_SEGMENT Gdtr;
    VMCB_SEGMENT Ldtr;
    VMCB_SEGMENT Idtr;
    VMCB_SEGMENT Tr;

    UINT8 Reserved0A0[42];
    UINT8 Vmpl;
    UINT8 Cpl;
    UINT8 Reserved0CC[4];

    UINT64 Efer;
    UINT8  Reserved0D8[112];

    UINT64 Cr4;
    UINT64 Cr3;
    UINT64 Cr0;
    UINT64 Dr7;
    UINT64 Dr6;

    UINT64 Rflags;
    UINT64 Rip;
    UINT8  Reserved180[88];

    UINT64 Rsp;
    UINT64 SCet;
    UINT64 Ssp;
    UINT64 IsstAddr;
    UINT64 Rax;
    UINT64 Star;
    UINT64 Lstar;
    UINT64 Cstar;
    UINT64 SfMask;
    UINT64 KernelGsBase;
    UINT64 SysenterCs;
    UINT64 SysenterEsp;
    UINT64 SysenterEip;
    UINT64 Cr2;

    UINT8  Reserved248[32];
    UINT64 Pat;
    UINT64 DebugCtl;
    UINT64 BrFrom;
    UINT64 BrTo;
    UINT64 LastExcpFrom;
    UINT64 LastExcpTo;
    UINT8  Reserved298[72];
    UINT64 SpecCtrl;
} VMCB_STATE_SAVE_AREA;

typedef struct _VMCB
{
    UINT8 Bytes[PAGE_SIZE];
} VMCB;

static __forceinline VMCB_CONTROL_AREA* VmcbControl(VMCB* Vmcb)
{
    return (VMCB_CONTROL_AREA*)&Vmcb->Bytes[0];
}

static __forceinline VMCB_STATE_SAVE_AREA* VmcbState(VMCB* Vmcb)
{
    return (VMCB_STATE_SAVE_AREA*)&Vmcb->Bytes[0x400];
}

#pragma pack(pop)
