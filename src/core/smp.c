#include <ntifs.h>
#include "smp.h"
#include "svm.h"

#define SMP_VCPU_TAG 'VmsP'
#define SMP_PNUM_TAG 'NmsP'

static VOID SmpFreeState(SMP_STATE* State)
{
    if (!State)
        return;

    if (State->Vcpus)
    {
        for (ULONG i = 0; i < State->ProcessorCount; i++)
        {
            if (State->Vcpus[i])
                SvmShutdown(State->Vcpus[i]);
        }
    }

    if (State->Vcpus)
        ExFreePoolWithTag(State->Vcpus, SMP_VCPU_TAG);

    if (State->ProcessorNumbers)
        ExFreePoolWithTag(State->ProcessorNumbers, SMP_PNUM_TAG);

    RtlZeroMemory(State, sizeof(*State));
}

NTSTATUS SmpInitialize(SMP_STATE* State, ULONG MaxVcpus)
{
    if (!State)
        return STATUS_INVALID_PARAMETER;

    RtlZeroMemory(State, sizeof(*State));

    ULONG available = KeQueryActiveProcessorCountEx(ALL_PROCESSOR_GROUPS);
    if (available == 0)
        return STATUS_NOT_SUPPORTED;

    ULONG target = available;
    if (MaxVcpus && MaxVcpus < target)
        target = MaxVcpus;

    DbgPrint("SVM-HV: SMP init, available=%lu target=%lu\n", available, target);

    State->ProcessorCount = target;
    State->Vcpus = ExAllocatePoolWithTag(NonPagedPoolNx, sizeof(VCPU*) * target, SMP_VCPU_TAG);
    State->ProcessorNumbers = ExAllocatePoolWithTag(NonPagedPoolNx, sizeof(PROCESSOR_NUMBER) * target, SMP_PNUM_TAG);

    if (!State->Vcpus || !State->ProcessorNumbers)
    {
        DbgPrint("SVM-HV: SMP alloc failed (vcpus=%p, pnums=%p)\n",
            State->Vcpus, State->ProcessorNumbers);
        SmpFreeState(State);
        return HV_STATUS_SMP_ALLOC;
    }

    RtlZeroMemory(State->Vcpus, sizeof(VCPU*) * target);
    RtlZeroMemory(State->ProcessorNumbers, sizeof(PROCESSOR_NUMBER) * target);

    for (ULONG i = 0; i < target; i++)
        KeGetProcessorNumberFromIndex(i, &State->ProcessorNumbers[i]);

    for (ULONG i = 0; i < target; i++)
    {
        PROCESSOR_NUMBER pn = State->ProcessorNumbers[i];
        GROUP_AFFINITY affinity = { 0 };
        GROUP_AFFINITY previous = { 0 };

        affinity.Group = pn.Group;
        affinity.Mask = 1ull << pn.Number;

        KeSetSystemGroupAffinityThread(&affinity, &previous);
        NTSTATUS st = SvmInit(&State->Vcpus[i]);
        if (NT_SUCCESS(st))
        {
            VMCB_CONTROL_AREA* c = VmcbControl(State->Vcpus[i]->Vmcb);
            c->GuestAsid = i + 1;
        }
        KeRevertToUserGroupAffinityThread(&previous);

        if (!NT_SUCCESS(st))
        {
            DbgPrint("SVM-HV: SvmInit failed on cpu=%lu (status=0x%X)\n", i, st);
            SmpFreeState(State);
            if (HV_STATUS_IS_RESOURCE(st))
                return (HV_STATUS_SVMINIT_CPU_BASE + i);
            return st;
        }
    }

    return STATUS_SUCCESS;
}

NTSTATUS SmpLaunch(SMP_STATE* State)
{
    if (!State || !State->Vcpus)
        return STATUS_INVALID_PARAMETER;

    NTSTATUS st = STATUS_SUCCESS;

    for (ULONG i = 0; i < State->ProcessorCount; i++)
    {
        if (!State->Vcpus[i])
            continue;

        PROCESSOR_NUMBER pn = State->ProcessorNumbers[i];
        GROUP_AFFINITY affinity = { 0 };
        GROUP_AFFINITY previous = { 0 };

        affinity.Group = pn.Group;
        affinity.Mask = 1ull << pn.Number;

        KeSetSystemGroupAffinityThread(&affinity, &previous);
        st = SvmLaunch(State->Vcpus[i]);
        KeRevertToUserGroupAffinityThread(&previous);

        if (!NT_SUCCESS(st))
            return st;
    }

    return st;
}

VOID SmpShutdown(SMP_STATE* State)
{
    SmpFreeState(State);
}
