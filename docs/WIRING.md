# Esquema de Conexiones - ESP32-C3 a GPS NMEA y iNav

## ESP32-C3 Pinout

```
        ┌─────────────────────┐
        │    ESP32-C3         │
    USB │ ▲         GND ───┬──│── GND
        │                  │  │
    5V  │ ──────────────┬──┘  │
        │               │     │
        │ GPIO2 (TX)────┤─────│── Serial Debug (optional)
        │ GPIO3 (RX)    │     │
        │               │     │
        │ GPIO20 (RX)───┼─────│── GPS NMEA Input
        │ GPIO21 (TX)   │     │
        │               │     │
        │ GPIO9 (TX)────┼─────│── iNav u-blox Output
        │ GPIO8 (RX)    │     │
        │               │     │
        │ GND ──────────┴─────│── Common GND
        └─────────────────────┘
```

## Conexiones Detalladas

### GPS AK721-JM (NMEA Input)

| GPS Pin | Signal | ESP32-C3 | Función |
|---------|--------|----------|---------|
| 1 | VCC (5V) | 5V | Alimentación |
| 2 | RX | GPIO21 | Envío de datos desde GPS |
| 3 | TX | GPIO20 | Recepción de datos al GPS |
| 4 | GND | GND | Tierra común |

**Velocidad:** 9600 baud (por defecto AK721-JM)

### iNav 9.1 Flight Controller (u-blox Output)

| iNav Pin | Signal | ESP32-C3 | Función |
|----------|--------|----------|---------|
| UART RX | GPS_RX | GPIO9 | Envío de datos a iNav |
| UART TX | GPS_TX | GPIO8 | Recepción desde iNav (opcional) |
| GND | GND | GND | Tierra común |

**Velocidad:** 115200 baud (típico para iNav)

### Debug Serial (USB - Opcional)

| Function | ESP32-C3 | Propósito |
|----------|----------|----------|
| TX (Debug) | GPIO2 | Ver logs y estadísticas |
| USB | ESP32-C3 | Conexión a PC |

## Diagrama Completo

```
    ┌────────────┐         ┌───────────────┐         ┌──────────────┐
    │  GPS       │         │  ESP32-C3     │         │  iNav 9.1    │
    │ AK721-JM   │         │  Translator   │         │ Flight Ctrl  │
    │            │         │               │         │              │
    │ TX ────────┼─────────┼─ GPIO20 (RX) │         │              │
    │            │  9600   │               │         │              │
    │ RX ────────┼─────────┼─ GPIO21 (TX) │         │              │
    │            │  baud   │               │         │              │
    │ 5V ────────┼─────────┼─ 5V          │         │              │
    │            │         │               │         │              │
    │ GND ───────┼─────────┼─ GND         │         │              │
    └────────────┘         │               │         │              │
                           │               │         │              │
                           │ GPIO9 (TX) ───┼─────────┼─ GPS RX      │
                           │               │ 115200  │              │
                           │ GPIO8 (RX) ───┼─────────┼─ GPS TX      │
                           │               │  baud   │              │
                           │ GND ──────────┼─────────┼─ GND        │
                           │               │         │              │
    ┌─ USB (PC) ────────── │               │         └──────────────┘
    │ DEBUG SERIAL         │               │
    │ GPIO2 (TX)          │               │
    └────────────────────── │               │
                           └───────────────┘
```

## Notas Importantes

1. **Voltaje:** ESP32-C3 es de 3.3V. Si el GPS envía 5V, usa un divisor de voltaje en GPIO20:
   ```
   GPS TX (5V) ──┬─── 10kΩ ──┬─── GND
                 │            │
                 └─ 20kΩ ──┬──┴─ GPIO20
                          │
   ```

2. **GND Común:** Asegúrate que GPS, ESP32-C3 e iNav compartan la misma tierra.

3. **Capacitores:** Añade capacitores de desacople de 100nF cerca de la alimentación de ESP32-C3.

4. **Cables:** Usa cables cortos y de buena calidad, especialmente UART.

## Configuración en Arduino IDE

```
Board: ESP32-C3-DevKitM-1
Upload Speed: 460800
CPU Frequency: 160 MHz
Flash Mode: DIO
Flash Size: 4MB
Partition Scheme: Default
```

## Verificación

1. Sube el código a ESP32-C3
2. Abre Serial Monitor (115200 baud)
3. Deberías ver "NMEA to u-blox Translator"
4. Espera a que el GPS adquiera señal (puede tomar minutos)
5. Verás mensajes GGA, RMC, GSA recibidos
6. Los datos se envían a iNav por GPIO9 (TX)

## Solución de Problemas

**No se reciben datos NMEA:**
- Verifica conexión GPIO20 (RX)
- Confirma velocidad 9600 baud
- Prueba con Serial Monitor en GPS_RX para verificar datos

**iNav no recibe datos u-blox:**
- Verifica conexión GPIO9 (TX) a iNav
- Confirma que iNav está configurado para protocolo GPS_UBLOX
- Verifica voltaje: 3.3V en GPIO9

**Errores de checksum NMEA:**
- Verifica conexión física
- Reduce longitud de cables UART
