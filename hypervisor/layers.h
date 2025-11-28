#pragma once
#include "vcpu.h"

// High level helper to arm all stealth/entry tricks
VOID HvActivateLayeredPipeline(VCPU* V);

// Handle hardware entry + IPC backed by NPT triggers
BOOLEAN HvHandleLayeredNpf(VCPU* V, UINT64 faultGpa);

// Anti-exit + cloaking updates
VOID HvRefreshExecLayer(VCPU* V, UINT64 exitCode);
