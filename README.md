# NMEA to u-blox Translator

Traductor de protocolo NMEA0183 a u-blox binario para ESP32-C3, diseñado para usar un GPS NMEA (AK721-JM) con iNav 9.1.

## Características

- **Parseo NMEA0183**: GGA, RMC, GSA
- **Conversión a u-blox**: NAV-PVT, NAV-STATUS
- **Hardware**: ESP32-C3
- **GPS**: AK721-JM (NMEA0183)
- **Destino**: iNav 9.1 Flight Controller

## Estructura del Proyecto

```
nmea-to-ublox/
├── src/
│   ├── main.cpp              # Código principal
│   ├── nmea_parser.h         # Parser NMEA0183
│   ├── nmea_parser.cpp
│   ├── ublox_converter.h     # Convertidor a u-blox
│   ├── ublox_converter.cpp
│   └── config.h              # Configuración
├── docs/
│   ├── WIRING.md             # Esquema de conexiones
│   ├── INAV_SETUP.md         # Configuración iNav
│   └── NMEA_PROTOCOL.md      # Referencia NMEA
├── platformio.ini            # Configuración PlatformIO
└── README.md
```

## Requisitos

- ESP32-C3
- GPS AK721-JM (NMEA0183)
- iNav 9.1
- PlatformIO o Arduino IDE

## Instalación Rápida

1. Clona el repositorio
2. Configura PlatformIO (recomendado)
3. Carga el firmware en ESP32-C3
4. Conecta según el esquema en `docs/WIRING.md`
5. Configura iNav según `docs/INAV_SETUP.md`

## Protocolo

### Mensajes NMEA Soportados

- **GGA**: Global Positioning System Fix Data
- **RMC**: Recommended Minimum Navigation Information
- **GSA**: GPS DOP and Active Satellites

### Mensajes u-blox Generados

- **NAV-PVT**: Navigation Position Velocity Time
- **NAV-STATUS**: Navigation Status

## Documentación

Ver carpeta `docs/` para:
- Esquema de conexiones completo
- Guía de configuración iNav
- Referencia de protocolos

## Licencia

MIT

## Autor

capblack
