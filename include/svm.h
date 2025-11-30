#pragma once
#include <ntifs.h>

#define SVM_EXIT_CPUID        0x72
#define SVM_EXIT_HLT          0x78
#define SVM_EXIT_IOIO         0x7B
#define SVM_EXIT_MSR          0x7C
#define SVM_EXIT_VMMCALL      0x81
#define SVM_EXIT_NPF          0x400

#define MSR_EFER              0xC0000080
#define EFER_SVME             (1ULL << 12)

#define MSR_VM_CR             0xC0010114
#define VM_CR_SVMDIS          (1ULL << 4)

#define MSR_VM_HSAVE_PA       0xC0010117

//
// ÂÀÆÍÎ: ñòðóêòóðà çäåñü ÍÅ ÎÁÚßÂËßÅÒÑß!
// Ìû äåëàåì òîëüêî forward declaration
//

struct _VCPU;

NTSTATUS SvmInit(struct _VCPU** Out);
NTSTATUS SvmLaunch(struct _VCPU* V);
VOID     SvmShutdown(struct _VCPU* V);

NTSTATUS HypervisorHandleExit(struct _VCPU* V);
