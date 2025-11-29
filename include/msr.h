#pragma once
#include <ntifs.h>

#define MSR_EFER                 0xC0000080
#define EFER_SVME                (1ULL << 12)

#define MSR_VM_CR                0xC0010114
#define VM_CR_SVMDIS             (1ULL << 4)

#define MSR_VM_HSAVE_PA          0xC0010117


#define MSR_STAR                  0xC0000081
#define MSR_LSTAR                 0xC0000082
#define MSR_SFMASK                0xC0000084

static __forceinline UINT64 MsrRead(ULONG msr)
{
    return __readmsr(msr);
}
    
static __forceinline VOID MsrWrite(ULONG msr, UINT64 value)
{
    __writemsr(msr, value);
}
