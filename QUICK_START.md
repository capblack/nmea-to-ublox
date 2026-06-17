# Quick Start Guide

## Instalación Rápida (5 minutos)

### 1. Hardware Necesario

- ESP32-C3 DevKit
- GPS NMEA0183 (AK721-JM)
- Cable USB para ESP32-C3
- Jumpers/cables para conexiones
- iNav 9.1 Flight Controller

### 2. Conexiones

Sigue el esquema en `docs/WIRING.md`:

```
GPS TX    → ESP32-C3 GPIO20 (RX)
GPS RX    → ESP32-C3 GPIO21 (TX)
GPS 5V    → ESP32-C3 5V
GPS GND   → ESP32-C3 GND

ESP32-C3 GPIO9 (TX)  → iNav GPS RX
ESP32-C3 GPIO8 (RX)  → iNav GPS TX
ESP32-C3 GND        → iNav GND
```

### 3. Instalar Arduino IDE

1. Descarga Arduino IDE: https://www.arduino.cc/en/software
2. Instala el board ESP32: https://github.com/espressif/arduino-esp32
3. Selecciona Board: **ESP32-C3-DevKitM-1**

### 4. Cargar el Código

```bash
# Clona el repositorio
git clone https://github.com/capblack/nmea-to-ublox
cd nmea-to-ublox

# Abre Arduino IDE
# File > Open > nmea_to_ublox.ino
# Selecciona tu puerto COM
# Click Upload
```

### 5. Verificar

1. Abre Serial Monitor (115200 baud)
2. Deberías ver:
   ```
   === NMEA to u-blox Translator ===
   Starting up...
   UART initialized...
   Waiting for GPS data...
   ```

3. Espera a que el GPS adquiera satélites (2-5 minutos)
4. Verás mensajes como:
   ```
   GGA: Lat=40.123456 Lon=-73.654321 Alt=125.5m Sats=12 Fix=1
   ```

### 6. Configurar iNav

1. Abre Betaflight Configurator
2. Ve a Configuration → GPS
3. Selecciona:
   - GPS Protocol: **UBLOX**
   - Baudrate: **115200**
4. Guarda la configuración

## Troubleshooting

### "No veo datos NMEA"
- Verifica conexión GPIO20 (RX del GPS)
- Asegúrate que el GPS está encendido
- Comprueba que tiene vista al cielo

### "GPS conectado pero sin fix"
- El GPS necesita tiempo para adquirir satélites
- Coloca antena en lugar abierto
- Aleja de interferencias electromagnéticas

### "iNav no recibe datos"
- Verifica que iNav está configurado para GPS_UBLOX
- Comprueba voltaje en GPIO9: debe ser 3.3V
- Asegúrate que el puerto UART en iNav es correcto

## Próximos Pasos

1. Lee `docs/INAV_SETUP.md` para configuración completa
2. Lee `docs/WIRING.md` para esquema detallado
3. Revisa `docs/NMEA_PROTOCOL.md` para entender los datos

## Soporte

- Issues: https://github.com/capblack/nmea-to-ublox/issues
- iNav Wiki: https://github.com/iNavFlight/inav/wiki
- u-blox Docs: https://www.u-blox.com
