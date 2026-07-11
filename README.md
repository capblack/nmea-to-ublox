# NMEA to u-blox Translator

NMEA0183-to-u-blox binary protocol translator for ESP32-C3/S3, designed to use NMEA GPS modules (AK721-JM, LD-29, etc.) with iNav 9.1.

## Features

- **NMEA0183 parsing**: GGA, RMC, GSA
- **u-blox conversion**: NAV-PVT + legacy NAV messages for compatibility
- **Hardware**: ESP32-C3 / ESP32-S3
- **GPS input**: NMEA0183 modules
- **Target**: iNav 9.1 Flight Controller

## Project Structure

```
nmea-to-ublox/
├── src/
│   ├── main.cpp              # Main code
│   ├── nmea_parser.h         # NMEA0183 parser
│   ├── nmea_parser.cpp
│   ├── ublox_converter.h     # u-blox converter
│   ├── ublox_converter.cpp
│   └── config.h              # Configuration
├── docs/
│   ├── WIRING.md             # Wiring guide
│   ├── INAV_SETUP.md         # iNav setup
│   └── NMEA_PROTOCOL.md      # NMEA reference
├── platformio.ini            # PlatformIO config
└── README.md
```

## Requirements

- ESP32-C3 or ESP32-S3
- NMEA0183 GPS module
- iNav 9.1
- PlatformIO or Arduino IDE

## Quick Install

1. Clone the repository
2. Configure PlatformIO (recommended) or Arduino IDE
3. Flash firmware to ESP32
4. Wire devices using `docs/WIRING.md`
5. Configure iNav using `docs/INAV_SETUP.md`

## Protocol

### Supported NMEA Messages

- **GGA**: Global Positioning System Fix Data
- **RMC**: Recommended Minimum Navigation Information
- **GSA**: GPS DOP and Active Satellites

### Generated u-blox Messages

- **NAV-PVT**: Navigation Position Velocity Time
- **NAV-STATUS / NAV-POSLLH / NAV-SOL / NAV-DOP / NAV-SAT**: Compatibility stream for iNav

## Documentation

See the `docs/` folder for:
- Full wiring guide
- iNav setup guide
- Protocol reference

## License

MIT

## Author

capblack
