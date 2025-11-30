#pragma once
#include <ntifs.h>

#pragma pack(push, 1)

typedef struct _VMCB_CONTROL_AREA
{
    UINT32 InterceptCrRead;
    UINT32 InterceptCrWrite;
    UINT32 InterceptDrRead;
    UINT32 InterceptDrWrite;
    UINT32 InterceptException;
    UINT32 InterceptInstruction1;
    UINT32 InterceptInstruction2;

    UINT8  Reserved0[0x3C];

    UINT16 PauseFilterThreshold;
    UINT16 PauseFilterCount;

    UINT64 IopmBasePa;
    UINT64 MsrpmBasePa;

    UINT64 TscOffset;

    UINT32 GuestAsid;
    UINT32 TlbControl;

    UINT64 VIntrVector;
    UINT64 VIntrControl;

    UINT64 ExitCode;
    UINT64 ExitInfo1;
    UINT64 ExitInfo2;
    UINT64 ExitIntInfo;
    UINT64 ExitIntInfoErrorCode;

    UINT64 NptControl;

    UINT64 Ncr3;

    UINT8 Reserved1[0x70];

    UINT64 VmcbCleanBits;
    UINT64 Nrip;

    UINT8 Reserved2[0x2C0];

} VMCB_CONTROL_AREA;

typedef struct _VMCB_STATE_SAVE_AREA
{
    UINT16 EsSelector;  UINT16 EsAttributes;  UINT32 EsLimit;  UINT64 EsBase;
    UINT16 CsSelector;  UINT16 CsAttributes;  UINT32 CsLimit;  UINT64 CsBase;
    UINT16 SsSelector;  UINT16 SsAttributes;  UINT32 SsLimit;  UINT64 SsBase;
    UINT16 DsSelector;  UINT16 DsAttributes;  UINT32 DsLimit;  UINT64 DsBase;
    UINT16 FsSelector;  UINT16 FsAttributes;  UINT32 FsLimit;  UINT64 FsBase;
    UINT16 GsSelector;  UINT16 GsAttributes;  UINT32 GsLimit;  UINT64 GsBase;

    UINT16 GdtrLimit;
    UINT64 GdtrBase;

    UINT16 IdtrLimit;
    UINT64 IdtrBase;

    UINT16 LdtrSelector;
    UINT16 LdtrAttributes;
    UINT32 LdtrLimit;
    UINT64 LdtrBase;

    UINT16 TrSelector;
    UINT16 TrAttributes;
    UINT32 TrLimit;
    UINT64 TrBase;

    UINT8 Reserved0[0x2E];

    UINT64 Efer;
    UINT64 Cr4;
    UINT64 Cr3;
    UINT64 Cr0;
    UINT64 Dr7;
    UINT64 Dr6;

    UINT64 Rflags;
    UINT64 Rip;
    UINT64 Rsp;

    UINT64 Rax;
    UINT64 Rbx;
    UINT64 Rcx;
    UINT64 Rdx;
    UINT64 Star;
    UINT64 Lstar;
    UINT64 Cstar;
    UINT64 SfMask;
    UINT64 KernelGsBase;
    UINT64 SysenterCs;
    UINT64 SysenterEsp;
    UINT64 SysenterEip;
    UINT64 Cr2;

    UINT8 Reserved1[0x20];

    UINT64 Pat;
    UINT64 DebugCtl;
    UINT64 DebugExcpMask;
    UINT64 DebugExcp2;
    UINT64 BrFrom;
    UINT64 BrTo;
    UINT64 LsFrom;
    UINT64 LsTo;

    UINT8 Reserved2[0x1A0];

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
