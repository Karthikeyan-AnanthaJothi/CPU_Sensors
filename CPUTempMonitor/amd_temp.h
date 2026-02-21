#pragma once
#include <Windows.h>
#include <vector>
#include "cpuid_info.h"

struct AmdTempResult {
    float tctl;                    // Tctl (raw sensor, may have offset)
    float tdie;                    // Tdie (true junction temp)
    std::vector<float> ccdTemps;   // Per-CCD temperatures
    bool  valid;
};

// Reads Tctl/Tdie and per-CCD temperatures for AMD Zen CPUs.
AmdTempResult ReadAmdTemperatures(HANDLE hDriver, const CpuInfo& cpu);
