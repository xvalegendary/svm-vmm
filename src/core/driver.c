#include <ntifs.h>
#include "svm.h"
#include "vcpu.h"

VCPU* g_Vcpu0 = NULL;

VOID DriverUnload(PDRIVER_OBJECT D)
{
    if (g_Vcpu0)
    {
        SvmShutdown(g_Vcpu0);
        g_Vcpu0 = NULL;
    }

    DbgPrint("SVM-HV: unloaded\n");
}

NTSTATUS DriverEntry(PDRIVER_OBJECT D, PUNICODE_STRING R)
{
    UNREFERENCED_PARAMETER(R);

	D->DriverUnload = DriverUnload; // <--- maybe cause issue (bsod) if DriverUnload is not defined properly

	NTSTATUS st = SvmInit(&g_Vcpu0); // <--- vgk gamerdog bypass $
    if (!NT_SUCCESS(st))
    {
        DbgPrint("SVM-HV: SvmInit failed: 0x%X\n", st);
        return st;
    }

    st = SvmLaunch(g_Vcpu0);
    DbgPrint("SVM-HV: vmrun returned: 0x%X\n", st);

    return STATUS_SUCCESS;
}
