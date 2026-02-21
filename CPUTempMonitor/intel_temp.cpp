#include "intel_temp.h"
#include "ring0_interface.h"

// Intel MSR addresses
static const uint32_t MSR_TEMPERATURE_TARGET  = 0x1A2; // TjMax in bits [23:16]
static const uint32_t IA32_THERM_STATUS       = 0x19C; // Core temp delta in bits [22:16]
static const uint32_t IA32_PACKAGE_THERM_STATUS = 0x1B1; // Package temp delta in bits [22:16]

// Get TjMax for a specific core
static float GetTjMax(HANDLE hDriver, uint32_t coreIndex)
{
    uint32_t eax = 0, edx = 0;
    if (Ring0ReadMsr(hDriver, MSR_TEMPERATURE_TARGET, coreIndex, eax, edx)) {
        // TjMax is in bits [23:16]
        uint32_t tjMax = (eax >> 16) & 0xFF;
        if (tjMax > 0) return (float)tjMax;
    }
    return 100.0f; // Safe default
}

IntelTempResult ReadIntelTemperatures(HANDLE hDriver, const CpuInfo& cpu)
{
    IntelTempResult result = {};
    result.valid = false;

    uint32_t numCores = cpu.physicalCores;
    if (numCores == 0) numCores = cpu.logicalCores;
    if (numCores == 0) numCores = 1;

    // Get TjMax from core 0
    result.tjMax = GetTjMax(hDriver, 0);
    result.coreTemps.resize(numCores, 0.0f);

    bool anyValid = false;

    // Read per-core temperatures
    for (uint32_t core = 0; core < numCores; core++) {
        uint32_t eax = 0, edx = 0;
        if (Ring0ReadMsr(hDriver, IA32_THERM_STATUS, core, eax, edx)) {
            // Bit 31: reading valid
            if (eax & 0x80000000) {
                // Bits [22:16]: digital readout (delta below TjMax)
                uint32_t deltaT = (eax >> 16) & 0x7F;
                result.coreTemps[core] = result.tjMax - (float)deltaT;
                anyValid = true;
            }
        }
    }

    // Read package temperature (uses core 0 affinity, reads package MSR)
    {
        uint32_t eax = 0, edx = 0;
        if (Ring0ReadMsr(hDriver, IA32_PACKAGE_THERM_STATUS, 0, eax, edx)) {
            if (eax & 0x80000000) {
                uint32_t deltaT = (eax >> 16) & 0x7F;
                result.packageTemp = result.tjMax - (float)deltaT;
            }
        }
    }

    result.valid = anyValid;
    return result;
}
