#pragma once
#include <ntifs.h>
#include "vcpu.h"

#define HV_MAILBOX_SIGNATURE 0x484D42584D41554EULL // 'N' 'A' 'M' 'X' 'B' 'M' 'H'

typedef struct _HV_COMM_MESSAGE
{
    UINT64 Signature;
    UINT64 Code;
    UINT64 Arg0;
    UINT64 Arg1;
} HV_COMM_MESSAGE, *PHV_COMM_MESSAGE;

VOID CommInit(VCPU* Vcpu, UINT64 MailboxGpa);
VOID CommHandleDoorbell(VCPU* Vcpu, UINT64 DoorbellValue);
BOOLEAN CommSend(VCPU* Vcpu, const HV_COMM_MESSAGE* Message);
BOOLEAN CommReceive(VCPU* Vcpu, HV_COMM_MESSAGE* Message);
