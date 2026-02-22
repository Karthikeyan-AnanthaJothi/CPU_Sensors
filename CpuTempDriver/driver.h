#pragma once
#include <ntddk.h>
#include "shared_ioctl.h"
#include "msr_ops.h"
#include "pci_ops.h"

// Driver Entry Point
extern "C" DRIVER_INITIALIZE DriverEntry;

// Unload Routine
void DriverUnload(_In_ PDRIVER_OBJECT DriverObject);

// Dispatch Routines
_Dispatch_type_(IRP_MJ_CREATE)
_Dispatch_type_(IRP_MJ_CLOSE)
NTSTATUS DispatchCreateClose(_In_ PDEVICE_OBJECT DeviceObject, _In_ PIRP Irp);

_Dispatch_type_(IRP_MJ_DEVICE_CONTROL)
NTSTATUS DispatchDeviceControl(_In_ PDEVICE_OBJECT DeviceObject, _In_ PIRP Irp);
