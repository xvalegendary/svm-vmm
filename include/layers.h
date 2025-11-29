#pragma once
#include "vcpu.h"


VOID HvActivateLayeredPipeline(VCPU* V);


BOOLEAN HvHandleLayeredNpf(VCPU* V, UINT64 faultGpa);


VOID HvRefreshExecLayer(VCPU* V, UINT64 exitCode);
