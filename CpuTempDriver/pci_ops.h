#pragma once
#include <ntddk.h>
#include "shared_ioctl.h"

// Reads from PCI configuration space
NTSTATUS PciRead(PPCI_READ_REQUEST input, PPCI_READ_RESPONSE output);

// Writes to PCI configuration space
NTSTATUS PciWrite(PPCI_WRITE_REQUEST input);
