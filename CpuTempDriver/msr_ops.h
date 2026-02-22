#pragma once
#include <ntddk.h>
#include "shared_ioctl.h"

// Reads an MSR from a specific logical processor
NTSTATUS MsrRead(PMSR_REQUEST input, PMSR_RESPONSE output);
