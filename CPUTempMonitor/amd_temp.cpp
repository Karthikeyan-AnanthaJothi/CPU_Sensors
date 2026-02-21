#include "amd_temp.h"
#include "ring0_interface.h"

// AMD SMN access is via PCI Bus 0, Device 0, Function 0
// Write SMN address to offset 0x60, read data from offset 0x64
static const uint32_t AMD_PCI_BUS  = 0;
static const uint32_t AMD_PCI_DEV  = 0;
static const uint32_t AMD_PCI_FUNC = 0;
static const uint32_t AMD_SMN_ADDR_REG = 0x60;
static const uint32_t AMD_SMN_DATA_REG = 0x64;

// SMN register addresses
static const uint32_t THM_TCON_CUR_TMP = 0x00059800; // Tctl/Tdie
// Zen2/3 CCD temps (up to 8 CCDs)
static const uint32_t F17H_M70H_CCD1_TEMP = 0x00059954;
// Zen4/5 CCD temps
static const uint32_t F19H_M61H_CCD1_TEMP = 0x00059B08;

// Read a 32-bit value from AMD SMN bus
static bool SmnRead(HANDLE hDriver, uint32_t smnAddr, uint32_t& outValue)
{
    // Step 1: Write the SMN address to the index register
    if (!Ring0WritePci(hDriver, AMD_PCI_BUS, AMD_PCI_DEV, AMD_PCI_FUNC,
                       AMD_SMN_ADDR_REG, smnAddr))
        return false;

    // Step 2: Read the data from the data register
    return Ring0ReadPci(hDriver, AMD_PCI_BUS, AMD_PCI_DEV, AMD_PCI_FUNC,
                        AMD_SMN_DATA_REG, outValue);
}

AmdTempResult ReadAmdTemperatures(HANDLE hDriver, const CpuInfo& cpu)
{
    AmdTempResult result = {};
    result.valid = false;

    // --- Read Tctl/Tdie from THM_TCON_CUR_TMP ---
    uint32_t raw = 0;
    if (!SmnRead(hDriver, THM_TCON_CUR_TMP, raw)) {
        return result;
    }

    // Bits [31:21] = temperature value in units of 0.125°C
    // Tctl = raw_value / 8.0
    uint32_t rawVal = (raw >> 21) & 0x7FF;
    float tctl = (float)rawVal / 8.0f;

    // Bit 19: hardware offset flag (applies -49°C for specific SKUs like 1600X/1700X/1800X)
    bool hasHwOffset = (raw >> 19) & 1;
    if (hasHwOffset) tctl -= 49.0f;

    // Model-specific software offsets (from LibreHardwareMonitor / AMD documentation)
    // These convert Tctl → Tdie (true junction temperature)
    float tcOffset = 0.0f;
    if (cpu.family == 0x17) {
        switch (cpu.model) {
            case 0x01: tcOffset = -49.0f; break; // Zen  - Summit Ridge (1000 series)
            case 0x08: tcOffset = -49.0f; break; // Zen+ - Pinnacle Ridge (2000 series)
            case 0x11: tcOffset = -49.0f; break; // Zen  - Raven Ridge APU
            case 0x18: tcOffset = -49.0f; break; // Zen+ - Picasso APU
            case 0x31: tcOffset =   0.0f; break; // Zen2 - Castle Peak (Threadripper 3000)
            case 0x60: tcOffset = -49.0f; break; // Zen2 - Renoir APU
            case 0x68: tcOffset = -49.0f; break; // Zen2 - Lucienne APU
            case 0x71: tcOffset =   0.0f; break; // Zen2 - Matisse (3000 series)
            default:   tcOffset =   0.0f; break;
        }
    } else if (cpu.family == 0x19) {
        switch (cpu.model) {
            case 0x01: tcOffset =   0.0f; break; // Zen3 - Chagall (Threadripper 5000)
            case 0x08: tcOffset =   0.0f; break; // Zen3 - Cezanne APU
            case 0x21: tcOffset =   0.0f; break; // Zen3 - Vermeer (5000 series) ← YOUR CPU
            case 0x40: tcOffset =   0.0f; break; // Zen3+ - Rembrandt APU
            case 0x44: tcOffset =   0.0f; break; // Zen3+ - Rembrandt-R APU
            case 0x50: tcOffset =   0.0f; break; // Zen3+ - Cezanne APU
            case 0x61: tcOffset =   0.0f; break; // Zen4 - Raphael (7000 series)
            case 0x74: tcOffset =   0.0f; break; // Zen4 - Phoenix APU
            default:   tcOffset =   0.0f; break;
        }
    } else if (cpu.family == 0x1A) {
        tcOffset = 0.0f; // Zen5 - Granite Ridge (9000 series)
    }

    float tdie = tctl + tcOffset;

    result.tctl = tctl;
    result.tdie = tdie;
    result.valid = true;

    // --- Read per-CCD temperatures ---
    // Zen2/3 (family 0x17 model >= 0x31, family 0x19)
    bool isZen4Plus = (cpu.family == 0x19 && cpu.model >= 0x61) ||
                      (cpu.family == 0x1A);

    uint32_t ccdBase = isZen4Plus ? F19H_M61H_CCD1_TEMP : F17H_M70H_CCD1_TEMP;
    uint32_t maxCcds = 8;

    for (uint32_t i = 0; i < maxCcds; i++) {
        uint32_t ccdRaw = 0;
        uint32_t ccdAddr = ccdBase + (i * 4);
        if (!SmnRead(hDriver, ccdAddr, ccdRaw)) break;

        // Bits [11:0] = raw CCD temp
        uint32_t ccdVal = ccdRaw & 0xFFF;
        if (ccdVal == 0) break; // No more CCDs

        // Formula: temp = (ccdVal * 125 - 305000) / 1000.0f
        float ccdTemp = ((float)ccdVal * 125.0f - 305000.0f) / 1000.0f;

        if (ccdTemp > -40.0f && ccdTemp < 150.0f) {
            result.ccdTemps.push_back(ccdTemp);
        }
    }

    return result;
}
