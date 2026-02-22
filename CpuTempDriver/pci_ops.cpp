#include "pci_ops.h"

NTSTATUS PciRead(PPCI_READ_REQUEST input, PPCI_READ_RESPONSE output)
{
    if (!input || !output)
        return STATUS_INVALID_PARAMETER;

    PCI_SLOT_NUMBER slot = {0};
    slot.u.bits.DeviceNumber = input->Device;
    slot.u.bits.FunctionNumber = input->Function;

    // Read 4 bytes (DWORD) from config space
    ULONG bytesRead = HalGetBusDataByOffset(
        PCIConfiguration,
        input->Bus,
        slot.u.AsULONG,
        &output->Value,
        input->Offset,
        sizeof(ULONG)
    );

    if (bytesRead != sizeof(ULONG))
    {
        return STATUS_UNSUCCESSFUL;
    }

    return STATUS_SUCCESS;
}

NTSTATUS PciWrite(PPCI_WRITE_REQUEST input)
{
    if (!input)
        return STATUS_INVALID_PARAMETER;

    PCI_SLOT_NUMBER slot = {0};
    slot.u.bits.DeviceNumber = input->Device;
    slot.u.bits.FunctionNumber = input->Function;

    // Write 4 bytes (DWORD) to config space
    ULONG bytesWritten = HalSetBusDataByOffset(
        PCIConfiguration,
        input->Bus,
        slot.u.AsULONG,
        &input->Value,
        input->Offset,
        sizeof(ULONG)
    );

    if (bytesWritten != sizeof(ULONG))
    {
        return STATUS_UNSUCCESSFUL;
    }

    return STATUS_SUCCESS;
}
