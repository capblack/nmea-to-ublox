// Configuration file for NMEA to u-blox translator
// Auto-detecta el board seleccionado en Arduino IDE

#ifndef CONFIG_H
#define CONFIG_H

// ===== AUTO-DETECT BOARD =====
#ifdef ARDUINO_ESP32S3_DEV
  #define BOARD_NAME "ESP32-S3"
  #define BOARD_TYPE 1
#elif defined(ARDUINO_ESP32C3_DEV)
  #define BOARD_NAME "ESP32-C3"
  #define BOARD_TYPE 2
#else
  #error "Debes seleccionar: ESP32-S3-DevKit o ESP32-C3-DevKitM-1"
#endif

// ===== UART Configuration (Auto-detectado por board) =====

#if BOARD_TYPE == 1  // ESP32-S3 Mini/Dev
  // ESP32-S3 tiene 3 UARTs: UART0 (USB), UART1, UART2
  #define GPS_RX_PIN    16  // UART1 RX - GPS NMEA input
  #define GPS_TX_PIN    17  // UART1 TX - GPS NMEA output (optional)
  
  #define INAV_RX_PIN   19  // UART2 RX - iNav feedback (optional)
  #define INAV_TX_PIN   18  // UART2 TX - iNav u-blox output
  
  #define GPS_UART      UART_NUM_1   // UART1 for GPS
  #define INAV_UART     UART_NUM_2   // UART2 for iNav
  #define DEBUG_UART    UART_NUM_0   // UART0 for USB debug (automatic)
  
  #define BOARD_DESCRIPTION "ESP32-S3: 3 UARTs (Debug via USB, GPS, iNav)"

#elif BOARD_TYPE == 2  // ESP32-C3 Dev
  // ESP32-C3 tiene 2 UARTs: UART0 (USB), UART1
  // Usamos SoftwareSerial para la segunda UART
  #define GPS_RX_PIN    20  // UART1 RX - GPS NMEA input
  #define GPS_TX_PIN    21  // UART1 TX - GPS NMEA output (optional)
  
  #define INAV_RX_PIN   8   // SoftwareSerial RX - iNav feedback
  #define INAV_TX_PIN   9   // SoftwareSerial TX - iNav u-blox output
  
  #define GPS_UART      UART_NUM_1   // UART1 for GPS
  #define USE_SOFTWARE_SERIAL 1      // Use SoftwareSerial for iNav
  #define DEBUG_UART    UART_NUM_0   // UART0 for USB debug (automatic)
  
  #define BOARD_DESCRIPTION "ESP32-C3: 2 UARTs + SoftwareSerial (Debug via USB, GPS, iNav*)"

#endif

// ===== Baud rates =====
#define GPS_BAUD_RATE    9600   // AK721-JM default baud rate
#define INAV_BAUD_RATE   115200 // iNav typical baud rate
#define DEBUG_BAUD_RATE  115200 // Debug serial baud rate

// ===== Buffer Configuration =====
#define NMEA_BUFFER_SIZE   128   // NMEA sentence max length
#define UBLOX_BUFFER_SIZE  256   // u-blox message max length
#define RX_BUFFER_SIZE     256   // Serial RX buffer

// ===== Timing Configuration =====
#define NMEA_TIMEOUT_MS    2000  // Timeout for NMEA sentence
#define UPDATE_RATE_MS     100   // Output update rate (100ms = 10Hz)
#define LOOP_DELAY_MS      10    // Main loop delay

// ===== Debug Configuration =====
#define ENABLE_DEBUG       1     // Enable serial debug output (0=disable)
#define DEBUG_LEVEL        2     // 0=minimal, 1=normal, 2=verbose

// ===== GPS Configuration =====
#define GPS_MODEL          "AK721-JM"
#define NMEA_VARIANT       "NMEA0183"

// NMEA sentences to expect
#define EXPECT_GGA         1
#define EXPECT_RMC         1
#define EXPECT_GSA         1

// ===== iNav Configuration =====
#define INAV_VERSION       "9.1"
#define INAV_GPS_PROTOCOL  1     // 1=UBX (u-blox)

// ===== Feature Flags =====
#define FEATURE_CRC_CHECK  1     // Validate NMEA checksums
#define FEATURE_STATISTICS 1     // Track and report statistics
#define FEATURE_LED_STATUS 0     // Use LED for status indication (optional)
#define STATUS_LED_PIN     -1    // GPIO pin for status LED (unused by default)

// ===== Board Info for Debug =====
#if ENABLE_DEBUG
  #define PRINT_BOARD_INFO() \
    Serial.print("Board: "); Serial.println(BOARD_NAME); \
    Serial.print("Type: "); Serial.println(BOARD_DESCRIPTION); \
    Serial.print("GPS UART: "); Serial.print(GPS_RX_PIN); \
    Serial.print(" RX, "); Serial.print(GPS_TX_PIN); Serial.println(" TX"); \
    Serial.print("iNav UART: "); Serial.print(INAV_TX_PIN); \
    Serial.print(" TX"); \
    if (BOARD_TYPE == 1) { Serial.print(", "); Serial.print(INAV_RX_PIN); Serial.println(" RX"); } \
    else { Serial.println(" (SoftwareSerial)"); }
#else
  #define PRINT_BOARD_INFO()
#endif

#endif // CONFIG_H
