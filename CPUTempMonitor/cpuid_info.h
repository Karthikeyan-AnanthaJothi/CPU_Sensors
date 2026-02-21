#pragma once
#include <cstdint>
#include <string>

enum class CpuVendor { Intel, AMD, Unknown };

struct CpuInfo {
    CpuVendor vendor;
    uint32_t  family;       // Extended family
    uint32_t  model;        // Extended model
    uint32_t  stepping;
    uint32_t  logicalCores; // Logical processor count
    uint32_t  physicalCores;
    std::string brandString;
};

// Detects CPU vendor, family, model, and core counts using CPUID.
CpuInfo DetectCpu();
