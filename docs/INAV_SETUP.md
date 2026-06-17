# Configuración de iNav 9.1 para u-blox GPS

## Descripción General

iNav 9.1 soporta varios protocolos GPS. Este documento guía la configuración para recibir datos del traductor NMEA a u-blox corriendo en ESP32-C3.

## Configuración Inicial

### 1. Acceso a la Configuración

1. Conecta tu Flight Controller a Betaflight Configurator
2. Ve a la pestaña "Configuration"
3. Busca la sección "GPS"

### 2. Seleccionar Protocolo GPS

**GPS Protocol:** Selecciona `UBLOX` (no NMEA)

```
GPS Protocol: UBLOX
```

### 3. Configurar Puerto Serial

**GPS UART/Serial Port:**
- Selecciona el puerto UART donde conectaste el ESP32-C3 (GPIO9 - TX)
- Velocidad: `115200 baud` (debe coincidir con `INAV_BAUD_RATE` en config.h)

Ejemplo para Betaflight:
```
UART 4 (o el que uses): 115200 baud, GPS Protocol
```

### 4. Habilitar GPS

En iNav, asegúrate que:
- `[✓] GPS` esté habilitado
- `[✓] Dynamic Filter` esté habilitado (si aplica)

## Configuración Avanzada

### Sensibilidad de Posición

En Configurator → Configuration → GPS:

```
GPS Type: UBLOX
GPS Data Rate: AUTO (permite que el GPS reporte a su velocidad)
GPS Altitude Source: GPS (usar altitud del GPS)
```

### Accuracy (Precisión)

iNav usará automáticamente los valores de precisión enviados por el u-blox NAV-PVT:
- HDOP (Horizontal Dilution of Precision)
- VDOP (Vertical Dilution of Precision)

Estos se envían automáticamente desde el traductor.

### Timeout

Si el GPS no envía datos después de **2 segundos**, iNav asumirá pérdida de señal:

```
GPS TIMEOUT: 2000 ms (configuración típica)
```

## Verificación en iNav

### 1. Chequeo de Señal GPS

En iNav Configurator → Configuration:
- Verás "GPS" con el estado
- Debería mostrar número de satélites cuando hay señal

### 2. CLI Commands (Opcional)

Puedes verificar el estado del GPS con:

```
gps_info
```

Debería mostrar algo como:
```
Detected GPS: UBLOX
Satellites: 12
Fix Type: 3D
```

### 3. OSD (On-Screen Display)

Si tu flight controller tiene OSD, configura elementos de GPS:
- Satélite count
- GPS status
- GPS lat/lon (para debugging)
- Altitude

## Troubleshooting

### "GPS Not Detected"

1. **Verifica conexión física:**
   - GPIO9 (ESP32-C3 TX) → iNav GPS RX
   - Tierra común

2. **Verifica configuración de puerto:**
   - Asegúrate que el UART en iNav esté configurado correctamente
   - Velocidad debe ser 115200 baud

3. **Verifica que el traductor envía datos:**
   - Mira los logs en Serial Monitor del ESP32-C3
   - Deberías ver "ENVIANDO u-blox" cada 10 frames

### "GPS Detected but No Fix"

1. El GPS necesita tiempo para obtener satélites (2-5 minutos)
2. Asegúrate que el GPS (AK721-JM) tiene vista al cielo
3. Verifica que el GPS recibe alimentación correcta (5V)

### "Position Drifting"

1. Verifica HDOP (debe ser < 3.0 para buena precisión)
2. Reduce interferencia electromagnética
3. Verifica antena GPS

## Comandos útiles en CLI de iNav

```bash
# Ver estado GPS
gps_info

# Ver estadísticas
status

# Resetear configuración GPS a defaults
set gps_provider = UBLOX
set gps_sbas_mode = AUTO
set gps_auto_baud = OFF
save
```

## Calibración de Magnetómetro (Compass)

Si tienes un compass/magnetómetro en tu FC:
1. Ve a Configuration → Compass
2. Ejecuta calibración de compass
3. Rota el FC en todas las direcciones

El GPS complementa los datos de navegación del compass.

## Performance Tips

1. **Ubicación del GPS:**
   - Coloca GPS lo más alto posible en el drone
   - Lejos de antenas de radio
   - Lejos de motores y ESCs

2. **Antena:**
   - Usa buena antena GPS de cerámica
   - Apunta hacia el cielo
   - Aleja de materiales metálicos

3. **Configuración de iNav:**
   - GPS Altitude Source = GPS
   - Dynamic Filter habilitado
   - Usar GPS para navegación automática

## Referencias

- Documentación iNav: https://github.com/iNavFlight/inav/wiki
- u-blox Protocol: https://www.u-blox.com/en/product/neo-m8-series
- Betaflight Configurator: https://github.com/betaflight/betaflight-configurator

---

**Nota:** El traductor NMEA a u-blox maneja automáticamente la conversión de protocolos. No necesitas hacer nada especial en iNav más allá de configurar el protocolo GPS a UBLOX y el puerto serial correspondiente.
