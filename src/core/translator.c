#include "translator.h"
#include "guest_mem.h"

VA_TRANSLATION_RESULT TranslatorTranslate(VCPU* Vcpu, UINT64 GuestVirtualAddress)
{
    VA_TRANSLATION_RESULT result = { 0 };
    if (!Vcpu)
        return result;

    PHYSICAL_ADDRESS gpa = GuestTranslateGvaToGpa(Vcpu, GuestVirtualAddress);
    if (!gpa.QuadPart)
        return result;

    result.GuestPhysical = gpa;
    result.HostPhysical = GuestTranslateGpaToHpa(Vcpu, gpa.QuadPart);
    result.Valid = (result.HostPhysical.QuadPart != 0);
    return result;
}

PHYSICAL_ADDRESS TranslatorGpaToHpa(VCPU* Vcpu, UINT64 GuestPhysicalAddress)
{
    PHYSICAL_ADDRESS hpa = { 0 };
    if (!Vcpu)
        return hpa;

    return GuestTranslateGpaToHpa(Vcpu, GuestPhysicalAddress);
}
