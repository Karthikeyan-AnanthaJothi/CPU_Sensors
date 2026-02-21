#pragma once
#include <Windows.h>
#include <cstdint>

// Reads an MSR from the specified logical core.
// Returns true on success.
bool Ring0ReadMsr(HANDLE hDriver, uint32_t msrIndex, uint32_t coreIndex,
                  uint32_t& outEax, uint32_t& outEdx);

// Reads a DWORD from PCI configuration space.
bool Ring0ReadPci(HANDLE hDriver, uint32_t bus, uint32_t device, uint32_t func,
                  uint32_t offset, uint32_t& outValue);

// Writes a DWORD to PCI configuration space.
bool Ring0WritePci(HANDLE hDriver, uint32_t bus, uint32_t device, uint32_t func,
                   uint32_t offset, uint32_t value);
