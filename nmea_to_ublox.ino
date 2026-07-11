/*
 * NMEA to u-blox Protocol Translator
 * For ESP32-C3/S3 with NMEA0183 GPS modules (AK721-JM class)
 * Target: iNav 9.1 Flight Controller
 * 
 * This program:
 * 1. Reads NMEA messages from GPS (GGA, RMC, GSA) in a non-blocking way
 * 2. Converts them to u-blox binary protocol
 * 3. Sends them to the iNav flight controller
 */

#include "src/config.h"
#include "src/nmea_parser.h"
#include "src/ublox_converter.h"
#include <string.h>
#include <stdio.h>

// Global objects
NMEAParser nmeaParser;
UBLOXConverter converter;

// Buffers
char nmeaBuffer[NMEA_BUFFER_SIZE];
uint8_t ubloxBuffer[UBLOX_BUFFER_SIZE];
int nmeaBufferIndex = 0;

// iNav command buffer for reading config commands
char inavCmdBuffer[NMEA_BUFFER_SIZE];
int inavCmdBufferIndex = 0;

// iNav binary u-blox buffer
uint8_t inavBinaryBuffer[UBLOX_BUFFER_SIZE];
int inavBinaryBufferIndex = 0;
bool inavBinaryMode = false;

// Dynamic GPS identity (filled from proprietary NMEA if available)
char gpsModelLabel[16] = "UBLOX10";
char gpsFwExtLabel[30] = "FWVER=SPGL1L5 6.00";
uint32_t detectedGpsBaud = GPS_BAUD_RATE;

// Timing
unsigned long lastUpdateTime = 0;
unsigned long lastDebugPrint = 0;
unsigned long setupTime = 0;
uint16_t updateRateMs = UPDATE_RATE_MS;
unsigned long inavStreamStartAt = 0;
unsigned long ledBlinkTimestamp = 0;
bool ledBlinkState = false;
bool statusLedAvailable = false;

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
uint32_t detectarBaudGPS();
void enviarComandosRateGPS();
void enviarNMEAConChecksum(const char* body);
void procesarNMEA(const char* sentence);
void detectarIdentidadGPS(const char* sentence);
void procesarComandoINav(const char* command);
void procesarComandoBinarioINav(uint8_t* buffer, int length);
void responderACKUBX(uint8_t cls, uint8_t id);
void procesarCFGRATE(const uint8_t* payload, uint16_t payloadLen);
void responderMONVER();
void responderMONGNSS();
void enviarMensajesLegacyAiNav(const NAVPVT& pvt);
void enviarDatosAiNav();
void initializeStatusLed();
void setStatusLedColor(uint8_t r, uint8_t g, uint8_t b);
void updateStatusLed(const NAVPVT& pvt, unsigned long nowMs);
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
  Serial.println("\nTo inject test GPS data, type in Serial Monitor:");
  Serial.println("  $GPGGA,123519,4807.038,N,01131.000,E,1,08,0.9,545.4,M,46.9,M,,*47");
  Serial.println("  $GPRMC,123519,A,4807.038,N,01131.000,E,022.4,084.4,230394,003.1,W*6A");
  Serial.println("  $GPGSA,A,3,04,05,,09,12,,,24,,,,,2.5,1.3,2.1*30");
  Serial.println();

  if (STARTUP_INIT_DELAY_MS > 0) {
    Serial.print("Power-up settle delay: ");
    Serial.print(STARTUP_INIT_DELAY_MS);
    Serial.println(" ms");
    delay(STARTUP_INIT_DELAY_MS);
  }
  
  // Initialize serials with error handling
  initializeSerials();
  initializeStatusLed();
  inavStreamStartAt = millis() + INAV_STREAM_GRACE_MS;
  Serial.print("iNav stream grace: ");
  Serial.print(INAV_STREAM_GRACE_MS);
  Serial.println(" ms");
  
  systemReady = true;
  Serial.println("System ready. Waiting for GPS data...");
  Serial.println("-----------------------------------");
}

void initializeSerials() {
  // Initialize GPS UART (UART1)
  #if ENABLE_GPS_AUTO_BAUD
    detectedGpsBaud = detectarBaudGPS();
  #else
    detectedGpsBaud = GPS_BAUD_RATE;
    Serial1.begin(detectedGpsBaud, SERIAL_8N1, GPS_RX_PIN, GPS_TX_PIN);
  #endif
  gpsInitialized = true;
  Serial.print("✓ GPS UART1: RX=GPIO");
  Serial.print(GPS_RX_PIN);
  Serial.print(" TX=GPIO");
  Serial.print(GPS_TX_PIN);
  Serial.print(" @ ");
  Serial.print(detectedGpsBaud);
  Serial.println(" baud");

#if ENABLE_GPS_RATE_PROBE
  enviarComandosRateGPS();
#endif
  
  // Initialize iNav UART (UART2 for S3, SoftwareSerial for C3)
  #ifdef USE_SOFTWARE_SERIAL
    Serial.println("Note: Using SoftwareSerial for iNav (ESP32-C3)");
    inavInitialized = true;
  #else
    Serial2.begin(INAV_BAUD_RATE, SERIAL_8N1, INAV_RX_PIN, INAV_TX_PIN);
    inavInitialized = true;
    Serial.print("✓ iNav UART2: RX=GPIO");
    Serial.print(INAV_RX_PIN);
    Serial.print(" TX=GPIO");
    Serial.print(INAV_TX_PIN);
    Serial.print(" @ ");
    Serial.print(INAV_BAUD_RATE);
    Serial.println(" baud");
  #endif
}

void enviarNMEAConChecksum(const char* body) {
  if (!body || body[0] == '\0') {
    return;
  }
  uint8_t cs = 0;
  for (int i = 0; body[i] != '\0'; i++) {
    cs ^= (uint8_t)body[i];
  }
  Serial1.print("$");
  Serial1.print(body);
  Serial1.print("*");
  if (cs < 0x10) {
    Serial1.print("0");
  }
  Serial1.print(cs, HEX);
  Serial1.print("\r\n");
}

void enviarComandosRateGPS() {
  const uint16_t measMs = (GPS_TARGET_RATE_HZ > 0) ? (uint16_t)(1000 / GPS_TARGET_RATE_HZ) : 100;
  Serial.print("GPS rate probe: target ");
  Serial.print(GPS_TARGET_RATE_HZ);
  Serial.println(" Hz");

  // UBX-CFG-RATE (works on u-blox-like firmware)
  {
    uint8_t cfgRate[] = {
      0xB5, 0x62, 0x06, 0x08, 0x06, 0x00,
      (uint8_t)(measMs & 0xFF), (uint8_t)((measMs >> 8) & 0xFF), // measRate ms
      0x01, 0x00, // navRate
      0x01, 0x00, // timeRef (GPS time)
      0x00, 0x00  // CK_A CK_B placeholder
    };
    uint8_t ckA = 0;
    uint8_t ckB = 0;
    for (int i = 2; i < 12; i++) {
      ckA += cfgRate[i];
      ckB += ckA;
    }
    cfgRate[12] = ckA;
    cfgRate[13] = ckB;
    Serial1.write(cfgRate, sizeof(cfgRate));
    delay(20);
  }

  // UBX-CFG-CFG save (best effort)
  {
    uint8_t saveCfg[] = {
      0xB5, 0x62, 0x06, 0x09, 0x0D, 0x00,
      0x00, 0x00, 0x00, 0x00, // clearMask
      0xFF, 0xFF, 0x00, 0x00, // saveMask
      0x00, 0x00, 0x00, 0x00, // loadMask
      0x07,                   // deviceMask (all)
      0x00, 0x00              // CK_A CK_B placeholder
    };
    uint8_t ckA = 0;
    uint8_t ckB = 0;
    for (int i = 2; i < 19; i++) {
      ckA += saveCfg[i];
      ckB += ckA;
    }
    saveCfg[19] = ckA;
    saveCfg[20] = ckB;
    Serial1.write(saveCfg, sizeof(saveCfg));
    delay(20);
  }

  // PMTK (MTK3339 / compatibles)
  char pmtk220[24];
  snprintf(pmtk220, sizeof(pmtk220), "PMTK220,%u", measMs);
  enviarNMEAConChecksum(pmtk220);
  delay(20);
  enviarNMEAConChecksum("PMTK397,0");
  delay(20);

  // CASIC/AT-family variants seen in low-cost modules
  char pcas02[24];
  snprintf(pcas02, sizeof(pcas02), "PCAS02,%u", measMs);
  enviarNMEAConChecksum(pcas02);
  delay(20);
  enviarNMEAConChecksum("PCAS03,1,1,1,1,1,0,0,0");
  delay(20);

  // Quectel/GLONASS-style vendor command candidate shared by users
  enviarNMEAConChecksum("PQGLORATE,GPS,10");
  delay(20);

  Serial.println("GPS rate probe: commands sent");
}

uint32_t detectarBaudGPS() {
  const uint32_t candidates[] = {115200, 9600, 57600, 38400, 19200};
  char line[96];
  int idx = 0;

  Serial.println("GPS auto-baud: probing...");

  for (size_t c = 0; c < (sizeof(candidates) / sizeof(candidates[0])); c++) {
    const uint32_t baud = candidates[c];
    Serial1.begin(baud, SERIAL_8N1, GPS_RX_PIN, GPS_TX_PIN);
    delay(60);
    while (Serial1.available() > 0) {
      (void)Serial1.read();
    }

    unsigned long start = millis();
    idx = 0;

    while (millis() - start < GPS_AUTO_BAUD_WINDOW_MS) {
      while (Serial1.available() > 0) {
        char ch = (char)Serial1.read();
        if (ch == '$') {
          idx = 0;
          line[idx++] = ch;
        } else if ((ch == '\n' || ch == '\r') && idx > 6) {
          line[idx] = '\0';
          if (NMEAParser::validateChecksum(line)) {
            Serial.print("GPS auto-baud: detected ");
            Serial.println(baud);
            return baud;
          }
          idx = 0;
        } else if (idx > 0 && idx < (int)sizeof(line) - 1) {
          line[idx++] = ch;
        }
      }
      delay(1);
    }
  }

  Serial.print("GPS auto-baud: fallback ");
  Serial.println(GPS_BAUD_RATE);
  Serial1.begin(GPS_BAUD_RATE, SERIAL_8N1, GPS_RX_PIN, GPS_TX_PIN);
  return GPS_BAUD_RATE;
}

void loop() {
  stats.loopCount++;
  unsigned long currentTime = millis();
  
  // Handle commands from USB serial (test GPS injection)
  if (Serial.available() > 0) {
    char c = Serial.read();
    if (c == '$') {
      // Start of NMEA sentence from USB (for testing)
      nmeaBufferIndex = 0;
      nmeaBuffer[nmeaBufferIndex++] = c;
    } else if ((c == '\n' || c == '\r') && nmeaBufferIndex > 0) {
      // End of sentence from USB
      nmeaBuffer[nmeaBufferIndex] = '\0';
      Serial.print("TEST: Processing: ");
      Serial.println(nmeaBuffer);
      procesarNMEA(nmeaBuffer);
      nmeaBufferIndex = 0;
    } else if (nmeaBufferIndex > 0 && nmeaBufferIndex < NMEA_BUFFER_SIZE - 1) {
      nmeaBuffer[nmeaBufferIndex++] = c;
    }
  }
  
  // Read iNav commands from GPIO15 (UART2 RX) - CRITICAL: Must be non-blocking
  if (inavInitialized && Serial2.available() > 0) {
    uint8_t c = Serial2.read();
    
    // Print ALL bytes received for debugging
    Serial.print("iNav RX: 0x");
    if (c < 0x10) Serial.print("0");
    Serial.print(c, HEX);
    Serial.print(" (");
    if (c >= 32 && c < 127) {
      Serial.print((char)c);
    } else {
      Serial.print(".");
    }
    Serial.println(")");
    
    // Check for u-blox binary message start
    if (c == 0xB5 && !inavBinaryMode) {
      // Potential u-blox sync byte
      inavBinaryBufferIndex = 0;
      inavBinaryBuffer[inavBinaryBufferIndex++] = c;
      inavBinaryMode = true;
    } else if (inavBinaryMode && inavBinaryBufferIndex < UBLOX_BUFFER_SIZE) {
      // Collecting binary message
      inavBinaryBuffer[inavBinaryBufferIndex++] = c;
      
      // Check if we have sync bytes (0xB5 0x62)
      if (inavBinaryBufferIndex == 2 && inavBinaryBuffer[1] != 0x62) {
        // Not a valid u-blox message, treat as NMEA
        inavBinaryMode = false;
        inavBinaryBufferIndex = 0;
        c = inavBinaryBuffer[0];  // Fall through to NMEA parsing
      } else if (inavBinaryBufferIndex >= 6) {
        // We have at least: sync(2) + class(1) + id(1) + len(2)
        uint16_t payloadLen = inavBinaryBuffer[4] | (inavBinaryBuffer[5] << 8);
        uint16_t totalLen = payloadLen + 8;  // sync(2) + class(1) + id(1) + len(2) + payload + ck(2)
        
        if (inavBinaryBufferIndex >= totalLen) {
          // Complete message received
          Serial.print("iNav MON-VER REQUEST: ");
          for (int i = 0; i < inavBinaryBufferIndex && i < 12; i++) {
            Serial.print("0x");
            if (inavBinaryBuffer[i] < 0x10) Serial.print("0");
            Serial.print(inavBinaryBuffer[i], HEX);
            Serial.print(" ");
          }
          Serial.println();
          
          procesarComandoBinarioINav(inavBinaryBuffer, inavBinaryBufferIndex);
          inavBinaryMode = false;
          inavBinaryBufferIndex = 0;
        }
      }
    } else if (!inavBinaryMode) {
      // NMEA-style command
      if (c == '$') {
        inavCmdBufferIndex = 0;
        inavCmdBuffer[inavCmdBufferIndex++] = c;
      } else if ((c == '\n' || c == '\r') && inavCmdBufferIndex > 0) {
        inavCmdBuffer[inavCmdBufferIndex] = '\0';
        Serial.print("iNav CMD COMPLETE: ");
        Serial.println(inavCmdBuffer);
        procesarComandoINav(inavCmdBuffer);
        inavCmdBufferIndex = 0;
      } else if (inavCmdBufferIndex > 0 && inavCmdBufferIndex < NMEA_BUFFER_SIZE - 1) {
        inavCmdBuffer[inavCmdBufferIndex++] = c;
      }
    }
  }
  
  // Read GPS data from UART1 - Non-blocking, with timeout protection
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
  if (inavInitialized && currentTime >= inavStreamStartAt && currentTime - lastUpdateTime >= updateRateMs) {
    lastUpdateTime = currentTime;
    enviarDatosAiNav();
  }

  updateStatusLed(converter.getPVTData(), currentTime);
  
  // Print debug info periodically
  if (currentTime - lastDebugPrint >= 5000) {  // Every 5 seconds
    lastDebugPrint = currentTime;
    printDebugInfo();
  }
  
  // Minimal delay to prevent watchdog timeout
  delay(LOOP_DELAY_MS);
}

void procesarNMEA(const char* sentence) {
  detectarIdentidadGPS(sentence);

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
      Serial.print(gga.fixQuality);
      Serial.println(" ✓");
      
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
      Serial.print(rmc.status);
      Serial.println(" ✓");
      
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
      Serial.print(gsa.vdop);
      Serial.println(" ✓");
      
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

void detectarIdentidadGPS(const char* sentence) {
  if (!sentence || sentence[0] != '$') {
    return;
  }
  if (!NMEAParser::validateChecksum(sentence)) {
    return;
  }

  char upper[128];
  int j = 0;
  for (int i = 0; sentence[i] != '\0' && sentence[i] != '*' && j < (int)(sizeof(upper) - 1); i++) {
    char c = sentence[i];
    if (c >= 'a' && c <= 'z') {
      c = (char)(c - ('a' - 'A'));
    }
    upper[j++] = c;
  }
  upper[j] = '\0';

  const char* model = nullptr;
  const char* fw = nullptr;

  if (strstr(upper, "LD-29") != nullptr || strstr(upper, "LD29") != nullptr) {
    model = "LD-29";
    fw = "FWVER=LD-29 NMEA";
  } else if (strstr(upper, "AK721-JM") != nullptr || strstr(upper, "AK721") != nullptr) {
    model = "AK721-JM";
    fw = "FWVER=AK721-JM NMEA";
  } else if (strstr(upper, "BK1662G") != nullptr || strstr(upper, "BK1662") != nullptr) {
    model = "BK1662G";
    fw = "FWVER=BK1662G NMEA";
  }

  if (model != nullptr) {
    if (strcmp(gpsModelLabel, model) == 0) {
      return;
    }
    strncpy(gpsModelLabel, model, sizeof(gpsModelLabel) - 1);
    gpsModelLabel[sizeof(gpsModelLabel) - 1] = '\0';
    strncpy(gpsFwExtLabel, fw, sizeof(gpsFwExtLabel) - 1);
    gpsFwExtLabel[sizeof(gpsFwExtLabel) - 1] = '\0';

    Serial.print("Detected GPS identity from NMEA: ");
    Serial.println(gpsModelLabel);
  }
}

static void enviarUBX(uint8_t msgClass, uint8_t msgId, const uint8_t* payload, uint16_t payloadLen) {
  uint8_t frame[128];
  if ((uint16_t)(payloadLen + 8) > sizeof(frame)) {
    return;
  }

  int idx = 0;
  frame[idx++] = 0xB5;
  frame[idx++] = 0x62;
  frame[idx++] = msgClass;
  frame[idx++] = msgId;
  frame[idx++] = (uint8_t)(payloadLen & 0xFF);
  frame[idx++] = (uint8_t)((payloadLen >> 8) & 0xFF);
  for (uint16_t i = 0; i < payloadLen; i++) {
    frame[idx++] = payload[i];
  }

  uint8_t ckA = 0;
  uint8_t ckB = 0;
  for (int i = 2; i < idx; i++) {
    ckA += frame[i];
    ckB += ckA;
  }
  frame[idx++] = ckA;
  frame[idx++] = ckB;

  Serial2.write(frame, idx);
}

static void writeU2LE(uint8_t* out, int offset, uint16_t value) {
  out[offset] = (uint8_t)(value & 0xFF);
  out[offset + 1] = (uint8_t)((value >> 8) & 0xFF);
}

static void writeU4LE(uint8_t* out, int offset, uint32_t value) {
  out[offset] = (uint8_t)(value & 0xFF);
  out[offset + 1] = (uint8_t)((value >> 8) & 0xFF);
  out[offset + 2] = (uint8_t)((value >> 16) & 0xFF);
  out[offset + 3] = (uint8_t)((value >> 24) & 0xFF);
}

static void writeI4LE(uint8_t* out, int offset, int32_t value) {
  writeU4LE(out, offset, (uint32_t)value);
}

static void writeI2LE(uint8_t* out, int offset, int16_t value) {
  out[offset] = (uint8_t)(value & 0xFF);
  out[offset + 1] = (uint8_t)((value >> 8) & 0xFF);
}

void enviarMensajesLegacyAiNav(const NAVPVT& pvt) {
  // NAV-POSLLH (0x01 0x02)
  uint8_t posllh[28] = {0};
  writeU4LE(posllh, 0, pvt.iTOW);
  writeI4LE(posllh, 4, pvt.lon);
  writeI4LE(posllh, 8, pvt.lat);
  writeI4LE(posllh, 12, pvt.height);
  writeI4LE(posllh, 16, pvt.hMSL);
  writeU4LE(posllh, 20, pvt.hAcc);
  writeU4LE(posllh, 24, pvt.vAcc);
  enviarUBX(0x01, 0x02, posllh, sizeof(posllh));

  // NAV-STATUS (0x01 0x03)
  uint8_t status[16] = {0};
  writeU4LE(status, 0, pvt.iTOW);
  status[4] = (pvt.fixType >= 2) ? pvt.fixType : 0;
  status[5] = (pvt.fixType >= 2) ? 0x01 : 0x00; // gpsFixOK
  writeU4LE(status, 8, 1000); // ttff (dummy non-zero)
  enviarUBX(0x01, 0x03, status, sizeof(status));

  // NAV-SOL (0x01 0x06)
  uint8_t sol[52] = {0};
  writeU4LE(sol, 0, pvt.iTOW);
  sol[10] = (pvt.fixType >= 2) ? pvt.fixType : 0;
  sol[11] = (pvt.fixType >= 2) ? 0x01 : 0x00; // GPS fix ok
  writeU4LE(sol, 24, pvt.hAcc);
  writeU4LE(sol, 40, pvt.sAcc);
  writeU2LE(sol, 44, pvt.pDOP);
  sol[47] = pvt.numSV; // satellites used
  enviarUBX(0x01, 0x06, sol, sizeof(sol));

  // NAV-DOP (0x01 0x04)
  uint8_t dop[18] = {0};
  const uint16_t pDop = (pvt.pDOP > 0) ? pvt.pDOP : 150;
  const uint16_t hDop = (pDop > 0) ? pDop : 150;
  const uint16_t vDop = (pDop > 0) ? pDop : 200;
  writeU4LE(dop, 0, pvt.iTOW);
  writeU2LE(dop, 4, pDop); // gDOP
  writeU2LE(dop, 6, pDop); // pDOP
  writeU2LE(dop, 8, pDop); // tDOP
  writeU2LE(dop, 10, vDop); // vDOP
  writeU2LE(dop, 12, hDop); // hDOP
  writeU2LE(dop, 14, hDop); // nDOP
  writeU2LE(dop, 16, hDop); // eDOP
  enviarUBX(0x01, 0x04, dop, sizeof(dop));

  // NAV-SAT (0x01 0x35) - minimal satellite list so FC can populate SATS
  uint8_t sat[104] = {0}; // header(8) + 8 blocks * 12
  const uint8_t satCount = (pvt.numSV > 8) ? 8 : (pvt.numSV == 0 ? 1 : pvt.numSV);
  writeU4LE(sat, 0, pvt.iTOW);
  sat[5] = 0x01; // version
  sat[6] = satCount;
  sat[7] = 12; // bytes per block
  int off = 8;
  for (uint8_t i = 0; i < satCount; i++) {
    sat[off + 0] = 0;                 // gnssId = GPS
    sat[off + 1] = (uint8_t)(10 + i); // svId
    sat[off + 2] = 35;                // cno dBHz
    sat[off + 3] = 45;                // elev deg
    writeU2LE(sat, off + 4, 18000);   // azim (deg * 100)
    writeI2LE(sat, off + 6, 0);       // prRes cm
    writeU4LE(sat, off + 8, 0x00000019); // quality + used flags
    off += 12;
  }
  const uint16_t satPayloadLen = (uint16_t)(8 + (satCount * 12));
  enviarUBX(0x01, 0x35, sat, satPayloadLen);
}

void enviarDatosAiNav() {
  NAVPVT pvt = converter.getPVTData();
  
  // DEBUG: Check pvtData before sending
  if ((stats.ubloxFrames % 50) == 0) {
    Serial.print("DEBUG PVT: fixType=");
    Serial.print(pvt.fixType);
    Serial.print(" numSV=");
    Serial.print(pvt.numSV);
    Serial.print(" lat=");
    Serial.print(pvt.lat);
    Serial.print(" lon=");
    Serial.println(pvt.lon);
  }

  int length = 0;
  if (converter.buildNAVPVT(ubloxBuffer, length)) {
    Serial2.write(ubloxBuffer, length);
    enviarMensajesLegacyAiNav(pvt);
    
    // Debug: print hex dump every 10 frames
    if ((stats.ubloxFrames % 10) == 0) {
      Serial.print("SENDING u-blox (");
      Serial.print(length);
      Serial.print(" bytes): ");
      for (int i = 0; i < length && i < 24; i++) {  // Print first 24 bytes
        Serial.print("0x");
        if (ubloxBuffer[i] < 0x10) Serial.print("0");
        Serial.print(ubloxBuffer[i], HEX);
        Serial.print(" ");
      }
      Serial.println();
      
      Serial.print("  Lat=");
      Serial.print(pvt.lat / 10000000.0, 6);
      Serial.print(" Lon=");
      Serial.print(pvt.lon / 10000000.0, 6);
      Serial.print(" Fix=");
      Serial.print(pvt.fixType);
      Serial.print(" Sats=");
      Serial.println(pvt.numSV);
    }
  }
}

void initializeStatusLed() {
#if FEATURE_RGB_STATUS
  #ifdef RGB_BUILTIN
    pinMode(RGB_BUILTIN, OUTPUT);
    statusLedAvailable = true;
  #elif (RGB_WS_PIN >= 0)
    pinMode(RGB_WS_PIN, OUTPUT);
    statusLedAvailable = true;
  #elif (RGB_LED_R_PIN >= 0) && (RGB_LED_G_PIN >= 0) && (RGB_LED_B_PIN >= 0)
    pinMode(RGB_LED_R_PIN, OUTPUT);
    pinMode(RGB_LED_G_PIN, OUTPUT);
    pinMode(RGB_LED_B_PIN, OUTPUT);
    statusLedAvailable = true;
  #endif
#endif
  setStatusLedColor(0, 0, 0);
}

void setStatusLedColor(uint8_t r, uint8_t g, uint8_t b) {
  if (!statusLedAvailable) {
    return;
  }

#if FEATURE_RGB_STATUS
  #ifdef RGB_BUILTIN
    neopixelWrite(RGB_BUILTIN, r, g, b);
  #elif (RGB_WS_PIN >= 0)
    neopixelWrite(RGB_WS_PIN, r, g, b);
  #elif (RGB_LED_R_PIN >= 0) && (RGB_LED_G_PIN >= 0) && (RGB_LED_B_PIN >= 0)
    const uint8_t rOut = RGB_LED_ACTIVE_HIGH ? r : (uint8_t)(255 - r);
    const uint8_t gOut = RGB_LED_ACTIVE_HIGH ? g : (uint8_t)(255 - g);
    const uint8_t bOut = RGB_LED_ACTIVE_HIGH ? b : (uint8_t)(255 - b);
    analogWrite(RGB_LED_R_PIN, rOut);
    analogWrite(RGB_LED_G_PIN, gOut);
    analogWrite(RGB_LED_B_PIN, bOut);
  #endif
#endif
}

void updateStatusLed(const NAVPVT& pvt, unsigned long nowMs) {
  const bool hasFix = (pvt.fixType >= 2) && (pvt.numSV > 0);

  if (hasFix) {
    setStatusLedColor(0, 24, 0); // Green (fixed)
    return;
  }

  if (nowMs - ledBlinkTimestamp >= 350) {
    ledBlinkTimestamp = nowMs;
    ledBlinkState = !ledBlinkState;
  }

  if (ledBlinkState) {
    setStatusLedColor(24, 0, 0); // Red blink (no fix)
  } else {
    setStatusLedColor(0, 0, 0);
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

void procesarComandoBinarioINav(uint8_t* buffer, int length) {
  // Parse u-blox binary message: sync(2) + class(1) + id(1) + len(2) + payload(...) + ck_a(1) + ck_b(1)
  if (length < 8) return;
  
  uint8_t messageClass = buffer[2];
  uint8_t messageId = buffer[3];
  uint16_t payloadLen = (uint16_t)buffer[4] | ((uint16_t)buffer[5] << 8);
  const uint8_t* payload = (payloadLen > 0 && length >= (int)(payloadLen + 8)) ? (buffer + 6) : nullptr;
  
  Serial.print("  Binary u-blox: Class=0x");
  Serial.print(messageClass, HEX);
  Serial.print(" ID=0x");
  Serial.println(messageId, HEX);
  
  // MON-VER request (class 0x0A, ID 0x04)
  if (messageClass == 0x0A && messageId == 0x04) {
    Serial.println("  → iNav requesting MON-VER (version info)");
    responderMONVER();
    return;
  }
  if (messageClass == 0x0A && messageId == 0x28) {
    Serial.println("  → iNav requesting MON-GNSS");
    responderMONGNSS();
    return;
  }

  // ACK any CFG message from iNav to keep the handshake moving
  if (messageClass == 0x06) {
    if (messageId == 0x08 && payload != nullptr) { // CFG-RATE
      procesarCFGRATE(payload, payloadLen);
    }
    Serial.println("  → iNav CFG message, sending UBX-ACK-ACK");
    responderACKUBX(messageClass, messageId);
  }
}

void procesarCFGRATE(const uint8_t* payload, uint16_t payloadLen) {
  if (!payload || payloadLen < 6) {
    return;
  }

  uint16_t measRate = (uint16_t)payload[0] | ((uint16_t)payload[1] << 8); // ms
  if (measRate < 50) {
    measRate = 50;
  } else if (measRate > 1000) {
    measRate = 1000;
  }

  updateRateMs = measRate;
  Serial.print("  → CFG-RATE applied: ");
  Serial.print(updateRateMs);
  Serial.println(" ms");
}

void responderMONGNSS() {
  // UBX-MON-GNSS payload (8 bytes, simplified but valid):
  // version, supported, defaultGnss, enabled, simultaneous, reserved[3]
  uint8_t payload[8] = {
    0x00, // version
    0x07, // supported: GPS/GAL/BDS
    0x01, // default GNSS: GPS
    0x07, // enabled: GPS/GAL/BDS
    0x03, // simultaneous
    0x00, 0x00, 0x00
  };
  enviarUBX(0x0A, 0x28, payload, sizeof(payload));
}

void procesarComandoINav(const char* command) {
  // NMEA-style commands from iNav
  if (strstr(command, "PUBX,41") != NULL) {
    Serial.println("  → iNav requesting port configuration");
    // Echo back the PUBX,41 command to confirm
    const char* response = "$PUBX,41,1,0003,0001,115200,0*1E\r\n";
    Serial.print("  SENDING NMEA response: ");
    Serial.print(response);
    Serial2.print(response);
    Serial2.flush();
  }
}

void responderACKUBX(uint8_t cls, uint8_t id) {
  uint8_t ack[10];
  int idx = 0;

  ack[idx++] = 0xB5;
  ack[idx++] = 0x62;
  ack[idx++] = 0x05;  // ACK class
  ack[idx++] = 0x01;  // ACK-ACK
  ack[idx++] = 0x02;  // length LSB
  ack[idx++] = 0x00;  // length MSB
  ack[idx++] = cls;   // acknowledged class
  ack[idx++] = id;    // acknowledged ID

  uint8_t ckA = 0;
  uint8_t ckB = 0;
  for (int i = 2; i < idx; i++) {
    ckA += ack[i];
    ckB += ckA;
  }

  ack[idx++] = ckA;
  ack[idx++] = ckB;

  Serial2.write(ack, idx);
}

void responderMONVER() {
  // Build an extended MON-VER frame like a real modern u-blox.
  // 40-byte base + 7 extension strings (30 bytes each) = 250 bytes payload.
  const uint16_t payloadLen = 250;
  uint8_t response[2 + 1 + 1 + 2 + payloadLen + 2];
  int idx = 0;

  response[idx++] = 0xB5;
  response[idx++] = 0x62;
  response[idx++] = 0x0A;
  response[idx++] = 0x04;
  response[idx++] = (uint8_t)(payloadLen & 0xFF);
  response[idx++] = (uint8_t)((payloadLen >> 8) & 0xFF);

  char sw[31];
  snprintf(sw, sizeof(sw), "EXT %s NMEA bridge", gpsModelLabel);
  const char* hw = "000A0000";
  const char* ext[7] = {
    "ROM BASE 0x6C1C37B0",
    gpsFwExtLabel,
    "PROTVER=40.00",
    "MOD=NMEA-BRIDGE",
    "GPS;GAL;BDS",
    "SBAS;QZSS",
    "NAVIC"
  };

  const size_t swLen = strlen(sw);
  const size_t hwLen = strlen(hw);
  for (int i = 0; i < 30; i++) {
    response[idx++] = (i < (int)swLen) ? (uint8_t)sw[i] : 0x00;
  }
  for (int i = 0; i < 10; i++) {
    response[idx++] = (i < (int)hwLen) ? (uint8_t)hw[i] : 0x00;
  }
  for (int e = 0; e < 7; e++) {
    const size_t extLen = strlen(ext[e]);
    for (int i = 0; i < 30; i++) {
      response[idx++] = (i < (int)extLen) ? (uint8_t)ext[e][i] : 0x00;
    }
  }

  uint8_t CK_A = 0;
  uint8_t CK_B = 0;
  for (int i = 2; i < idx; i++) {
    CK_A += response[i];
    CK_B += CK_A;
  }

  response[idx++] = CK_A;
  response[idx++] = CK_B;

  Serial.print("  SENDING MON-VER: ");
  for (int i = 0; i < idx && i < 20; i++) {
    Serial.print("0x");
    if (response[i] < 0x10) Serial.print("0");
    Serial.print(response[i], HEX);
    Serial.print(" ");
  }
  Serial.println("...");
  
  Serial2.write(response, idx);
  Serial2.flush();
}
