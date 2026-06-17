// u-blox Protocol Converter
// Converts NMEA data to u-blox binary protocol (UBX)

#ifndef UBLOX_CONVERTER_H
#define UBLOX_CONVERTER_H

#include <stdint.h>
#include "nmea_parser.h"

// u-blox message classes
#define UBX_CLASS_NAV 0x01
#define UBX_CLASS_RXM 0x02
#define UBX_CLASS_INF 0x04
#define UBX_CLASS_ACK 0x05
#define UBX_CLASS_CFG 0x06
#define UBX_CLASS_UPD 0x09
#define UBX_CLASS_MON 0x0A
#define UBX_CLASS_AID 0x0B
#define UBX_CLASS_TIM 0x0D
#define UBX_CLASS_ESF 0x10
#define UBX_CLASS_MGA 0x13
#define UBX_CLASS_LOG 0x21
#define UBX_CLASS_SEC 0x27
#define UBX_CLASS_HNR 0x28

// u-blox NAV message IDs
#define UBX_NAV_PVT   0x07  // Navigation Position Velocity Time
#define UBX_NAV_STATUS 0x03 // Navigation Status
#define UBX_NAV_POSLLH 0x02 // Position in LLH coordinates

// u-blox message structure
struct UBXMessage {
  uint8_t sync1;      // 0xB5
  uint8_t sync2;      // 0x62
  uint8_t msgClass;
  uint8_t msgID;
  uint16_t length;    // little-endian
  uint8_t* payload;
  uint8_t ckA;
  uint8_t ckB;
};

// NAV-PVT payload structure (minimum required)
struct __attribute__((packed)) NAVPVT {
  uint32_t iTOW;      // GPS time of week (ms)
  uint16_t year;      // Year (1999..2099)
  uint8_t month;      // Month (1..12)
  uint8_t day;        // Day (1..31)
  uint8_t hour;       // Hour (0..23)
  uint8_t min;        // Minute (0..59)
  uint8_t sec;        // Second (0..59)
  uint8_t valid;      // Validity flags
  uint32_t tAcc;      // Time accuracy estimate (ns)
  int32_t nano;       // Nanoseconds of second
  uint8_t fixType;    // GNSS fix type: 0=no fix, 1=deadreckoning, 2=2D, 3=3D
  uint8_t flags;      // Fix status flags
  uint8_t reserved1;
  uint8_t numSV;      // Number of satellites
  int32_t lon;        // Longitude (deg * 1e-7)
  int32_t lat;        // Latitude (deg * 1e-7)
  int32_t height;     // Height above ellipsoid (mm)
  int32_t hMSL;       // Height above mean sea level (mm)
  uint32_t hAcc;      // Horizontal accuracy estimate (mm)
  uint32_t vAcc;      // Vertical accuracy estimate (mm)
  int32_t velN;       // NED north velocity (mm/s)
  int32_t velE;       // NED east velocity (mm/s)
  int32_t velD;       // NED down velocity (mm/s)
  int32_t gSpeed;     // Ground speed (mm/s)
  int32_t heading;    // Heading of motion 2D (deg * 1e-5)
  uint32_t sAcc;      // Speed accuracy estimate (mm/s)
  uint32_t headingAcc;// Heading accuracy estimate (deg * 1e-5)
  uint16_t pDOP;      // Position DOP (1/100)
  uint8_t reserved2[6];
  int32_t headVeh;    // Heading of vehicle 2D (deg * 1e-5)
  int16_t magDec;     // Magnetic declination (deg * 1e-2)
  uint16_t magAcc;    // Magnetic declination accuracy (deg * 1e-2)
};

class UBLOXConverter {
public:
  UBLOXConverter();
  
  // Convert NMEA GGA data to u-blox NAV-PVT
  bool convertGGAToPVT(const GGAData& gga, uint8_t* buffer, int& length);
  
  // Convert NMEA RMC data to u-blox NAV-PVT
  bool convertRMCToPVT(const RMCData& rmc, uint8_t* buffer, int& length);
  
  // Convert NMEA GSA data to u-blox NAV-PVT (DOP values)
  bool convertGSAToPVT(const GSAData& gsa, uint8_t* buffer, int& length);
  
  // Build complete NAV-PVT message
  bool buildNAVPVT(uint8_t* buffer, int& length);
  
  // Get current PVT data
  NAVPVT getPVTData() const { return pvtData; }
  
private:
  NAVPVT pvtData;
  
  // Helper functions
  static void calculateChecksum(uint8_t* message, int length, uint8_t& ckA, uint8_t& ckB);
  static int32_t degreesToINT32(double degrees);
  static uint32_t getTimeOfWeek(const char* utcTime, const char* date);
  static void parseTimeString(const char* timeStr, uint8_t& hour, uint8_t& min, uint8_t& sec);
  static void parseDateString(const char* dateStr, uint16_t& year, uint8_t& month, uint8_t& day);
};

#endif // UBLOX_CONVERTER_H