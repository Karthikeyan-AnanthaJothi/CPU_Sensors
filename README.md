<div align="center">

# 🌡️ CPU Temperature Monitor

**A fully self-contained, dependency-free CPU temperature monitor for Windows.**  
Works on Intel & AMD processors. No WinRing0. No services. Just two files.

[![Platform](https://img.shields.io/badge/Platform-Windows%2010%2F11%20x64-0078D4?style=for-the-badge&logo=windows)](https://www.microsoft.com/windows)
[![Language](https://img.shields.io/badge/Language-C%2B%2B17-00599C?style=for-the-badge&logo=cplusplus)](https://isocpp.org/)
[![License](https://img.shields.io/badge/License-MIT-22c55e?style=for-the-badge)](LICENSE)
[![Kernel](https://img.shields.io/badge/Driver-WDM%20Kernel%20Mode-red?style=for-the-badge)](https://learn.microsoft.com/en-us/windows-hardware/drivers/kernel/)

</div>

---

## 📸 Preview

```
CPU Temperature Monitor  (Ctrl+C to stop)
CPU: AMD Ryzen 5 5600X 6-Core Processor
Family: 0x19  Model: 0x21  Cores: 6 physical / 12 logical

  AMD CPU Temperatures
  Tctl (sensor):   49.0°C  (raw sensor)
  CCD Temps (most accurate):
    CCD 0:  48.8°C
  CCD Max:  48.8°C
```

---

## ✨ Features

| Feature | Details |
|---------|---------|
| 🔵 **Intel Support** | Per-core + package temps via MSR (`0x19C`, `0x1B1`, `0x1A2`) |
| 🔴 **AMD Support** | Tctl/Tdie + per-CCD temps via SMN bus (Zen through Zen5) |
| 📦 **Zero Dependencies** | Ships as 2 files — `.exe` + `.sys` |
| 🔄 **Live Monitoring** | Continuous mode with configurable refresh interval |
| 🎨 **Color Output** | Green / Yellow / Red based on temperature thresholds |
| 🔧 **Self-Loading Driver** | Loads and unloads the kernel driver automatically at runtime |
| 🛡️ **No WinRing0** | Avoids [CVE-2020-14979](https://www.cvedetails.com/cve/CVE-2020-14979/) — implements hardware access directly |

---

## 🚀 Quick Start

### Prerequisites
- Windows 10 / 11 (x64)
- Administrator privileges
- Test signing enabled (one-time setup)

### Step 1 — Enable Test Signing
Open **Command Prompt as Administrator**:
```cmd
bcdedit /set testsigning on
```
Reboot your PC. A small "Test Mode" watermark will appear — this is normal.

### Step 2 — Run
```cmd
CPUTempMonitor.exe
```

### Step 3 — Live Monitoring
```cmd
CPUTempMonitor.exe --loop --interval 2
```

> **Deploy folder:** Copy `CPUTempMonitor.exe` + `CpuTempDriver.sys` to the **same folder**. That's all you need.

---

## ⚙️ CLI Options

| Flag | Default | Description |
|------|---------|-------------|
| `--loop` | off | Continuously refresh temperatures |
| `--interval <N>` | `1` | Refresh every N seconds |
| `--help` | — | Show usage |

---

## 🏗️ Architecture

```
CPU_Sensors/
├── CpuTempDriver/          # Kernel-mode WDM driver (Ring 0)
│   ├── driver.cpp          # DriverEntry, IRP dispatch
│   ├── msr_ops.cpp         # RDMSR via __readmsr intrinsic
│   ├── pci_ops.cpp         # PCI config space via HAL
│   └── shared_ioctl.h      # Shared IOCTL codes & structs
│
└── CPUTempMonitor/         # User-mode console app (Ring 3)
    ├── main.cpp            # Entry point, CLI, display
    ├── driver_loader.cpp   # SCM API: load/unload driver
    ├── ring0_interface.cpp # DeviceIoControl wrappers
    ├── cpuid_info.cpp      # CPU detection via CPUID
    ├── intel_temp.cpp      # Intel MSR temperature logic
    └── amd_temp.cpp        # AMD SMN temperature logic
```

### How It Works

```
User App  ──DeviceIoControl──▶  Kernel Driver (Ring 0)
                                      │
                        ┌─────────────┴─────────────┐
                        │                           │
                   Intel RDMSR                AMD SMN Bus
                  MSR 0x19C/0x1B1          PCI → 0x59800+
                        │                           │
                  Temperature °C             Temperature °C
                        └─────────────┬─────────────┘
                               Console Output
```

| Layer | Key APIs |
|-------|---------|
| Driver loading | `CreateService`, `StartService` (advapi32) |
| Intel temps | `__readmsr` → MSR `0x1A2`, `0x19C`, `0x1B1` |
| AMD Tctl/Tdie | `HalGetBusDataByOffset` → SMN `0x00059800` |
| AMD CCD temps | SMN `0x00059954+` (Zen2/3), `0x00059B08+` (Zen4/5) |
| CPU detection | `__cpuid` intrinsic |

---

## 🖥️ Supported Hardware

### Intel — 6th to 14th Gen (Skylake → Raptor Lake)
Reads TjMax, per-core temperature delta, and package temperature from MSRs.

### AMD Zen Family

| Series | Codename | Family | Model |
|--------|----------|--------|-------|
| Ryzen 1000 | Summit Ridge | `0x17` | `0x01` |
| Ryzen 2000 | Pinnacle Ridge | `0x17` | `0x08` |
| Ryzen 3000 | Matisse | `0x17` | `0x71` |
| Ryzen 5000 | **Vermeer** ✅ *tested* | `0x19` | `0x21` |
| Ryzen 7000 | Raphael | `0x19` | `0x61` |
| Ryzen 9000 | Granite Ridge | `0x1A` | — |

---

## 🔨 Build from Source

### Requirements
- [Visual Studio 2022+](https://visualstudio.microsoft.com/) with **C++ Desktop Development**
- [Windows Driver Kit (WDK)](https://learn.microsoft.com/en-us/windows-hardware/drivers/download-the-wdk) matching your VS version
- **Spectre-mitigated libs** — VS Installer → Individual Components → search "Spectre"

### Build
```
1. Open  CPU_Sensors.sln
2. Select  Release | x64
3. Build > Build Solution  (Ctrl+Shift+B)

Output:
  x64\Release\CPUTempMonitor.exe
  x64\Release\CpuTempDriver\CpuTempDriver.sys
```

Copy both files into the same folder to deploy.

---

## 🔒 Security

- ✅ No WinRing0 — avoids [CVE-2020-14979](https://www.cvedetails.com/cve/CVE-2020-14979/)
- ✅ Driver exposes only MSR read and PCI config read/write — no arbitrary memory access
- ✅ Service is created on-demand and **deleted after each run** — no persistent background process
- ⚠️ Test signing is for **development only** — for distribution, use [Microsoft Attestation Signing](https://learn.microsoft.com/en-us/windows-hardware/drivers/dashboard/code-signing-attestation)

---

## 📄 License

MIT © [Karthikeyan A](https://github.com/Karthikeyan-AnanthaJothi)

---

## 🙏 References

- Temperature algorithms inspired by [LibreHardwareMonitor](https://github.com/LibreHardwareMonitor/LibreHardwareMonitor)
- AMD SMN register map from AMD's BIOS and Kernel Developer's Guide (BKDG)
- Intel MSR definitions from Intel's Software Developer Manual (SDM)