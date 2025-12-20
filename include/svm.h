#pragma once
#include <ntifs.h>

#define SVM_EXIT_CPUID        0x72
#define SVM_EXIT_HLT          0x78
#define SVM_EXIT_IOIO         0x7B
#define SVM_EXIT_MSR          0x7C
#define SVM_EXIT_VMMCALL      0x81
#define SVM_EXIT_NPF          0x400
#define SVM_INTERCEPT_WORD3   3
#define SVM_INTERCEPT_WORD4   4

#define SVM_INTERCEPT_CPUID   (1u << 18)
#define SVM_INTERCEPT_HLT     (1u << 24)
#define SVM_INTERCEPT_IOIO    (1u << 27)
#define SVM_INTERCEPT_MSR     (1u << 28)

#define SVM_INTERCEPT_VMMCALL (1u << 1)

#define SVM_NESTED_CTL_NP_ENABLE 0x1
#define HV_STATUS_BASE                ((NTSTATUS)0xC0F00000)
#define HV_STATUS_ALLOC_VCPU          (HV_STATUS_BASE + 0x01)
#define HV_STATUS_ALLOC_HOSTSAVE      (HV_STATUS_BASE + 0x02)
#define HV_STATUS_ALLOC_VMCB          (HV_STATUS_BASE + 0x03)
#define HV_STATUS_ALLOC_GUEST_STACK   (HV_STATUS_BASE + 0x04)
#define HV_STATUS_ALLOC_MSRPM         (HV_STATUS_BASE + 0x05)
#define HV_STATUS_ALLOC_IOPM          (HV_STATUS_BASE + 0x06)
#define HV_STATUS_NPT_RANGES          (HV_STATUS_BASE + 0x10)
#define HV_STATUS_NPT_FAKEPAGE        (HV_STATUS_BASE + 0x11)
#define HV_STATUS_NPT_PML4            (HV_STATUS_BASE + 0x12)
#define HV_STATUS_NPT_PDPT            (HV_STATUS_BASE + 0x13)
#define HV_STATUS_NPT_PD              (HV_STATUS_BASE + 0x14)
#define HV_STATUS_SMP_ALLOC           (HV_STATUS_BASE + 0x20)
#define HV_STATUS_SVMINIT_CPU_BASE    (HV_STATUS_BASE + 0x30)

#define HV_STATUS_IS_RESOURCE(s) \
    ((s) == STATUS_INSUFFICIENT_RESOURCES || (((s) & 0xFFFF0000) == HV_STATUS_BASE))


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
