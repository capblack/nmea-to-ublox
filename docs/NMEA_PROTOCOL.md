# NMEA0183 Protocol Reference

## Introduction

NMEA0183 is a serial protocol standard used by GPS receivers. This document summarizes the NMEA sentences handled by this bridge.

## NMEA Sentence Structure

Each sentence follows:

```text
$<talker><sentence>,<field1>,<field2>,...<fieldN>*<checksum>\r\n
```

Components:

- `$`: start character
- `talker`: 2-character source (GP, GL, GN, etc.)
- `sentence`: 3-character type (GGA, RMC, GSA, ...)
- `fields`: comma-separated data fields
- `*`: checksum separator
- `checksum`: XOR of all chars between `$` and `*`

Example:

```text
$GPGGA,123519,4807.038,N,01131.000,E,1,08,0.9,545.4,M,46.9,M,,*47
```

## Supported Messages

### GGA - Global Positioning System Fix Data

Provides position, altitude, and fix quality.

```text
$GPGGA,<time>,<lat>,<latDir>,<lon>,<lonDir>,<quality>,<numSat>,<hdop>,<alt>,<altUnit>,<geoid>,<geoidUnit>*<checksum>
```

| Field | Example | Description |
|-------|---------|-------------|
| Time | 123519 | hhmmss.ss (UTC) |
| Latitude | 4807.038 | ddmm.mmmmm |
| Lat Dir | N | N/S |
| Longitude | 01131.000 | dddmm.mmmmm |
| Lon Dir | E | E/W |
| Quality | 1 | 0=invalid, 1=GPS, 2=DGPS |
| NumSat | 08 | Satellites used |
| HDOP | 0.9 | Horizontal precision |
| Altitude | 545.4 | MSL altitude (m) |
| Geoid | 46.9 | Geoid separation (m) |

### RMC - Recommended Minimum Navigation Information

Provides position, speed, course, and date.

```text
$GPRMC,<time>,<status>,<lat>,<latDir>,<lon>,<lonDir>,<speed>,<track>,<date>*<checksum>
```

| Field | Example | Description |
|-------|---------|-------------|
| Time | 123519 | hhmmss.ss (UTC) |
| Status | A | A=active, V=void |
| Latitude | 4807.038 | ddmm.mmmmm |
| Lat Dir | N | N/S |
| Longitude | 01131.000 | dddmm.mmmmm |
| Lon Dir | E | E/W |
| Speed | 022.4 | Knots |
| Track | 084.4 | Degrees |
| Date | 230394 | ddmmyy |

### GSA - GPS DOP and Active Satellites

Provides fix type and DOP values.

```text
$GPGSA,<selMode>,<fixType>,<satId1>,<satId2>,...<satId12>,<pdop>,<hdop>,<vdop>*<checksum>
```

| Field | Example | Description |
|-------|---------|-------------|
| SelMode | A | A=automatic, M=manual |
| FixType | 3 | 1=no fix, 2=2D, 3=3D |
| SatId 1-12 | 04 | Satellite IDs used for fix |
| PDOP | 2.5 | Position DOP |
| HDOP | 1.3 | Horizontal DOP |
| VDOP | 2.1 | Vertical DOP |

## Conversion to UBX NAV-PVT

The bridge maps NMEA data into UBX NAV-PVT and companion NAV messages.

| NMEA Source | UBX Field | Conversion |
|-------------|-----------|------------|
| GGA Lat/Lon | `lat` / `lon` | degrees x 1e-7 |
| GGA Altitude | `height` / `hMSL` | meters x 1000 |
| RMC Speed | `gSpeed` | knots x 514.444 |
| RMC Track | `heading` | degrees x 1e-5 |
| GGA HDOP | `hAcc` | scaled estimate |
| GSA PDOP | `pDOP` | value x 100 |

## NMEA Checksum Validation

The checksum is the XOR of all characters between `$` and `*`.

```c
char checksum = 0;
for (all chars between '$' and '*') {
  checksum ^= c;
}
```

## Common Issues

1. Invalid checksum -> corrupted serial stream or wrong baud rate.
2. Empty fields -> data unavailable for that epoch.
3. GGA quality = 0 -> no GPS fix yet.
4. RMC status = V -> navigation data invalid.

## Typical Baud Rates

- GPS input (NMEA): depends on module (often 9600 or 115200)
- iNav output (UBX): 115200
- Debug serial: 115200

## References

- NMEA 0183 overview: https://en.wikipedia.org/wiki/NMEA_0183
- u-blox protocol resources: https://www.u-blox.com
