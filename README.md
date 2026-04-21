# HaLow Bridge (Seeed XIAO ESP32-S3 / C6)

A **HaLow-to-Wi‑Fi bridge** for Seeed Studio XIAO microcontrollers paired with the Morse Micro Wio-WM6108 HaLow module. The device connects to a HaLow (802.11ah) network as a station and runs a 2.4 GHz Wi‑Fi AP with NAT, allowing standard Wi-Fi devices to access the HaLow network and the internet.

This project is based on the **Morse Micro MM-IoT-SDK v2.10.4**, which provides official support for **RISC-V** architectures like the **ESP32-C6**.

## Features

- **Multi-Target Support** — Fully compatible with **XIAO ESP32-S3** (Xtensa) and **XIAO ESP32-C6** (RISC-V).
- **HaLow STA** — Connects to a HaLow AP (900 MHz) using credentials from the web UI.
- **2.4 GHz AP** — Creates a Wi‑Fi AP; clients get DHCP and are NAT'd over the HaLow link.
- **Web configuration** — Open **http://192.168.4.1** in your browser to set:
  - HaLow & AP SSID/Passphrases  
  - Country code  
  - Language (English, Spanish, 中文)
- **RSSI** — Real-time signal strength monitoring via web and serial console.
- **LED status** — Adaptive pinout:
  - **XIAO C6:** GPIO 15
  - **XIAO S3:** GPIO 21
  - (1 blink = connected, 3 blinks = disconnected)
- **DNS forwarding** — Integrated UDP 53 forwarder for client internet access.

## Hardware

- **Seeed Studio XIAO ESP32-C6** (Recommended) or **ESP32-S3**
- **Seeed Studio Wio-WM6108** (Morse Micro MM6108) Wi-Fi HaLow Module

### SPI Pinout (XIAO C6)
| Signal | Pin |
|--------|-----|
| SCK    | 19  |
| MOSI   | 18  |
| MISO   | 20  |
| CS     | 21  |
| IRQ    | 2   |
| RESET  | 1   |

## Prerequisites

- [ESP-IDF](https://docs.espressif.com/projects/esp-idf/en/latest/esp32c6/) v5.4+
- Environment variable **`MMIOT_ROOT`** set to this repository root.

## Building

```bash
cd examples/iperf
export MMIOT_ROOT=$(pwd)/../..
idf.py set-target esp32c6   # Or esp32s3
idf.py build
```

## Flashing

You can flash using `idf.py` or use the pre-compiled binaries in the `release_binaries/` folder.

```bash
idf.py -p /dev/ttyACM0 flash monitor
```

## Release Binaries

Pre-compiled production binaries for the **XIAO ESP32-C6** are available in:
`release_binaries/esp32c6/`

To flash these manually:
```bash
python -m esptool --chip esp32c6 write_flash 0x0 bootloader.bin 0x8000 partition-table.bin 0x10000 iperf.bin
```

## License

Apache License 2.0. See [examples/iperf/LICENSE](examples/iperf/LICENSE).

**Updated by [HRTCELL](https://github.com/HRTCELL) for ESP32-C6 Support**  
**Original Built by [Gavin Greenwood](https://github.com/gtgreenw)**
