#include "process_manager.h"
#include <intrin.h>

#define EPROCESS_DIRECTORY_TABLE_BASE 0x28

static VOID ProcessFillInfo(PEPROCESS Process, HANDLE Pid, PPROCESS_DETAILS Details)
{
    Details->ProcessId = Pid;
    Details->ImageBase = (UINT64)PsGetProcessSectionBaseAddress(Process);

    // Accessing DirectoryTableBase is not part of the public WDK surface but
    // is stable enough for private use inside this research hypervisor.
    Details->DirectoryTableBase = *(UINT64*)((PUCHAR)Process + EPROCESS_DIRECTORY_TABLE_BASE);
}

NTSTATUS ProcessQueryByPid(HANDLE Pid, PPROCESS_DETAILS Details)
{
    if (!Details)
        return STATUS_INVALID_PARAMETER;

    PEPROCESS process = NULL;
    NTSTATUS status = PsLookupProcessByProcessId(Pid, &process);
    if (!NT_SUCCESS(status))
        return status;

    ProcessFillInfo(process, Pid, Details);
    ObDereferenceObject(process);
    return STATUS_SUCCESS;
}

NTSTATUS ProcessQueryCurrent(PPROCESS_DETAILS Details)
{
    if (!Details)
        return STATUS_INVALID_PARAMETER;

    PEPROCESS process = PsGetCurrentProcess();
    ProcessFillInfo(process, PsGetCurrentProcessId(), Details);
    return STATUS_SUCCESS;
}
