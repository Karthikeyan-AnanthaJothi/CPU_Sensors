🌡️ CPU Temperature Monitor
A self-contained, dependency-free CPU temperature monitor for Windows that works on both Intel and AMD processors. No WinRing0, no LibreHardwareMonitor, no external services — just two files.

Platform Language License Architecture

✨ Features
✅ Intel CPUs — Per-core & package temperatures via MSR (0x19C, 0x1B1)
✅ AMD Zen CPUs — Tctl/Tdie & per-CCD temperatures via SMN bus
✅ Zero dependencies — Ships as just 2 files: .exe + .sys
✅ Continuous monitoring mode with configurable interval
✅ Color-coded output (green/yellow/red based on temperature)
✅ Self-loading driver — no manual driver installation needed
✅ Supports Zen, Zen+, Zen2, Zen3, Zen4, Zen5 and Intel 6th–14th gen
🖥️ Sample Output
CPU Temperature Monitor  (Ctrl+C to stop)
CPU: AMD Ryzen 5 5600X 6-Core Processor
Family: 0x19  Model: 0x21  Cores: 6 physical / 12 logical

  AMD CPU Temperatures
  Tctl (sensor):   49.0°C  (raw sensor, may include AMD offset)
  CCD Temps (most accurate):
    CCD 0:  48.8°C
  CCD Max:  48.8°C
🚀 Usage
Requirements
Windows 10 / 11 — x64 only
Administrator privileges
Test signing enabled (for development builds — see below)
Quick Start
Step 1 — Enable test signing (one-time, requires reboot):

# Run as Administrator
bcdedit /set testsigning on
Reboot your PC. A "Test Mode" watermark will appear in the bottom-right corner.

Step 2 — Run (as Administrator):

cd path\to\Deploy
CPUTempMonitor.exe
Step 3 — Continuous monitoring:

CPUTempMonitor.exe --loop --interval 2
CLI Options
Flag	Description
--loop	Continuously refresh temperatures
--interval <N>	Refresh every N seconds (default: 1)
--help	Show usage
🏗️ Building from Source
Prerequisites
Visual Studio 2022+ with C++ Desktop Development workload
Windows Driver Kit (WDK) matching your VS version
Spectre-mitigated libraries (from VS Installer → Individual Components → search "Spectre")
Build Steps
1. Open CPU_Sensors.sln in Visual Studio
2. Select Debug | x64 or Release | x64
3. Build > Build Solution  (Ctrl+Shift+B)
4. Output:  x64\Debug\CPUTempMonitor.exe
           x64\Debug\CpuTempDriver\CpuTempDriver.sys
Note: Copy CpuTempDriver.sys into the same folder as CPUTempMonitor.exe before running.

🔧 Architecture
CPU_Sensors/
├── CpuTempDriver/          # Kernel-mode WDM driver (Ring 0)
│   ├── driver.cpp          # DriverEntry, IRP dispatch, device setup
│   ├── msr_ops.cpp         # RDMSR instruction via __readmsr intrinsic
│   ├── pci_ops.cpp         # PCI config space via HalGetBusDataByOffset
│   └── shared_ioctl.h      # IOCTL codes & request/response structs
│
└── CPUTempMonitor/         # User-mode console app (Ring 3)
    ├── main.cpp            # Entry point, CLI, display loop
    ├── driver_loader.cpp   # SCM API: load/unload driver at runtime
    ├── ring0_interface.cpp # DeviceIoControl wrappers
    ├── cpuid_info.cpp      # CPU detection via CPUID instruction
    ├── intel_temp.cpp      # Intel MSR temperature algorithm
    └── amd_temp.cpp        # AMD SMN temperature algorithm
How It Works
User App  →  DeviceIoControl(IOCTL)  →  Kernel Driver
                                              ↓
                                    Intel: __readmsr(0x19C)
                                    AMD:   HalGetBusDataByOffset → SMN 0x59800
                                              ↓
                                    Temperature in °C  →  Console
Component	API / Mechanism
Driver loading	CreateService + StartService (advapi32)
Intel temps	RDMSR instruction — MSRs 0x19C, 0x1B1, 0x1A2
AMD Tctl/Tdie	SMN register 0x00059800 via PCI config space
AMD CCD temps	SMN registers 0x00059954+ (Zen2/3), 0x00059B08+ (Zen4/5)
CPU detection	__cpuid intrinsic — vendor, family, model, core count
Thread affinity	KeSetSystemAffinityThreadEx — per-core MSR reads
🖥️ Supported CPUs
Intel
Generation	Codename	Family
6th–14th gen	Skylake → Raptor Lake	0x06
Reads: TjMax (MSR 0x1A2), per-core delta (MSR 0x19C), package (MSR 0x1B1)

AMD
Series	Codename	Family	Model
Ryzen 1000	Summit Ridge	0x17	0x01
Ryzen 2000	Pinnacle Ridge	0x17	0x08
Ryzen 3000	Matisse	0x17	0x71
Ryzen 5000	Vermeer	0x19	0x21
Ryzen 7000	Raphael	0x19	0x61
Ryzen 9000	Granite Ridge	0x1A	—
🔒 Security
No WinRing0 — avoids CVE-2020-14979
Driver only exposes MSR read and PCI config read/write — no arbitrary memory access
Service is created on-demand and deleted after use — no persistent background service
For production use, sign the driver via Microsoft Attestation Signing
📋 For Production Distribution
Test signing is for development only. To distribute publicly:

Go to Microsoft Hardware Dev Center
Submit the driver for Attestation Signing (free, no WHQL testing required)
Users won't need test signing mode enabled
📄 License
MIT License — see LICENSE for details.

🙏 Acknowledgements
Temperature algorithms inspired by LibreHardwareMonitor
AMD SMN register map from AMD's BIOS and Kernel Developer's Guide (BKDG)
Intel MSR definitions from Intel's Software Developer Manual (SDM)