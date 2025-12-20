#include <ntifs.h>
#include "svm.h"
#include "vcpu.h"
#include "smp.h"



//        ,
//        `-._           __
//         \\  `-..____,.'  `.
//          :`.         /    `.
//          :  )       :      : \
//           ;'        '   ;  |  :
//           )..      .. .:.`.;  :
//          /::...  .:::...   ` ;
//          ; _ '    __        /:\
//          `:o>   /\o_>      ;:. `.
//         `-`.__ ;   __..--- /:.   \
//         === \_/   ;=====_.':.     ;
//          ,/'`--'...`--....        ;
//               ;                    ;
//             .'                      ;
//           .'                        ;
//         .'     ..     ,      .       ;
//        :       ::..  /      ;::.     |
//       /      `.;::.  |       ;:..    ;
//      :         |:.   :       ;:.    ;
//      :         ::     ;:..   |.    ;
//       :       :;      :::....|     |
//       /\     ,/ \      ;:::::;     ;
//     .:. \:..|    :     ; '.--|     ;
//    ::.  :''  `-.,,;     ;'   ;     ;
//  .-'. _.'\      / `;      \,__:      \
//  `---'    `----'   ;      /    \,.,,,/
//                    `----`              sad



static SMP_STATE g_Smp = { 0 };

#define SMP_INIT_MAX_VCPUS SMP_MAX_VCPUS_ALL

VOID DriverUnload(PDRIVER_OBJECT D)
{
    if (g_Smp.Vcpus)
        SmpShutdown(&g_Smp);

    DbgPrint("SVM-HV: unloaded\n");
}

NTSTATUS DriverEntry(PDRIVER_OBJECT D, PUNICODE_STRING R)
{
    UNREFERENCED_PARAMETER(R);

    if (D)
    {
        D->DriverUnload = DriverUnload;
    }
    else
    {
        DbgPrint("SVM-HV: DriverEntry called without DriverObject (mapper load), skipping unload registration.\n");
    }

	NTSTATUS st = SmpInitialize(&g_Smp, SMP_INIT_MAX_VCPUS);
    if (!NT_SUCCESS(st))
    {
        DbgPrint("SVM-HV: SmpInitialize failed: 0x%X\n", st);
        if (HV_STATUS_IS_RESOURCE(st))
        {
            DbgPrint("SVM-HV: retrying with single VCPU\n");
            st = SmpInitialize(&g_Smp, 1);
        }

        if (!NT_SUCCESS(st))
            return st;
    }

    st = SmpLaunch(&g_Smp);
    if (!NT_SUCCESS(st))
    {
        DbgPrint("SVM-HV: SmpLaunch failed: 0x%X\n", st);
        SmpShutdown(&g_Smp);
        return st;
    }
    DbgPrint("SVM-HV: vmrun returned: 0x%X\n", st);

    return STATUS_SUCCESS;
}
