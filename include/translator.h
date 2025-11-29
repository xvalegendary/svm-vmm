#pragma once
#include <ntifs.h>
#include "vcpu.h"

typedef struct _VA_TRANSLATION_RESULT
{
    BOOLEAN Valid;
    PHYSICAL_ADDRESS GuestPhysical;
    PHYSICAL_ADDRESS HostPhysical;
} VA_TRANSLATION_RESULT, *PVA_TRANSLATION_RESULT;

VA_TRANSLATION_RESULT TranslatorTranslate(VCPU* Vcpu, UINT64 GuestVirtualAddress);
PHYSICAL_ADDRESS TranslatorGpaToHpa(VCPU* Vcpu, UINT64 GuestPhysicalAddress);
