# Quick Start Guide

## 5-Minute Setup

### 1. Required Hardware

- ESP32-C3 or ESP32-S3
- NMEA GPS module
- USB cable
- Jumper wires
- Flight controller running iNav 9.1

### 2. Wiring

Use `docs/WIRING.md` and your board profile in `src/config.h`.

### 3. Flash Firmware

```bash
git clone https://github.com/capblack/nmea-to-ublox
cd nmea-to-ublox
```

Open `nmea_to_ublox.ino` in Arduino IDE, select the correct ESP32 board and COM port, then upload.

### 4. Verify Serial Output

Open Serial Monitor at `115200` and confirm startup logs, then verify NMEA input and UBX output activity.

### 5. Configure iNav

In iNav Configurator:

- GPS protocol: `UBLOX`
- GPS baud: `115200`
- UART: the one wired to ESP32 iNav TX/RX

## Troubleshooting

### No NMEA data

- Check GPS TX/RX wiring
- Check GPS power
- Check GPS baud rate

### GPS detected but no fix

- Wait 2-5 minutes outdoors
- Verify antenna placement and connection

### iNav does not receive data

- Verify FC GPS RX is connected to ESP32 iNav TX
- Confirm iNav UART and protocol settings

## Next

- `docs/INAV_SETUP.md` for full iNav configuration
- `docs/WIRING.md` for complete wiring details
- `docs/NMEA_PROTOCOL.md` for message reference
