#include "driver_loader.h"
#include <string>
#include <stdio.h>

static const wchar_t* SERVICE_NAME = L"CpuTempDriver";

bool DriverLoad(const wchar_t* driverPath)
{
    SC_HANDLE hScm = OpenSCManager(nullptr, nullptr, SC_MANAGER_ALL_ACCESS);
    if (!hScm) {
        wprintf(L"[ERROR] OpenSCManager failed: %lu\n", GetLastError());
        return false;
    }

    // Try to create the service
    SC_HANDLE hSvc = CreateServiceW(
        hScm,
        SERVICE_NAME,
        SERVICE_NAME,
        SERVICE_ALL_ACCESS,
        SERVICE_KERNEL_DRIVER,
        SERVICE_DEMAND_START,
        SERVICE_ERROR_NORMAL,
        driverPath,
        nullptr, nullptr, nullptr, nullptr, nullptr
    );

    if (!hSvc) {
        DWORD err = GetLastError();
        if (err == ERROR_SERVICE_EXISTS) {
            // Service already exists — just open it
            hSvc = OpenServiceW(hScm, SERVICE_NAME, SERVICE_ALL_ACCESS);
        }
        if (!hSvc) {
            wprintf(L"[ERROR] CreateService/OpenService failed: %lu\n", err);
            CloseServiceHandle(hScm);
            return false;
        }
    }

    // Start the service (load the driver)
    if (!StartServiceW(hSvc, 0, nullptr)) {
        DWORD err = GetLastError();
        if (err != ERROR_SERVICE_ALREADY_RUNNING) {
            wprintf(L"[ERROR] StartService failed: %lu\n", err);
            CloseServiceHandle(hSvc);
            CloseServiceHandle(hScm);
            return false;
        }
    }

    CloseServiceHandle(hSvc);
    CloseServiceHandle(hScm);
    return true;
}

void DriverUnload()
{
    SC_HANDLE hScm = OpenSCManager(nullptr, nullptr, SC_MANAGER_ALL_ACCESS);
    if (!hScm) return;

    SC_HANDLE hSvc = OpenServiceW(hScm, SERVICE_NAME, SERVICE_ALL_ACCESS);
    if (hSvc) {
        SERVICE_STATUS status = {};
        ControlService(hSvc, SERVICE_CONTROL_STOP, &status);
        DeleteService(hSvc);
        CloseServiceHandle(hSvc);
    }

    CloseServiceHandle(hScm);
}

HANDLE DriverOpen()
{
    HANDLE h = CreateFileW(
        L"\\\\.\\CpuTempDriver",
        GENERIC_READ | GENERIC_WRITE,
        0,
        nullptr,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        nullptr
    );
    return h;
}
