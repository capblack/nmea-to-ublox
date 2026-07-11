# iNav 9.1 Setup for u-blox GPS Input

## Overview

iNav 9.1 supports multiple GPS protocols. This guide explains how to configure iNav to receive data from this NMEA-to-u-blox bridge running on ESP32.

## Initial Setup

### 1. Open Configuration

1. Connect your flight controller to iNav Configurator.
2. Open the **Configuration** tab.
3. Find the **GPS** section.

### 2. Select GPS Protocol

Set:

```text
GPS Protocol: UBLOX
```

### 3. Configure Serial Port

Select the UART where ESP32 TX is connected to the FC GPS RX input.

- **Baud rate:** `115200` (must match `INAV_BAUD_RATE` in `config.h`)

### 4. Enable GPS

Make sure:

- GPS is enabled
- Dynamic filter is enabled (if your setup uses it)

## Recommended Advanced Settings

```text
GPS Type: UBLOX
GPS Data Rate: AUTO
GPS Altitude Source: GPS
```

iNav will consume precision fields coming from UBX NAV-PVT (HDOP/VDOP-related data).

## Verification

### Configurator

In iNav Configurator, verify:

- GPS status is detected
- Satellite count increases when sky view is available
- Fix type changes to 2D/3D once locked

### CLI

Useful commands:

```bash
gps_info
status
```

Expected output includes lines similar to:

```text
Detected GPS: UBLOX
Satellites: 12
Fix Type: 3D
```

## Troubleshooting

### GPS not detected

1. Check wiring:
   - ESP32 TX -> FC GPS RX
   - Common GND between ESP32, GPS, and FC
2. Confirm correct UART and baud rate in iNav (`115200`).
3. Check ESP32 serial logs to confirm UBX messages are being sent.

### GPS detected but no fix

1. Wait 2-5 minutes in open sky.
2. Check antenna placement and connection.
3. Confirm GPS module power is stable.

### Position drift

1. Verify HDOP quality (lower is better, typically < 3.0).
2. Reduce electromagnetic noise.
3. Re-check GPS antenna placement.

## Useful CLI Reset

```bash
set gps_provider = UBLOX
set gps_sbas_mode = AUTO
set gps_auto_baud = OFF
save
```

## References

- iNav docs: https://github.com/iNavFlight/inav/wiki
- u-blox protocol reference: https://www.u-blox.com
