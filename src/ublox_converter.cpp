#include "ublox_converter.h"
#include <string.h>
#include <math.h>

UBLOXConverter::UBLOXConverter() {
  memset(&pvtData, 0, sizeof(NAVPVT));
  pvtData.iTOW = 0;
  pvtData.valid = 0x07; // Time and date valid
  pvtData.fixType = 0;  // No fix initially
  pvtData.flags = 0;
}

bool UBLOXConverter::convertGGAToPVT(const GGAData& gga, uint8_t* buffer, int& length) {
  if (!buffer || gga.fixQuality == 0) {
    return false;
  }
  
  // Set fix type based on GGA fix quality
  switch (gga.fixQuality) {
    case 1:
      pvtData.fixType = 3; // GPS fix (3D)
      break;
    case 2:
      pvtData.fixType = 3; // DGPS fix
      break;
    default:
      pvtData.fixType = 0; // No fix
      return false;
  }
  
  // Convert latitude and longitude to int32
  double lat = gga.latitude;
  double lon = gga.longitude;
  
  if (gga.latDir == 'S') lat = -lat;
  if (gga.lonDir == 'W') lon = -lon;
  
  pvtData.lat = (int32_t)(lat * 10000000.0);  // degrees * 1e-7
  pvtData.lon = (int32_t)(lon * 10000000.0);
  
  // Altitude
  pvtData.height = (int32_t)(gga.altitude * 1000.0); // meters to mm
  pvtData.hMSL = (int32_t)(gga.geoidHeight * 1000.0);
  
  // Number of satellites
  pvtData.numSV = gga.numSatellites;
  
  // Accuracy estimates (based on HDOP)
  uint32_t accuracy = (uint32_t)(gga.hdop * 1000.0); // mm
  pvtData.hAcc = accuracy;
  pvtData.vAcc = accuracy * 2; // Vertical is typically worse
  
  // Time
  parseTimeString(gga.utcTime, pvtData.hour, pvtData.min, pvtData.sec);
  
  pvtData.flags = 0x01; // gnssFixOK
  pvtData.valid = 0x07; // Time, date, and fix are valid
  
  return buildNAVPVT(buffer, length);
}

bool UBLOXConverter::convertRMCToPVT(const RMCData& rmc, uint8_t* buffer, int& length) {
  if (!buffer || rmc.status != 'A') {
    return false;
  }
  
  // RMC is active
  pvtData.fixType = 2; // At least 2D fix from RMC
  
  // Convert latitude and longitude
  double lat = rmc.latitude;
  double lon = rmc.longitude;
  
  if (rmc.latDir == 'S') lat = -lat;
  if (rmc.lonDir == 'W') lon = -lon;
  
  pvtData.lat = (int32_t)(lat * 10000000.0);
  pvtData.lon = (int32_t)(lon * 10000000.0);
  
  // Speed (knots to mm/s: 1 knot = 514.444 mm/s)
  pvtData.gSpeed = (int32_t)(rmc.speedKnots * 514.444);
  
  // Heading
  pvtData.heading = (int32_t)(rmc.trackTrue * 100000.0);
  
  // Time and date
  parseTimeString(rmc.utcTime, pvtData.hour, pvtData.min, pvtData.sec);
  parseDateString(rmc.date, pvtData.year, pvtData.month, pvtData.day);
  
  pvtData.flags = 0x01; // gnssFixOK
  pvtData.valid = 0x07;
  
  return buildNAVPVT(buffer, length);
}

bool UBLOXConverter::convertGSAToPVT(const GSAData& gsa, uint8_t* buffer, int& length) {
  if (!buffer) {
    return false;
  }
  
  // Update fix type from GSA
  pvtData.fixType = gsa.fixType;
  
  // Set DOP values (1/100 format)
  pvtData.pDOP = (uint16_t)(gsa.pdop * 100.0);
  
  // Store in NAV-PVT structure (hAcc uses HDOP)
  uint32_t hAccuracy = (uint32_t)(gsa.hdop * 1000.0);
  pvtData.hAcc = hAccuracy;
  
  // VDOP for vertical accuracy
  uint32_t vAccuracy = (uint32_t)(gsa.vdop * 1000.0);
  pvtData.vAcc = vAccuracy;
  
  return buildNAVPVT(buffer, length);
}

bool UBLOXConverter::buildNAVPVT(uint8_t* buffer, int& length) {
  if (!buffer) {
    return false;
  }
  
  // UBX frame structure:
  // Sync1 (0xB5), Sync2 (0x62), Class, ID, Length(L), Length(H), Payload, CK_A, CK_B
  
  int payloadLength = sizeof(NAVPVT);
  int messageLength = payloadLength + 8; // +8 for sync, class, id, length, checksums
  
  int idx = 0;
  buffer[idx++] = 0xB5;           // Sync char 1
  buffer[idx++] = 0x62;           // Sync char 2
  buffer[idx++] = UBX_CLASS_NAV;  // Class
  buffer[idx++] = UBX_NAV_PVT;    // Message ID
  buffer[idx++] = payloadLength & 0xFF;        // Length low byte
  buffer[idx++] = (payloadLength >> 8) & 0xFF; // Length high byte
  
  // Copy payload
  uint8_t* payloadPtr = (uint8_t*)&pvtData;
  memcpy(buffer + idx, payloadPtr, payloadLength);
  idx += payloadLength;
  
  // Calculate checksums
  uint8_t ckA = 0, ckB = 0;
  calculateChecksum(buffer + 2, idx - 2, ckA, ckB);
  
  buffer[idx++] = ckA;
  buffer[idx++] = ckB;
  
  length = idx;
  return true;
}

void UBLOXConverter::calculateChecksum(uint8_t* data, int length, uint8_t& ckA, uint8_t& ckB) {
  ckA = 0;
  ckB = 0;
  
  for (int i = 0; i < length; i++) {
    ckA += data[i];
    ckB += ckA;
  }
}

int32_t UBLOXConverter::degreesToINT32(double degrees) {
  return (int32_t)(degrees * 10000000.0);
}

void UBLOXConverter::parseTimeString(const char* timeStr, uint8_t& hour, uint8_t& min, uint8_t& sec) {
  if (!timeStr || strlen(timeStr) < 6) {
    hour = min = sec = 0;
    return;
  }
  
  char buffer[3];
  strncpy(buffer, timeStr, 2);
  buffer[2] = '\0';
  hour = atoi(buffer);
  
  strncpy(buffer, timeStr + 2, 2);
  buffer[2] = '\0';
  min = atoi(buffer);
  
  strncpy(buffer, timeStr + 4, 2);
  buffer[2] = '\0';
  sec = atoi(buffer);
}

void UBLOXConverter::parseDateString(const char* dateStr, uint16_t& year, uint8_t& month, uint8_t& day) {
  if (!dateStr || strlen(dateStr) < 6) {
    year = 2000;
    month = day = 1;
    return;
  }
  
  char buffer[3];
  strncpy(buffer, dateStr, 2);
  buffer[2] = '\0';
  day = atoi(buffer);
  
  strncpy(buffer, dateStr + 2, 2);
  buffer[2] = '\0';
  month = atoi(buffer);
  
  strncpy(buffer, dateStr + 4, 2);
  buffer[2] = '\0';
  int yearSuffix = atoi(buffer);
  year = 2000 + yearSuffix;
}
