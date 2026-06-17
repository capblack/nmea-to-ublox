// Configuration file for NMEA to u-blox translator

#ifndef CONFIG_H
#define CONFIG_H

// ===== UART Configuration =====
// ESP32-C3 UART pins
#define GPS_RX_PIN    20  // GPIO20 - RX from GPS (NMEA input)
#define GPS_TX_PIN    21  // GPIO21 - TX to GPS (optional)

#define INAV_RX_PIN   8   // GPIO8 - RX from iNav (optional, for feedback)
#define INAV_TX_PIN   9   // GPIO9 - TX to iNav (u-blox output)

// Baud rates
#define GPS_BAUD_RATE    9600   // AK721-JM default baud rate
#define INAV_BAUD_RATE   115200 // iNav typical baud rate

// UART hardware assignments
#define GPS_UART   UART_NUM_0   // UART0 for GPS input
#define INAV_UART  UART_NUM_1   // UART1 for iNav output

// ===== Buffer Configuration =====
#define NMEA_BUFFER_SIZE   128   // NMEA sentence max length
#define UBLOX_BUFFER_SIZE  256   // u-blox message max length
#define RX_BUFFER_SIZE     256   // Serial RX buffer

// ===== Timing Configuration =====
#define NMEA_TIMEOUT_MS    2000  // Timeout for NMEA sentence
#define UPDATE_RATE_MS     100   // Output update rate (100ms = 10Hz)
#define LOOP_DELAY_MS      10    // Main loop delay

// ===== Debug Configuration =====
#define ENABLE_DEBUG       1     // Enable serial debug output
#define DEBUG_BAUD_RATE    115200 // Debug serial baud rate
#define DEBUG_RX_PIN       -1    // Not used
#define DEBUG_TX_PIN       2     // GPIO2 - TX for debug (USB)

// ===== GPS Configuration =====
#define GPS_MODEL          "AK721-JM"
#define NMEA_VARIANT       "NMEA0183"

// NMEA sentences to expect
#define EXPECT_GGA         1
#define EXPECT_RMC         1
#define EXPECT_GSA         1

// ===== iNav Configuration =====
#define INAV_VERSION       "9.1"
#define INAV_GPS_PROTOCOL  2     // 0=NMEA, 1=UBX, 2=AUTO

// ===== Feature Flags =====
#define FEATURE_CRC_CHECK  1     // Validate NMEA checksums
#define FEATURE_LOGGING    0     // Log all sentences (requires SPIFFS)
#define FEATURE_LED_STATUS 0     // Use LED for status indication
#define STATUS_LED_PIN     -1    // GPIO pin for status LED (unused)

#endif // CONFIG_H