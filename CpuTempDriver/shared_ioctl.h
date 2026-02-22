#pragma once

#ifndef CTL_CODE
#include <devioctl.h>
#endif

// Device name
#define DEVICE_NAME     L"\\Device\\CpuTempDriver"
#define DEVICE_SYMLINK  L"\\DosDevices\\CpuTempDriver"

// IOCTL Codes
// We use FILE_DEVICE_UNKNOWN and functions 0x800-0x802
#define IOCTL_READ_MSR \
    CTL_CODE(FILE_DEVICE_UNKNOWN, 0x800, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_READ_PCI_CONFIG \
    CTL_CODE(FILE_DEVICE_UNKNOWN, 0x801, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_WRITE_PCI_CONFIG \
    CTL_CODE(FILE_DEVICE_UNKNOWN, 0x802, METHOD_BUFFERED, FILE_ANY_ACCESS)

#pragma pack(push, 1)

// MSR Request/Response
typedef struct _MSR_REQUEST {
    ULONG  MsrIndex;     // The MSR register to read (e.g. 0x19C)
    ULONG  CoreAffinity; // Target core index (0 to N-1)
} MSR_REQUEST, *PMSR_REQUEST;

typedef struct _MSR_RESPONSE {
    ULONG  Eax;
    ULONG  Edx;
} MSR_RESPONSE, *PMSR_RESPONSE;

// PCI Request/Response
typedef struct _PCI_READ_REQUEST {
    ULONG Bus;
    ULONG Device;
    ULONG Function;
    ULONG Offset;
} PCI_READ_REQUEST, *PPCI_READ_REQUEST;

typedef struct _PCI_READ_RESPONSE {
    ULONG Value;
} PCI_READ_RESPONSE, *PPCI_READ_RESPONSE;

typedef struct _PCI_WRITE_REQUEST {
    ULONG Bus;
    ULONG Device;
    ULONG Function;
    ULONG Offset;
    ULONG Value;
} PCI_WRITE_REQUEST, *PPCI_WRITE_REQUEST;

#pragma pack(pop)
