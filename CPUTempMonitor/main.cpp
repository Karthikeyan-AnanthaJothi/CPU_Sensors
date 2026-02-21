#include <Windows.h>
#include <stdio.h>
#include <string>
#include <vector>
#include "driver_loader.h"
#include "ring0_interface.h"
#include "cpuid_info.h"
#include "intel_temp.h"
#include "amd_temp.h"

// ANSI color codes for console output
#define COL_RESET   "\033[0m"
#define COL_CYAN    "\033[96m"
#define COL_GREEN   "\033[92m"
#define COL_YELLOW  "\033[93m"
#define COL_RED     "\033[91m"
#define COL_BOLD    "\033[1m"

static bool g_running = true;

BOOL WINAPI CtrlHandler(DWORD ctrlType) {
    if (ctrlType == CTRL_C_EVENT || ctrlType == CTRL_BREAK_EVENT) {
        g_running = false;
        return TRUE;
    }
    return FALSE;
}

static const char* TempColor(float temp) {
    if (temp < 50.0f) return COL_GREEN;
    if (temp < 75.0f) return COL_YELLOW;
    return COL_RED;
}

static void PrintIntelResults(const IntelTempResult& r, const CpuInfo& cpu)
{
    printf(COL_BOLD COL_CYAN "\n  Intel CPU Temperatures\n" COL_RESET);
    printf("  TjMax: %.0f°C\n", r.tjMax);
    printf("  Package: %s%.1f°C" COL_RESET "\n", TempColor(r.packageTemp), r.packageTemp);
    printf("  Cores:\n");
    for (size_t i = 0; i < r.coreTemps.size(); i++) {
        printf("    Core %2zu: %s%.1f°C" COL_RESET "\n",
               i, TempColor(r.coreTemps[i]), r.coreTemps[i]);
    }
}

static void PrintAmdResults(const AmdTempResult& r, const CpuInfo& cpu)
{
    printf(COL_BOLD COL_CYAN "\n  AMD CPU Temperatures\n" COL_RESET);
    printf("  Tctl (sensor):  %s%.1f°C" COL_RESET "  (raw sensor, may include AMD offset)\n",
           TempColor(r.tctl), r.tctl);
    if (r.tdie != r.tctl)
        printf("  Tdie (junction):%s%.1f°C" COL_RESET "  (true die temperature)\n",
               TempColor(r.tdie), r.tdie);
    if (!r.ccdTemps.empty()) {
        printf("  CCD Temps (most accurate):\n");
        float maxCcd = 0.0f;
        for (size_t i = 0; i < r.ccdTemps.size(); i++) {
            printf("    CCD %zu: %s%.1f°C" COL_RESET "\n",
                   i, TempColor(r.ccdTemps[i]), r.ccdTemps[i]);
            if (r.ccdTemps[i] > maxCcd) maxCcd = r.ccdTemps[i];
        }
        printf("  CCD Max: %s%.1f°C" COL_RESET "\n", TempColor(maxCcd), maxCcd);
    }
}

static void PrintUsage(const char* exe) {
    printf("Usage: %s [--loop] [--interval <seconds>]\n", exe);
    printf("  --loop              Continuously update temperatures\n");
    printf("  --interval <N>      Update interval in seconds (default: 1)\n");
}

int main(int argc, char* argv[])
{
    // Enable ANSI colors in Windows console
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD consoleMode = 0;
    GetConsoleMode(hConsole, &consoleMode);
    SetConsoleMode(hConsole, consoleMode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);

    // Parse arguments
    bool loopMode = false;
    int intervalSec = 1;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--loop") == 0) loopMode = true;
        if (strcmp(argv[i], "--interval") == 0 && i + 1 < argc) {
            intervalSec = atoi(argv[++i]);
            if (intervalSec < 1) intervalSec = 1;
        }
        if (strcmp(argv[i], "--help") == 0) { PrintUsage(argv[0]); return 0; }
    }

    // Check admin privileges
    BOOL isAdmin = FALSE;
    HANDLE hToken = nullptr;
    if (OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken)) {
        TOKEN_ELEVATION elevation = {};
        DWORD size = sizeof(elevation);
        if (GetTokenInformation(hToken, TokenElevation, &elevation, size, &size))
            isAdmin = elevation.TokenIsElevated;
        CloseHandle(hToken);
    }
    if (!isAdmin) {
        fprintf(stderr, "[ERROR] This program requires Administrator privileges.\n");
        fprintf(stderr, "        Right-click and select 'Run as administrator'.\n");
        return 1;
    }

    // Find driver path (same directory as the .exe)
    wchar_t exePath[MAX_PATH] = {};
    GetModuleFileNameW(nullptr, exePath, MAX_PATH);
    // Replace exe filename with driver filename
    wchar_t* lastSlash = wcsrchr(exePath, L'\\');
    if (lastSlash) *(lastSlash + 1) = L'\0';
    wchar_t driverPath[MAX_PATH] = {};
    swprintf_s(driverPath, L"%sCpuTempDriver.sys", exePath);

    printf(COL_BOLD "CPU Temperature Monitor\n" COL_RESET);
    printf("Driver path: ");
    wprintf(L"%s\n", driverPath);

    // Load the driver
    printf("Loading driver...\n");
    if (!DriverLoad(driverPath)) {
        fprintf(stderr, "[ERROR] Failed to load driver. Ensure CpuTempDriver.sys is in the same folder.\n");
        return 1;
    }
    printf("Driver loaded.\n");

    // Open device handle
    HANDLE hDriver = DriverOpen();
    if (hDriver == INVALID_HANDLE_VALUE) {
        fprintf(stderr, "[ERROR] Failed to open driver device: %lu\n", GetLastError());
        DriverUnload();
        return 1;
    }

    // Detect CPU
    CpuInfo cpu = DetectCpu();
    printf("\nCPU: %s\n", cpu.brandString.c_str());
    printf("Family: 0x%X  Model: 0x%X  Cores: %u physical / %u logical\n",
           cpu.family, cpu.model, cpu.physicalCores, cpu.logicalCores);

    SetConsoleCtrlHandler(CtrlHandler, TRUE);

    if (loopMode) {
        printf("\nMonitoring... Press Ctrl+C to stop.\n");
    }

    do {
        if (loopMode) {
            system("cls");
            printf(COL_BOLD "CPU Temperature Monitor" COL_RESET " (Ctrl+C to stop)\n");
            printf("CPU: %s\n", cpu.brandString.c_str());
        }

        if (cpu.vendor == CpuVendor::Intel) {
            IntelTempResult r = ReadIntelTemperatures(hDriver, cpu);
            if (r.valid) PrintIntelResults(r, cpu);
            else printf("[WARN] Intel temperature read failed.\n");
        }
        else if (cpu.vendor == CpuVendor::AMD) {
            AmdTempResult r = ReadAmdTemperatures(hDriver, cpu);
            if (r.valid) PrintAmdResults(r, cpu);
            else printf("[WARN] AMD temperature read failed.\n");
        }
        else {
            printf("[WARN] Unknown CPU vendor. Cannot read temperatures.\n");
        }

        if (loopMode && g_running) {
            Sleep(intervalSec * 1000);
        }

    } while (loopMode && g_running);

    // Cleanup
    CloseHandle(hDriver);
    DriverUnload();
    printf("\nDriver unloaded. Goodbye.\n");

    return 0;
}
