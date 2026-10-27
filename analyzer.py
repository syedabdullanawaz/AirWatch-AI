"""
WiFi Security Anomaly Detector
------------------------------
Real-time AI-driven Wi-Fi security monitor that streams telemetry from an ESP8266
hardware sniffer and uses an unsupervised Isolation Forest machine learning model
to detect rogue transmitters, signal spoofing, and abnormal RF behavior.
"""

import argparse
import sys
import time
import serial
import serial.tools.list_ports
import pandas as pd
from sklearn.ensemble import IsolationForest

def parse_args():
    parser = argparse.ArgumentParser(description="WiFi Security - Real-time ML Anomaly Detector")
    parser.add_argument("--port", type=str, default="COM7", help="Serial COM port (default: COM7)")
    parser.add_argument("--baud", type=int, default=115200, help="Baud rate (default: 115200)")
    parser.add_argument("--baseline", type=int, default=50, help="Number of packets for baseline training (default: 50)")
    parser.add_argument("--contamination", type=float, default=0.1, help="Expected anomaly rate (default: 0.1 / 10%%)")
    parser.add_argument("--log", type=str, default="forensics_log.csv", help="Forensic alert log file path")
    return parser.parse_args()

def list_available_ports():
    ports = serial.tools.list_ports.comports()
    if not ports:
        return "No COM ports found. Connect your ESP8266 via USB."
    return ", ".join([f"{p.device} ({p.description})" for p in ports])

def main():
    args = parse_args()
    
    print("=" * 60)
    print("  WiFi Security - AI Anomaly & Threat Detection Engine")
    print("=" * 60)
    print(f"Target Port        : {args.port}")
    print(f"Baud Rate          : {args.baud}")
    print(f"Baseline Size      : {args.baseline} packets")
    print(f"Contamination Rate : {args.contamination * 100:.1f}%")
    print(f"Forensics Log      : {args.log}")
    print("-" * 60)

    try:
        ser = serial.Serial(args.port, args.baud, timeout=1)
        time.sleep(2)  # Allow serial connection to settle
        print(f"[+] Successfully connected to {args.port}.")
        print("[*] Listening for telemetry and collecting baseline RF data...\n")
    except Exception as e:
        print(f"[!] Error: Could not open serial port {args.port}: {e}")
        print(f"[*] Available system ports: {list_available_ports()}")
        print("[*] Tip: Ensure the Arduino Serial Monitor or other serial terminals are closed.")
        sys.exit(1)

    packets = []
    clf = IsolationForest(contamination=args.contamination, random_state=42)
    trained = False

    try:
        while True:
            raw_line = ser.readline().decode("utf-8", errors="ignore").strip()
            
            if not raw_line:
                continue

            # Expected telemetry format: DATA,MAC,RSSI,CHANNEL
            if raw_line.startswith("DATA,"):
                parts = raw_line.split(",")
                if len(parts) < 4:
                    continue
                
                try:
                    mac = parts[1].strip()
                    rssi = int(parts[2].strip())
                    channel = int(parts[3].strip())
                except ValueError:
                    continue

                packets.append([rssi, channel])

                # Train baseline model once baseline quota is reached
                if not trained and len(packets) >= args.baseline:
                    print("-" * 60)
                    print(f"[*] Baseline collected ({len(packets)} samples). Fitting Isolation Forest model...")
                    clf.fit(packets)
                    trained = True
                    print("[✓] MODEL TRAINED & ACTIVE. NOW MONITORING FOR ANOMALIES / ATTACKS...")
                    print("-" * 60)

                if trained:
                    # Predict anomaly: -1 denotes anomaly, 1 denotes normal baseline
                    prediction = clf.predict([[rssi, channel]])[0]
                    timestamp = time.strftime("%Y-%m-%d %H:%M:%S")

                    if prediction == -1:
                        print(f"[ALERT | {timestamp}] Anomaly Detected! MAC: {mac} | RSSI: {rssi:4d} dBm | Ch: {channel:2d}")
                        
                        # Append event to forensics log
                        with open(args.log, "a", encoding="utf-8") as f:
                            f.write(f"{timestamp},{mac},{rssi},{channel},ANOMALY\n")
                    else:
                        print(f"[Normal| {timestamp}] Telemetry OK: MAC: {mac} | RSSI: {rssi:4d} dBm | Ch: {channel:2d}")

    except KeyboardInterrupt:
        print("\n[*] Monitoring stopped by user. Closing serial connection.")
    finally:
        if 'ser' in locals() and ser.is_open:
            ser.close()

if __name__ == "__main__":
    main()
