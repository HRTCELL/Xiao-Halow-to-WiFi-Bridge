# HaLow Bridge (Xiao ESP32-S3) - Project Context

This project is a **HaLow-to-Wi-Fi bridge** designed for the Seeed Studio XIAO ESP32-S3 equipped with a Morse Micro HaLow module. It allows 2.4 GHz Wi-Fi devices (phones, laptops) to access a HaLow (802.11ah) network and the internet through NAT.

## Project Overview

- **Core Functionality:** Bridges a 900 MHz HaLow station connection to a local 2.4 GHz Wi-Fi Access Point.
- **NAT Routing:** Implements NAPT (Network Address Port Translation) to route traffic between the Wi-Fi AP clients and the HaLow network.
- **Configuration:** Includes a web-based UI (accessible at `http://192.168.4.1`) for managing HaLow and Wi-Fi credentials, country codes, and system settings.
- **Hardware:** Specifically targets the Seeed Studio XIAO ESP32-S3 and Morse Micro MM6108 HaLow silicon.

## Technologies & Frameworks

- **ESP-IDF v5.x:** The primary development framework for the ESP32-S3.
- **Morse Micro MM-IoT-SDK:** Integrated as a framework within the project to handle HaLow radio operations.
- **LwIP:** Used for networking, specifically with NAPT enabled for routing.
- **NVS (Non-Volatile Storage):** Used for persisting user configuration.

## Architecture & Key Components

### Application Logic (`examples/iperf/main/src/`)

| File | Description |
| :--- | :--- |
| `iperf.c` | Application entry point (`app_main`), initializes HaLow and starts the AP. |
| `nat_router.c` | Manages the 2.4 GHz Wi-Fi AP, sets up NAPT, and handles default routing. |
| `web_config.c` | HTTP server and web UI. Supports English, Spanish, and Chinese. Handles form parsing and NVS updates. |
| `dns_forwarder.c` | UDP 53 forwarder with a 16-entry response cache (120s TTL) to reduce HaLow link latency. |
| `settings.c` | Handles loading and saving configuration parameters to NVS. |
| `mm_app_common.c` | Common HaLow initialization, link management, and LED/RSSI status logic. |

### Technical Details

- **DNS Forwarding:** Listens on UDP 53 on all interfaces. Queries from local AP clients (192.168.4.x) are forwarded to the HaLow gateway (default 10.41.0.1). A small cache is implemented to improve performance over the sub-GHz link.
- **NAPT:** Enabled via `CONFIG_LWIP_IPV4_NAPT`. Routing is configured to prioritize the HaLow interface for upstream traffic while maintaining the local 2.4 GHz AP subnet.
- **Multi-language UI:** The web interface provides localized strings for EN, ES, and ZH, stored as static structures in `web_config.c`.
- **NVS Settings:** SSID, passwords, country codes, and buffer size presets are stored in the "storage" NVS partition.

### Framework (`framework/`)

- `mm_shims/`: ESP-IDF specific hardware abstraction layer (HAL) and OS abstraction layer (OSAL) for Morse Micro.
- `morselib/`: Contains the core Morse Micro HaLow binary libraries.
- `morsefirmware/`: Firmware binaries for the Morse Micro MM6108 chip.
- `mmipal/`, `mmiperf/`, `mmutils/`: Utility components for IP abstraction, performance testing, and general helpers.

## Building and Running

### Prerequisites

1.  **ESP-IDF v5.x** installed and sourced in your terminal.
2.  **`MMIOT_ROOT`** environment variable must be set to the root of this repository.

### Commands

- **Set Target:** `idf.py set-target esp32s3` (run from `examples/iperf`)
- **Build:** `idf.py build`
- **Flash & Monitor:** `idf.py -p <PORT> flash monitor`
- **Full Clean:** `idf.py fullclean`

### TCP Buffer Size Configuration

The TCP buffer size is selected at build time by choosing one of the `sdkconfig.defaults` variants in `examples/iperf/`:
- `sdkconfig.defaults` (Large, 50 KB - Default)
- `sdkconfig.defaults.medium` (32 KB)
- `sdkconfig.defaults.small` (16 KB)

To change: `cp sdkconfig.defaults.small sdkconfig.defaults && idf.py build`.

## Development Conventions

- **Licensing:** Bridge application code is under Apache-2.0. Framework components may have different licenses (see `LICENSES/`).
- **Configuration:** Use the Web UI for runtime changes. Build-time changes should be made via `sdkconfig` or by selecting different default files.
- **Status Indication:** User LED (GPIO 21) indicates connection status (1 blink = connected, 3 blinks = disconnected).
- **Auto-reconnect:** The system automatically attempts to reconnect to HaLow every 15 seconds if the link is lost.
