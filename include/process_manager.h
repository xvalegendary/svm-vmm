#pragma once
#include <ntifs.h>

typedef struct _PROCESS_DETAILS
{
    HANDLE ProcessId;
    UINT64 ImageBase;
    UINT64 DirectoryTableBase;
} PROCESS_DETAILS, *PPROCESS_DETAILS;

NTSTATUS ProcessQueryByPid(HANDLE Pid, PPROCESS_DETAILS Details);
NTSTATUS ProcessQueryCurrent(PPROCESS_DETAILS Details);
