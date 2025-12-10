#include "communication.h"
#include "guest_mem.h"

/* todo: make communication (shared memory) */

VOID CommInit(VCPU* Vcpu, UINT64 MailboxGpa)
{
    if (!Vcpu)
        return;

    Vcpu->Ipc.MailboxGpa = MailboxGpa;
    Vcpu->Ipc.LastMessage = 0;
    Vcpu->Ipc.Active = TRUE;
}

static BOOLEAN CommMailboxValid(VCPU* Vcpu)
{
    return Vcpu && Vcpu->Ipc.Active && Vcpu->Ipc.MailboxGpa != 0;
}

BOOLEAN CommSend(VCPU* Vcpu, const HV_COMM_MESSAGE* Message)
{
    if (!CommMailboxValid(Vcpu) || !Message)
        return FALSE;

    HV_COMM_MESSAGE msg = *Message;
    msg.Signature = HV_MAILBOX_SIGNATURE;

    return GuestWriteGpa(Vcpu, Vcpu->Ipc.MailboxGpa, &msg, sizeof(msg));
}

VOID CommHandleDoorbell(VCPU* Vcpu, UINT64 DoorbellValue)
{
    if (!CommMailboxValid(Vcpu))
        return;

    
    HV_COMM_MESSAGE msg = { 0 };
    if (GuestReadGpa(Vcpu, Vcpu->Ipc.MailboxGpa, &msg, sizeof(msg)))
    {
        if (msg.Signature == HV_MAILBOX_SIGNATURE)
        {
            Vcpu->Ipc.LastMessage = DoorbellValue;
        }
    }
}

BOOLEAN CommReceive(VCPU* Vcpu, HV_COMM_MESSAGE* Message)
{
    if (!CommMailboxValid(Vcpu) || !Message)
        return FALSE;

    HV_COMM_MESSAGE msg = { 0 };
    if (!GuestReadGpa(Vcpu, Vcpu->Ipc.MailboxGpa, &msg, sizeof(msg)))
        return FALSE;

    if (msg.Signature != HV_MAILBOX_SIGNATURE)
        return FALSE;

    *Message = msg;
    return TRUE;
}
