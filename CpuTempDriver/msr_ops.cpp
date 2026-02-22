#include "msr_ops.h"
#include <intrin.h>

NTSTATUS MsrRead(PMSR_REQUEST input, PMSR_RESPONSE output)
{
    if (!input || !output)
        return STATUS_INVALID_PARAMETER;

    // Validate core index (basic check against active processor count)
    ULONG activeProcessors = KeQueryActiveProcessorCount(NULL);
    if (input->CoreAffinity >= activeProcessors)
    {
        return STATUS_INVALID_PARAMETER;
    }

    // Set thread affinity to the target core
    KAFFINITY targetAffinity = (KAFFINITY)1 << input->CoreAffinity;
    KAFFINITY oldAffinity = KeSetSystemAffinityThreadEx(targetAffinity);

    NTSTATUS status = STATUS_SUCCESS;
    __try
    {
        // Read the MSR
        unsigned __int64 msrValue = __readmsr(input->MsrIndex);
        output->Eax = (ULONG)(msrValue & 0xFFFFFFFF);
        output->Edx = (ULONG)(msrValue >> 32);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        status = STATUS_PRIVILEGED_INSTRUCTION; // Or generic error
    }

    // Restore original affinity
    KeRevertToUserAffinityThreadEx(oldAffinity);

    return status;
}
