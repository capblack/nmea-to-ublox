#include "nmea_parser.h"

NMEAParser::NMEAParser() {
  lastSentenceType = NMEA_UNKNOWN;
  memset(&ggaData, 0, sizeof(GGAData));
  memset(&rmcData, 0, sizeof(RMCData));
  memset(&gsaData, 0, sizeof(GSAData));
}

bool NMEAParser::parseSentence(const char* sentence) {
  if (!sentence || strlen(sentence) == 0) {
    return false;
  }
  
  // Validate checksum
  if (!validateChecksum(sentence)) {
    return false;
  }
  
  // Determine sentence type
  if (strstr(sentence, "$GPGGA") || strstr(sentence, "$GNGGA")) {
    lastSentenceType = NMEA_GGA;
    return parseGGA(sentence);
  } else if (strstr(sentence, "$GPRMC") || strstr(sentence, "$GNRMC")) {
    lastSentenceType = NMEA_RMC;
    return parseRMC(sentence);
  } else if (strstr(sentence, "$GPGSA") || strstr(sentence, "$GNGSA")) {
    lastSentenceType = NMEA_GSA;
    return parseGSA(sentence);
  }
  
  lastSentenceType = NMEA_UNKNOWN;
  return false;
}

bool NMEAParser::validateChecksum(const char* sentence) {
  if (!sentence || strlen(sentence) < 5) {
    return false;
  }
  
  // Find $ and * positions
  const char* start = strchr(sentence, '$');
  const char* checksumPos = strchr(sentence, '*');
  
  if (!start || !checksumPos) {
    return false;
  }
  
  // Calculate checksum
  unsigned char checksum = 0;
  for (const char* p = start + 1; p < checksumPos; p++) {
    checksum ^= *p;
  }
  
  // Compare with provided checksum
  unsigned char providedChecksum = strtol(checksumPos + 1, NULL, 16);
  return checksum == providedChecksum;
}

bool NMEAParser::parseGGA(const char* sentence) {
  char buffer[20];
  
  // UTC Time (field 1)
  if (!getField(sentence, 1, buffer, sizeof(buffer))) return false;
  strncpy(ggaData.utcTime, buffer, sizeof(ggaData.utcTime) - 1);
  
  // Latitude (field 2)
  if (!getField(sentence, 2, buffer, sizeof(buffer))) return false;
  ggaData.latitude = convertDMtoDegrees(atof(buffer));
  
  // Latitude direction (field 3)
  if (!getField(sentence, 3, buffer, sizeof(buffer))) return false;
  ggaData.latDir = buffer[0];
  
  // Longitude (field 4)
  if (!getField(sentence, 4, buffer, sizeof(buffer))) return false;
  ggaData.longitude = convertDMtoDegrees(atof(buffer));
  
  // Longitude direction (field 5)
  if (!getField(sentence, 5, buffer, sizeof(buffer))) return false;
  ggaData.lonDir = buffer[0];
  
  // Fix quality (field 6)
  if (!getField(sentence, 6, buffer, sizeof(buffer))) return false;
  ggaData.fixQuality = atoi(buffer);
  
  // Number of satellites (field 7)
  if (!getField(sentence, 7, buffer, sizeof(buffer))) return false;
  ggaData.numSatellites = atoi(buffer);
  
  // HDOP (field 8)
  if (!getField(sentence, 8, buffer, sizeof(buffer))) return false;
  ggaData.hdop = atof(buffer);
  
  // Altitude (field 9)
  if (!getField(sentence, 9, buffer, sizeof(buffer))) return false;
  ggaData.altitude = atof(buffer);
  
  // Geoid height (field 11)
  if (!getField(sentence, 11, buffer, sizeof(buffer))) return false;
  ggaData.geoidHeight = atof(buffer);
  
  return true;
}

bool NMEAParser::parseRMC(const char* sentence) {
  char buffer[20];
  
  // UTC Time (field 1)
  if (!getField(sentence, 1, buffer, sizeof(buffer))) return false;
  strncpy(rmcData.utcTime, buffer, sizeof(rmcData.utcTime) - 1);
  
  // Status (field 2)
  if (!getField(sentence, 2, buffer, sizeof(buffer))) return false;
  rmcData.status = buffer[0];
  
  // Latitude (field 3)
  if (!getField(sentence, 3, buffer, sizeof(buffer))) return false;
  rmcData.latitude = convertDMtoDegrees(atof(buffer));
  
  // Latitude direction (field 4)
  if (!getField(sentence, 4, buffer, sizeof(buffer))) return false;
  rmcData.latDir = buffer[0];
  
  // Longitude (field 5)
  if (!getField(sentence, 5, buffer, sizeof(buffer))) return false;
  rmcData.longitude = convertDMtoDegrees(atof(buffer));
  
  // Longitude direction (field 6)
  if (!getField(sentence, 6, buffer, sizeof(buffer))) return false;
  rmcData.lonDir = buffer[0];
  
  // Speed in knots (field 7)
  if (!getField(sentence, 7, buffer, sizeof(buffer))) return false;
  rmcData.speedKnots = atof(buffer);
  
  // Track angle (field 8)
  if (!getField(sentence, 8, buffer, sizeof(buffer))) return false;
  rmcData.trackTrue = atof(buffer);
  
  // Date (field 9)
  if (!getField(sentence, 9, buffer, sizeof(buffer))) return false;
  strncpy(rmcData.date, buffer, sizeof(rmcData.date) - 1);
  
  return true;
}

bool NMEAParser::parseGSA(const char* sentence) {
  char buffer[20];
  
  // Selection mode (field 1)
  if (!getField(sentence, 1, buffer, sizeof(buffer))) return false;
  gsaData.selectionMode = buffer[0];
  
  // Fix type (field 2)
  if (!getField(sentence, 2, buffer, sizeof(buffer))) return false;
  gsaData.fixType = atoi(buffer);
  
  // Satellite IDs (fields 3-14)
  for (int i = 0; i < 12; i++) {
    if (getField(sentence, 3 + i, buffer, sizeof(buffer)) && strlen(buffer) > 0) {
      gsaData.satUsed[i] = atoi(buffer);
    } else {
      gsaData.satUsed[i] = 0;
    }
  }
  
  // PDOP (field 15)
  if (!getField(sentence, 15, buffer, sizeof(buffer))) return false;
  gsaData.pdop = atof(buffer);
  
  // HDOP (field 16)
  if (!getField(sentence, 16, buffer, sizeof(buffer))) return false;
  gsaData.hdop = atof(buffer);
  
  // VDOP (field 17)
  if (!getField(sentence, 17, buffer, sizeof(buffer))) return false;
  gsaData.vdop = atof(buffer);
  
  return true;
}

bool NMEAParser::getField(const char* sentence, int fieldNum, char* buffer, int bufferSize) {
  if (!sentence || !buffer || bufferSize <= 0) {
    return false;
  }
  
  buffer[0] = '\0';
  int currentField = 0;
  int charIndex = 0;
  
  for (int i = 0; sentence[i] != '\0' && sentence[i] != '*'; i++) {
    if (sentence[i] == ',') {
      if (currentField == fieldNum) {
        buffer[charIndex] = '\0';
        return true;
      }
      currentField++;
      charIndex = 0;
    } else if (currentField == fieldNum) {
      if (charIndex < bufferSize - 1) {
        buffer[charIndex++] = sentence[i];
      }
    }
  }
  
  if (currentField == fieldNum) {
    buffer[charIndex] = '\0';
    return true;
  }
  
  return false;
}

float NMEAParser::convertDMtoDegrees(float dmValue) {
  int degrees = (int)(dmValue / 100.0);
  float minutes = dmValue - (degrees * 100.0);
  return degrees + (minutes / 60.0);
}
