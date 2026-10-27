#  WiFi Security: AI-Powered RF Sniffer & Anomaly Detector

An edge-to-host Wi-Fi intrusion and anomaly detection system combining an **ESP8266** hardware packet sniffer with an unsupervised **Machine Learning (Isolation Forest)** analytics engine in Python.

The ESP8266 scans all 2.4 GHz 802.11 Wi-Fi channels (1–13) in promiscuous/station monitor mode, streams live RF telemetry (BSSID, RSSI signal strength, and channel number) over serial UART, and the host Python engine learns the ambient RF baseline to detect anomalies such as Evil Twin access points, deauthentication floods, rogue APs, and signal spoofing in real time.

---

##  Architecture & Data Flow

```text
+-------------------+      UART / USB Serial       +-----------------------+
|  ESP8266 Board    |  ==========================> |   Host PC (Python)    |
| (Passive Sniffer) |      (115200 Baud Rate)      |   - Serial Ingestion  |
+-------------------+                              +-----------+-----------+
        |                                                      |
        | [Channels 1-13]                                      v
        v                                          +-----------------------+
(Nearby 802.11 APs,                               |   Scikit-Learn        |
 Beacons & Probes)                                 |   Isolation Forest    |
                                                   +-----------+-----------+
                                                               |
                                            +------------------+------------------+
                                            |                                     |
                                            v                                     v
                                    [Normal Baseline]                  [Security Incident]
                                    Status: Normal RF                  [!!! ALERT !!!] Anomaly
                                                                       Saved to forensics_log.csv
```

---

##  Hardware Wiring & Pin Connections

Depending on your board version, follow the wiring instructions below:

### Option 1: NodeMCU / WeMos D1 Mini (Recommended)
If you are using a development board with an integrated USB port (NodeMCU v2/v3, WeMos D1 Mini):
* **Connection:** Plug the board directly into your PC using a **Micro-USB Data Cable** (ensure the cable supports data transfer, not just power).
* **Onboard Bridge:** The board features an onboard USB-to-UART bridge (CP2102 or CH340).
* **Drivers:** If your computer does not assign a COM port, install the [CH340 Driver](http://www.wch-ic.com/downloads/CH341SER_EXE.html) or [CP210x Driver](https://www.silabs.com/developers/usb-to-uart-bridge-vcp-drivers).

---

### Option 2: Standalone ESP8266 (ESP-01 / ESP-12E) via USB-to-TTL FTDI Adapter

If using an external USB-to-Serial TTL adapter (FTDI232 / CH340 / CP2102):

| ESP8266 Pin | USB-to-TTL Adapter Pin | Notes |
|:---|:---|:---|
| **VCC** | **3.3V** |  **DO NOT USE 5V** - 5V will permanently damage the ESP8266! |
| **GND** | **GND** | Common ground reference |
| **TX** | **RX** | Transmit connects to Receive |
| **RX** | **TX** | Receive connects to Transmit |
| **CH_PD / EN** | **3.3V** | Pull HIGH to enable chip |
| **RST** | **3.3V (Float)** | Reset line (connect through 10k resistor to 3.3V) |
| **GPIO0** | **GND (Flashing) / 3.3V (Run)** | Connect to **GND** while uploading firmware; leave floating or pull HIGH for normal running |
| **GPIO15** | **GND** (ESP-12 only) | Must be tied LOW for normal boot |

> [!CAUTION]
> **Power Notice:** The ESP8266 can pull peak currents of ~250mA during RF calibration and scanning. If your USB-to-TTL adapter cannot supply sufficient 3.3V current, use an external 3.3V regulator (e.g., AMS1117-3.3) with common ground to prevent brownout resets.

---

### Option 3: Optional Hardware Alert Peripherals (Stand-Alone Alarm)

You can attach hardware indicators directly to the ESP8266 for physical alerting:
* **Buzzer / LED Indicator:**
  * Positive (Anode via 220Ω resistor) $\rightarrow$ `GPIO4 (D2)`
  * Negative (Cathode) $\rightarrow$ `GND`
* **0.96" I2C OLED Display (SSD1306):**
  * `SDA` $\rightarrow$ `GPIO4 (D2)`
  * `SCL` $\rightarrow$ `GPIO5 (D1)`
  * `VCC` $\rightarrow$ `3.3V`
  * `GND` $\rightarrow$ `GND`

---

##  Repository Structure

```text
Wifi_Security/
├── firmware/
│   └── esp8266_sniffer/
│       └── esp8266_sniffer.ino   # ESP8266 C++ Wi-Fi scanner & serial broadcaster
├── analyzer.py                   # Python real-time ML anomaly detection engine
├── requirements.txt              # Python library dependencies
├── .gitignore                    # Git ignore file for temp logs & venv
└── README.md                     # Comprehensive documentation & connection guide
```

---

##  Getting Started

### 1. Flash the ESP8266 Firmware
1. Download and install the [Arduino IDE](https://www.arduino.cc/en/software).
2. Open **File > Preferences** and add the following URL to **Additional Boards Manager URLs**:
   ```text
   http://arduino.esp8266.com/stable/package_esp8266com_index.json
   ```
3. Go to **Tools > Board > Boards Manager**, search for `esp8266`, and click **Install**.
4. Open [`firmware/esp8266_sniffer/esp8266_sniffer.ino`](firmware/esp8266_sniffer/esp8266_sniffer.ino).
5. Under **Tools**, select your board (e.g. *NodeMCU 1.0 (ESP-12E Module)*) and your board's **Port** (e.g. `COM7`).
6. Click **Upload** (Arrow icon).

---

### 2. Configure Host Python Environment

1. Clone the repository and navigate into the folder:
   ```bash
   git clone <YOUR_GITHUB_REPO_URL>
   cd Wifi_Security
   ```

2. Create and activate a Python virtual environment:
   ```bash
   # Windows (PowerShell)
   python -m venv .venv
   .\.venv\Scripts\Activate.ps1

   # Linux / macOS
   python3 -m venv .venv
   source .venv/bin/activate
   ```

3. Install required dependencies:
   ```bash
   pip install -r requirements.txt
   ```

---

### 3. Run the Anomaly Detector

Find your device COM port in Windows Device Manager (under *Ports (COM & LPT)*) and launch the analyzer:

```bash
python analyzer.py --port COM7 --baud 115200 --baseline 50
```

#### CLI Parameters:
* `--port`: The serial port of your ESP8266 (e.g. `COM7` on Windows, `/dev/ttyUSB0` on Linux).
* `--baud`: Serial communication speed (Default: `115200`).
* `--baseline`: Number of baseline packets to capture before activating AI detection (Default: `50`).
* `--contamination`: Proportion of suspected outliers in the RF environment (Default: `0.1`).
* `--log`: Path to output forensic CSV file (Default: `forensics_log.csv`).

---

##  Machine Learning Engine: How It Works

1. **Feature Extraction:**
   The analyzer parses the stream for two primary RF features:
   * **RSSI ($dBm$):** Received Signal Strength Indication (typically ranges from $-90$ dBm to $-30$ dBm).
   * **Channel:** The 2.4 GHz frequency channel ($1$ through $13$).

2. **Isolation Forest Algorithm:**
   * Isolates anomalies rather than profiling normal points.
   * Recursively generates random partitions on the feature space.
   * Anomalous points (e.g. sudden high-power rogue access points, unexpected channel hops, or deauthentication bursts) require significantly fewer splits to isolate, resulting in shorter average path lengths.

3. **Forensic Logging:**
   When an anomaly is flagged (`prediction == -1`), it is timestamped and recorded into `forensics_log.csv`:
   ```csv
   2026-10-27 16:45:10,AA:BB:CC:DD:EE:FF,-22,6,ANOMALY
   ```

---

##  Security & Educational Disclaimer

> [!WARNING]
> This project is designed strictly for **authorized network monitoring, defensive security research, and educational purposes**. Always ensure you have permission to monitor wireless networks within your testing environment.

---

## 📄 License
This project is licensed under the [MIT License](LICENSE).
