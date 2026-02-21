#include "cpuid_info.h"
#include <intrin.h>
#include <Windows.h>
#include <array>

static uint32_t GetLogicalCoreCount()
{
    SYSTEM_INFO si = {};
    GetSystemInfo(&si);
    return si.dwNumberOfProcessors;
}

CpuInfo DetectCpu()
{
    CpuInfo info = {};
    int regs[4] = {};

    // --- Vendor string ---
    __cpuid(regs, 0);
    char vendor[13] = {};
    memcpy(vendor + 0, &regs[1], 4); // EBX
    memcpy(vendor + 4, &regs[3], 4); // EDX
    memcpy(vendor + 8, &regs[2], 4); // ECX
    vendor[12] = '\0';

    if (strcmp(vendor, "GenuineIntel") == 0)
        info.vendor = CpuVendor::Intel;
    else if (strcmp(vendor, "AuthenticAMD") == 0)
        info.vendor = CpuVendor::AMD;
    else
        info.vendor = CpuVendor::Unknown;

    // --- Family, Model, Stepping ---
    __cpuid(regs, 1);
    uint32_t eax = (uint32_t)regs[0];
    uint32_t stepping   = (eax >> 0) & 0xF;
    uint32_t baseModel  = (eax >> 4) & 0xF;
    uint32_t baseFamily = (eax >> 8) & 0xF;
    uint32_t extModel   = (eax >> 16) & 0xF;
    uint32_t extFamily  = (eax >> 20) & 0xFF;

    info.stepping = stepping;
    info.family   = (baseFamily == 0xF) ? (baseFamily + extFamily) : baseFamily;
    info.model    = (baseFamily == 0xF || baseFamily == 0x6)
                    ? ((extModel << 4) | baseModel)
                    : baseModel;

    // --- Logical core count ---
    info.logicalCores = GetLogicalCoreCount();

    // --- Physical core count ---
    if (info.vendor == CpuVendor::Intel) {
        __cpuid(regs, 4);
        uint32_t coresPerPackage = ((regs[0] >> 26) & 0x3F) + 1;
        info.physicalCores = coresPerPackage;
    } else {
        // AMD: use leaf 0x80000008
        __cpuid(regs, (int)0x80000008);
        info.physicalCores = ((uint32_t)regs[2] & 0xFF) + 1;
    }

    // --- Brand string ---
    char brand[49] = {};
    int brandRegs[4];
    __cpuid(brandRegs, (int)0x80000002);
    memcpy(brand +  0, brandRegs, 16);
    __cpuid(brandRegs, (int)0x80000003);
    memcpy(brand + 16, brandRegs, 16);
    __cpuid(brandRegs, (int)0x80000004);
    memcpy(brand + 32, brandRegs, 16);
    brand[48] = '\0';
    info.brandString = brand;

    return info;
}

