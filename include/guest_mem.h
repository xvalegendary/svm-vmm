#pragma once
#include <ntifs.h>
#include "vcpu.h"

BOOLEAN GuestReadGva(VCPU* Vcpu, UINT64 GuestVirtualAddress, PVOID Buffer, SIZE_T Size);
BOOLEAN GuestWriteGva(VCPU* Vcpu, UINT64 GuestVirtualAddress, PVOID Buffer, SIZE_T Size);
BOOLEAN GuestReadGpa(VCPU* Vcpu, UINT64 GuestPhysicalAddress, PVOID Buffer, SIZE_T Size);
BOOLEAN GuestWriteGpa(VCPU* Vcpu, UINT64 GuestPhysicalAddress, PVOID Buffer, SIZE_T Size);

PHYSICAL_ADDRESS GuestTranslateGvaToGpa(VCPU* Vcpu, UINT64 Gva);
PHYSICAL_ADDRESS GuestTranslateGpaToHpa(VCPU* Vcpu, UINT64 Gpa);
PHYSICAL_ADDRESS GuestTranslateGvaToHpa(VCPU* Vcpu, UINT64 Gva);
