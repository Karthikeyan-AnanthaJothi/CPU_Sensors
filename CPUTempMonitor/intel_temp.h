#pragma once
#include <Windows.h>
#include <vector>
#include "cpuid_info.h"

struct IntelTempResult {
    std::vector<float> coreTemps;  // Per-core temperatures in Celsius
    float packageTemp;             // Package temperature in Celsius
    float tjMax;                   // TjMax in Celsius
    bool  valid;
};

// Reads per-core and package temperatures for Intel CPUs.
IntelTempResult ReadIntelTemperatures(HANDLE hDriver, const CpuInfo& cpu);
