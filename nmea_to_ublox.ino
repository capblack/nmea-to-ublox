/*
 * NMEA to u-blox Protocol Translator
 * Para ESP32-C3/S3 con GPS NMEA0183 (AK721-JM)
 * Destino: iNav 9.1 Flight Controller
 * 
 * Este programa:
 * 1. Lee mensajes NMEA del GPS (GGA, RMC, GSA) - No-bloqueante
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
unsigned long lastDebugPrint = 0;
unsigned long setupTime = 0;

// State flags
bool gpsInitialized = false;
bool inavInitialized = false;
bool systemReady = false;

// Statistics
struct {
  unsigned long gpsFrames;
  unsigned long ubloxFrames;
  unsigned long errors;
  unsigned long lastFrame;
  unsigned long serialErrors;
  unsigned long loopCount;
} stats = {0, 0, 0, 0, 0, 0};

// Forward declarations
void initializeSerials();
void procesarNMEA(const char* sentence);
void enviarDatosAiNav();
void printDebugInfo();

void setup() {
  // Debug serial (USB) - Start immediately, don't block
  Serial.begin(DEBUG_BAUD_RATE);
  
  setupTime = millis();
  
  // Small delay to let USB serial stabilize
  for (int i = 0; i < 50; i++) {
    delay(10);
    if (Serial) break;  // Exit early if serial is ready
  }
  
  Serial.println("\n\n=== NMEA to u-blox Translator ===");
  Serial.print("Board: ");
  Serial.println(BOARD_NAME);
  Serial.println("Starting up...");
  
  // Initialize serials with error handling
  initializeSerials();
  
  systemReady = true;
  Serial.println("System ready. Waiting for GPS data...");
  Serial.println("-----------------------------------");
}

void initializeSerials() {
  // Initialize GPS UART (UART1)
  if (!Serial1.begin(GPS_BAUD_RATE, SERIAL_8N1, GPS_RX_PIN, GPS_TX_PIN)) {
    Serial.println("ERROR: Failed to initialize GPS UART");
    stats.serialErrors++;
  } else {
    gpsInitialized = true;
    Serial.print("✓ GPS UART1: RX=GPIO");
    Serial.print(GPS_RX_PIN);
    Serial.print(" TX=GPIO");
    Serial.print(GPS_TX_PIN);
    Serial.print(" @ ");
    Serial.print(GPS_BAUD_RATE);
    Serial.println(" baud");
  }
  
  // Initialize iNav UART (UART2 for S3, SoftwareSerial for C3)
  #ifdef USE_SOFTWARE_SERIAL
    Serial.println("Note: Using SoftwareSerial for iNav (ESP32-C3)");
    inavInitialized = true;
  #else
    if (!Serial2.begin(INAV_BAUD_RATE, SERIAL_8N1, INAV_RX_PIN, INAV_TX_PIN)) {
      Serial.println("ERROR: Failed to initialize iNav UART");
      stats.serialErrors++;
    } else {
      inavInitialized = true;
      Serial.print("✓ iNav UART2: RX=GPIO");
      Serial.print(INAV_RX_PIN);
      Serial.print(" TX=GPIO");
      Serial.print(INAV_TX_PIN);
      Serial.print(" @ ");
      Serial.print(INAV_BAUD_RATE);
      Serial.println(" baud");
    }
  #endif
}

void loop() {
  stats.loopCount++;
  unsigned long currentTime = millis();
  
  // Read GPS data - Non-blocking, with timeout protection
  if (gpsInitialized && Serial1.available() > 0) {
    char c = Serial1.read();
    
    if (c == '$') {
      // Start of new NMEA sentence
      nmeaBufferIndex = 0;
      nmeaBuffer[nmeaBufferIndex++] = c;
    } else if ((c == '\n' || c == '\r') && nmeaBufferIndex > 0) {
      // End of sentence
      nmeaBuffer[nmeaBufferIndex] = '\0';
      procesarNMEA(nmeaBuffer);
      nmeaBufferIndex = 0;
    } else if (nmeaBufferIndex > 0 && nmeaBufferIndex < NMEA_BUFFER_SIZE - 1) {
      // Add character to buffer (only if within bounds)
      nmeaBuffer[nmeaBufferIndex++] = c;
    } else if (nmeaBufferIndex >= NMEA_BUFFER_SIZE - 1) {
      // Buffer overflow - reset
      Serial.println("ERROR: NMEA buffer overflow, resetting");
      nmeaBufferIndex = 0;
      stats.errors++;
    }
  }
  
  // Send update to iNav at specified rate - Non-blocking
  if (inavInitialized && currentTime - lastUpdateTime >= UPDATE_RATE_MS) {
    lastUpdateTime = currentTime;
    enviarDatosAiNav();
  }
  
  // Print debug info periodically
  if (currentTime - lastDebugPrint >= 5000) {  // Every 5 seconds
    lastDebugPrint = currentTime;
    printDebugInfo();
  }
  
  // Minimal delay to prevent watchdog timeout
  delay(LOOP_DELAY_MS);
}

void procesarNMEA(const char* sentence) {
  // Validate and parse NMEA sentence
  if (!nmeaParser.parseSentence(sentence)) {
    stats.errors++;
    return;
  }
  
  stats.gpsFrames++;
  stats.lastFrame = millis();
  
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
      
      int length = 0;
      if (converter.convertGSAToPVT(gsa, ubloxBuffer, length)) {
        stats.ubloxFrames++;
      }
      break;
    }
    
    default:
      break;
  }
}

void enviarDatosAiNav() {
  NAVPVT pvt = converter.getPVTData();
  
  // Only send if valid fix exists
  if (pvt.fixType == 0) {
    return;
  }
  
  int length = 0;
  if (converter.buildNAVPVT(ubloxBuffer, length)) {
    // Check if Serial2 is available before writing
    if (Serial2) {
      Serial2.write(ubloxBuffer, length);
    }
    
    // Debug output every 10 frames
    if ((stats.ubloxFrames % 10) == 0) {
      Serial.print("SENDING u-blox: Lat=");
      Serial.print(pvt.lat / 10000000.0, 6);
      Serial.print(" Lon=");
      Serial.print(pvt.lon / 10000000.0, 6);
      Serial.print(" Sats=");
      Serial.println(pvt.numSV);
    }
  }
}

void printDebugInfo() {
  Serial.println("\n--- System Status ---");
  Serial.print("Uptime: ");
  Serial.print((millis() - setupTime) / 1000);
  Serial.println("s");
  Serial.print("GPS Initialized: ");
  Serial.println(gpsInitialized ? "YES" : "NO");
  Serial.print("iNav Initialized: ");
  Serial.println(inavInitialized ? "YES" : "NO");
  Serial.print("GPS Frames: ");
  Serial.println(stats.gpsFrames);
  Serial.print("u-blox Frames: ");
  Serial.println(stats.ubloxFrames);
  Serial.print("Errors: ");
  Serial.println(stats.errors);
  Serial.print("Serial Errors: ");
  Serial.println(stats.serialErrors);
  Serial.print("Loop Count: ");
  Serial.println(stats.loopCount);
  Serial.println("-------------------\n");
}
