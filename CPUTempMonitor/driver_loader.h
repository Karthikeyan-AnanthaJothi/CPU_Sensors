#pragma once
#include <Windows.h>

// Loads CpuTempDriver.sys from the given path and starts it.
// Returns true on success. Requires Administrator privileges.
bool DriverLoad(const wchar_t* driverPath);

// Stops and removes the CpuTempDriver service.
void DriverUnload();

// Opens a handle to the driver device.
// Returns INVALID_HANDLE_VALUE on failure.
HANDLE DriverOpen();
