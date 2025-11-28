#pragma once

// Stealth toggles
VOID StealthEnable();
VOID StealthDisable();
BOOLEAN StealthIsEnabled();

// CPUID / MSR masking helpers
VOID StealthMaskCpuid(UINT32 leaf, UINT32* ecx, UINT32* edx);
UINT64 StealthMaskMsrRead(UINT32 msr, UINT64 value);

// CR3 obfuscation helpers
UINT64 StealthEncryptCr3(UINT64 cr3);
UINT64 StealthDecryptCr3(UINT64 cr3_enc);

// Memory hiding helpers
VOID StealthHideHypervisorMemory(struct _VCPU* V);
VOID StealthCleanVmcb(struct _VCPU* V);

// Basic anti-analysis guard
BOOLEAN StealthPreventVmrunDetection();
