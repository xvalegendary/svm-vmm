#pragma once


VOID StealthEnable();
VOID StealthDisable();
BOOLEAN StealthIsEnabled();


VOID StealthMaskCpuid(UINT32 leaf, UINT32* ecx, UINT32* edx);
UINT64 StealthMaskMsrRead(UINT32 msr, UINT64 value);


UINT64 StealthEncryptCr3(UINT64 cr3);
UINT64 StealthDecryptCr3(UINT64 cr3_enc);


VOID StealthHideHypervisorMemory(struct _VCPU* V);
VOID StealthCleanVmcb(struct _VCPU* V);


BOOLEAN StealthPreventVmrunDetection();
