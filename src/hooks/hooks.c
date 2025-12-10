#include <ntifs.h>
#include "hooks.h"
#include "vcpu.h"
#include "vmcb.h"
#include "guest_mem.h"
#include "npt.h"
#include "stealth.h"
#include "msr.h"
#include "translator.h"
#include "process_manager.h"
#include "communication.h"



static UINT64 g_OriginalLstar = 0;        
static UINT64 g_OriginalStar = 0;
static UINT64 g_OriginalSfMask = 0;

static UINT64 g_HvSyscallHandler = 0;     
static BOOLEAN g_SyscallHookEnabled = FALSE;

static BOOLEAN g_Cr3EncryptionEnabled = FALSE;
static UINT64 g_Cr3XorKey = 0xCAFEBABE1337ULL;

static BOOLEAN HookIsCr3PagePresent(VCPU* V, UINT64 cr3)
{
    UINT64 pml4 = cr3 & ~0xFFFULL;
    UINT64 entry = 0;

   
	if (!GuestReadGpa(V, pml4, &entry, sizeof(entry))) // <-- ReadGuestPhysical(IVAN_KUST TECHNOLOGIES (INSOMIA.SOLUTIONS!!!! PRESENTS$))
        return FALSE;

    return (entry & 1ULL) != 0;
}



VOID HookCpuidEmulate(UINT32 leaf, UINT32 subleaf,
    UINT32* eax, UINT32* ebx, UINT32* ecx, UINT32* edx)
{
    //
    // :  vendor-string
    //
    if (leaf == 0)
    {
        *ebx = 'V', 'M', 'S', 'V';  // vmsv
        *ecx = 'H', 'V', 'A', 'M';  // hvam
        *edx = 'S', 'T', 'E', 'L';  // stel
    }


    *ecx &= ~(1 << 31);  
}



UINT64 HookHandleMsrRead(VCPU* V, UINT64 msr)
{
    switch (msr)
    {
    case MSR_LSTAR:
        if (g_SyscallHookEnabled)
            return g_HvSyscallHandler;
        return g_OriginalLstar;

    case MSR_STAR:
        return g_OriginalStar;

    case MSR_SFMASK:
        return g_OriginalSfMask;

    default:
        return StealthMaskMsrRead((UINT32)msr, __readmsr(msr));
    }
}

VOID HookInstallSyscall(VCPU* V)
{
    if (g_SyscallHookEnabled) return;

    g_OriginalLstar = __readmsr(MSR_LSTAR);
    g_OriginalStar = __readmsr(MSR_STAR);
    g_OriginalSfMask = __readmsr(MSR_SFMASK);

    //
    //  syscall entry    
    //
    if (g_HvSyscallHandler != 0)
    {
        __writemsr(MSR_LSTAR, g_HvSyscallHandler);
        g_SyscallHookEnabled = TRUE;
    }
}

VOID HookRemoveSyscall()
{
    if (!g_SyscallHookEnabled) return;

    __writemsr(MSR_LSTAR, g_OriginalLstar);
    __writemsr(MSR_STAR, g_OriginalStar);
    __writemsr(MSR_SFMASK, g_OriginalSfMask);

    g_SyscallHookEnabled = FALSE;
}

VOID HookHandleMsrWrite(VCPU* V, UINT64 msr, UINT64 value)
{
    switch (msr)
    {
    case MSR_LSTAR:
    {
        g_OriginalLstar = value;
        return;
    }
    case MSR_STAR:
    {
        g_OriginalStar = value;
        return;
    }
    case MSR_SFMASK:
    {
        g_OriginalSfMask = value;
        return;
    }
    default:
        __writemsr(msr, value);
        return;
    }
}



UINT64 HookEncryptCr3(UINT64 cr3)
{
    if (!g_Cr3EncryptionEnabled)
        return cr3;

    return cr3 ^ g_Cr3XorKey;
}

UINT64 HookDecryptCr3(VCPU* V, UINT64 cr3_enc)
{
    if (!g_Cr3EncryptionEnabled)
        return cr3_enc;

    UINT64 candidate = cr3_enc ^ g_Cr3XorKey;

    
    if (V && HookIsCr3PagePresent(V, candidate))
        return candidate;

   
    return candidate;
}

VOID HookEnableCr3Encryption()
{
    g_Cr3EncryptionEnabled = TRUE;
}

VOID HookDisableCr3Encryption()
{
    g_Cr3EncryptionEnabled = FALSE;
}



BOOLEAN HookNptHandleFault(VCPU* V, UINT64 faultingGpa)
{
    UINT64 page = faultingGpa & ~0xFFFULL;

    if (V->Npt.ShadowHook.Active && page == V->Npt.ShadowHook.TargetGpaPage)
    {
        NptHookPage(&V->Npt, page, V->Npt.ShadowHook.NewHpaPage);
        return TRUE;
    }

    return FALSE;
}



UINT64 HookVmmcallDispatch(VCPU* V, UINT64 code, UINT64 a1, UINT64 a2, UINT64 a3)
{
    switch (code)
    {
    case 0x100:   // read guest virtual mem
    {
        UINT8 buf[8];
        if (GuestReadGva(V, a1, buf, sizeof(buf)))
            return *(UINT64*)buf;
        return 0;
    }

    case 0x101:   // write guest memory
    {
        UINT64 value = a2;
        GuestWriteGva(V, a1, &value, sizeof(value));
        return TRUE;
    }

    case 0x102:   // enable CR3 XOR hook
        HookEnableCr3Encryption();
        return TRUE;

    case 0x103:   // disable CR3 XOR hook
        HookDisableCr3Encryption();
        return TRUE;

    case 0x110:   // install shadow EPT hook (a1 = target GVA, a2 = new HPA/GPA)
    {
        PHYSICAL_ADDRESS gpa = GuestTranslateGvaToGpa(V, a1);
        if (!gpa.QuadPart)
            return FALSE;

        return NptInstallShadowHook(&V->Npt, gpa.QuadPart, a2);
    }

    case 0x111:   // clear shadow hook
        NptClearShadowHook(&V->Npt);
        return TRUE;

    case 0x200:  // stealth mode enable
        StealthEnable();
        return TRUE;

    case 0x201:
        StealthDisable();
        return TRUE;

    case 0x210: // fetch last mailbox payload
    {
        HV_COMM_MESSAGE message = { 0 };
        if (CommReceive(V, &message))
            return message.Code;
        return 0;
    }

    case 0x211: // send mailbox payload (a1..a3)
    {
        HV_COMM_MESSAGE message = { 0 };
        message.Code = a1;
        message.Arg0 = a2;
        message.Arg1 = a3;
        return CommSend(V, &message);
    }

    case 0x220: // translate guest virtual to guest physical
    {
        VA_TRANSLATION_RESULT tx = TranslatorTranslate(V, a1);
        return tx.Valid ? tx.GuestPhysical.QuadPart : 0;
    }

    case 0x221: // translate guest virtual to host physical
    {
        VA_TRANSLATION_RESULT tx = TranslatorTranslate(V, a1);
        return tx.Valid ? tx.HostPhysical.QuadPart : 0;
    }

    case 0x222: // translate guest physical to host physical
    {
        PHYSICAL_ADDRESS hpa = TranslatorGpaToHpa(V, a1);
        return hpa.QuadPart;
    }

    case 0x320: // query current process base
    {
        PROCESS_DETAILS details = { 0 };
        if (NT_SUCCESS(ProcessQueryCurrent(&details)))
            return details.ImageBase;
        return 0;
    }

    case 0x321: // query process base by pid
    {
        PROCESS_DETAILS details = { 0 };
        if (NT_SUCCESS(ProcessQueryByPid((HANDLE)a1, &details)))
            return details.ImageBase;
        return 0;
    }

    case 0x322: // query process dirbase by pid
    {
        PROCESS_DETAILS details = { 0 };
        if (NT_SUCCESS(ProcessQueryByPid((HANDLE)a1, &details)))
            return details.DirectoryTableBase;
        return 0;
    }

    case 0x300: // enable syscall hook
        HookInstallSyscall(V);
        return TRUE;

    case 0x301:
        HookRemoveSyscall();
        return TRUE;

    default:
        return 0xDEADBEEF;
    }
}



VOID HookIoIntercept(VCPU* V)
{
    //  todo:    IO 
}

