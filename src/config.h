// Configuration file for NMEA to u-blox translator
// Auto-detect the board selected in Arduino IDE

#ifndef CONFIG_H
#define CONFIG_H

// Enable this for development/debugging without hardware
// #define DEBUG_MODE_NO_GPS 1

// ===== AUTO-DETECT BOARD =====
#ifdef ARDUINO_ESP32S3_DEV
  #define BOARD_NAME "ESP32-S3 Super Mini"
  #define BOARD_TYPE 1
#elif defined(ARDUINO_ESP32C3_DEV)
  #define BOARD_NAME "ESP32-C3"
  #define BOARD_TYPE 2
#else
  #error "Select ESP32-S3 Dev Module or ESP32-C3-DevKitM-1"
#endif

// ===== UART Configuration (Auto-detected by board) =====

#if BOARD_TYPE == 1  // ESP32-S3 Super Mini
  // ESP32-S3 Super Mini has 3 UARTs: UART0 (USB), UART1, UART2
  #define GPS_RX_PIN    18  // UART1 RX - GPS NMEA input
  #define GPS_TX_PIN    17  // UART1 TX - GPS NMEA output (optional)
  
  #define INAV_RX_PIN   15  // UART2 RX - iNav feedback (optional)
  #define INAV_TX_PIN   16  // UART2 TX - iNav u-blox output
  
  #define GPS_UART      UART_NUM_1   // UART1 for GPS
  #define INAV_UART     UART_NUM_2   // UART2 for iNav
  #define DEBUG_UART    UART_NUM_0   // UART0 for USB debug (automatic)
  
  #define BOARD_DESCRIPTION "ESP32-S3 Super Mini: 3 UARTs (Debug via USB, GPS, iNav)"

#elif BOARD_TYPE == 2  // ESP32-C3 Dev
  // ESP32-C3 has 2 UARTs: UART0 (USB), UART1
  // Use SoftwareSerial for the second UART
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
#define GPS_BAUD_RATE    115200 // AK721-JM configured at 115200 baud
#define INAV_BAUD_RATE   115200 // iNav typical baud rate
#define DEBUG_BAUD_RATE  115200 // Debug serial baud rate
#define ENABLE_GPS_AUTO_BAUD 1  // Try common GPS baud rates at startup
#define GPS_AUTO_BAUD_WINDOW_MS 900 // Listen window per baud candidate
#define ENABLE_GPS_RATE_PROBE 1 // Try setting real GPS output rate on startup
#define GPS_TARGET_RATE_HZ 10   // Desired real GPS output rate (e.g. 10Hz => 100ms)

// ===== Buffer Configuration =====
#define NMEA_BUFFER_SIZE   128   // NMEA sentence max length
#define UBLOX_BUFFER_SIZE  256   // u-blox message max length
#define RX_BUFFER_SIZE     256   // Serial RX buffer

// ===== Timing Configuration =====
#define NMEA_TIMEOUT_MS    2000  // Timeout for NMEA sentence
#define UPDATE_RATE_MS     100   // Output update rate (100ms = 10Hz)
#define LOOP_DELAY_MS      1     // Main loop delay (reduced to 1ms for responsiveness)
#define STARTUP_INIT_DELAY_MS 1200 // Delay after power-up before UART init
#define INAV_STREAM_GRACE_MS 2500  // Delay before starting UBX nav stream

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
#define FEATURE_RGB_STATUS 1     // 1=enable RGB fix status indication
#define RGB_WS_PIN         48    // WS2812/WS281x data pin (-1 to disable)
#define RGB_LED_ACTIVE_HIGH 1    // 1=common cathode, 0=common anode
#define RGB_LED_R_PIN      -1    // Optional external RGB R pin
#define RGB_LED_G_PIN      -1    // Optional external RGB G pin
#define RGB_LED_B_PIN      -1    // Optional external RGB B pin

// ===== Watchdog Configuration =====
#define ENABLE_WATCHDOG    0     // Enable watchdog timer (optional, for stability)
#define WATCHDOG_TIMEOUT_S 10    // Watchdog timeout in seconds

// ===== Board Info for Debug =====
#if ENABLE_DEBUG
  #define PRINT_BOARD_INFO() \
    Serial.print("Board: "); Serial.println(BOARD_NAME); \
    Serial.print("Type: "); Serial.println(BOARD_DESCRIPTION); \
    Serial.print("GPS UART: GPIO"); Serial.print(GPS_RX_PIN); \
    Serial.print(" RX, GPIO"); Serial.print(GPS_TX_PIN); Serial.println(" TX"); \
    Serial.print("iNav UART: GPIO"); Serial.print(INAV_TX_PIN); \
    Serial.print(" TX, GPIO"); Serial.print(INAV_RX_PIN); Serial.println(" RX");
#else
  #define PRINT_BOARD_INFO()
#endif

#endif // CONFIG_H
