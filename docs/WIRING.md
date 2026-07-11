# Wiring Guide - ESP32, NMEA GPS, and iNav

## Core Connections

### GPS <-> ESP32

| GPS Pin | Signal | ESP32 Pin | Purpose |
|---------|--------|-----------|---------|
| VCC | 5V | 5V | Power |
| TX | NMEA out | GPS RX pin | GPS data into ESP32 |
| RX | NMEA in | GPS TX pin | Optional commands from ESP32 to GPS |
| GND | Ground | GND | Common ground |

### ESP32 <-> Flight Controller (iNav)

| FC Pin | Signal | ESP32 Pin | Purpose |
|--------|--------|-----------|---------|
| GPS RX | UART RX | iNav TX pin | UBX stream from ESP32 to FC |
| GPS TX | UART TX | iNav RX pin | Optional requests from FC to ESP32 |
| GND | Ground | GND | Common ground |

## Default Pins

Check `nmea_to_ublox/src/config.h` for the active board profile and exact pins.

- **ESP32-S3 profile**
  - GPS: RX=18, TX=17
  - iNav: RX=15, TX=16
- **ESP32-C3 profile**
  - GPS: RX=20, TX=21
  - iNav: RX=8, TX=9

## Electrical Notes

1. ESP32 logic is **3.3V**.
2. If GPS TX outputs 5V TTL, use a level shifter or resistor divider into ESP32 RX.
3. Keep UART wires short and routed away from high-current motor/ESC paths.
4. Always share a common ground across GPS, ESP32, and FC.

## Bring-up Checklist

1. Flash firmware to ESP32.
2. Open serial monitor at `115200`.
3. Confirm startup logs and UART initialization.
4. Confirm NMEA sentences are received.
5. Confirm UBX stream is produced toward iNav.
6. In iNav, set GPS protocol to `UBLOX` and matching baud.

## Troubleshooting

### No NMEA input

- Verify GPS TX -> ESP32 GPS RX wiring.
- Verify GPS module power.
- Verify module baud rate.

### iNav does not detect GPS

- Verify ESP32 iNav TX -> FC GPS RX.
- Verify iNav UART port and baud rate.
- Confirm UBX protocol is selected in iNav.

### Intermittent data / checksum errors

- Improve grounding and cable routing.
- Shorten UART wires.
- Verify stable power rails for GPS and ESP32.
