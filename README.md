# HaLow Bridge (Xiao ESP32-S3)

A **HaLow-to-Wi‑Fi bridge** for the Seeed Studio XIAO ESP32-S3 with Morse Micro HaLow. The device connects to a HaLow (802.11ah) access point as a station, then runs a 2.4 GHz Wi‑Fi AP with NAT so phones and laptops can reach the HaLow network and the internet through it.

Based on the [Morse Micro MM-IoT-SDK](https://github.com/MorseMicro/MM-IoT-SDK) iperf example, extended with web configuration, NAPT, DNS forwarding, and multi-language support.

## Features

- **HaLow STA** — Connects to a HaLow AP (e.g. 900 MHz) using SSID and passphrase from the web UI or NVS.
- **2.4 GHz AP** — Creates a Wi‑Fi AP; clients get DHCP and are NAT'd over the HaLow link.
- **Web configuration** — Connect to the device's AP and open **http://192.168.4.1** to set:
  - HaLow SSID and passphrase  
  - 2.4 GHz AP SSID and password  
  - Country code  
  - TCP buffer size (Small / Medium / Large)  
  - Language (English, Spanish, 中文)
- **RSSI** — HaLow RSSI shown on the web page (refreshes every 15 s) and printed to the serial console every 15 seconds.
- **LED status** — On the XIAO user LED (GPIO 21): **1 blink** every 10 s = connected, **3 blinks** every 10 s = disconnected (saves battery).
- **Auto-reconnect** — If the HaLow link goes down, the device automatically tries to reconnect after 15 seconds.
- **DNS forwarding** — Client DNS (e.g. to 192.168.4.1) is forwarded so internet access works through the bridge.

## Hardware

- **Seeed Studio XIAO ESP32-S3** (or compatible ESP32-S3 board)
- **Morse Micro HaLow** module (e.g. MM6108-EVB or integrated HaLow board)
- This example is part of the MM-IoT-SDK and expects the framework's HaLow/SPI integration.

## Prerequisites

- [ESP-IDF](https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/) v5.x (or version required by the MM-IoT-SDK)
- Environment variable **`MMIOT_ROOT`** set to the path of this repo (the directory that contains `framework/`, `examples/`, etc.)

## Building

From this repo root:

```bash
cd examples/iperf
export MMIOT_ROOT=/path/to/mm-iot-esp32   # path to this repo, if not already set
idf.py set-target esp32s3
idf.py build
```

## Flashing and monitor

```bash
idf.py -p /dev/your_port flash monitor
```

Replace `/dev/your_port` with the correct serial port (e.g. `COM3` on Windows).

## Usage

1. **Power the device** — It will connect to the configured HaLow AP (or use defaults: SSID `Halow1`, passphrase `letmein111`).
2. **Connect to its Wi‑Fi** — From a phone or laptop, join the 2.4 GHz AP (default SSID `XIAO_S3_HALOW`, password `letmein111`).
3. **Open the web UI** — In a browser go to **http://192.168.4.1**.
4. **Change settings** — Edit HaLow/AP credentials, country, buffer size, and language; click **Save**. Reboot the device so Wi‑Fi and buffer changes take effect.
5. **Reboot** — Use the "Reboot device" button on the page when needed.

After saving, the page shows: *"Settings saved. Reboot to apply changes."*

## TCP buffer size (Small / Medium / Large)

Buffer size is chosen at **build time** via `sdkconfig.defaults` in `examples/iperf/`:

| Option   | Config file                  | TCP window | Use when        |
|----------|------------------------------|------------|-----------------|
| Small    | `sdkconfig.defaults.small`   | 16 KB      | Low memory use  |
| Medium   | `sdkconfig.defaults.medium`  | 32 KB      | Balanced        |
| Large    | `sdkconfig.defaults` (default) | 50 KB   | Best throughput |

To use Small or Medium, copy the matching file over `sdkconfig.defaults` in `examples/iperf/`, then build:

```bash
cd examples/iperf
cp sdkconfig.defaults.small sdkconfig.defaults
idf.py build
```

Select the same option in the web UI so the device "preset" matches the build. Buffer size takes effect after reboot.

## LED (GPIO 21)

- **1 blink** every 10 seconds → HaLow **connected**
- **3 blinks** every 10 seconds → HaLow **disconnected** or connecting  

Runs only every 10 s to save battery.

## Project structure (HaLow bridge sources)

All under `examples/iperf/main/src/`:

| File              | Role                                      |
|-------------------|-------------------------------------------|
| `iperf.c`         | App entry, HaLow init, starts AP & iperf |
| `mm_app_common.c` | HaLow init, link/reconnect, LED, RSSI    |
| `nat_router.c`    | 2.4 GHz AP, NAPT, default route, DNS      |
| `dns_forwarder.c` | DNS forwarder for client internet access |
| `settings.c/h`    | NVS load/save for web settings            |
| `web_config.c`    | HTTP server and web UI (EN/ES/ZH)         |

## License

Apache License 2.0. See [examples/iperf/LICENSE](examples/iperf/LICENSE).

**Built by [Gavin Greenwood](https://github.com/gtgreenw)**
