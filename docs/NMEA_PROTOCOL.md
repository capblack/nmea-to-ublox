# Referencia de Protocolo NMEA0183

## Introducción

NMEA0183 es un estándar de protocolo serie para sistemas GPS marinos. Este documento describe los mensajes soportados por este traductor.

## Estructura NMEA

Cada sentencia NMEA tiene el siguiente formato:

```
$<talker><sentence>,<field1>,<field2>,...<fieldN>*<checksum>\r\n
```

### Componentes:

- **$**: Carácter de inicio
- **talker**: 2 caracteres (GP=GPS, GL=GLONASS, GN=Multi-GNSS)
- **sentence**: 3 caracteres (GGA, RMC, GSA, etc.)
- **fields**: Datos separados por comas
- **\***: Separador de checksum
- **checksum**: XOR de todos los caracteres entre $ y *

### Ejemplo:
```
$GPGGA,123519,4807.038,N,01131.000,E,1,08,0.9,545.4,M,46.9,M,,*47
```

## Mensajes Soportados

### GGA - Global Positioning System Fix Data

Proporciona posición actual, altitud y precisión.

**Formato:**
```
$GPGGA,<time>,<lat>,<latDir>,<lon>,<lonDir>,<quality>,<numSat>,<hdop>,<alt>,<altUnit>,<geoid>,<geoidUnit>*<checksum>
```

**Campos:**

| Campo | Ejemplo | Descripción |
|-------|---------|-------------|
| Time | 123519 | hhmmss.ss (UTC) |
| Latitude | 4807.038 | ddmm.mmmmm |
| Lat Dir | N | N/S (North/South) |
| Longitude | 01131.000 | dddmm.mmmmm |
| Lon Dir | E | E/W (East/West) |
| Quality | 1 | 0=invalid, 1=GPS, 2=DGPS |
| NumSat | 08 | Satélites usados (0-12) |
| HDOP | 0.9 | Precisión horizontal |
| Altitude | 545.4 | Altura sobre mar (metros) |
| Geoid | 46.9 | Altura geoidea (metros) |

**Ejemplo Completo:**
```
$GPGGA,123519,4807.038,N,01131.000,E,1,08,0.9,545.4,M,46.9,M,,*47
```

### RMC - Recommended Minimum Navigation Information

Proporciona posición, velocidad, rumbo y fecha.

**Formato:**
```
$GPRMC,<time>,<status>,<lat>,<latDir>,<lon>,<lonDir>,<speed>,<track>,<date>*<checksum>
```

**Campos:**

| Campo | Ejemplo | Descripción |
|-------|---------|-------------|
| Time | 123519 | hhmmss.ss (UTC) |
| Status | A | A=active, V=void |
| Latitude | 4807.038 | ddmm.mmmmm |
| Lat Dir | N | N/S |
| Longitude | 01131.000 | dddmm.mmmmm |
| Lon Dir | E | E/W |
| Speed | 022.4 | Velocidad (nudos) |
| Track | 084.4 | Rumbo (grados) |
| Date | 230394 | ddmmyy |

**Ejemplo Completo:**
```
$GPRMC,123519,A,4807.038,N,01131.000,E,022.4,084.4,230394,003.1,W*6A
```

### GSA - GPS DOP and Active Satellites

Proporciona tipo de fix y valores de dilución de precisión (DOP).

**Formato:**
```
$GPGSA,<selMode>,<fixType>,<satId1>,<satId2>,...<satId12>,<pdop>,<hdop>,<vdop>*<checksum>
```

**Campos:**

| Campo | Ejemplo | Descripción |
|-------|---------|-------------|
| SelMode | A | A=automatic, M=manual |
| FixType | 3 | 1=no fix, 2=2D, 3=3D |
| SatId 1-12 | 04 | ID de satélite usado (puede estar vacío) |
| PDOP | 2.5 | Position DOP (0.5-99.9) |
| HDOP | 1.3 | Horizontal DOP |
| VDOP | 1.9 | Vertical DOP |

**Ejemplo Completo:**
```
$GPGSA,A,3,04,05,09,12,24,,,,,,,2.5,1.3,2.1*30
```

## Conversión a u-blox

Este traductor convierte los datos NMEA a u-blox NAV-PVT:

### Mapeo de Datos

| NMEA | u-blox NAV-PVT | Conversión |
|------|----------------|------------|
| GGA Lat/Lon | lat/lon | degrees × 1e-7 |
| GGA Altitude | height | metros × 1000 → mm |
| RMC Speed | gSpeed | nudos × 514.444 → mm/s |
| RMC Track | heading | grados × 1e-5 |
| GGA HDOP | hAcc | HDOP × 1000 → mm |
| GSA PDOP | pDOP | PDOP × 100 → 1/100 |

### Estructura NAV-PVT (Simplificada)

```c
struct NAVPVT {
  uint32_t iTOW;        // GPS time of week (ms)
  uint16_t year;        // Year (1999-2099)
  uint8_t month;        // Month (1-12)
  uint8_t day;          // Day (1-31)
  uint8_t hour;         // Hour (0-23)
  uint8_t min;          // Minute (0-59)
  uint8_t sec;          // Second (0-59)
  uint8_t fixType;      // 0=none, 1=DR, 2=2D, 3=3D
  uint8_t numSV;        // Number of satellites
  int32_t lon;          // Longitude (deg × 1e-7)
  int32_t lat;          // Latitude (deg × 1e-7)
  int32_t height;       // Height above ellipsoid (mm)
  int32_t hMSL;         // Height above MSL (mm)
  uint32_t hAcc;        // Horizontal accuracy (mm)
  uint32_t vAcc;        // Vertical accuracy (mm)
  int32_t velN;         // North velocity (mm/s)
  int32_t velE;         // East velocity (mm/s)
  int32_t velD;         // Down velocity (mm/s)
  int32_t gSpeed;       // Ground speed (mm/s)
  int32_t heading;      // Heading (deg × 1e-5)
  uint32_t sAcc;        // Speed accuracy (mm/s)
  uint16_t pDOP;        // Position DOP (1/100)
};
```

## Validación

### Checksum NMEA

El checksum es XOR de todos los caracteres entre $ y *:

```c
char checksum = 0;
for (cada carácter entre $ y *) {
    checksum ^= carácter;
}
```

El resultado se convierte a hexadecimal (dos dígitos).

### Errores Comunes

1. **Checksum inválido**: Verificar integridad de datos
2. **Campo vacío**: El dato no está disponible
3. **Quality = 0**: No hay fix GPS
4. **Status = V**: Datos inválidos en RMC

## Velocidades de Baudios

- **GPS (NMEA entrada)**: 9600 baud típico (AK721-JM)
- **iNav (u-blox salida)**: 115200 baud típico
- **Debug Serial**: 115200 baud

## Referencias

- NMEA 0183 Standard: https://en.wikipedia.org/wiki/NMEA_0183
- GPS Receivers: https://www.gpsworld.com/
- u-blox Protocol: https://www.u-blox.com
