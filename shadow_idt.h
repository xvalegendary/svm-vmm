#pragma once

#include "vcpu.h"

extern VCPU* g_CurrentVcpu;

VOID ShadowIdtInitialize(VCPU* V);
VOID ShadowIdtDisable(VCPU* V);
