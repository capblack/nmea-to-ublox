// NMEA0183 Parser for GPS data
// Parses GGA, RMC, GSA sentences

#ifndef NMEA_PARSER_H
#define NMEA_PARSER_H

#include <string.h>
#include <stdlib.h>
#include <math.h>

// NMEA sentence types
enum NMEASentenceType {
  NMEA_UNKNOWN,
  NMEA_GGA,
  NMEA_RMC,
  NMEA_GSA
};

// GGA - Global Positioning System Fix Data
struct GGAData {
  char utcTime[10];        // hhmmss.ss
  float latitude;          // degrees
  char latDir;             // N/S
  float longitude;         // degrees
  char lonDir;             // E/W
  int fixQuality;          // 0=invalid, 1=GPS, 2=DGPS
  int numSatellites;       // satellites used
  float hdop;              // horizontal dilution of precision
  float altitude;          // meters above mean sea level
  float geoidHeight;       // geoid height
  int ageOfDgpsData;       // age of DGPS data
};

// RMC - Recommended Minimum Navigation Information
struct RMCData {
  char utcTime[10];        // hhmmss.ss
  char status;             // A=active, V=void
  float latitude;          // degrees
  char latDir;             // N/S
  float longitude;         // degrees
  char lonDir;             // E/W
  float speedKnots;        // speed in knots
  float trackTrue;         // track angle in degrees
  char date[7];            // ddmmyy
};

// GSA - GPS DOP and Active Satellites
struct GSAData {
  char selectionMode;      // M=manual, A=automatic
  int fixType;             // 1=no fix, 2=2D, 3=3D
  int satUsed[12];         // satellite IDs used for fix
  float pdop;              // position dilution of precision
  float hdop;              // horizontal dilution of precision
  float vdop;              // vertical dilution of precision
};

class NMEAParser {
public:
  NMEAParser();
  
  // Parse NMEA sentence
  bool parseSentence(const char* sentence);
  
  // Get parsed data
  GGAData getGGAData() const { return ggaData; }
  RMCData getRMCData() const { return rmcData; }
  GSAData getGSAData() const { return gsaData; }
  
  // Get sentence type of last parsed sentence
  NMEASentenceType getLastSentenceType() const { return lastSentenceType; }
  
  // Checksum validation
  static bool validateChecksum(const char* sentence);
  
private:
  GGAData ggaData;
  RMCData rmcData;
  GSAData gsaData;
  NMEASentenceType lastSentenceType;
  
  // Parsing functions
  bool parseGGA(const char* sentence);
  bool parseRMC(const char* sentence);
  bool parseGSA(const char* sentence);
  
  // Helper functions
  static int getFieldCount(const char* sentence);
  static bool getField(const char* sentence, int fieldNum, char* buffer, int bufferSize);
  static float convertDMtoDegrees(float dmValue);
  static unsigned char calculateChecksum(const char* sentence);
};

#endif // NMEA_PARSER_H