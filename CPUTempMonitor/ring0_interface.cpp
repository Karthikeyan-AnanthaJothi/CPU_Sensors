#include "ring0_interface.h"
#include "..\CpuTempDriver\shared_ioctl.h"
#include <Windows.h>

bool Ring0ReadMsr(HANDLE hDriver, uint32_t msrIndex, uint32_t coreIndex,
                  uint32_t& outEax, uint32_t& outEdx)
{
    MSR_REQUEST req = {};
    req.MsrIndex    = msrIndex;
    req.CoreAffinity = coreIndex;

    MSR_RESPONSE resp = {};
    DWORD bytesReturned = 0;

    BOOL ok = DeviceIoControl(
        hDriver,
        IOCTL_READ_MSR,
        &req, sizeof(req),
        &resp, sizeof(resp),
        &bytesReturned,
        nullptr
    );

    if (ok && bytesReturned == sizeof(resp)) {
        outEax = resp.Eax;
        outEdx = resp.Edx;
        return true;
    }
    return false;
}

bool Ring0ReadPci(HANDLE hDriver, uint32_t bus, uint32_t device, uint32_t func,
                  uint32_t offset, uint32_t& outValue)
{
    PCI_READ_REQUEST req = {};
    req.Bus      = bus;
    req.Device   = device;
    req.Function = func;
    req.Offset   = offset;

    PCI_READ_RESPONSE resp = {};
    DWORD bytesReturned = 0;

    BOOL ok = DeviceIoControl(
        hDriver,
        IOCTL_READ_PCI_CONFIG,
        &req, sizeof(req),
        &resp, sizeof(resp),
        &bytesReturned,
        nullptr
    );

    if (ok && bytesReturned == sizeof(resp)) {
        outValue = resp.Value;
        return true;
    }
    return false;
}

bool Ring0WritePci(HANDLE hDriver, uint32_t bus, uint32_t device, uint32_t func,
                   uint32_t offset, uint32_t value)
{
    PCI_WRITE_REQUEST req = {};
    req.Bus      = bus;
    req.Device   = device;
    req.Function = func;
    req.Offset   = offset;
    req.Value    = value;

    DWORD bytesReturned = 0;

    BOOL ok = DeviceIoControl(
        hDriver,
        IOCTL_WRITE_PCI_CONFIG,
        &req, sizeof(req),
        nullptr, 0,
        &bytesReturned,
        nullptr
    );

    return ok != FALSE;
}
