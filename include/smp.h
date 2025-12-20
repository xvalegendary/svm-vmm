#pragma once

#include <ntifs.h>
#include "vcpu.h"

typedef struct _SMP_STATE
{
    ULONG ProcessorCount;
    VCPU** Vcpus;
    PPROCESSOR_NUMBER ProcessorNumbers;
} SMP_STATE;

#define SMP_MAX_VCPUS_ALL 0

NTSTATUS SmpInitialize(SMP_STATE* State, ULONG MaxVcpus);
NTSTATUS SmpLaunch(SMP_STATE* State);
VOID SmpShutdown(SMP_STATE* State);
