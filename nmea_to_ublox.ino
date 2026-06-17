/*
 * NMEA to u-blox Protocol Translator
 * Para ESP32-C3 con GPS NMEA0183 (AK721-JM)
 * Destino: iNav 9.1 Flight Controller
 * 
 * Este programa:
 * 1. Lee mensajes NMEA del GPS (GGA, RMC, GSA)
 * 2. Los convierte al protocolo binario u-blox
 * 3. Los envía al flight controller iNav
 */

#include "src/config.h"
#include "src/nmea_parser.h"
#include "src/ublox_converter.h"

// Global objects
NMEAParser nmeaParser;
UBLOXConverter converter;

// Buffers
char nmeaBuffer[NMEA_BUFFER_SIZE];
uint8_t ubloxBuffer[UBLOX_BUFFER_SIZE];
int nmeaBufferIndex = 0;

// Timing
unsigned long lastUpdateTime = 0;

// Statistics
struct {
  unsigned long gpsFrames;
  unsigned long ubloxFrames;
  unsigned long errors;
  unsigned long lastFrame;
} stats = {0, 0, 0, 0};

void setup() {
  // Debug serial (USB)
  Serial.begin(DEBUG_BAUD_RATE);
  delay(1000);
  
  Serial.println("\n\n=== NMEA to u-blox Translator ===");
  Serial.println("Starting up...");
  
  // Initialize UART0 for GPS input (NMEA)
  Serial1.begin(GPS_BAUD_RATE, SERIAL_8N1, GPS_RX_PIN, GPS_TX_PIN);
  
  // Initialize UART1 for iNav output (u-blox)
  Serial2.begin(INAV_BAUD_RATE, SERIAL_8N1, INAV_RX_PIN, INAV_TX_PIN);
  
  Serial.println("UART initialized:");
  Serial.print("  GPS input (NMEA): Pin RX=");
  Serial.print(GPS_RX_PIN);
  Serial.print(" TX=");
  Serial.print(GPS_TX_PIN);
  Serial.print(" @ ");
  Serial.print(GPS_BAUD_RATE);
  Serial.println(" baud");
  
  Serial.print("  iNav output (u-blox): Pin RX=");
  Serial.print(INAV_RX_PIN);
  Serial.print(" TX=");
  Serial.print(INAV_TX_PIN);
  Serial.print(" @ ");
  Serial.print(INAV_BAUD_RATE);
  Serial.println(" baud");
  
  Serial.println("\nWaiting for GPS data...");
  Serial.println("-----------------------------------");
}

void loop() {
  // Leer datos del GPS (NMEA)
  while (Serial1.available()) {
    char c = Serial1.read();
    
    if (c == '$') {
      // Inicio de nueva sentencia NMEA
      nmeaBufferIndex = 0;
      nmeaBuffer[nmeaBufferIndex++] = c;
    } else if (c == '\n' || c == '\r') {
      // Fin de sentencia
      if (nmeaBufferIndex > 0) {
        nmeaBuffer[nmeaBufferIndex] = '\0';
        procesarNMEA(nmeaBuffer);
        nmeaBufferIndex = 0;
      }
    } else {
      // Agregar carácter al buffer
      if (nmeaBufferIndex < NMEA_BUFFER_SIZE - 1) {
        nmeaBuffer[nmeaBufferIndex++] = c;
      }
    }
  }
  
  // Enviar actualización a iNav a cierta velocidad
  unsigned long currentTime = millis();
  if (currentTime - lastUpdateTime >= UPDATE_RATE_MS) {
    lastUpdateTime = currentTime;
    enviarDatosAiNav();
  }
  
  delay(LOOP_DELAY_MS);
}

void procesarNMEA(const char* sentence) {
  // Validar y parsear sentencia NMEA
  if (!nmeaParser.parseSentence(sentence)) {
    stats.errors++;
    return;
  }
  
  stats.gpsFrames++;
  stats.lastFrame = millis();
  
  // Mostrar la sentencia parseada
  NMEASentenceType sentenceType = nmeaParser.getLastSentenceType();
  
  switch (sentenceType) {
    case NMEA_GGA: {
      GGAData gga = nmeaParser.getGGAData();
      Serial.print("GGA: Lat=");
      Serial.print(gga.latitude, 6);
      Serial.print(" Lon=");
      Serial.print(gga.longitude, 6);
      Serial.print(" Alt=");
      Serial.print(gga.altitude);
      Serial.print("m Sats=");
      Serial.print(gga.numSatellites);
      Serial.print(" Fix=");
      Serial.println(gga.fixQuality);
      
      // Convertir GGA a u-blox NAV-PVT
      int length = 0;
      if (converter.convertGGAToPVT(gga, ubloxBuffer, length)) {
        stats.ubloxFrames++;
      }
      break;
    }
    
    case NMEA_RMC: {
      RMCData rmc = nmeaParser.getRMCData();
      Serial.print("RMC: Lat=");
      Serial.print(rmc.latitude, 6);
      Serial.print(" Lon=");
      Serial.print(rmc.longitude, 6);
      Serial.print(" Speed=");
      Serial.print(rmc.speedKnots);
      Serial.print(" kn Track=");
      Serial.print(rmc.trackTrue);
      Serial.print("° Status=");
      Serial.println(rmc.status);
      
      // Convertir RMC a u-blox NAV-PVT
      int length = 0;
      if (converter.convertRMCToPVT(rmc, ubloxBuffer, length)) {
        stats.ubloxFrames++;
      }
      break;
    }
    
    case NMEA_GSA: {
      GSAData gsa = nmeaParser.getGSAData();
      Serial.print("GSA: Fix=");
      Serial.print(gsa.fixType);
      Serial.print("D PDOP=");
      Serial.print(gsa.pdop);
      Serial.print(" HDOP=");
      Serial.print(gsa.hdop);
      Serial.print(" VDOP=");
      Serial.println(gsa.vdop);
      
      // Convertir GSA a u-blox NAV-PVT (DOP values)
      int length = 0;
      if (converter.convertGSAToPVT(gsa, ubloxBuffer, length)) {
        stats.ubloxFrames++;
      }
      break;
    }
    
    default:
      Serial.println("UNKNOWN sentence type");
      break;
  }
}

void enviarDatosAiNav() {
  // Obtener datos PVT del convertidor
  NAVPVT pvt = converter.getPVTData();
  
  // Si no hay fix válido, no enviar
  if (pvt.fixType == 0) {
    return;
  }
  
  // Construir y enviar mensaje u-blox
  int length = 0;
  if (converter.buildNAVPVT(ubloxBuffer, length)) {
    Serial2.write(ubloxBuffer, length);
    
    // Debug: mostrar datos que se envían
    if ((stats.ubloxFrames % 10) == 0) {  // Mostrar cada 10 frames
      Serial.print("ENVIANDO u-blox: Lat=");
      Serial.print(pvt.lat / 10000000.0, 6);
      Serial.print(" Lon=");
      Serial.print(pvt.lon / 10000000.0, 6);
      Serial.print(" Sats=");
      Serial.println(pvt.numSV);
    }
  }
}
