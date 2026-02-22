#include "driver.h"
#include "msr_ops.h" // MsrRead declared here
#include "pci_ops.h" // PciRead, PciWrite declared here

// Driver Entry Point
extern "C" NTSTATUS DriverEntry(_In_ PDRIVER_OBJECT DriverObject, _In_ PUNICODE_STRING RegisterPath)
{
    UNREFERENCED_PARAMETER(RegisterPath);

    NTSTATUS status;
    PDEVICE_OBJECT deviceObject = NULL;
    UNICODE_STRING deviceName;
    UNICODE_STRING symLinkName;

    RtlInitUnicodeString(&deviceName, DEVICE_NAME);
    RtlInitUnicodeString(&symLinkName, DEVICE_SYMLINK);

    // Create the device object
    status = IoCreateDevice(
        DriverObject,
        0,
        &deviceName,
        FILE_DEVICE_UNKNOWN,
        FILE_DEVICE_SECURE_OPEN,
        FALSE,
        &deviceObject
    );

    if (!NT_SUCCESS(status))
    {
        return status;
    }

    // Create the symbolic link so user mode can access it
    status = IoCreateSymbolicLink(&symLinkName, &deviceName);

    if (!NT_SUCCESS(status))
    {
        IoDeleteDevice(deviceObject);
        return status;
    }

    // Set dispatch routines
    DriverObject->MajorFunction[IRP_MJ_CREATE] = DispatchCreateClose;
    DriverObject->MajorFunction[IRP_MJ_CLOSE] = DispatchCreateClose;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = DispatchDeviceControl;
    DriverObject->DriverUnload = DriverUnload;

    return STATUS_SUCCESS;
}

// Unload Routine
void DriverUnload(_In_ PDRIVER_OBJECT DriverObject)
{
    UNICODE_STRING symLinkName;
    RtlInitUnicodeString(&symLinkName, DEVICE_SYMLINK);

    IoDeleteSymbolicLink(&symLinkName);
    IoDeleteDevice(DriverObject->DeviceObject);
}

// Create/Close Dispatch Routine
NTSTATUS DispatchCreateClose(_In_ PDEVICE_OBJECT DeviceObject, _In_ PIRP Irp)
{
    UNREFERENCED_PARAMETER(DeviceObject);

    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return STATUS_SUCCESS;
}

// Device Control Dispatch Routine
NTSTATUS DispatchDeviceControl(_In_ PDEVICE_OBJECT DeviceObject, _In_ PIRP Irp)
{
    UNREFERENCED_PARAMETER(DeviceObject);

    PIO_STACK_LOCATION stack = IoGetCurrentIrpStackLocation(Irp);
    ULONG controlCode = stack->Parameters.DeviceIoControl.IoControlCode;
    ULONG inputLen = stack->Parameters.DeviceIoControl.InputBufferLength;
    ULONG outputLen = stack->Parameters.DeviceIoControl.OutputBufferLength;
    PVOID buffer = Irp->AssociatedIrp.SystemBuffer;

    NTSTATUS status = STATUS_SUCCESS;
    ULONG bytesReturned = 0;

    switch (controlCode)
    {
        case IOCTL_READ_MSR:
        {
            if (inputLen < sizeof(MSR_REQUEST) || outputLen < sizeof(MSR_RESPONSE))
            {
                status = STATUS_BUFFER_TOO_SMALL;
                break;
            }

            PMSR_REQUEST req = (PMSR_REQUEST)buffer;
            PMSR_RESPONSE resp = (PMSR_RESPONSE)buffer;
            
            // Call our helper function
            status = MsrRead(req, resp);
            if (NT_SUCCESS(status))
            {
                bytesReturned = sizeof(MSR_RESPONSE);
            }
            break;
        }

        case IOCTL_READ_PCI_CONFIG:
        {
            if (inputLen < sizeof(PCI_READ_REQUEST) || outputLen < sizeof(PCI_READ_RESPONSE))
            {
                status = STATUS_BUFFER_TOO_SMALL;
                break;
            }

            PPCI_READ_REQUEST req = (PPCI_READ_REQUEST)buffer;
            PPCI_READ_RESPONSE resp = (PPCI_READ_RESPONSE)buffer;

            // Call our helper function
            status = PciRead(req, resp);
            if (NT_SUCCESS(status))
            {
                bytesReturned = sizeof(PCI_READ_RESPONSE);
            }
            break;
        }

        case IOCTL_WRITE_PCI_CONFIG:
        {
            if (inputLen < sizeof(PCI_WRITE_REQUEST))
            {
                status = STATUS_BUFFER_TOO_SMALL;
                break;
            }

            PPCI_WRITE_REQUEST req = (PPCI_WRITE_REQUEST)buffer;

            // Call our helper function
            status = PciWrite(req);
            // No bytes returned for write
            break;
        }

        default:
            status = STATUS_INVALID_DEVICE_REQUEST;
            break;
    }

    Irp->IoStatus.Status = status;
    Irp->IoStatus.Information = bytesReturned;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return status;
}
